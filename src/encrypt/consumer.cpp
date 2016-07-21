/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2016 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * @author: From ndn-group-encrypt src/consumer https://github.com/named-data/ndn-group-encrypt
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

#include <stdexcept>
#include "../util/logging.hpp"
#include <ndn-cpp/encrypt/algo/aes-algorithm.hpp>
#include <ndn-cpp/encrypt/algo/rsa-algorithm.hpp>
#include <ndn-cpp/encrypt/algo/encryptor.hpp>
#include <ndn-cpp/encrypt/consumer.hpp>

using namespace std;
using namespace ndn::func_lib;

INIT_LOGGER("ndn.Consumer");

namespace ndn {

Consumer::Impl::Impl
  (Face* face, KeyChain* keyChain, const Name& groupName,
   const Name& consumerName, const ptr_lib::shared_ptr<ConsumerDb>& database)
: face_(face),
  keyChain_(keyChain),
  groupName_(groupName),
  consumerName_(consumerName),
  database_(database)
{
}

void
Consumer::Impl::consume
  (const Name& contentName, const OnConsumeComplete& onConsumeComplete,
   const EncryptError::OnError& onError)
{
  ptr_lib::shared_ptr<Interest> interest(new Interest(contentName));

  // Prepare the callbacks. We make a shared_ptr object since it needs to
  // exist after we call expressInterest and return.
  class Callbacks : public ptr_lib::enable_shared_from_this<Callbacks> {
  public:
    Callbacks
      (Consumer::Impl* parent, const ptr_lib::shared_ptr<Interest>& interest,
       const OnConsumeComplete& onConsumeComplete,
       const EncryptError::OnError& onError)
    : parent_(parent), interest_(interest),
      onConsumeComplete_(onConsumeComplete), onError_(onError)
    {}

    void
    onData
      (const ptr_lib::shared_ptr<const Interest>& contentInterest,
       const ptr_lib::shared_ptr<Data>& contentData)
    {
      // Save this for calling onConsumeComplete.
      contentData_ = contentData;

      // The Interest has no selectors, so assume the library correctly
      // matched with the Data name before calling onData.

      try {
        parent_->keyChain_->verifyData
          (contentData,
           bind(&Callbacks::onContentVerified, shared_from_this(), _1),
           bind(&Impl::onVerifyFailed, _1, onError_));
      } catch (const std::exception& ex) {
        try {
          onError_(EncryptError::ErrorCode::General,
                  string("verifyData error: ") + ex.what());
        } catch (const std::exception& ex) {
          _LOG_ERROR("Error in onError: " << ex.what());
        } catch (...) {
          _LOG_ERROR("Error in onError.");
        }
        return;
      }
    }

    void
    onContentVerified(const ptr_lib::shared_ptr<Data>& validData)
    {
      parent_->decryptContent
        (*validData,
         bind(&Callbacks::onContentPlainText, shared_from_this(), _1),
         onError_);
    }

    void
    onContentPlainText(const Blob& plainText)
    {
      try {
        onConsumeComplete_(contentData_, plainText);
      } catch (const std::exception& ex) {
        _LOG_ERROR("Error in onConsumeComplete: " << ex.what());
      } catch (...) {
        _LOG_ERROR("Error in onConsumeComplete.");
      }
    }

    void
    onTimeout(const ptr_lib::shared_ptr<const Interest>& dKeyInterest)
    {
      // We should re-try at least once.
      try {
        parent_->face_->expressInterest
          (*interest_, bind(&Callbacks::onData, shared_from_this(), _1, _2),
           bind(&Impl::onFinalTimeout, _1, onError_));
      } catch (const std::exception& ex) {
        try {
          onError_(EncryptError::ErrorCode::General,
                   string("expressInterest error: ") + ex.what());
        } catch (const std::exception& ex) {
          _LOG_ERROR("Error in onError: " << ex.what());
        } catch (...) {
          _LOG_ERROR("Error in onError.");
        }
        return;
      }
    }

    Consumer::Impl* parent_;
    ptr_lib::shared_ptr<Interest> interest_;
    OnConsumeComplete onConsumeComplete_;
    EncryptError::OnError onError_;
    ptr_lib::shared_ptr<Data> contentData_;
  };

  ptr_lib::shared_ptr<Callbacks> callbacks(new Callbacks
    (this, interest, onConsumeComplete, onError));
  // Express the Interest.
  try {
    face_->expressInterest
      (*interest, bind(&Callbacks::onData, callbacks, _1, _2),
       bind(&Callbacks::onTimeout, callbacks, _1));
  } catch (const std::exception& ex) {
    try {
      onError(EncryptError::ErrorCode::General,
              string("expressInterest error: ") + ex.what());
    } catch (const std::exception& ex) {
      _LOG_ERROR("Error in onError: " << ex.what());
    } catch (...) {
      _LOG_ERROR("Error in onError.");
    }
    return;
  }
}

void
Consumer::Impl::addDecryptionKey(const Name& keyName, const Blob& keyBlob)
{
  if (!(consumerName_.match(keyName)))
    throw runtime_error
      ("addDecryptionKey: The consumer name must be a prefix of the key name");

  database_->addKey(keyName, keyBlob);
}

void
Consumer::Impl::decrypt
  (const Blob& encryptedBlob, const Blob& keyBits,
   const OnPlainText& onPlainText, const EncryptError::OnError& onError)
{
  EncryptedContent encryptedContent;
  try {
    encryptedContent.wireDecode(encryptedBlob);
  } catch (const std::exception& ex) {
    try {
      onError(EncryptError::ErrorCode::InvalidEncryptedFormat, ex.what());
    } catch (const std::exception& ex) {
      _LOG_ERROR("Error in onError: " << ex.what());
    } catch (...) {
      _LOG_ERROR("Error in onError.");
    }
    return;
  }

  decryptEncryptedContent(encryptedContent, keyBits, onPlainText, onError);
}

void
Consumer::Impl::decryptEncryptedContent
  (const EncryptedContent& encryptedContent, const Blob& keyBits,
   const OnPlainText& onPlainText, const EncryptError::OnError& onError)
{
  Blob payload = encryptedContent.getPayload();

  if (encryptedContent.getAlgorithmType() == ndn_EncryptAlgorithmType_AesCbc) {
    // Prepare the parameters.
    EncryptParams decryptParams(ndn_EncryptAlgorithmType_AesCbc);
    decryptParams.setInitialVector(encryptedContent.getInitialVector());

    // Decrypt the content.
    Blob content;
    try {
      content = AesAlgorithm::decrypt(keyBits, payload, decryptParams);
    } catch (const std::exception& ex) {
      try {
        onError(EncryptError::ErrorCode::InvalidEncryptedFormat, ex.what());
      } catch (const std::exception& ex) {
        _LOG_ERROR("Error in onError: " << ex.what());
      } catch (...) {
        _LOG_ERROR("Error in onError.");
      }
      return;
    }
    try {
      onPlainText(content);
    } catch (const std::exception& ex) {
      _LOG_ERROR("Error in onPlainText: " << ex.what());
    } catch (...) {
      _LOG_ERROR("Error in onPlainText.");
    }
  }
  else if (encryptedContent.getAlgorithmType() == ndn_EncryptAlgorithmType_RsaOaep) {
    // Prepare the parameters.
    EncryptParams decryptParams(ndn_EncryptAlgorithmType_RsaOaep);

    // Decrypt the content.
    Blob content;
    try {
      content = RsaAlgorithm::decrypt(keyBits, payload, decryptParams);
    } catch (const std::exception& ex) {
      try {
        onError(EncryptError::ErrorCode::InvalidEncryptedFormat, ex.what());
      } catch (const std::exception& ex) {
        _LOG_ERROR("Error in onError: " << ex.what());
      } catch (...) {
        _LOG_ERROR("Error in onError.");
      }
      return;
    }
    try {
      onPlainText(content);
    } catch (const std::exception& ex) {
      _LOG_ERROR("Error in onPlainText: " << ex.what());
    } catch (...) {
      _LOG_ERROR("Error in onPlainText.");
    }
  }
  else {
    try {
      onError
        (EncryptError::ErrorCode::UnsupportedEncryptionScheme,
         "UnsupportedEncryptionScheme");
    } catch (const std::exception& ex) {
      _LOG_ERROR("Error in onError: " << ex.what());
    } catch (...) {
      _LOG_ERROR("Error in onError.");
    }
  }
}

void
Consumer::Impl::decryptContent
  (const Data& data, const OnPlainText& onPlainText,
   const EncryptError::OnError& onError)
{
  // Get the encrypted content.
  // Make this a shared_ptr so we can pass it in callbacks.
  ptr_lib::shared_ptr<EncryptedContent> dataEncryptedContent
    (new EncryptedContent());
  try {
    dataEncryptedContent->wireDecode(data.getContent());
  } catch (const std::exception& ex) {
    try {
      onError(EncryptError::ErrorCode::InvalidEncryptedFormat, ex.what());
    } catch (const std::exception& ex) {
      _LOG_ERROR("Error in onError: " << ex.what());
    } catch (...) {
      _LOG_ERROR("Error in onError.");
    }
    return;
  }
  const Name& cKeyName = dataEncryptedContent->getKeyLocator().getKeyName();

  // Check if the content key is already in the store.
  if (dKeyMap_.find(cKeyName) != dKeyMap_.end())
    decryptEncryptedContent
      (*dataEncryptedContent, dKeyMap_[cKeyName], onPlainText, onError);
  else {
    // Retrieve the C-KEY Data from the network.
    Name interestName(cKeyName);
    interestName.append(Encryptor::getNAME_COMPONENT_FOR()).append(groupName_);
    ptr_lib::shared_ptr<Interest> interest(new Interest(interestName));

    // Prepare the callbacks. We make a shared_ptr object since it needs to
    // exist after we call expressInterest and return.
    class Callbacks : public ptr_lib::enable_shared_from_this<Callbacks> {
    public:
      Callbacks
        (Consumer::Impl* parent,
         const ptr_lib::shared_ptr<EncryptedContent>& dataEncryptedContent,
         const Name& cKeyName, const ptr_lib::shared_ptr<Interest>& interest,
         const OnPlainText& onPlainText,
         const EncryptError::OnError& onError)
      : parent_(parent), dataEncryptedContent_(dataEncryptedContent),
        cKeyName_(cKeyName), interest_(interest), onPlainText_(onPlainText),
        onError_(onError)
      {}

      void
      onData
        (const ptr_lib::shared_ptr<const Interest>& dKeyInterest,
         const ptr_lib::shared_ptr<Data>& cKeyData)
      {
        // The Interest has no selectors, so assume the library correctly
        // matched with the Data name before calling onData.

        try {
          parent_->keyChain_->verifyData
            (cKeyData,
             bind(&Callbacks::onCKeyVerified, shared_from_this(), _1),
             bind(&Impl::onVerifyFailed, _1, onError_));
        } catch (const std::exception& ex) {
          try {
            onError_(EncryptError::ErrorCode::General,
                    string("verifyData error: ") + ex.what());
          } catch (const std::exception& ex) {
            _LOG_ERROR("Error in onError: " << ex.what());
          } catch (...) {
            _LOG_ERROR("Error in onError.");
          }
          return;
        }
      }

      void
      onCKeyVerified(const ptr_lib::shared_ptr<Data>& validCKeyData)
      {
        parent_->decryptCKey
          (*validCKeyData,
           bind(&Callbacks::onCKeyPlainText, shared_from_this(), _1),
           onError_);
      }

      void
      onCKeyPlainText(const Blob& cKeyBits)
      {
        parent_->cKeyMap_[cKeyName_] = cKeyBits;
        parent_->decryptEncryptedContent
          (*dataEncryptedContent_, cKeyBits, onPlainText_, onError_);
      }

      void
      onTimeout(const ptr_lib::shared_ptr<const Interest>& dKeyInterest)
      {
        // We should re-try at least once.
        try {
          parent_->face_->expressInterest
            (*interest_, bind(&Callbacks::onData, shared_from_this(), _1, _2),
             bind(&Impl::onFinalTimeout, _1, onError_));
        } catch (const std::exception& ex) {
          try {
            onError_(EncryptError::ErrorCode::General,
                     string("expressInterest error: ") + ex.what());
          } catch (const std::exception& ex) {
            _LOG_ERROR("Error in onError: " << ex.what());
          } catch (...) {
            _LOG_ERROR("Error in onError.");
          }
          return;
        }
      }

      Consumer::Impl* parent_;
      ptr_lib::shared_ptr<EncryptedContent> dataEncryptedContent_;
      Name cKeyName_;
      ptr_lib::shared_ptr<Interest> interest_;
      OnPlainText onPlainText_;
      EncryptError::OnError onError_;
    };

    ptr_lib::shared_ptr<Callbacks> callbacks(new Callbacks
      (this, dataEncryptedContent, cKeyName, interest, onPlainText, onError));
    // Express the Interest.
    try {
      face_->expressInterest
        (*interest, bind(&Callbacks::onData, callbacks, _1, _2),
         bind(&Callbacks::onTimeout, callbacks, _1));
    } catch (const std::exception& ex) {
      try {
        onError(EncryptError::ErrorCode::General,
                string("expressInterest error: ") + ex.what());
      } catch (const std::exception& ex) {
        _LOG_ERROR("Error in onError: " << ex.what());
      } catch (...) {
        _LOG_ERROR("Error in onError.");
      }
      return;
    }
  }
}

void
Consumer::Impl::decryptCKey
  (const Data& cKeyData, const OnPlainText& onPlainText, 
   const EncryptError::OnError& onError)
{
  // Get the encrypted content.
  Blob cKeyContent = cKeyData.getContent();
  // Make this a shared_ptr so we can pass it in callbacks.
  ptr_lib::shared_ptr<EncryptedContent> cKeyEncryptedContent
    (new EncryptedContent());
  try {
    cKeyEncryptedContent->wireDecode(cKeyContent);
  } catch (const std::exception& ex) {
    try {
      onError(EncryptError::ErrorCode::InvalidEncryptedFormat, ex.what());
    } catch (const std::exception& ex) {
      _LOG_ERROR("Error in onError: " << ex.what());
    } catch (...) {
      _LOG_ERROR("Error in onError.");
    }
    return;
  }
  const Name& eKeyName = cKeyEncryptedContent->getKeyLocator().getKeyName();
  Name dKeyName = eKeyName.getPrefix(-3);
  dKeyName.append(Encryptor::getNAME_COMPONENT_D_KEY())
    .append(eKeyName.getSubName(-2));

  // Check if the decryption key is already in the store.
  if (dKeyMap_.find(dKeyName) != dKeyMap_.end())
    decryptEncryptedContent
      (*cKeyEncryptedContent, dKeyMap_[dKeyName], onPlainText, onError);
  else {
    // Get the D-Key Data.
    Name interestName(dKeyName);
    interestName.append(Encryptor::getNAME_COMPONENT_FOR()).append(consumerName_);
    ptr_lib::shared_ptr<Interest> interest(new Interest(interestName));

    // Prepare the callbacks. We make a shared_ptr object since it needs to
    // exist after we call expressInterest and return.
    class Callbacks : public ptr_lib::enable_shared_from_this<Callbacks> {
    public:
      Callbacks
        (Consumer::Impl* parent, 
         const ptr_lib::shared_ptr<EncryptedContent>& cKeyEncryptedContent,
         const Name& dKeyName, const ptr_lib::shared_ptr<Interest>& interest,
         const OnPlainText& onPlainText,
         const EncryptError::OnError& onError)
      : parent_(parent), cKeyEncryptedContent_(cKeyEncryptedContent),
        dKeyName_(dKeyName), interest_(interest), onPlainText_(onPlainText),
        onError_(onError)
      {}

      void
      onData
        (const ptr_lib::shared_ptr<const Interest>& dKeyInterest,
         const ptr_lib::shared_ptr<Data>& dKeyData)
      {
        // The Interest has no selectors, so assume the library correctly
        // matched with the Data name before calling onData.

        try {
          parent_->keyChain_->verifyData
            (dKeyData,
             bind(&Callbacks::onDKeyVerified, shared_from_this(), _1),
             bind(&Impl::onVerifyFailed, _1, onError_));
        } catch (const std::exception& ex) {
          try {
            onError_(EncryptError::ErrorCode::General,
                    string("verifyData error: ") + ex.what());
          } catch (const std::exception& ex) {
            _LOG_ERROR("Error in onError: " << ex.what());
          } catch (...) {
            _LOG_ERROR("Error in onError.");
          }
          return;
        }
      }

      void
      onDKeyVerified(const ptr_lib::shared_ptr<Data>& validDKeyData)
      {
        parent_->decryptDKey
          (*validDKeyData,
           bind(&Callbacks::onDKeyPlainText, shared_from_this(), _1),
           onError_);
      }

      void
      onDKeyPlainText(const Blob& dKeyBits)
      {
        parent_->dKeyMap_[dKeyName_] = dKeyBits;
        parent_->decryptEncryptedContent
          (*cKeyEncryptedContent_, dKeyBits, onPlainText_, onError_);
      }

      void
      onTimeout(const ptr_lib::shared_ptr<const Interest>& dKeyInterest)
      {
        // We should re-try at least once.
        try {
          parent_->face_->expressInterest
            (*interest_, bind(&Callbacks::onData, shared_from_this(), _1, _2),
             bind(&Impl::onFinalTimeout, _1, onError_));
        } catch (const std::exception& ex) {
          try {
            onError_(EncryptError::ErrorCode::General,
                     string("expressInterest error: ") + ex.what());
          } catch (const std::exception& ex) {
            _LOG_ERROR("Error in onError: " << ex.what());
          } catch (...) {
            _LOG_ERROR("Error in onError.");
          }
          return;
        }
      }

      Consumer::Impl* parent_;
      ptr_lib::shared_ptr<EncryptedContent> cKeyEncryptedContent_;
      Name dKeyName_;
      ptr_lib::shared_ptr<Interest> interest_;
      OnPlainText onPlainText_;
      EncryptError::OnError onError_;
    };

    ptr_lib::shared_ptr<Callbacks> callbacks(new Callbacks
      (this, cKeyEncryptedContent, dKeyName, interest, onPlainText, onError));
    // Express the Interest.
    try {
      face_->expressInterest
        (*interest, bind(&Callbacks::onData, callbacks, _1, _2),
         bind(&Callbacks::onTimeout, callbacks, _1));
    } catch (const std::exception& ex) {
      try {
        onError(EncryptError::ErrorCode::General,
                string("expressInterest error: ") + ex.what());
      } catch (const std::exception& ex) {
        _LOG_ERROR("Error in onError: " << ex.what());
      } catch (...) {
        _LOG_ERROR("Error in onError.");
      }
      return;
    }
  }
}

void
Consumer::Impl::decryptDKey
  (const Data& dKeyData, const OnPlainText& onPlainText,
   const EncryptError::OnError& onError)
{
  // Get the encrypted content.
  Blob dataContent = dKeyData.getContent();

  // Process the nonce.
  // dataContent is a sequence of the two EncryptedContent.
  EncryptedContent encryptedNonce;
  try {
    encryptedNonce.wireDecode(dataContent);
  } catch (const std::exception& ex) {
    try {
      onError(EncryptError::ErrorCode::InvalidEncryptedFormat, ex.what());
    } catch (const std::exception& ex) {
      _LOG_ERROR("Error in onError: " << ex.what());
    } catch (...) {
      _LOG_ERROR("Error in onError.");
    }
    return;
  }
  Name consumerKeyName = encryptedNonce.getKeyLocator().getKeyName();

  // Get consumer decryption key.
  Blob consumerKeyBlob;
  try {
    consumerKeyBlob = getDecryptionKey(consumerKeyName);
  } catch (const std::exception& ex) {
    try {
      onError(EncryptError::ErrorCode::NoDecryptKey, ex.what());
    } catch (const std::exception& ex) {
      _LOG_ERROR("Error in onError: " << ex.what());
    } catch (...) {
      _LOG_ERROR("Error in onError.");
    }
    return;
  }
  if (consumerKeyBlob.size() == 0) {
    try {
      onError(EncryptError::ErrorCode::NoDecryptKey,
        "The desired consumer decryption key in not in the database");
    } catch (const std::exception& ex) {
      _LOG_ERROR("Error in onError: " << ex.what());
    } catch (...) {
      _LOG_ERROR("Error in onError.");
    }
    return;
  }

  // Process the D-KEY.
  // Use the size of encryptedNonce to find the start of encryptedPayload.
  size_t encryptedNonceSize = encryptedNonce.wireEncode().size();
  EncryptedContent encryptedPayload;
  Blob encryptedPayloadBlob
    (dataContent.buf() + encryptedNonceSize,
     dataContent.size() - encryptedNonceSize);
  if (encryptedPayloadBlob.size() == 0) {
    try {
      onError(EncryptError::ErrorCode::InvalidEncryptedFormat,
        "The data packet does not satisfy the D-KEY packet format");
    } catch (const std::exception& ex) {
      _LOG_ERROR("Error in onError: " << ex.what());
    } catch (...) {
      _LOG_ERROR("Error in onError.");
    }
    return;
  }

  // Decrypt the D-KEY.
  decryptEncryptedContent
    (encryptedNonce, consumerKeyBlob,
     bind(&Consumer::Impl::decrypt, encryptedPayloadBlob, _1, onPlainText, onError),
     onError);
}

void
Consumer::Impl::onVerifyFailed
  (const ptr_lib::shared_ptr<Data>& data, const EncryptError::OnError& onError)
{
  try {
    onError(EncryptError::ErrorCode::Validation, "verifyData failed");
  } catch (const std::exception& ex) {
    _LOG_ERROR("Error in onError: " << ex.what());
  } catch (...) {
    _LOG_ERROR("Error in onError.");
  }
}

void
Consumer::Impl::onFinalTimeout
  (const ptr_lib::shared_ptr<const Interest>& interest,
   const EncryptError::OnError& onError)
{
  try {
    onError(EncryptError::ErrorCode::Timeout, interest->getName().toUri());
  } catch (const std::exception& ex) {
    _LOG_ERROR("Error in onError: " << ex.what());
  } catch (...) {
    _LOG_ERROR("Error in onError.");
  }
}

}
