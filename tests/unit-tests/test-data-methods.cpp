/**
 * Copyright (C) 2014-2019 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * From PyNDN unit-tests by Adeola Bannis.
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

#include "gtest/gtest.h"
#include "ndn-cpp/lite/util/crypto-lite.hpp"
#include <sstream>
#include <ndn-cpp/data.hpp>
#include <ndn-cpp/security/identity/memory-identity-storage.hpp>
#include <ndn-cpp/security/identity/memory-private-key-storage.hpp>
#include <ndn-cpp/security/policy/self-verify-policy-manager.hpp>
#include <ndn-cpp/security/key-chain.hpp>
#include <ndn-cpp/sha256-with-rsa-signature.hpp>
#include <ndn-cpp/generic-signature.hpp>
#include <ndn-cpp/lite/util/crypto-lite.hpp>
#include <ndn-cpp/lite/lp/lp-packet-lite.hpp>
#include <ndn-cpp/encoding/tlv-wire-format.hpp>
#include <ndn-cpp/lite/encoding/tlv-0_2-wire-format-lite.hpp>
#include "../../src/lp/lp-packet.hpp"

using namespace std;
using namespace ndn;
using namespace ndn::func_lib;

static const uint8_t DEFAULT_RSA_PUBLIC_KEY_DER[] = {
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

static const uint8_t DEFAULT_RSA_PRIVATE_KEY_DER[] = {
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

static const uint8_t DEFAULT_EC_PUBLIC_KEY_DER[] = {
  0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a,
  0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x98, 0x9a, 0xf0, 0x61, 0x70,
  0x43, 0x2e, 0xb6, 0x12, 0x92, 0xf5, 0x57, 0x08, 0x07, 0xe7, 0xaf, 0x23, 0xab, 0x79, 0x0b, 0x05,
  0xaf, 0xa0, 0x3f, 0x8f, 0x23, 0x04, 0x50, 0xd2, 0x30, 0x47, 0x00, 0x1a, 0xff, 0x77, 0xba, 0x08,
  0x5b, 0x9a, 0xb1, 0xe6, 0x1a, 0xc4, 0x6a, 0x38, 0x00, 0x79, 0x15, 0xf8, 0x92, 0x3d, 0x9d, 0x8e,
  0x16, 0x29, 0x57, 0x34, 0x0b, 0xd4, 0x66, 0xb2, 0xe7, 0x54, 0x0b
};

// For OpenSSL, we use the inner PKCS #1 private key with full parameters.
static const uint8_t DEFAULT_EC_PRIVATE_KEY_DER[] = {
  0x30, 0x82, 0x01, 0x22, 0x02, 0x01, 0x01, 0x04, 0x20, 0x49, 0x35, 0xef, 0x6c, 0xbf, 0xca, 0x40,
  0x55, 0xfc, 0x63, 0x61, 0x69, 0xa2, 0x8a, 0x5d, 0x1e, 0x48, 0x7b, 0x83, 0x44, 0xf4, 0x65, 0xd3,
  0xe2, 0xab, 0x2b, 0xc0, 0xbc, 0x8d, 0x6f, 0x17, 0x1b, 0xa0, 0x81, 0xfa, 0x30, 0x81, 0xf7, 0x02,
  0x01, 0x01, 0x30, 0x2c, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x01, 0x01, 0x02, 0x21, 0x00,
  0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x30, 0x5b, 0x04, 0x20, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfc, 0x04, 0x20, 0x5a, 0xc6, 0x35, 0xd8, 0xaa, 0x3a, 0x93, 0xe7, 0xb3, 0xeb,
  0xbd, 0x55, 0x76, 0x98, 0x86, 0xbc, 0x65, 0x1d, 0x06, 0xb0, 0xcc, 0x53, 0xb0, 0xf6, 0x3b, 0xce,
  0x3c, 0x3e, 0x27, 0xd2, 0x60, 0x4b, 0x03, 0x15, 0x00, 0xc4, 0x9d, 0x36, 0x08, 0x86, 0xe7, 0x04,
  0x93, 0x6a, 0x66, 0x78, 0xe1, 0x13, 0x9d, 0x26, 0xb7, 0x81, 0x9f, 0x7e, 0x90, 0x04, 0x41, 0x04,
  0x6b, 0x17, 0xd1, 0xf2, 0xe1, 0x2c, 0x42, 0x47, 0xf8, 0xbc, 0xe6, 0xe5, 0x63, 0xa4, 0x40, 0xf2,
  0x77, 0x03, 0x7d, 0x81, 0x2d, 0xeb, 0x33, 0xa0, 0xf4, 0xa1, 0x39, 0x45, 0xd8, 0x98, 0xc2, 0x96,
  0x4f, 0xe3, 0x42, 0xe2, 0xfe, 0x1a, 0x7f, 0x9b, 0x8e, 0xe7, 0xeb, 0x4a, 0x7c, 0x0f, 0x9e, 0x16,
  0x2b, 0xce, 0x33, 0x57, 0x6b, 0x31, 0x5e, 0xce, 0xcb, 0xb6, 0x40, 0x68, 0x37, 0xbf, 0x51, 0xf5,
  0x02, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xbc, 0xe6, 0xfa, 0xad, 0xa7, 0x17, 0x9e, 0x84, 0xf3, 0xb9, 0xca, 0xc2, 0xfc,
  0x63, 0x25, 0x51, 0x02, 0x01, 0x01
};

static const uint8_t codedData[] = {
0x06, 0xCE, // NDN Data
  0x07, 0x0A, 0x08, 0x03, 0x6E, 0x64, 0x6E, 0x08, 0x03, 0x61, 0x62, 0x63, // Name
  0x14, 0x0A, // MetaInfo
    0x19, 0x02, 0x13, 0x88, // FreshnessPeriod
    0x1A, 0x04, // FinalBlockId
      0x08, 0x02, 0x00, 0x09, // NameComponent
  0x15, 0x08, 0x53, 0x55, 0x43, 0x43, 0x45, 0x53, 0x53, 0x21, // Content
  0x16, 0x28, // SignatureInfo
    0x1B, 0x01, 0x01, // SignatureType
    0x1C, 0x23, // KeyLocator
      0x07, 0x21, // Name
        0x08, 0x08, 0x74, 0x65, 0x73, 0x74, 0x6E, 0x61, 0x6D, 0x65,
        0x08, 0x03, 0x4B, 0x45, 0x59,
        0x08, 0x07, 0x44, 0x53, 0x4B, 0x2D, 0x31, 0x32, 0x33,
        0x08, 0x07, 0x49, 0x44, 0x2D, 0x43, 0x45, 0x52, 0x54,
  0x17, 0x80, // SignatureValue
    0x1A, 0x03, 0xC3, 0x9C, 0x4F, 0xC5, 0x5C, 0x36, 0xA2, 0xE7, 0x9C, 0xEE, 0x52, 0xFE, 0x45, 0xA7,
    0xE1, 0x0C, 0xFB, 0x95, 0xAC, 0xB4, 0x9B, 0xCC, 0xB6, 0xA0, 0xC3, 0x4A, 0xAA, 0x45, 0xBF, 0xBF,
    0xDF, 0x0B, 0x51, 0xD5, 0xA4, 0x8B, 0xF2, 0xAB, 0x45, 0x97, 0x1C, 0x24, 0xD8, 0xE2, 0xC2, 0x8A,
    0x4D, 0x40, 0x12, 0xD7, 0x77, 0x01, 0xEB, 0x74, 0x35, 0xF1, 0x4D, 0xDD, 0xD0, 0xF3, 0xA6, 0x9A,
    0xB7, 0xA4, 0xF1, 0x7F, 0xA7, 0x84, 0x34, 0xD7, 0x08, 0x25, 0x52, 0x80, 0x8B, 0x6C, 0x42, 0x93,
    0x04, 0x1E, 0x07, 0x1F, 0x4F, 0x76, 0x43, 0x18, 0xF2, 0xF8, 0x51, 0x1A, 0x56, 0xAF, 0xE6, 0xA9,
    0x31, 0xCB, 0x6C, 0x1C, 0x0A, 0xA4, 0x01, 0x10, 0xFC, 0xC8, 0x66, 0xCE, 0x2E, 0x9C, 0x0B, 0x2D,
    0x7F, 0xB4, 0x64, 0xA0, 0xEE, 0x22, 0x82, 0xC8, 0x34, 0xF7, 0x9A, 0xF5, 0x51, 0x12, 0x2A, 0x84,
1
};

static const int experimentalSignatureType = 100;
static const uint8_t experimentalSignatureInfo[] = {
0x16, 0x08, // SignatureInfo
  0x1B, 0x01, experimentalSignatureType, // SignatureType
  0x81, 0x03, 1, 2, 3 // Experimental info
};

static const uint8_t experimentalSignatureInfoNoSignatureType[] = {
0x16, 0x05, // SignatureInfo
  0x81, 0x03, 1, 2, 3 // Experimental info
};

static const uint8_t experimentalSignatureInfoBadTlv[] = {
0x16, 0x08, // SignatureInfo
  0x1B, 0x01, experimentalSignatureType, // SignatureType
  0x81, 0x10, 1, 2, 3 // Bad TLV encoding (length 0x10 doesn't match the value length.
};

static const uint8_t CONGESTION_MARK_PACKET[] = {
  0x64, 0xfd, 0x03, 0x5f, // LpPacket
    0xfd, 0x03, 0x40, 0x01, 0x01, // CongestionMark = 1
    0x50, 0xfd, 0x03, 0x56, // Fragment
      0x06, 0xfd, 0x03, 0x52, // NDN Data
        0x07, 0x18, 0x08, 0x04, 0x74, 0x65, 0x73, 0x74, 0x08, 0x09, 0xfd, 0x00, 0x00, 0x01, 0x62, 0xd5,
        0x29, 0x3f, 0xa8, 0x08, 0x05, 0x00, 0x00, 0x01, 0x57, 0xc3, 0x14, 0x0d, 0x19, 0x02, 0x27, 0x10,
        0x1a, 0x07, 0x08, 0x05, 0x00, 0x00, 0x02, 0xda, 0xcc, 0x15, 0xfd, 0x01, 0xf4, 0x65, 0x64, 0x20,
        0x43, 0x72, 0x79, 0x70, 0x74, 0x6f, 0x20, 0x74, 0x6f, 0x20, 0x6e, 0x6f, 0x74, 0x20, 0x63, 0x6c,
        0x61, 0x73, 0x68, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x74, 0x68, 0x65, 0x20, 0x62, 0x72, 0x6f,
        0x77, 0x73, 0x65, 0x72, 0x27, 0x73, 0x20, 0x63, 0x72, 0x79, 0x70, 0x74, 0x6f, 0x2e, 0x73, 0x75,
        0x62, 0x74, 0x6c, 0x65, 0x2e, 0x0a, 0x2f, 0x2a, 0x2a, 0x20, 0x40, 0x69, 0x67, 0x6e, 0x6f, 0x72,
        0x65, 0x20, 0x2a, 0x2f, 0x0a, 0x76, 0x61, 0x72, 0x20, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x61, 0x6e,
        0x74, 0x73, 0x20, 0x3d, 0x20, 0x72, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x28, 0x27, 0x63, 0x6f,
        0x6e, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x73, 0x27, 0x29, 0x3b, 0x20, 0x2f, 0x2a, 0x2a, 0x20, 0x40,
        0x69, 0x67, 0x6e, 0x6f, 0x72, 0x65, 0x20, 0x2a, 0x2f, 0x0a, 0x76, 0x61, 0x72, 0x20, 0x43, 0x72,
        0x79, 0x70, 0x74, 0x6f, 0x20, 0x3d, 0x20, 0x72, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x28, 0x27,
        0x2e, 0x2e, 0x2f, 0x2e, 0x2e, 0x2f, 0x63, 0x72, 0x79, 0x70, 0x74, 0x6f, 0x2e, 0x6a, 0x73, 0x27,
        0x29, 0x3b, 0x20, 0x2f, 0x2a, 0x2a, 0x20, 0x40, 0x69, 0x67, 0x6e, 0x6f, 0x72, 0x65, 0x20, 0x2a,
        0x2f, 0x0a, 0x76, 0x61, 0x72, 0x20, 0x4b, 0x65, 0x79, 0x54, 0x79, 0x70, 0x65, 0x20, 0x3d, 0x20,
        0x72, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x28, 0x27, 0x2e, 0x2e, 0x2f, 0x73, 0x65, 0x63, 0x75,
        0x72, 0x69, 0x74, 0x79, 0x2d, 0x74, 0x79, 0x70, 0x65, 0x73, 0x27, 0x29, 0x2e, 0x4b, 0x65, 0x79,
        0x54, 0x79, 0x70, 0x65, 0x3b, 0x20, 0x2f, 0x2a, 0x2a, 0x20, 0x40, 0x69, 0x67, 0x6e, 0x6f, 0x72,
        0x65, 0x20, 0x2a, 0x2f, 0x0a, 0x76, 0x61, 0x72, 0x20, 0x45, 0x6e, 0x63, 0x72, 0x79, 0x70, 0x74,
        0x41, 0x6c, 0x67, 0x6f, 0x72, 0x69, 0x74, 0x68, 0x6d, 0x54, 0x79, 0x70, 0x65, 0x20, 0x3d, 0x20,
        0x72, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x28, 0x27, 0x2e, 0x2e, 0x2f, 0x2e, 0x2e, 0x2f, 0x65,
        0x6e, 0x63, 0x72, 0x79, 0x70, 0x74, 0x2f, 0x61, 0x6c, 0x67, 0x6f, 0x2f, 0x65, 0x6e, 0x63, 0x72,
        0x79, 0x70, 0x74, 0x2d, 0x70, 0x61, 0x72, 0x61, 0x6d, 0x73, 0x2e, 0x6a, 0x73, 0x27, 0x29, 0x2e,
        0x45, 0x6e, 0x63, 0x72, 0x79, 0x70, 0x74, 0x41, 0x6c, 0x67, 0x6f, 0x72, 0x69, 0x74, 0x68, 0x6d,
        0x54, 0x79, 0x70, 0x65, 0x3b, 0x20, 0x2f, 0x2a, 0x2a, 0x20, 0x40, 0x69, 0x67, 0x6e, 0x6f, 0x72,
        0x65, 0x20, 0x2a, 0x2f, 0x0a, 0x76, 0x61, 0x72, 0x20, 0x44, 0x69, 0x67, 0x65, 0x73, 0x74, 0x41,
        0x6c, 0x67, 0x6f, 0x72, 0x69, 0x74, 0x68, 0x6d, 0x20, 0x3d, 0x20, 0x72, 0x65, 0x71, 0x75, 0x69,
        0x72, 0x65, 0x28, 0x27, 0x2e, 0x2e, 0x2f, 0x73, 0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x2d,
        0x74, 0x79, 0x70, 0x65, 0x73, 0x2e, 0x6a, 0x73, 0x27, 0x29, 0x2e, 0x44, 0x69, 0x67, 0x65, 0x73,
        0x74, 0x41, 0x6c, 0x67, 0x6f, 0x72, 0x69, 0x74, 0x68, 0x6d, 0x3b, 0x20, 0x2f, 0x2a, 0x2a, 0x20,
        0x40, 0x69, 0x67, 0x6e, 0x6f, 0x72, 0x65, 0x20, 0x2a, 0x2f, 0x0a, 0x76, 0x61, 0x72, 0x20, 0x44,
        0x61, 0x74, 0x61, 0x55, 0x74, 0x69, 0x6c, 0x73, 0x20, 0x3d, 0x20, 0x72, 0x65, 0x71, 0x75, 0x69,
        0x72, 0x65, 0x28, 0x27, 0x2e, 0x2e, 0x2f, 0x2e, 0x2e, 0x2f, 0x65, 0x6e, 0x63, 0x6f, 0x64, 0x69,
        0x6e, 0x67, 0x2f, 0x64, 0x61, 0x74, 0x61, 0x2d, 0x75, 0x74, 0x69, 0x6c, 0x73, 0x2e, 0x6a, 0x73,
        0x27, 0x16, 0x2b, 0x1b, 0x01, 0x01, 0x1c, 0x26, 0x07, 0x24, 0x08, 0x09, 0x6c, 0x6f, 0x63, 0x61,
        0x6c, 0x68, 0x6f, 0x73, 0x74, 0x08, 0x08, 0x6f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x6f, 0x72, 0x08,
        0x03, 0x4b, 0x45, 0x59, 0x08, 0x08, 0xfb, 0x5d, 0x48, 0xd6, 0xf6, 0x2a, 0x80, 0x4a, 0x17, 0xfd,
        0x01, 0x00, 0x77, 0x1e, 0x6f, 0x13, 0x53, 0x08, 0x1b, 0xf6, 0x11, 0x2e, 0xaf, 0x82, 0x60, 0x86,
        0xb7, 0x64, 0x42, 0xf5, 0xf5, 0x7e, 0x66, 0xf1, 0xb4, 0x22, 0x51, 0x52, 0xaf, 0x3c, 0x73, 0x87,
        0xed, 0x73, 0xcf, 0xbf, 0x8b, 0x0c, 0x60, 0x61, 0xc7, 0x44, 0x5d, 0x4b, 0xb7, 0x2b, 0x13, 0x3b,
        0xa9, 0xab, 0x1a, 0x35, 0x71, 0x8b, 0x68, 0xd1, 0xf6, 0xa1, 0x10, 0xdd, 0x85, 0x1f, 0x07, 0x56,
        0x99, 0xcb, 0x5e, 0xba, 0x1c, 0x9b, 0x22, 0x34, 0xbd, 0x85, 0x54, 0xf3, 0x21, 0x01, 0xb1, 0x45,
        0x30, 0x98, 0xca, 0xcb, 0x24, 0x76, 0x1b, 0xe9, 0xa3, 0x47, 0x67, 0x3e, 0x27, 0x35, 0x33, 0x68,
        0x77, 0xb2, 0x83, 0x4c, 0xb9, 0x28, 0x42, 0x09, 0xeb, 0xbe, 0x50, 0x7b, 0xbd, 0xf2, 0xbc, 0xf6,
        0xa1, 0xdf, 0x43, 0x09, 0x55, 0x74, 0xb9, 0x55, 0x9f, 0xb2, 0x8f, 0x2b, 0xe5, 0xc6, 0x74, 0x38,
        0x5b, 0x38, 0x38, 0xbf, 0xed, 0x29, 0x4d, 0x9f, 0xaa, 0xcd, 0xef, 0xf4, 0x06, 0x20, 0x29, 0xad,
        0x6a, 0x14, 0xfa, 0x4a, 0xca, 0x9c, 0x8c, 0xe5, 0xc6, 0x98, 0x07, 0xa5, 0x18, 0xaf, 0x39, 0x15,
        0x2b, 0xb8, 0x28, 0x6f, 0xc6, 0x87, 0xc7, 0x03, 0x38, 0xbe, 0x3a, 0xeb, 0x0a, 0x9f, 0xb5, 0x71,
        0xc2, 0xa8, 0xd6, 0xc4, 0xad, 0xe6, 0x4d, 0x8c, 0x74, 0x08, 0x5d, 0x9b, 0xe7, 0xbf, 0xe2, 0xe0,
        0xe8, 0x1f, 0x44, 0x2c, 0x8e, 0xb2, 0x2a, 0x3b, 0x9c, 0xf0, 0xc1, 0xa0, 0xab, 0x8b, 0x2d, 0x66,
        0x07, 0x96, 0xde, 0xc0, 0x2a, 0x24, 0xce, 0x42, 0x5f, 0xcf, 0xd3, 0xc9, 0xc1, 0xc1, 0x83, 0x36,
        0xfd, 0x69, 0x58, 0x9f, 0x5c, 0x3f, 0x57, 0xcc, 0x5f, 0x7d, 0x14, 0x55, 0xa9, 0x35, 0x7f, 0xe3,
        0x9a, 0x36, 0x1a, 0x8b, 0xdc, 0xed, 0x1b, 0xd6, 0x45, 0x66, 0x05, 0x23, 0xa4, 0xda, 0x19, 0x85,
        0xfd, 0xe1
};

static string dump(const string& s1) { return s1; }
static string dump(const string& s1, const string& s2) { return s1 + " " + s2; }

static string
toString(double value)
{
  ostringstream buffer;
  buffer << value;
  return buffer.str();
}

static vector<string>
dumpData(const Data& data)
{
  vector<string> result;

  result.push_back(dump("name:", data.getName().toUri()));
  if (data.getContent().size() > 0) {
    string raw;
    for (size_t i = 0; i < data.getContent().size(); ++i)
      raw += (*data.getContent())[i];
    result.push_back(dump("content (raw):", raw));
    result.push_back(dump("content (hex):", data.getContent().toHex()));
  }
  else
    result.push_back(dump("content: <empty>"));
  if (data.getMetaInfo().getType() != ndn_ContentType_BLOB) {
    result.push_back(dump("metaInfo.type:",
      data.getMetaInfo().getType() == ndn_ContentType_LINK ? "LINK" :
      (data.getMetaInfo().getType() == ndn_ContentType_KEY ? "KEY" :
       "unknown")));
  }
  result.push_back(dump("metaInfo.freshnessPeriod (milliseconds):",
    data.getMetaInfo().getFreshnessPeriod() >= 0 ?
      toString(data.getMetaInfo().getFreshnessPeriod()) : string("<none>")));
  result.push_back(dump("metaInfo.finalBlockId:",
    data.getMetaInfo().getFinalBlockId().getValue().size() > 0 ?
      data.getMetaInfo().getFinalBlockId().toEscapedString() : "<none>"));
  const Sha256WithRsaSignature *signature = dynamic_cast<const Sha256WithRsaSignature*>(data.getSignature());
  if (signature) {
    result.push_back(dump("signature.signature:",
      signature->getSignature().size() == 0 ? "<none>" :
        signature->getSignature().toHex()));
    if ((int)signature->getKeyLocator().getType() >= 0) {
      if (signature->getKeyLocator().getType() ==
          ndn_KeyLocatorType_KEY_LOCATOR_DIGEST)
        result.push_back(dump("signature.keyLocator: KeyLocatorDigest:",
          signature->getKeyLocator().getKeyData().toHex()));
      else if (signature->getKeyLocator().getType() == ndn_KeyLocatorType_KEYNAME)
        result.push_back(dump("signature.keyLocator: KeyName:",
          signature->getKeyLocator().getKeyName().toUri()));
      else
        result.push_back(dump("signature.keyLocator: <unrecognized KeyLocatorType"));
    }
    else
      result.push_back(dump("signature.keyLocator: <none>"));
  }

  return result;
};

static const char* initialDumpValues[] = {
  "name: /ndn/abc",
  "content (raw): SUCCESS!",
  "content (hex): 5355434345535321",
  "metaInfo.freshnessPeriod (milliseconds): 5000",
  "metaInfo.finalBlockId: %00%09",
  "signature.signature: 1a03c39c4fc55c36a2e79cee52fe45a7e10cfb95acb49bccb6a0c34aaa45bfbfdf0b51d5a48bf2ab45971c24d8e2c28a4d4012d77701eb7435f14dddd0f3a69ab7a4f17fa78434d7082552808b6c4293041e071f4f764318f2f8511a56afe6a931cb6c1c0aa40110fcc866ce2e9c0b2d7fb464a0ee2282c834f79af551122a84",
  "signature.keyLocator: KeyName: /testname/KEY/DSK-123/ID-CERT"
};

/**
 * Return a copy of the strings array, removing any string that start with prefix.
 */
