/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013-2014 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include <stdexcept>
#include <ndn-cpp/common.hpp>
#include <ndn-cpp/forwarding-entry.hpp>
#include "c/forwarding-entry.h"

using namespace std;

namespace ndn {
  
void 
ForwardingEntry::set(const struct ndn_ForwardingEntry& forwardingEntryStruct) 
{
  if (forwardingEntryStruct.action.value && forwardingEntryStruct.action.length > 0)
    action_ = string(forwardingEntryStruct.action.value, forwardingEntryStruct.action.value + forwardingEntryStruct.action.length);
  else
    action_ = "";
            
  prefix_.set(forwardingEntryStruct.prefix);
  publisherPublicKeyDigest_.set(forwardingEntryStruct.publisherPublicKeyDigest);  
  faceId_ = forwardingEntryStruct.faceId;
  forwardingFlags_ = forwardingEntryStruct.forwardingFlags;
  freshnessSeconds_ = forwardingEntryStruct.freshnessSeconds;
}

void 
ForwardingEntry::get(struct ndn_ForwardingEntry& forwardingEntryStruct) const 
{
  prefix_.get(forwardingEntryStruct.prefix);
  publisherPublicKeyDigest_.get(forwardingEntryStruct.publisherPublicKeyDigest);
  forwardingEntryStruct.faceId = faceId_;
  forwardingEntryStruct.forwardingFlags = forwardingFlags_;
  forwardingEntryStruct.freshnessSeconds = freshnessSeconds_;

  forwardingEntryStruct.action.length = action_.size();
  if (action_.size() > 0)
    forwardingEntryStruct.action.value = (uint8_t *)&action_[0];
  else
    forwardingEntryStruct.action.value = 0;
}

}
