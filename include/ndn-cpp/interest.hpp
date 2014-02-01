/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#ifndef NDN_INTEREST_HPP
#define NDN_INTEREST_HPP

#include "name.hpp"
#include "publisher-public-key-digest.hpp"
#include "key-locator.hpp"
#include "c/interest-types.h"
#include "encoding/wire-format.hpp"
#include "util/change-counter.hpp"
#include "exclude.hpp"

struct ndn_Interest;

namespace ndn {
  
/**
 * An Interest holds a Name and other fields for an interest.
 */
class Interest {
public:    
  /**
   * Create a new Interest for the given name and values.
   * @deprecated This constructor sets the nonce which is deprecated because you should let let the wire encoder 
   * generate a random nonce internally before sending the interest.
   */
  Interest(const Name& name, int minSuffixComponents, int maxSuffixComponents, 
    const PublisherPublicKeyDigest& publisherPublicKeyDigest, const Exclude& exclude, int childSelector, int answerOriginKind, 
    int scope, Milliseconds interestLifetimeMilliseconds, const Blob& nonce) 
  : name_(name), minSuffixComponents_(minSuffixComponents), maxSuffixComponents_(maxSuffixComponents),
    publisherPublicKeyDigest_(publisherPublicKeyDigest), exclude_(exclude), childSelector_(childSelector), 
    answerOriginKind_(answerOriginKind), scope_(scope), interestLifetimeMilliseconds_(interestLifetimeMilliseconds),
    nonce_(nonce), getNonceChangeCount_(0), changeCount_(0)
  {
  }

  /**
   * Create a new Interest with the given name and values, and "none" for the nonce and keyLocator.
   * @deprecated You should use the constructor which has KeyLocator instead of the deprecated PublisherPublicKeyDigest.
   */
  Interest(const Name& name, int minSuffixComponents, int maxSuffixComponents, 
    const PublisherPublicKeyDigest& publisherPublicKeyDigest, const Exclude& exclude, int childSelector, int answerOriginKind, 
    int scope, Milliseconds interestLifetimeMilliseconds) 
  : name_(name), minSuffixComponents_(minSuffixComponents), maxSuffixComponents_(maxSuffixComponents),
    publisherPublicKeyDigest_(publisherPublicKeyDigest), exclude_(exclude), childSelector_(childSelector), 
    answerOriginKind_(answerOriginKind), scope_(scope), interestLifetimeMilliseconds_(interestLifetimeMilliseconds),
    getNonceChangeCount_(0), changeCount_(0)
  {
  }

  /**
   * Create a new Interest with the given name and values, and "none" for the nonce.
   */
  Interest(const Name& name, int minSuffixComponents, int maxSuffixComponents, 
    const KeyLocator& keyLocator, const Exclude& exclude, int childSelector, int answerOriginKind, 
    int scope, Milliseconds interestLifetimeMilliseconds) 
  : name_(name), minSuffixComponents_(minSuffixComponents), maxSuffixComponents_(maxSuffixComponents),
    keyLocator_(keyLocator), exclude_(exclude), childSelector_(childSelector), 
    answerOriginKind_(answerOriginKind), scope_(scope), interestLifetimeMilliseconds_(interestLifetimeMilliseconds),
    getNonceChangeCount_(0), changeCount_(0)
  {
  }

  /**
   * Create a new Interest with the given name and interest lifetime and "none" for other values.
   * @param name The name for the interest.
   * @param interestLifetimeMilliseconds The interest lifetime in milliseconds, or -1 for none.
   */
  Interest(const Name& name, Milliseconds interestLifetimeMilliseconds) 
  : name_(name), getNonceChangeCount_(0), changeCount_(0)
  {
    construct();
    interestLifetimeMilliseconds_ = interestLifetimeMilliseconds;
  }

  /**
   * Create a new Interest with the given name and "none" for other values.
   * @param name The name for the interest.
   */
  Interest(const Name& name) 
  : name_(name), getNonceChangeCount_(0), changeCount_(0)
  {
    construct();
  }

  /**
   * Create a new Interest with an empty name and "none" for all values.
   */
  Interest() 
  : getNonceChangeCount_(0), changeCount_(0)
  {
    construct();
  }
  
  /**
   * Encode this Interest for a particular wire format.
   * @param wireFormat A WireFormat object used to decode the input. If omitted, use WireFormat::getDefaultWireFormat().
   * @return The encoded byte array.
   */
  Blob 
  wireEncode(WireFormat& wireFormat = *WireFormat::getDefaultWireFormat()) const 
  {
    return wireFormat.encodeInterest(*this);
  }
  