static vector<string>
removeStartingWith(const vector<string>& strings, const string& prefix)
{
  vector<string> result;
  for (size_t i = 0; i < strings.size(); ++i) {
    if (strings[i].substr(0, prefix.size()) != prefix)
      result.push_back(strings[i]);
  }

  return result;
}

// ignoring signature, see if two data dumps are equal
static bool
dataDumpsEqual(const vector<string>& d1, const vector<string>& d2)
{
  string prefix("signature.signature:");
  return removeStartingWith(d1, prefix) == removeStartingWith(d2, prefix);
}

class CredentialStorage {
public:
  CredentialStorage()
  : identityStorage_(new MemoryIdentityStorage()),
    privateKeyStorage_(new MemoryPrivateKeyStorage()),
    keyChain_(ptr_lib::make_shared<IdentityManager>(identityStorage_, privateKeyStorage_),
              ptr_lib::make_shared<SelfVerifyPolicyManager>(identityStorage_.get()))
  {
    Name keyName("/testname/DSK-123");
    defaultCertName_ = keyName.getSubName(0, keyName.size() - 1).append
      ("KEY").append(keyName[-1]).append("ID-CERT").append("0");

    identityStorage_->addKey
      (keyName, KEY_TYPE_RSA, Blob(DEFAULT_RSA_PUBLIC_KEY_DER,
       sizeof(DEFAULT_RSA_PUBLIC_KEY_DER)));
    privateKeyStorage_->setKeyPairForKeyName
      (keyName, KEY_TYPE_RSA, DEFAULT_RSA_PUBLIC_KEY_DER,
       sizeof(DEFAULT_RSA_PUBLIC_KEY_DER), DEFAULT_RSA_PRIVATE_KEY_DER,
       sizeof(DEFAULT_RSA_PRIVATE_KEY_DER));

    Name ecdsaKeyName("/testEcdsa/DSK-123");
    ecdsaCertName_ = ecdsaKeyName.getSubName(0, ecdsaKeyName.size() - 1).append
      ("KEY").append(ecdsaKeyName[-1]).append("ID-CERT").append("0");

    identityStorage_->addKey
      (ecdsaKeyName, KEY_TYPE_EC, Blob(DEFAULT_EC_PUBLIC_KEY_DER,
       sizeof(DEFAULT_EC_PUBLIC_KEY_DER)));
    privateKeyStorage_->setKeyPairForKeyName
      (ecdsaKeyName, KEY_TYPE_EC, DEFAULT_EC_PUBLIC_KEY_DER,
       sizeof(DEFAULT_EC_PUBLIC_KEY_DER), DEFAULT_EC_PRIVATE_KEY_DER,
       sizeof(DEFAULT_EC_PRIVATE_KEY_DER));
  }

