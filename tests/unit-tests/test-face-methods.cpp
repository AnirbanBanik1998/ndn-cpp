/**
 * Copyright (C) 2014-2017 Regents of the University of California.
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
#include <ndn-cpp/face.hpp>
#include <sstream>
#if NDN_CPP_HAVE_TIME_H
#include <time.h>
#endif
#if NDN_CPP_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <ndn-cpp/security/identity/memory-identity-storage.hpp>
#include <ndn-cpp/security/identity/memory-private-key-storage.hpp>
#include <ndn-cpp/security/policy/no-verify-policy-manager.hpp>
#include <ndn-cpp/security/key-chain.hpp>

using namespace std;
using namespace ndn;
using namespace ndn::func_lib;

static MillisecondsSince1970
getNowMilliseconds()
{
  struct timeval t;
  // Note: configure.ac requires gettimeofday.
  gettimeofday(&t, 0);
  return t.tv_sec * 1000.0 + t.tv_usec / 1000.0;
}

class CallbackCounter
{
public:
  CallbackCounter()
  {
    onDataCallCount_ = 0;
    onTimeoutCallCount_ = 0;
    onNetworkNackCallCount_ = 0;
  }

  void
  onData(const ptr_lib::shared_ptr<const Interest>& interest,
         const ptr_lib::shared_ptr<Data>& data)
  {
    interest_ = *interest;
    data_ = *data;
    ++onDataCallCount_;
  }

  void
  onTimeout(const ptr_lib::shared_ptr<const Interest>& interest)
  {
    interest_ = *interest;
    ++onTimeoutCallCount_;
  }

  void
  onNetworkNack(const ptr_lib::shared_ptr<const Interest>& interest,
                const ptr_lib::shared_ptr<NetworkNack>& networkNack)
  {
    interest_ = *interest;
    networkNack_ = *networkNack;
    ++onNetworkNackCallCount_;
  }

  int onDataCallCount_;
  int onTimeoutCallCount_;
  int onNetworkNackCallCount_;
  Interest interest_;
  Data data_;
  NetworkNack networkNack_;

};

class RegisterCounter
{
public:
  RegisterCounter(KeyChain& keyChain, Name certificateName)
  : keyChain_(keyChain), certificateName_(certificateName)
  {
    onInterestCallCount_ = 0;
    onRegisterFailedCallCount_ = 0;
  }

  void
  onInterest
    (const ptr_lib::shared_ptr<const Name>& prefix,
     const ptr_lib::shared_ptr<const Interest>& interest, Face& face,
     uint64_t interestFilterId,
     const ptr_lib::shared_ptr<const InterestFilter>& filter)
  {
    ++onInterestCallCount_;

    Data data(interest->getName());
    string content("SUCCESS");
    data.setContent((const uint8_t *)&content[0], content.size());
    keyChain_.sign(data, certificateName_);
    face.putData(data);
  }

  void
  onRegisterFailed(const ptr_lib::shared_ptr<const Name>& prefix)
  {
    ++onRegisterFailedCallCount_;
  }

  KeyChain& keyChain_;
  Name certificateName_;
  int onInterestCallCount_;
  int onRegisterFailedCallCount_;
};

// Returns a CallbackCounter object so we can test data callback and timeout behavior.
CallbackCounter
runExpressNameTest
  (Face& face, const string& interestName, Milliseconds timeout = 10000,
   bool useOnNack = false)
{
  Name name(interestName);
  CallbackCounter counter;
  if (useOnNack)
    face.expressInterest
      (name, bind(&CallbackCounter::onData, &counter, _1, _2),
       bind(&CallbackCounter::onTimeout, &counter, _1),
       bind(&CallbackCounter::onNetworkNack, &counter, _1, _2));
  else
    face.expressInterest
      (name, bind(&CallbackCounter::onData, &counter, _1, _2),
       bind(&CallbackCounter::onTimeout, &counter, _1));

  MillisecondsSince1970 startTime = getNowMilliseconds();
  while (getNowMilliseconds() - startTime < timeout &&
         counter.onDataCallCount_ == 0 && counter.onTimeoutCallCount_ == 0) {
    face.processEvents();
    // We need to sleep for a few milliseconds so we don't use 100% of the CPU.
    usleep(10000);
  }

  return counter;
}

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

class TestFaceRegisterMethods : public ::testing::Test {
public:
  TestFaceRegisterMethods()
  : identityStorage(new MemoryIdentityStorage()),
    privateKeyStorage(new MemoryPrivateKeyStorage()),
    keyChain(ptr_lib::make_shared<IdentityManager>(identityStorage, privateKeyStorage),
             ptr_lib::make_shared<NoVerifyPolicyManager>())
  {
    Name keyName("/testname/DSK-123");
    certificateName = keyName.getSubName(0, keyName.size() - 1).append
      ("KEY").append(keyName[-1]).append("ID-CERT").append("0");

    identityStorage->addKey
      (keyName, KEY_TYPE_RSA, Blob(DEFAULT_RSA_PUBLIC_KEY_DER,
       sizeof(DEFAULT_RSA_PUBLIC_KEY_DER)));
    privateKeyStorage->setKeyPairForKeyName
      (keyName, KEY_TYPE_RSA, DEFAULT_RSA_PUBLIC_KEY_DER,
       sizeof(DEFAULT_RSA_PUBLIC_KEY_DER), DEFAULT_RSA_PRIVATE_KEY_DER,
       sizeof(DEFAULT_RSA_PRIVATE_KEY_DER));

    faceIn.setCommandSigningInfo(keyChain, certificateName);
    faceOut.setCommandSigningInfo(keyChain, certificateName);
  }

  virtual void
  TearDown()
  {
    faceIn.shutdown();
    faceOut.shutdown();
  }

  Face faceIn;
  Face faceOut;
  ptr_lib::shared_ptr<MemoryIdentityStorage> identityStorage;
  ptr_lib::shared_ptr<MemoryPrivateKeyStorage> privateKeyStorage;
  KeyChain keyChain;
  Name certificateName;
};

TEST_F(TestFaceRegisterMethods, RegisterPrefixResponse)
{
  Name prefixName("/test");

  RegisterCounter registerCounter(keyChain, certificateName);

  faceIn.registerPrefix
    (prefixName,
     bind(&RegisterCounter::onInterest, &registerCounter, _1, _2, _3, _4, _5),
     bind(&RegisterCounter::onRegisterFailed, &registerCounter, _1));

  // Give the "server" time to register the interest.
  Milliseconds timeout = 1000;
  MillisecondsSince1970 startTime = getNowMilliseconds();
  while (getNowMilliseconds() - startTime < timeout) {
    faceIn.processEvents();
    usleep(10000);
  }

  // Now express an interest on this new face, and see if onInterest is called.
  CallbackCounter counter;
  // Add the timestamp so it is unique and we don't get a cached response.
  ostringstream component;
  component << "hello" << (uint64_t)getNowMilliseconds();
  Name interestName = Name(prefixName).append(component.str());
  faceOut.expressInterest
    (interestName, bind(&CallbackCounter::onData, &counter, _1, _2),
     bind(&CallbackCounter::onTimeout, &counter, _1));

  // Process events for the in and out faces.
  timeout = 10000;
  startTime = getNowMilliseconds();
  while (getNowMilliseconds() - startTime < timeout) {
    faceIn.processEvents();
    faceOut.processEvents();

    bool done = true;
    if (registerCounter.onInterestCallCount_ == 0 &&
        registerCounter.onRegisterFailedCallCount_ == 0)
      // Still processing faceIn.
      done = false;
    if (counter.onDataCallCount_ == 0 && counter.onTimeoutCallCount_ == 0)
      // Still processing face_out.
      done = false;

    if (done)
      break;

    usleep(10000);
  }

  ASSERT_EQ(registerCounter.onRegisterFailedCallCount_, 0) <<
            "Failed to register prefix at all";
  ASSERT_EQ(registerCounter.onInterestCallCount_, 1) <<
            "Expected 1 onInterest callback";
  ASSERT_EQ(counter.onDataCallCount_, 1) <<
            "Expected 1 onData callback";

  // Check the message content.
  Data& data = counter.data_;
  string content("SUCCESS");
  Blob expectedBlob((const uint8_t *)&content[0], content.size());
  ASSERT_TRUE(expectedBlob.equals(data.getContent())) <<
              "Data received on face does not match expected format";
}

class TestFaceInterestMethods : public ::testing::Test {
public:
  TestFaceInterestMethods()
  : face("localhost")
  {
  }

  virtual void
  TearDown()
  {
    face.shutdown();
  }

  Face face;
};

TEST_F(TestFaceInterestMethods, AnyInterest)
{
  string uri = "/";
  CallbackCounter counter = runExpressNameTest(face, uri);
  ASSERT_TRUE(counter.onTimeoutCallCount_ == 0) << "Timeout on expressed interest";

  // check that the callback was correct
  ASSERT_EQ(counter.onDataCallCount_, 1) << "Expected 1 onData callback, got " << counter.onDataCallCount_;

  // just check that the interest was returned correctly?
  const Interest& callbackInterest = counter.interest_;
  ASSERT_TRUE(callbackInterest.getName().equals(Name(uri))) << "Interest returned on callback had different name";
}

/*
TODO: Replace this with a test that connects to a Face on localhost
def test_specific_interest(self):
  uri = "/ndn/edu/ucla/remap/ndn-js-test/howdy.txt/%FD%052%A1%DF%5E%A4"
  (dataCallback, timeoutCallback) = self.run_express_name_test(uri)
  self.assertTrue(timeoutCallback.call_count == 0, 'Unexpected timeout on expressed interest')

  // check that the callback was correct
  self.assertEqual(dataCallback.call_count, 1, 'Expected 1 onData callback, got '+str(dataCallback.call_count))

  onDataArgs = dataCallback.call_args[0] # the args are returned as ([ordered arguments], [keyword arguments])

  // just check that the interest was returned correctly?
  callbackInterest = onDataArgs[0]
  self.assertTrue(callbackInterest.getName().equals(Name(uri)), 'Interest returned on callback had different name')
*/

