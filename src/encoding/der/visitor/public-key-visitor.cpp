/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013-2014 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include "simple-visitor.hpp"
#include <ndn-cpp/security/certificate/public-key.hpp>
#include "../der.hpp"
#include "public-key-visitor.hpp"

using namespace std;

namespace ndn {

namespace der {
  
ndnboost::any 
PublicKeyVisitor::visit(DerSequence& derSeq)
{
  DerNodePtrList& children = derSeq.getChildren();

  SimpleVisitor simpleVisitor;
  ptr_lib::shared_ptr<DerSequence> algoSeq = ptr_lib::dynamic_pointer_cast<DerSequence>(children[0]); 
  Blob raw = derSeq.getRaw();   
  // TODO: Determine and pass the correct KeyType.
  return ndnboost::any(ptr_lib::shared_ptr<PublicKey>(new PublicKey(KEY_TYPE_RSA, raw)));    
}

} // der

}