  void
  signData(Data& data, Name certificateName = Name())
  {
    if (certificateName.size() == 0)
      certificateName = defaultCertName_;
    keyChain_.sign(data, certificateName);
  }

  void
  signDataWithSha256(Data& data)
  {
    keyChain_.signWithSha256(data);
  }

  void
  verifyData
    (const ptr_lib::shared_ptr<Data>& data, const OnVerified& verifiedCallback,
     const OnDataValidationFailed& failedCallback)
  {
    keyChain_.verifyData(data, verifiedCallback, failedCallback);
  }

  const Name&
  getEcdsaCertName() { return ecdsaCertName_; }

private:
  ptr_lib::shared_ptr<MemoryIdentityStorage> identityStorage_;
  ptr_lib::shared_ptr<MemoryPrivateKeyStorage> privateKeyStorage_;
  KeyChain keyChain_;
  Name defaultCertName_;
  Name ecdsaCertName_;
};

class VerifyCounter
{
public:
  VerifyCounter()
  {
    onVerifiedCallCount_ = 0;
    onValidationFailedCallCount_ = 0;
  }

  void
  onVerified(const ptr_lib::shared_ptr<Data>& data)
  {
    ++onVerifiedCallCount_;
  }

  void
  onValidationFailed
    (const ptr_lib::shared_ptr<Data>& data, const string& reason)
  {
    ++onValidationFailedCallCount_;
  }

