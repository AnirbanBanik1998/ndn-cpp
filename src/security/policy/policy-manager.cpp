/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2014 Regents of the University of California.
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

#include "../../c/util/crypto.h"
#include <ndn-cpp/security/security-exception.hpp>
#include <ndn-cpp/sha256-with-rsa-signature.hpp>
#include <ndn-cpp/security/policy/policy-manager.hpp>

using namespace std;

namespace ndn {

bool
PolicyManager::verifySha256WithRsaSignature
  (const Sha256WithRsaSignature* signature, const SignedBlob& signedBlob,
   const Blob& publicKeyDer)
{
  // Set signedPortionDigest to the digest of the signed portion of the wire encoding.
  uint8_t signedPortionDigest[SHA256_DIGEST_LENGTH];
  // wireEncode returns the cached encoding if available.
  ndn_digestSha256
    (signedBlob.signedBuf(), signedBlob.signedSize(), signedPortionDigest);

  // Verify the signedPortionDigest.
  // Use a temporary pointer since d2i updates it.
  const uint8_t *derPointer = publicKeyDer.buf();
  RSA *rsaPublicKey = d2i_RSA_PUBKEY(NULL, &derPointer, publicKeyDer.size());
  if (!rsaPublicKey)
    throw UnrecognizedKeyFormatException("Error decoding public key in d2i_RSAPublicKey");
  int success = RSA_verify
    (NID_sha256, signedPortionDigest, sizeof(signedPortionDigest), (uint8_t *)signature->getSignature().buf(),
     signature->getSignature().size(), rsaPublicKey);
  // Free the public key before checking for success.
  RSA_free(rsaPublicKey);

  // RSA_verify returns 1 for a valid signature.
  return (success == 1);
}


}