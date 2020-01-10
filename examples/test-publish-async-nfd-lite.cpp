/**
 * Copyright (C) 2017-2020 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version, with the additional exemption that
 * compiling, linking, and/or using OpenSSL is allowed.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * A copy of the GNU Lesser General Public License is in the file COPYING.
 */

/**
 * This connects to NFD at "localhost", and registers to receive Interest
 * packets with the prefix /testecho. When an Interest is received, make an echo
 * Data packet with the same name and send it. This needs NFD running on the
 * local host. The works with test-echo-consumer or test-echo-consumer-lite .
 * This uses the API for NDN-CPP Lite instead of the full NDN-CPP. Note that
 * this does not use the C++ Standard Library, or exceptions, or virtual
 * methods or malloc (except in the send functions which you can
 * change to not use malloc). Therefore the lightweight C++ code maps
 * directly onto the inner pure C functions, but has advantages of C++ such as
 * namespaces, inheritance, overloaded methods, "this" pointers, and calling
 * destructors automatically.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <ndn-cpp/lite/transport/tcp-transport-lite.hpp>
#include <ndn-cpp/lite/encoding/tlv-0_2-wire-format-lite.hpp>
#include <ndn-cpp/lite/util/dynamic-malloc-uint8-array-lite.hpp>
#include <ndn-cpp/lite/security/rsa-private-key-lite.hpp>
#include <ndn-cpp/lite/util/crypto-lite.hpp>

using namespace ndn;

static uint8_t DEFAULT_RSA_PUBLIC_KEY_DER[] = {
  0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
  0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00, 0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01,
  0x00, 0xb8, 0x09, 0xa7, 0x59, 0x82, 0x84, 0xec, 0x4f, 0x06, 0xfa, 0x1c, 0xb2, 0xe1, 0x38, 0x93,
  0x53, 0xbb, 0x7d, 0xd4, 0xac, 0x88, 0x1a, 0xf8, 0x25, 0x11, 0xe4, 0xfa, 0x1d, 0x61, 0x24, 0x5b,
  0x82, 0xca, 0xcd, 0x72, 0xce, 0xdb, 0x66, 0xb5, 0x8d, 0x54, 0xbd, 0xfb, 0x23, 0xfd, 0xe8, 0x8e,
  0xaf, 0xa7, 0xb3, 0x79, 0xbe, 0x94, 0xb5, 0xb7, 0xba, 0x17, 0xb6, 0x05, 0xae, 0xce, 0x43, 0xbe,
  0x3b, 0xce, 0x6e, 0xea, 0x07, 0xdb, 0xbf, 0x0a, 0x7e, 0xeb, 0xbc, 0xc9, 0x7b, 0x62, 0x3c, 0xf5,
  0xe1, 0xce, 0xe1, 0xd9, 0x8d, 0x9c, 0xfe, 0x1f, 0xc7, 0xf8, 0xfb, 0x59, 0xc0, 0x94, 0x0b, 0x2c,
  0xd9, 0x7d, 0xbc, 0x96, 0xeb, 0xb8, 0x79, 0x22, 0x8a, 0x2e, 0xa0, 0x12, 0x1d, 0x42, 0x07, 0xb6,
  0x5d, 0xdb, 0xe1, 0xf6, 0xb1, 0x5d, 0x7b, 0x1f, 0x54, 0x52, 0x1c, 0xa3, 0x11, 0x9b, 0xf9, 0xeb,
  0xbe, 0xb3, 0x95, 0xca, 0xa5, 0x87, 0x3f, 0x31, 0x18, 0x1a, 0xc9, 0x99, 0x01, 0xec, 0xaa, 0x90,
  0xfd, 0x8a, 0x36, 0x35, 0x5e, 0x12, 0x81, 0xbe, 0x84, 0x88, 0xa1, 0x0d, 0x19, 0x2a, 0x4a, 0x66,
  0xc1, 0x59, 0x3c, 0x41, 0x83, 0x3d, 0x3d, 0xb8, 0xd4, 0xab, 0x34, 0x90, 0x06, 0x3e, 0x1a, 0x61,
  0x74, 0xbe, 0x04, 0xf5, 0x7a, 0x69, 0x1b, 0x9d, 0x56, 0xfc, 0x83, 0xb7, 0x60, 0xc1, 0x5e, 0x9d,
  0x85, 0x34, 0xfd, 0x02, 0x1a, 0xba, 0x2c, 0x09, 0x72, 0xa7, 0x4a, 0x5e, 0x18, 0xbf, 0xc0, 0x58,
  0xa7, 0x49, 0x34, 0x46, 0x61, 0x59, 0x0e, 0xe2, 0x6e, 0x9e, 0xd2, 0xdb, 0xfd, 0x72, 0x2f, 0x3c,
  0x47, 0xcc, 0x5f, 0x99, 0x62, 0xee, 0x0d, 0xf3, 0x1f, 0x30, 0x25, 0x20, 0x92, 0x15, 0x4b, 0x04,
  0xfe, 0x15, 0x19, 0x1d, 0xdc, 0x7e, 0x5c, 0x10, 0x21, 0x52, 0x21, 0x91, 0x54, 0x60, 0x8b, 0x92,
  0x41, 0x02, 0x03, 0x01, 0x00, 0x01
};

static uint8_t DEFAULT_RSA_PRIVATE_KEY_DER[] = {
  0x30, 0x82, 0x04, 0xa5, 0x02, 0x01, 0x00, 0x02, 0x82, 0x01, 0x01, 0x00, 0xb8, 0x09, 0xa7, 0x59,
  0x82, 0x84, 0xec, 0x4f, 0x06, 0xfa, 0x1c, 0xb2, 0xe1, 0x38, 0x93, 0x53, 0xbb, 0x7d, 0xd4, 0xac,
  0x88, 0x1a, 0xf8, 0x25, 0x11, 0xe4, 0xfa, 0x1d, 0x61, 0x24, 0x5b, 0x82, 0xca, 0xcd, 0x72, 0xce,
  0xdb, 0x66, 0xb5, 0x8d, 0x54, 0xbd, 0xfb, 0x23, 0xfd, 0xe8, 0x8e, 0xaf, 0xa7, 0xb3, 0x79, 0xbe,
  0x94, 0xb5, 0xb7, 0xba, 0x17, 0xb6, 0x05, 0xae, 0xce, 0x43, 0xbe, 0x3b, 0xce, 0x6e, 0xea, 0x07,
  0xdb, 0xbf, 0x0a, 0x7e, 0xeb, 0xbc, 0xc9, 0x7b, 0x62, 0x3c, 0xf5, 0xe1, 0xce, 0xe1, 0xd9, 0x8d,
  0x9c, 0xfe, 0x1f, 0xc7, 0xf8, 0xfb, 0x59, 0xc0, 0x94, 0x0b, 0x2c, 0xd9, 0x7d, 0xbc, 0x96, 0xeb,
  0xb8, 0x79, 0x22, 0x8a, 0x2e, 0xa0, 0x12, 0x1d, 0x42, 0x07, 0xb6, 0x5d, 0xdb, 0xe1, 0xf6, 0xb1,
  0x5d, 0x7b, 0x1f, 0x54, 0x52, 0x1c, 0xa3, 0x11, 0x9b, 0xf9, 0xeb, 0xbe, 0xb3, 0x95, 0xca, 0xa5,
  0x87, 0x3f, 0x31, 0x18, 0x1a, 0xc9, 0x99, 0x01, 0xec, 0xaa, 0x90, 0xfd, 0x8a, 0x36, 0x35, 0x5e,
  0x12, 0x81, 0xbe, 0x84, 0x88, 0xa1, 0x0d, 0x19, 0x2a, 0x4a, 0x66, 0xc1, 0x59, 0x3c, 0x41, 0x83,
  0x3d, 0x3d, 0xb8, 0xd4, 0xab, 0x34, 0x90, 0x06, 0x3e, 0x1a, 0x61, 0x74, 0xbe, 0x04, 0xf5, 0x7a,
  0x69, 0x1b, 0x9d, 0x56, 0xfc, 0x83, 0xb7, 0x60, 0xc1, 0x5e, 0x9d, 0x85, 0x34, 0xfd, 0x02, 0x1a,
  0xba, 0x2c, 0x09, 0x72, 0xa7, 0x4a, 0x5e, 0x18, 0xbf, 0xc0, 0x58, 0xa7, 0x49, 0x34, 0x46, 0x61,
  0x59, 0x0e, 0xe2, 0x6e, 0x9e, 0xd2, 0xdb, 0xfd, 0x72, 0x2f, 0x3c, 0x47, 0xcc, 0x5f, 0x99, 0x62,
  0xee, 0x0d, 0xf3, 0x1f, 0x30, 0x25, 0x20, 0x92, 0x15, 0x4b, 0x04, 0xfe, 0x15, 0x19, 0x1d, 0xdc,
  0x7e, 0x5c, 0x10, 0x21, 0x52, 0x21, 0x91, 0x54, 0x60, 0x8b, 0x92, 0x41, 0x02, 0x03, 0x01, 0x00,
  0x01, 0x02, 0x82, 0x01, 0x01, 0x00, 0x8a, 0x05, 0xfb, 0x73, 0x7f, 0x16, 0xaf, 0x9f, 0xa9, 0x4c,
  0xe5, 0x3f, 0x26, 0xf8, 0x66, 0x4d, 0xd2, 0xfc, 0xd1, 0x06, 0xc0, 0x60, 0xf1, 0x9f, 0xe3, 0xa6,
  0xc6, 0x0a, 0x48, 0xb3, 0x9a, 0xca, 0x21, 0xcd, 0x29, 0x80, 0x88, 0x3d, 0xa4, 0x85, 0xa5, 0x7b,
  0x82, 0x21, 0x81, 0x28, 0xeb, 0xf2, 0x43, 0x24, 0xb0, 0x76, 0xc5, 0x52, 0xef, 0xc2, 0xea, 0x4b,
  0x82, 0x41, 0x92, 0xc2, 0x6d, 0xa6, 0xae, 0xf0, 0xb2, 0x26, 0x48, 0xa1, 0x23, 0x7f, 0x02, 0xcf,
  0xa8, 0x90, 0x17, 0xa2, 0x3e, 0x8a, 0x26, 0xbd, 0x6d, 0x8a, 0xee, 0xa6, 0x0c, 0x31, 0xce, 0xc2,
  0xbb, 0x92, 0x59, 0xb5, 0x73, 0xe2, 0x7d, 0x91, 0x75, 0xe2, 0xbd, 0x8c, 0x63, 0xe2, 0x1c, 0x8b,
  0xc2, 0x6a, 0x1c, 0xfe, 0x69, 0xc0, 0x44, 0xcb, 0x58, 0x57, 0xb7, 0x13, 0x42, 0xf0, 0xdb, 0x50,
  0x4c, 0xe0, 0x45, 0x09, 0x8f, 0xca, 0x45, 0x8a, 0x06, 0xfe, 0x98, 0xd1, 0x22, 0xf5, 0x5a, 0x9a,
  0xdf, 0x89, 0x17, 0xca, 0x20, 0xcc, 0x12, 0xa9, 0x09, 0x3d, 0xd5, 0xf7, 0xe3, 0xeb, 0x08, 0x4a,
  0xc4, 0x12, 0xc0, 0xb9, 0x47, 0x6c, 0x79, 0x50, 0x66, 0xa3, 0xf8, 0xaf, 0x2c, 0xfa, 0xb4, 0x6b,
  0xec, 0x03, 0xad, 0xcb, 0xda, 0x24, 0x0c, 0x52, 0x07, 0x87, 0x88, 0xc0, 0x21, 0xf3, 0x02, 0xe8,
  0x24, 0x44, 0x0f, 0xcd, 0xa0, 0xad, 0x2f, 0x1b, 0x79, 0xab, 0x6b, 0x49, 0x4a, 0xe6, 0x3b, 0xd0,
  0xad, 0xc3, 0x48, 0xb9, 0xf7, 0xf1, 0x34, 0x09, 0xeb, 0x7a, 0xc0, 0xd5, 0x0d, 0x39, 0xd8, 0x45,
  0xce, 0x36, 0x7a, 0xd8, 0xde, 0x3c, 0xb0, 0x21, 0x96, 0x97, 0x8a, 0xff, 0x8b, 0x23, 0x60, 0x4f,
  0xf0, 0x3d, 0xd7, 0x8f, 0xf3, 0x2c, 0xcb, 0x1d, 0x48, 0x3f, 0x86, 0xc4, 0xa9, 0x00, 0xf2, 0x23,
  0x2d, 0x72, 0x4d, 0x66, 0xa5, 0x01, 0x02, 0x81, 0x81, 0x00, 0xdc, 0x4f, 0x99, 0x44, 0x0d, 0x7f,
  0x59, 0x46, 0x1e, 0x8f, 0xe7, 0x2d, 0x8d, 0xdd, 0x54, 0xc0, 0xf7, 0xfa, 0x46, 0x0d, 0x9d, 0x35,
  0x03, 0xf1, 0x7c, 0x12, 0xf3, 0x5a, 0x9d, 0x83, 0xcf, 0xdd, 0x37, 0x21, 0x7c, 0xb7, 0xee, 0xc3,
  0x39, 0xd2, 0x75, 0x8f, 0xb2, 0x2d, 0x6f, 0xec, 0xc6, 0x03, 0x55, 0xd7, 0x00, 0x67, 0xd3, 0x9b,
  0xa2, 0x68, 0x50, 0x6f, 0x9e, 0x28, 0xa4, 0x76, 0x39, 0x2b, 0xb2, 0x65, 0xcc, 0x72, 0x82, 0x93,
  0xa0, 0xcf, 0x10, 0x05, 0x6a, 0x75, 0xca, 0x85, 0x35, 0x99, 0xb0, 0xa6, 0xc6, 0xef, 0x4c, 0x4d,
  0x99, 0x7d, 0x2c, 0x38, 0x01, 0x21, 0xb5, 0x31, 0xac, 0x80, 0x54, 0xc4, 0x18, 0x4b, 0xfd, 0xef,
  0xb3, 0x30, 0x22, 0x51, 0x5a, 0xea, 0x7d, 0x9b, 0xb2, 0x9d, 0xcb, 0xba, 0x3f, 0xc0, 0x1a, 0x6b,
  0xcd, 0xb0, 0xe6, 0x2f, 0x04, 0x33, 0xd7, 0x3a, 0x49, 0x71, 0x02, 0x81, 0x81, 0x00, 0xd5, 0xd9,
  0xc9, 0x70, 0x1a, 0x13, 0xb3, 0x39, 0x24, 0x02, 0xee, 0xb0, 0xbb, 0x84, 0x17, 0x12, 0xc6, 0xbd,
  0x65, 0x73, 0xe9, 0x34, 0x5d, 0x43, 0xff, 0xdc, 0xf8, 0x55, 0xaf, 0x2a, 0xb9, 0xe1, 0xfa, 0x71,
  0x65, 0x4e, 0x50, 0x0f, 0xa4, 0x3b, 0xe5, 0x68, 0xf2, 0x49, 0x71, 0xaf, 0x15, 0x88, 0xd7, 0xaf,
  0xc4, 0x9d, 0x94, 0x84, 0x6b, 0x5b, 0x10, 0xd5, 0xc0, 0xaa, 0x0c, 0x13, 0x62, 0x99, 0xc0, 0x8b,
  0xfc, 0x90, 0x0f, 0x87, 0x40, 0x4d, 0x58, 0x88, 0xbd, 0xe2, 0xba, 0x3e, 0x7e, 0x2d, 0xd7, 0x69,
  0xa9, 0x3c, 0x09, 0x64, 0x31, 0xb6, 0xcc, 0x4d, 0x1f, 0x23, 0xb6, 0x9e, 0x65, 0xd6, 0x81, 0xdc,
  0x85, 0xcc, 0x1e, 0xf1, 0x0b, 0x84, 0x38, 0xab, 0x93, 0x5f, 0x9f, 0x92, 0x4e, 0x93, 0x46, 0x95,
  0x6b, 0x3e, 0xb6, 0xc3, 0x1b, 0xd7, 0x69, 0xa1, 0x0a, 0x97, 0x37, 0x78, 0xed, 0xd1, 0x02, 0x81,
  0x80, 0x33, 0x18, 0xc3, 0x13, 0x65, 0x8e, 0x03, 0xc6, 0x9f, 0x90, 0x00, 0xae, 0x30, 0x19, 0x05,
  0x6f, 0x3c, 0x14, 0x6f, 0xea, 0xf8, 0x6b, 0x33, 0x5e, 0xee, 0xc7, 0xf6, 0x69, 0x2d, 0xdf, 0x44,
  0x76, 0xaa, 0x32, 0xba, 0x1a, 0x6e, 0xe6, 0x18, 0xa3, 0x17, 0x61, 0x1c, 0x92, 0x2d, 0x43, 0x5d,
  0x29, 0xa8, 0xdf, 0x14, 0xd8, 0xff, 0xdb, 0x38, 0xef, 0xb8, 0xb8, 0x2a, 0x96, 0x82, 0x8e, 0x68,
  0xf4, 0x19, 0x8c, 0x42, 0xbe, 0xcc, 0x4a, 0x31, 0x21, 0xd5, 0x35, 0x6c, 0x5b, 0xa5, 0x7c, 0xff,
  0xd1, 0x85, 0x87, 0x28, 0xdc, 0x97, 0x75, 0xe8, 0x03, 0x80, 0x1d, 0xfd, 0x25, 0x34, 0x41, 0x31,
  0x21, 0x12, 0x87, 0xe8, 0x9a, 0xb7, 0x6a, 0xc0, 0xc4, 0x89, 0x31, 0x15, 0x45, 0x0d, 0x9c, 0xee,
  0xf0, 0x6a, 0x2f, 0xe8, 0x59, 0x45, 0xc7, 0x7b, 0x0d, 0x6c, 0x55, 0xbb, 0x43, 0xca, 0xc7, 0x5a,
  0x01, 0x02, 0x81, 0x81, 0x00, 0xab, 0xf4, 0xd5, 0xcf, 0x78, 0x88, 0x82, 0xc2, 0xdd, 0xbc, 0x25,
  0xe6, 0xa2, 0xc1, 0xd2, 0x33, 0xdc, 0xef, 0x0a, 0x97, 0x2b, 0xdc, 0x59, 0x6a, 0x86, 0x61, 0x4e,
  0xa6, 0xc7, 0x95, 0x99, 0xa6, 0xa6, 0x55, 0x6c, 0x5a, 0x8e, 0x72, 0x25, 0x63, 0xac, 0x52, 0xb9,
  0x10, 0x69, 0x83, 0x99, 0xd3, 0x51, 0x6c, 0x1a, 0xb3, 0x83, 0x6a, 0xff, 0x50, 0x58, 0xb7, 0x28,
  0x97, 0x13, 0xe2, 0xba, 0x94, 0x5b, 0x89, 0xb4, 0xea, 0xba, 0x31, 0xcd, 0x78, 0xe4, 0x4a, 0x00,
  0x36, 0x42, 0x00, 0x62, 0x41, 0xc6, 0x47, 0x46, 0x37, 0xea, 0x6d, 0x50, 0xb4, 0x66, 0x8f, 0x55,
  0x0c, 0xc8, 0x99, 0x91, 0xd5, 0xec, 0xd2, 0x40, 0x1c, 0x24, 0x7d, 0x3a, 0xff, 0x74, 0xfa, 0x32,
  0x24, 0xe0, 0x11, 0x2b, 0x71, 0xad, 0x7e, 0x14, 0xa0, 0x77, 0x21, 0x68, 0x4f, 0xcc, 0xb6, 0x1b,
  0xe8, 0x00, 0x49, 0x13, 0x21, 0x02, 0x81, 0x81, 0x00, 0xb6, 0x18, 0x73, 0x59, 0x2c, 0x4f, 0x92,
  0xac, 0xa2, 0x2e, 0x5f, 0xb6, 0xbe, 0x78, 0x5d, 0x47, 0x71, 0x04, 0x92, 0xf0, 0xd7, 0xe8, 0xc5,
  0x7a, 0x84, 0x6b, 0xb8, 0xb4, 0x30, 0x1f, 0xd8, 0x0d, 0x58, 0xd0, 0x64, 0x80, 0xa7, 0x21, 0x1a,
  0x48, 0x00, 0x37, 0xd6, 0x19, 0x71, 0xbb, 0x91, 0x20, 0x9d, 0xe2, 0xc3, 0xec, 0xdb, 0x36, 0x1c,
  0xca, 0x48, 0x7d, 0x03, 0x32, 0x74, 0x1e, 0x65, 0x73, 0x02, 0x90, 0x73, 0xd8, 0x3f, 0xb5, 0x52,
  0x35, 0x79, 0x1c, 0xee, 0x93, 0xa3, 0x32, 0x8b, 0xed, 0x89, 0x98, 0xf1, 0x0c, 0xd8, 0x12, 0xf2,
  0x89, 0x7f, 0x32, 0x23, 0xec, 0x67, 0x66, 0x52, 0x83, 0x89, 0x99, 0x5e, 0x42, 0x2b, 0x42, 0x4b,
  0x84, 0x50, 0x1b, 0x3e, 0x47, 0x6d, 0x74, 0xfb, 0xd1, 0xa6, 0x10, 0x20, 0x6c, 0x6e, 0xbe, 0x44,
  0x3f, 0xb9, 0xfe, 0xbc, 0x8d, 0xda, 0xcb, 0xea, 0x8f
};

ndn_Error
sendNfdRegisterPrefix
  (const NameLite& prefix, const RsaPrivateKeyLite& privateKey,
   const NameLite& certificateName, TcpTransportLite& transport);

ndn_Error
signAndSendData
  (DataLite& data, const RsaPrivateKeyLite& privateKey,
   const NameLite& certificateName, uint8_t* signatureBuffer, 
    TcpTransportLite& transport);

/**
 * The Echo class extends ElementListenerLite and has an onReceivedElement to
 * process the incoming Interest packet by sending an echo Data packet.
 */