  int onVerifiedCallCount_;
  int onValidationFailedCallCount_;
};

class TestDataMethods : public ::testing::Test {
public:
  TestDataMethods()
  : initialDump(initialDumpValues,
      initialDumpValues + sizeof(initialDumpValues) / sizeof(initialDumpValues[0])),
    freshData(createFreshData())
  {
  }

  static ptr_lib::shared_ptr<Data>
  createFreshData()
  {
    ptr_lib::shared_ptr<Data> freshData(new Data(Name("/ndn/abc")));
    const uint8_t freshContent[] = "SUCCESS!";
    freshData->setContent(freshContent, sizeof(freshContent) - 1);
    freshData->getMetaInfo().setFreshnessPeriod(5000);
    freshData->getMetaInfo().setFinalBlockId(Name("/%00%09")[0]);

    return freshData;
  }

  vector<string> initialDump;
  CredentialStorage credentials;
  ptr_lib::shared_ptr<Data> freshData;
};

TEST_F(TestDataMethods, Dump)
{
  Data data;
  data.wireDecode(codedData, sizeof(codedData));
  ASSERT_EQ(dumpData(data), initialDump) << "Initial dump does not have expected format";
}

TEST_F(TestDataMethods, EncodeDecode)
{
  Data data;
  data.wireDecode(codedData, sizeof(codedData));
  // Set the content again to clear the cached encoding so we encode again.
  data.setContent(data.getContent());
  Blob encoding = data.wireEncode();

  Data reDecodedData;
  reDecodedData.wireDecode(encoding);
  ASSERT_EQ(dumpData(reDecodedData), initialDump) << "Re-decoded data does not match original dump";
}