TEST_F(TestFaceInterestMethods, Timeout)
{
  string uri = "/test123/timeout";
  CallbackCounter counter = runExpressNameTest(face, uri);

  // we're expecting a timeout callback, and only 1
  ASSERT_EQ(counter.onDataCallCount_, 0) << "Data callback called for invalid interest";

  ASSERT_TRUE(counter.onTimeoutCallCount_ == 1) << "Expected 1 timeout call, got " << counter.onTimeoutCallCount_;

  // just check that the interest was returned correctly?
  const Interest& callbackInterest = counter.interest_;
  ASSERT_TRUE(callbackInterest.getName().equals(Name(uri))) << "Interest returned on callback had different name";
}

TEST_F(TestFaceInterestMethods, RemovePending)
{
  Name name("/ndn/edu/ucla/remap/");
  CallbackCounter counter;
  uint64_t interestID = face.expressInterest
    (name, bind(&CallbackCounter::onData, &counter, _1, _2),
     bind(&CallbackCounter::onTimeout, &counter, _1));

  face.removePendingInterest(interestID);

  Milliseconds timeout = 10000;
  MillisecondsSince1970 startTime = getNowMilliseconds();
  while (getNowMilliseconds() - startTime < timeout &&
         counter.onDataCallCount_ == 0 && counter.onTimeoutCallCount_ == 0) {
    face.processEvents();
    // We need to sleep for a few milliseconds so we don't use 100% of the CPU.
    usleep(10000);
  }

  ASSERT_EQ(counter.onDataCallCount_, 0) << "Should not have called data callback after interest was removed";
  ASSERT_TRUE(counter.onTimeoutCallCount_ == 0) << "Should not have called timeout callback after interest was removed";
}