class Echo : public ElementListenerLite {
public:
  /**
   * Create an Echo.
   * @param prefix The prefix used for register prefix so that onReceivedElement
   * can check if the incoming Interest packet matches. This does not make a
   * copy; the referenced object must remain valid.
   * @param privateKey The private key for signing. This does not make a
   * copy; the referenced object must remain valid.
   * @param certificateName The name for the KeyLocator. This does not make a
   * copy; the referenced object must remain valid.
   * @param transport This calls transport.send.
   */
  Echo(const NameLite& prefix, const RsaPrivateKeyLite& privateKey,
   const NameLite& certificateName, TcpTransportLite& transport)
  : ElementListenerLite(&onReceivedElementWrapper),
    prefix_(prefix), privateKey_(privateKey), certificateName_(certificateName),
    transport_(transport), responseCount_(0)
  {
  }

  /**
   * This is called when an entire packet is received. If this is an Interest
   * for the prefix, create and send the echo Data packet.
   * @param element pointer to the element. This buffer is only valid during
   * this call.  If you need the data later, you must copy.
   * @param elementLength length of element
   */
  void
  onReceivedElement(const uint8_t *element, size_t elementLength);

  int responseCount_;

private:
  const NameLite& prefix_;
  const RsaPrivateKeyLite& privateKey_;
  const NameLite& certificateName_;
  TcpTransportLite& transport_;