  /**
   * Decode the input using a particular wire format and update this Interest.
   * @param input The input byte array to be decoded.
   * @param inputLength The length of input.
   * @param wireFormat A WireFormat object used to decode the input. If omitted, use WireFormat::getDefaultWireFormat().
   */
  void 
  wireDecode(const uint8_t *input, size_t inputLength, WireFormat& wireFormat = *WireFormat::getDefaultWireFormat()) 
  {
    wireFormat.decodeInterest(*this, input, inputLength);
  }
  
  /**
   * Decode the input using a particular wire format and update this Interest.
   * @param input The input byte array to be decoded.
   * @param wireFormat A WireFormat object used to decode the input. If omitted, use WireFormat::getDefaultWireFormat().
   */
  void 
  wireDecode(const std::vector<uint8_t>& input, WireFormat& wireFormat = *WireFormat::getDefaultWireFormat()) 
  {
    wireDecode(&input[0], input.size(), wireFormat);
  }
  
  /**
   * Encode the name according to the "NDN URI Scheme".  If there are interest selectors, append "?" and
   * added the selectors as a query string.  For example "/test/name?ndn.ChildSelector=1".
   * @note This is an experimental feature.  See the API docs for more detail at http://named-data.net/doc/ndn-ccl-api .
   * @return The URI string.
   */
  std::string
  toUri() const;
  
  /**
   * Set the interestStruct to point to the components in this interest, without copying any memory.
   * WARNING: The resulting pointers in interestStruct are invalid after a further use of this object which could reallocate memory.
   * @param interestStruct a C ndn_Interest struct where the name components array is already allocated.
   */
  void 
  get(struct ndn_Interest& interestStruct) const;

  Name& 
  getName() { return name_.get(); }
  
  const Name& 
  getName() const { return name_.get(); }
  
  int 
  getMinSuffixComponents() const { return minSuffixComponents_; }
  
  int 
  getMaxSuffixComponents() const { return maxSuffixComponents_; }
 
  /**
   * @deprecated.  The Interest publisherPublicKeyDigest is deprecated.  If you need a publisher public key digest, 
   * set the keyLocator keyLocatorType to KEY_LOCATOR_DIGEST and set its key data to the digest.
   */
  PublisherPublicKeyDigest& 
  getPublisherPublicKeyDigest() { return publisherPublicKeyDigest_.get(); }
  
  /**
   * @deprecated.  The Interest publisherPublicKeyDigest is deprecated.  If you need a publisher public key digest, 
   * set the keyLocator keyLocatorType to KEY_LOCATOR_DIGEST and set its key data to the digest.
   */
  const PublisherPublicKeyDigest& 
  getPublisherPublicKeyDigest() const { return publisherPublicKeyDigest_.get(); }

  const KeyLocator& 
  getKeyLocator() const { return keyLocator_.get(); }
  
  KeyLocator& 
  getKeyLocator() { return keyLocator_.get(); }
  
  Exclude& 
  getExclude() { return exclude_.get(); }
  
  const Exclude& 
  getExclude() const { return exclude_.get(); }
  
  int 
  getChildSelector() const { return childSelector_; }

  /**
   * @deprecated Use getMustBeFresh.
   */
  int 
  getAnswerOriginKind() const { return answerOriginKind_; }
  
  /**
   * Return true if the content must be fresh.
   * @return true if must be fresh, otherwise false.
   */
  bool
  getMustBeFresh() const
  {
    return answerOriginKind_ >= 0 && (answerOriginKind_ & ndn_Interest_ANSWER_STALE) == 0;
  }

  int 
  getScope() const { return scope_; }

  Milliseconds 
  getInterestLifetimeMilliseconds() const { return interestLifetimeMilliseconds_; }

  /**
   * Return the nonce value from the incoming interest.  If you change any of the fields in this Interest object,
   * then the nonce value is cleared.
   * @return 
   */
  const Blob& 
  getNonce() const 
  { 
    if (getNonceChangeCount_ != getChangeCount()) {
      // The values have changed, so the existing nonce is invalidated.
      // This method can be called on a const object, but we want to be able to update the default cached value.
      const_cast<Interest*>(this)->nonce_ = Blob();
      const_cast<Interest*>(this)->getNonceChangeCount_ = getChangeCount();
    }
    
    return nonce_; 
  }
  