TEST_F(TestFaceInterestMethods, MaxNdnPacketSize)
{
  // Construct an interest whose encoding is one byte larger than getMaxNdnPacketSize.
  const size_t targetSize = Face::getMaxNdnPacketSize() + 1;
  // Start with an interest which is almost the right size.
  uint8_t componentValue[targetSize];
  Interest interest;
  interest.getName().append(componentValue, targetSize);
  size_t initialSize = interest.wireEncode().size();
  // Now replace the component with the desired size which trims off the extra encoding.
  interest.setName
    (Name().append(componentValue, targetSize - (initialSize - targetSize)));
  size_t interestSize = interest.wireEncode().size();
  ASSERT_EQ(targetSize, interestSize) << "Wrong interest size for MaxNdnPacketSize";
  
  CallbackCounter counter;
  ASSERT_THROW
    (face.expressInterest
     (interest, bind(&CallbackCounter::onData, &counter, _1, _2),
      bind(&CallbackCounter::onTimeout, &counter, _1)),
     runtime_error) <<
    "expressInterest didn't throw an exception when the interest size exceeds getMaxNdnPacketSize()";
}

TEST_F(TestFaceInterestMethods, NetworkNack)
{
  ostringstream uri;
  uri << "/noroute" << getNowMilliseconds();
    // Use a short timeout since we expect an immediate Nack.
  CallbackCounter counter = runExpressNameTest(face, uri.str(), 1000, true);

  // We're expecting a network Nack callback, and only 1.
  ASSERT_EQ(0, counter.onDataCallCount_) <<
            "Data callback called for unroutable interest";
  ASSERT_EQ(0, counter.onTimeoutCallCount_) <<
            "Timeout callback called for unroutable interest";
  ASSERT_EQ(1, counter.onNetworkNackCallCount_) <<
            "Expected 1 network Nack call";

  ASSERT_EQ(counter.networkNack_.getReason(), ndn_NetworkNackReason_NO_ROUTE) <<
            "Network Nack has unexpected reason";
}

int
main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