  static void
  onReceivedElementWrapper
    (ElementListenerLite *self, const uint8_t *element, size_t elementLength);
};

void
Echo::onReceivedElement(const uint8_t *element, size_t elementLength)
{
  if (*element != 5)
    // Not a TLV Interest packet.
    return;

  // Reserve space for a large maximum number of name components and entries.
  // If you know your application requires less, you can use a smaller maximum.
  struct ndn_NameComponent interestNameComponents[100];
  struct ndn_ExcludeEntry excludeEntries[100];
  struct ndn_NameComponent interestKeyNameComponents[100];
  InterestLite interest
    (interestNameComponents,
     sizeof(interestNameComponents) / sizeof(interestNameComponents[0]),
     excludeEntries, sizeof(excludeEntries) / sizeof(excludeEntries[0]),
     interestKeyNameComponents,
     sizeof(interestKeyNameComponents) / sizeof(interestKeyNameComponents[0]));
  size_t signedPortionBeginOffset, signedPortionEndOffset;
  ndn_Error error;
  if ((error = Tlv0_2WireFormatLite::decodeInterest
       (interest, element, elementLength, &signedPortionBeginOffset,
        &signedPortionEndOffset))) {
    printf("Error decoding interest: %s\n", ndn_getErrorString(error));
    return;
  }

  if (!prefix_.match(interest.getName()) || interest.getName().size() != 2)
    // We got an Interest packet that is not for us.
    return;

  ++responseCount_;

  // Make a Data packet with the Interest's name.
  struct ndn_NameComponent dataNameComponents[10];
  struct ndn_NameComponent dataKeyNameComponents[10];
  DataLite data
    (dataNameComponents,
     sizeof(dataNameComponents) / sizeof(dataNameComponents[0]),
     dataKeyNameComponents,
     sizeof(dataKeyNameComponents) / sizeof(dataKeyNameComponents[0]));
  data.getName().append(interest.getName());

  // Set the content to a message with the name.
  char content[200];
  ::strcpy(content, "Echo /testecho/");
  size_t iFrom = 0, iTo = ::strlen(content);
  for (;
       iFrom < interest.getName().get(1).getValue().size() && iTo < sizeof(content);
       ++iFrom, ++iTo)
    content[iTo] = interest.getName().get(1).getValue().buf()[iFrom];
  data.setContent(BlobLite((const uint8_t*)content, iTo));

  // Sign and send the Data packet.
  uint8_t signatureBuffer[256];
  if ((error = signAndSendData
       (data, privateKey_, certificateName_, signatureBuffer, transport_))) {
    printf("Error in signAndSendData: %s\n", ndn_getErrorString(error));
    return;
  }

  printf("Sent content ");
  for (size_t i = 0; i < data.getContent().size(); ++i)
    printf("%c", (int)data.getContent().buf()[i]);
  printf("\n");
}

