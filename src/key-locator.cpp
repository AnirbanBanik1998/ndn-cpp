/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include <ndn-cpp/common.hpp>
#include <ndn-cpp/key-locator.hpp>
#include "c/key-locator.h"

using namespace std;

namespace ndn {
  
void 
KeyLocator::get(struct ndn_KeyLocator& keyLocatorStruct) const 
{
  keyLocatorStruct.type = type_;
  keyData_.get(keyLocatorStruct.keyData);
  keyName_.get().get(keyLocatorStruct.keyName);
  keyLocatorStruct.keyNameType = keyNameType_;
}

void 
KeyLocator::set(const struct ndn_KeyLocator& keyLocatorStruct)
{
  setType(keyLocatorStruct.type);
  setKeyData(Blob(keyLocatorStruct.keyData));
  if (keyLocatorStruct.type == ndn_KeyLocatorType_KEYNAME) {
    keyName_.get().set(keyLocatorStruct.keyName);
    setKeyNameType(keyLocatorStruct.keyNameType);
  }
  else {
    keyName_.get().clear();
    setKeyNameType((ndn_KeyNameType)-1);
  }
}

}