  /**
   * Clear this interest, and set the values by copying from the interest struct.
   * @param interestStruct a C ndn_Interest struct
   */
  void 
  set(const struct ndn_Interest& interestStruct);
  
  void
  setName(const Name& name) 
  { 
    name_.set(name);
    ++changeCount_;
  }
  
  void 
  setMinSuffixComponents(int minSuffixComponents) 
  { 
    minSuffixComponents_ = minSuffixComponents; 
    ++changeCount_;
  }
  
  void 
  setMaxSuffixComponents(int maxSuffixComponents) 
  { 
    maxSuffixComponents_ = maxSuffixComponents; 
    ++changeCount_;
  }
  
  void 
  setChildSelector(int childSelector) 
  { 
    childSelector_ = childSelector; 
    ++changeCount_;
  }

  /**
   * @deprecated Use setMustBeFresh.
   */
  void 
  setAnswerOriginKind(int answerOriginKind) 
  { 
    answerOriginKind_ = answerOriginKind; 
    ++changeCount_;
  }

  /**
   * Set the MustBeFresh flag.
   * @param mustBeFresh True if the content must be fresh, otherwise false.
   */
  void setMustBeFresh(bool mustBeFresh)
  {
    if (answerOriginKind_ < 0) {
      // It is is already the default where MustBeFresh is false.
      if (mustBeFresh)
        setAnswerOriginKind(0);
    }
    else {
      if (mustBeFresh)
        // Clear the stale bit.
        answerOriginKind_ &= ~ndn_Interest_ANSWER_STALE;
      else
        // Set the stale bit.
        answerOriginKind_ |= ndn_Interest_ANSWER_STALE;
      ++changeCount_;
    }
  }
  
  void 
  setScope(int scope) 
  { 
    scope_ = scope; 
    ++changeCount_;
  }

  void 
  setInterestLifetimeMilliseconds(Milliseconds interestLifetimeMilliseconds) 
  { 
    interestLifetimeMilliseconds_ = interestLifetimeMilliseconds; 
    ++changeCount_;
  }

  /**
   * @deprecated You should let the wire encoder generate a random nonce internally before sending the interest.
   */
  void 
  setNonce(const Blob& nonce) 
  { 
    nonce_ = nonce; 
    // Set getNonceChangeCount_ so that the next call to getNonce() won't clear nonce_.
    ++changeCount_;
    getNonceChangeCount_ = getChangeCount();
  }
  
  void 
  setKeyLocator(const KeyLocator& keyLocator) 
  { 
    keyLocator_ = keyLocator; 
    ++changeCount_;
  }

  /**
   * Get the change count, which is incremented each time this object (or a child object) is changed.
   * @return The change count.
   */
  uint64_t 
  getChangeCount() const
  {
    // Make sure each of the checkChanged is called.
    bool changed = name_.checkChanged();
    changed = publisherPublicKeyDigest_.checkChanged() || changed;
    changed = keyLocator_.checkChanged() || changed;
    changed = exclude_.checkChanged() || changed;
    if (changed)
      // A child object has changed, so update the change count.
      // This method can be called on a const object, but we want to be able to update the changeCount_.
      ++const_cast<Interest*>(this)->changeCount_;
    
    return changeCount_;    
  }

private:
  void 
  construct() 
  {
    minSuffixComponents_ = -1;
    maxSuffixComponents_ = -1;  
    childSelector_ = -1;
    answerOriginKind_ = -1;
    scope_ = -1;
    interestLifetimeMilliseconds_ = -1.0;
  }
  
  ChangeCounter<Name> name_;
  int minSuffixComponents_; /**< -1 for none */
  int maxSuffixComponents_; /**< -1 for none */
  /** @deprecated.  The Interest publisherPublicKeyDigest is deprecated.  If you need a publisher public key digest, 
   * set the keyLocator keyLocatorType to KEY_LOCATOR_DIGEST and set its key data to the digest. */
  ChangeCounter<PublisherPublicKeyDigest> publisherPublicKeyDigest_;
  ChangeCounter<KeyLocator> keyLocator_;
  ChangeCounter<Exclude> exclude_;
  int childSelector_;       /**< -1 for none */
  int answerOriginKind_;    /**< -1 for none. If >= 0 and the ndn_Interest_ANSWER_STALE bit is not set, then MustBeFresh. */
  int scope_;               /**< -1 for none */
  Milliseconds interestLifetimeMilliseconds_; /**< -1 for none */
  Blob nonce_;
  uint64_t getNonceChangeCount_;
  uint64_t changeCount_;
};
  
}

#endif