void
Echo::onReceivedElementWrapper
  (ElementListenerLite *self, const uint8_t *element, size_t elementLength)
{
  // Simply call the onReceivedElement method.
  ((Echo*)self)->onReceivedElement(element, elementLength);
}

int main(int argc, char** argv)
{
  // Set up the private key and certificateName for signing.
  ndn_Error error;
  RsaPrivateKeyLite privateKey;
  if ((error = privateKey.decode
       (DEFAULT_RSA_PRIVATE_KEY_DER, sizeof(DEFAULT_RSA_PRIVATE_KEY_DER)))) {
    // Don't expect this to happen.
    printf("Error decoding RSA private key DER: %s\n", ndn_getErrorString(error));
    return error;
  }

  ndn_NameComponent certificateNameComponents[5];
  NameLite certificateName
    (certificateNameComponents,
     sizeof(certificateNameComponents) / sizeof(certificateNameComponents[0]));
  certificateName.append("testname");
  certificateName.append("KEY");
  certificateName.append("DSK-123");
  certificateName.append("ID-CERT");
  if ((error = certificateName.append("0"))) {
    printf("Error in certificateName.append: %s\n", ndn_getErrorString(error));
    return error;
  }

  // Create the Prefix name.
  const char* prefixString = "testecho";
  struct ndn_NameComponent prefixNameComponents[1];
  NameLite prefix
    (prefixNameComponents,
     sizeof(prefixNameComponents) / sizeof(prefixNameComponents[0]));
  if ((error = prefix.append(prefixString))) {
    printf("Error in name append: %s\n", ndn_getErrorString(error));
    return error;
  }

  // Create the transport using a buffer which is large enough to receive an
  // entire packet so that we don't have to malloc memory. If the maximum size
  // of an expected packet is smaller then the smaller value can be used.
  // Alternatively, DynamicMallocUInt8ArrayLite can be used if the your platform
  // supports malloc.
  uint8_t elementBufferBytes[MAX_NDN_PACKET_SIZE];
  DynamicUInt8ArrayLite elementBuffer
    (elementBufferBytes, sizeof(elementBufferBytes), 0);
  TcpTransportLite transport(elementBuffer);

  // Set up the Echo object to receive the Interest packet and connect.
  Echo echo(prefix, privateKey, certificateName, transport);
  if ((error = transport.connect("localhost", 6363, echo))) {
    printf("Error in transport connect: %s\n", ndn_getErrorString(error));
    return error;
  }

  printf("Register prefix /%s\n", prefixString);
  sendNfdRegisterPrefix(prefix, privateKey, certificateName, transport);

  // The main event loop.
  // We're not using a timeout to check for a response to the register prefix
  // interest. To keep this example lightweight, we loop forever waiting for the
  // Interest packet, assuming that register prefix succeeds. To check for
  // timeout, we could use ndn_getNowMilliseconds() to get the start time, then
  // check the elapsed time with ndn_getNowMilliseconds() in the loop.
  while (echo.responseCount_ < 1) {
    // processEvents will use whatever buffer size is provided. A larger buffer
    // is more efficient but takes more memory.
    uint8_t buffer[1000];
    if ((error = transport.processEvents(buffer, sizeof(buffer)))) {
      printf("Error in processEvents: %s\n", ndn_getErrorString(error));
      return error;
    }

    // We need to sleep for a few milliseconds so we don't use 100% of the CPU.
    // If your platform doesn't have usleep then you can use another sleep
    // function, or don't sleep at all which could use more CPU.
    ::usleep(10000);
  }

  return 0;
}