TEST_F(TestDataMethods, EmptySignature)
{
  // make sure nothing is set in the signature of newly created data
  Data data;
  const Sha256WithRsaSignature *signature = dynamic_cast<const Sha256WithRsaSignature*>(data.getSignature());
  ASSERT_LT(signature->getKeyLocator().getType(), 0) << "Key locator type on unsigned data should not be set";
  ASSERT_TRUE(signature->getSignature().isNull()) << "Non-empty signature on unsigned data";
}

TEST_F(TestDataMethods, CopyFields)
{
  Data data(freshData->getName());
  data.setContent(freshData->getContent());
  data.setMetaInfo(freshData->getMetaInfo());
  credentials.signData(data);
  vector<string> freshDump = dumpData(data);
  ASSERT_TRUE(dataDumpsEqual(freshDump, initialDump)) << "Freshly created data does not match original dump";
}

TEST_F(TestDataMethods, Verify)
{
  VerifyCounter counter;

  credentials.signData(*freshData);

  credentials.verifyData
    (freshData, bind(&VerifyCounter::onVerified, &counter, _1),
     bind(&VerifyCounter::onValidationFailed, &counter, _1, _2));
  ASSERT_EQ(counter.onValidationFailedCallCount_, 0) << "Signature verification failed";
  ASSERT_EQ(counter.onVerifiedCallCount_, 1) << "Verification callback was not used.";
}

