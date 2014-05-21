/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013-2014 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * A copy of the GNU General Public License is in the file COPYING.
 */

// We can use ndnboost::iostreams because this is internal and will not conflict with the application if it uses boost::iostreams.
#include <ndnboost/iostreams/stream.hpp>
#include <ndnboost/iostreams/device/array.hpp>
#include <ndn-cpp/security//security-exception.hpp>
#include "../../c/util/crypto.h"
#include "../../encoding/der/der.hpp"
#include <ndn-cpp/security/certificate/public-key.hpp>

using namespace std;

namespace ndn {

ptr_lib::shared_ptr<der::DerNode>
PublicKey::toDer()
{
  ndnboost::iostreams::stream<ndnboost::iostreams::array_source> is((const char*)keyDer_.buf (), keyDer_.size ());

  return der::DerNode::parse(reinterpret_cast<der::InputIterator&> (is));
}

static int RSA_OID[] = { 1, 2, 840, 113549, 1, 1, 1 };

ptr_lib::shared_ptr<PublicKey>
PublicKey::fromDer(const Blob& keyDer)
{
  // Use a temporary pointer since d2i updates it.
  const uint8_t *derPointer = keyDer.buf();
  RSA *publicKey = d2i_RSA_PUBKEY(NULL, &derPointer, keyDer.size());
  if (!publicKey)
    throw UnrecognizedKeyFormatException("Error decoding public key DER");  
  RSA_free(publicKey);
  
  return ptr_lib::shared_ptr<PublicKey>(new PublicKey(OID(vector<int>(RSA_OID, RSA_OID + sizeof(RSA_OID))), keyDer));
}

Blob
PublicKey::getDigest(DigestAlgorithm digestAlgorithm) const
{
  if (digestAlgorithm == DIGEST_ALGORITHM_SHA256) {
    uint8_t digest[SHA256_DIGEST_LENGTH];
    ndn_digestSha256(keyDer_.buf(), keyDer_.size(), digest);
    
    return Blob(digest, sizeof(digest));
  }
  else
    throw UnrecognizedDigestAlgorithmException("Wrong format!");
}

}