/**
 * Make, sign and send an NFD register prefix command interest. This does not
 * wait for the register response.
 * @param prefix The name prefix to register.
 * @param privateKey The private key for signing.
 * @param certificateName The name for the KeyLocator.
 * @param transport This calls transport.send.
 * @return 0 for success, else an error code.
 */
ndn_Error
sendNfdRegisterPrefix
  (const NameLite& prefix, const RsaPrivateKeyLite& privateKey,
   const NameLite& certificateName, TcpTransportLite& transport)
{
  // Here we use a DynamicMallocUInt8ArrayLite. If your platform doesn't support
  // malloc then set "#if 0" to use DynamicUInt8ArrayLite directly with a
  // fixed-size array.
#if 1
  DynamicMallocUInt8ArrayLite interestEncoding(50);
  DynamicMallocUInt8ArrayLite controlParametersEncoding(50);
  DynamicMallocUInt8ArrayLite signatureInfoEncoding(50);
#else
  // Set this smaller if you expect a smaller max encoding size.
  uint8_t interestEncodingBuffer[MAX_NDN_PACKET_SIZE];
  DynamicUInt8ArrayLite interestEncoding
    (interestEncodingBuffer, sizeof(interestEncodingBuffer), 0);
  uint8_t controlParametersEncodingBuffer[1000];
  DynamicUInt8ArrayLite controlParametersEncoding
    (controlParametersEncodingBuffer, sizeof(controlParametersEncodingBuffer), 0);
  uint8_t signatureInfoEncodingBuffer[1000];
  DynamicUInt8ArrayLite signatureInfoEncoding
    (signatureInfoEncodingBuffer, sizeof(signatureInfoEncodingBuffer), 0);
#endif

  // Make and encode the ControlParameters.
  struct ndn_NameComponent controlParametersNameComponents[100];
  ControlParametersLite controlParameters
    (controlParametersNameComponents,
     sizeof(controlParametersNameComponents) / sizeof(controlParametersNameComponents[0]),
     0, 0);
  ndn_Error error;
  if ((error = controlParameters.setName(prefix)))
    return error;
  size_t controlParametersEncodingLength;
  if ((error = Tlv0_2WireFormatLite::encodeControlParameters
       (controlParameters, controlParametersEncoding,
        &controlParametersEncodingLength)))
    return error;

  // Make and encode the SignatureInfo.
  struct ndn_NameComponent keyNameComponents[100];
  SignatureLite signature
    (keyNameComponents, sizeof(keyNameComponents) / sizeof(keyNameComponents[0]));
  signature.getKeyLocator().setType(ndn_KeyLocatorType_KEYNAME);
  if ((error = signature.getKeyLocator().setKeyName(certificateName)))
    return error;
  signature.setType(ndn_SignatureType_Sha256WithRsaSignature);
  size_t signatureInfoEncodingLength;
  if ((error = Tlv0_2WireFormatLite::encodeSignatureInfo
       (signature, signatureInfoEncoding, &signatureInfoEncodingLength)))
    return error;

  // Prepare the register prefix command interest.
  struct ndn_NameComponent nameComponents[10];
  InterestLite interest
    (nameComponents, sizeof(nameComponents) / sizeof(nameComponents[0]),
     0, 0, 0, 0);
  interest.getName().append("localhost");
  interest.getName().append("nfd");
  interest.getName().append("rib");
  interest.getName().append("register");
  interest.getName().append
    (controlParametersEncoding.getArray(), controlParametersEncodingLength);

  // Add a timestamp (as a big-endian 64-bit value) and a random value as
  // required for a command interest.
  uint64_t timestamp = (uint64_t)::round(ndn_getNowMilliseconds());
  uint8_t timestampBuffer[8];
  for (int i = 0; i < 8; ++i)
    timestampBuffer[i] = (timestamp >> (8 * (7 - i))) & 0xff;
  interest.getName().append(BlobLite(timestampBuffer, 8));

  uint8_t randomBuffer[8];
  if ((error = CryptoLite::generateRandomBytes(randomBuffer, sizeof(randomBuffer))))
    return error;
  interest.getName().append(randomBuffer, sizeof(randomBuffer));

  // Add the SignatureInfo and an empty signature so that the "signedPortion" is
  // correct. Encode once to get the signed portion.
  interest.getName().append
    (signatureInfoEncoding.getArray(), signatureInfoEncodingLength);
  interest.getName().append(BlobLite(0, 0));
  size_t interestEncodingLength;
  size_t signedPortionBeginOffset, signedPortionEndOffset;
  if ((error = Tlv0_2WireFormatLite::encodeInterest
       (interest, &signedPortionBeginOffset, &signedPortionEndOffset,
        interestEncoding, &interestEncodingLength)))
    return error;

  // Compute the signature and replace the empty signature.
  uint8_t signatureBuffer[256];
  size_t signatureLength;
  if ((error = privateKey.signWithSha256
       (interestEncoding.getArray() + signedPortionBeginOffset,
        signedPortionEndOffset - signedPortionBeginOffset,
        signatureBuffer, signatureLength)))
    return error;
  interest.getName().pop();
  interest.getName().append(signatureBuffer, signatureLength);

  // Encode again and send.
  if ((error = Tlv0_2WireFormatLite::encodeInterest
       (interest, &signedPortionBeginOffset, &signedPortionEndOffset,
        interestEncoding, &interestEncodingLength)))
    return error;

  if ((error = transport.send(interestEncoding.getArray(), interestEncodingLength)))
    return error;

  return NDN_ERROR_success;
}