TEST_F(TestDataMethods, VerifyEcdsa)
{
  VerifyCounter counter;

  credentials.signData(*freshData, credentials.getEcdsaCertName());

  credentials.verifyData
    (freshData, bind(&VerifyCounter::onVerified, &counter, _1),
     bind(&VerifyCounter::onValidationFailed, &counter, _1, _2));
  ASSERT_EQ(counter.onValidationFailedCallCount_, 0) << "Signature verification failed";
  ASSERT_EQ(counter.onVerifiedCallCount_, 1) << "Verification callback was not used.";
}

TEST_F(TestDataMethods, VerifyDigestSha256)
{
  VerifyCounter counter;

  credentials.signDataWithSha256(*freshData);

  credentials.verifyData
    (freshData, bind(&VerifyCounter::onVerified, &counter, _1),
     bind(&VerifyCounter::onValidationFailed, &counter, _1, _2));
  ASSERT_EQ(counter.onValidationFailedCallCount_, 0) << "Signature verification failed";
  ASSERT_EQ(counter.onVerifiedCallCount_, 1) << "Verification callback was not used.";
}


TEST_F(TestDataMethods, GenericSignature)
{
  // Test correct encoding.
  ptr_lib::shared_ptr<GenericSignature> signature(new GenericSignature());
  signature->setSignatureInfoEncoding
    (Blob(experimentalSignatureInfo, sizeof(experimentalSignatureInfo)), -1);
  uint8_t signatureValueBytes[] = { 1, 2, 3, 4 };
  Blob signatureValue(signatureValueBytes, sizeof(signatureValueBytes));
  signature->setSignature(signatureValue);

  freshData->setSignature(*signature);
  Blob encoding = freshData->wireEncode();

  Data decodedData;
  decodedData.wireDecode(encoding);

  const GenericSignature *decodedSignature =
    dynamic_cast<const GenericSignature*>(decodedData.getSignature());
  ASSERT_EQ(decodedSignature->getTypeCode(), experimentalSignatureType);
  ASSERT_TRUE(Blob(experimentalSignatureInfo, sizeof(experimentalSignatureInfo)).equals
             (decodedSignature->getSignatureInfoEncoding()));
  ASSERT_TRUE(signatureValue.equals(decodedSignature->getSignature()));

  // Test bad encoding.
  signature.reset(new GenericSignature());
  signature->setSignatureInfoEncoding
    (Blob(experimentalSignatureInfoNoSignatureType, sizeof(experimentalSignatureInfoNoSignatureType)),
     -1);
  signature->setSignature(signatureValue);
  freshData->setSignature(*signature);
  ASSERT_THROW
    (freshData->wireEncode(),
    runtime_error) << "Expected encoding error for experimentalSignatureInfoNoSignatureType";

  signature.reset(new GenericSignature());
  signature->setSignatureInfoEncoding
    (Blob(experimentalSignatureInfoBadTlv, sizeof(experimentalSignatureInfoBadTlv)),
     -1);
  signature->setSignature(signatureValue);
  freshData->setSignature(*signature);
  ASSERT_THROW
    (freshData->wireEncode(),
    runtime_error) << "Expected encoding error for experimentalSignatureInfoBadTlv";
}

