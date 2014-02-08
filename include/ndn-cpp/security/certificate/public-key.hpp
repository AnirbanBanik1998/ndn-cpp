/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013-2014 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#ifndef NDN_PUBLIC_KEY_HPP
#define NDN_PUBLIC_KEY_HPP

#include "../../util/blob.hpp"
#include "../../encoding/oid.hpp"
#include "../security-common.hpp"

namespace ndn {

  namespace der { class DerNode; }

class PublicKey {
public:    
  /**
   * The default constructor.
   */
  PublicKey() {}

  /**
   * Create a new PublicKey with the given values.
   * @param algorithm The algorithm of the public key.
   * @param keyDer The blob of the PublicKeyInfo in terms of DER.
   */
  PublicKey(const OID& algorithm, const Blob& keyDer)
  : algorithm_(algorithm), keyDer_(keyDer)
  {
  }

  /**
   * Encode the public key into DER.
   * @return the encoded DER syntax tree.
   */
  ptr_lib::shared_ptr<der::DerNode>
  toDer();

  /**
   * Decode the public key from DER blob.
   * @param keyDer The DER blob.
   * @return The decoded public key.
   */
  static ptr_lib::shared_ptr<PublicKey>
  fromDer(const Blob& keyDer);

  /*
   * Get the digest of the public key.
   * @param digestAlgorithm The digest algorithm. If omitted, use DIGEST_ALGORITHM_SHA256 by default.
   */
  Blob 
  getDigest(DigestAlgorithm digestAlgorithm = DIGEST_ALGORITHM_SHA256) const;

  /*
   * Get the raw bytes of the public key in DER format.
   */
  const Blob& 
  getKeyDer() const { return keyDer_; }
    
private:
  OID algorithm_; /**< Algorithm */
  Blob keyDer_;   /**< PublicKeyInfo in DER */
};

}

#endif