/**
 * Sign the Data packet, set its SignatureInfo, encode it and send it through
 * the transport.
 * @param data The Data to encode and send. This modifies the object by adding
 * the SignatureInfo.
 * @param privateKey The private key for signing.
 * @param certificateName The name for the KeyLocator.
 * @param signatureBuffer A pointer to the signature output buffer. The caller
 * must provide a buffer large enough to receive the signature bytes based on the
 * private key. The caller must provide this buffer so that the pointer in the
 * data object is valid when this function returns.
 * @param transport This calls transport.send.
 * @return 0 for success, else an error code.
 */
ndn_Error
signAndSendData
  (DataLite& data, const RsaPrivateKeyLite& privateKey,
   const NameLite& certificateName, uint8_t* signatureBuffer, 
   TcpTransportLite& transport)
{
  // Here we use a DynamicMallocUInt8ArrayLite. If your platform doesn't support
  // malloc then set "#if 0" to use DynamicUInt8ArrayLite directly with a
  // fixed-size array.
#if 1
  DynamicMallocUInt8ArrayLite encoding(50);
#else
  // Set this smaller if you expect a smaller max encoding size.
  uint8_t encodingBuffer[MAX_NDN_PACKET_SIZE];
  DynamicUInt8ArrayLite encoding(encodingBuffer, sizeof(encodingBuffer), 0);
#endif

  // Set up the SignatureInfo.
  ndn_Error error;
  data.getSignature().getKeyLocator().setType(ndn_KeyLocatorType_KEYNAME);
  if ((error = data.getSignature().getKeyLocator().setKeyName(certificateName)))
    return error;
  data.getSignature().setType(ndn_SignatureType_Sha256WithRsaSignature);
  data.getSignature().setSignature(BlobLite(0, 0));

  // Encode once to get the signed portion.
  size_t encodingLength;
  size_t signedPortionBeginOffset, signedPortionEndOffset;
  if ((error = Tlv0_2WireFormatLite::encodeData
       (data, &signedPortionBeginOffset, &signedPortionEndOffset, encoding,
        &encodingLength)))
    return error;

  // Compute the signature.
  size_t signatureLength;
  if ((error = privateKey.signWithSha256
       (encoding.getArray() + signedPortionBeginOffset,
        signedPortionEndOffset - signedPortionBeginOffset,
        signatureBuffer, signatureLength)))
    return error;
  data.getSignature().setSignature
    (BlobLite(signatureBuffer, sizeof(signatureLength)));

  // Encode again and send.
  if ((error = Tlv0_2WireFormatLite::encodeData
       (data, &signedPortionBeginOffset, &signedPortionEndOffset, encoding,
        &encodingLength)))
    return error;

  if ((error = transport.send(encoding.getArray(), encodingLength)))
    return error;

  return NDN_ERROR_success;
}