TEST_F(TestDataMethods, FullName)
{
  Data data;
  data.wireDecode(codedData, sizeof(codedData));

  // Check the full name format.
  ASSERT_EQ(data.getName().size() + 1, data.getFullName()->size());
  ASSERT_EQ(data.getName(), data.getFullName()->getPrefix(-1));
  ASSERT_EQ(32, data.getFullName()->get(-1).getValue().size());

  // Check the independent digest calculation.
  uint8_t newDigest[ndn_SHA256_DIGEST_SIZE];
  CryptoLite::digestSha256(codedData, sizeof(codedData), newDigest);
  ASSERT_TRUE(Blob(newDigest, sizeof(newDigest)).equals
                   (data.getFullName()->get(-1).getValue()));

  // Check the expected URI.
  ASSERT_EQ
    ("/ndn/abc/sha256digest="
       "96556d685dcb1af04be4ae57f0e7223457d4055ea9b3d07c0d337bef4a8b3ee9",
     data.getFullName()->toUri());

  // Changing the Data packet should change the full name.
  Name saveFullName(*data.getFullName());
  data.setContent(Blob());
  ASSERT_FALSE(data.getFullName()->get(-1).equals(saveFullName.get(-1)));
}


TEST_F(TestDataMethods, CongestionMark)
{
  // Imitate onReceivedElement.
  struct ndn_LpPacketHeaderField headerFields[5];
  LpPacketLite lpPacketLite
    (headerFields, sizeof(headerFields) / sizeof(headerFields[0]));

  ndn_Error error;
  if ((error = Tlv0_2WireFormatLite::decodeLpPacket
       (lpPacketLite, CONGESTION_MARK_PACKET, sizeof(CONGESTION_MARK_PACKET))))
    throw runtime_error(ndn_getErrorString(error));
  const uint8_t *element = lpPacketLite.getFragmentWireEncoding().buf();
  size_t elementLength = lpPacketLite.getFragmentWireEncoding().size();

  // We have saved the wire encoding, so clear to copy it to lpPacket.
  lpPacketLite.setFragmentWireEncoding(BlobLite());

  ptr_lib::shared_ptr<LpPacket> lpPacket(new LpPacket());
  lpPacket->set(lpPacketLite);

  ptr_lib::shared_ptr<Data> data(new Data());
  data->wireDecode(element, elementLength, *TlvWireFormat::get());
  data->setLpPacket(lpPacket);

  ASSERT_EQ(1, data->getCongestionMark());
}

int
main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
