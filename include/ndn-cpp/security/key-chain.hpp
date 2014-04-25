/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013-2014 Regents of the University of California.
 * @author: Yingdi Yu <yingdi@cs.ucla.edu>
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#ifndef NDN_KEY_CHAIN_HPP
#define NDN_KEY_CHAIN_HPP

#include "../data.hpp"
#include "../interest.hpp"
#include "../face.hpp"
#include "identity/identity-manager.hpp"
#include "encryption/encryption-manager.hpp"
#include "policy/validation-request.hpp"

namespace ndn {

class PolicyManager;
  
/**
 * KeyChain is the main class of the security library.
 *
 * The KeyChain class provides a set of interfaces to the security library such as identity management, policy configuration 
 * and packet signing and verification.
 * @note This class is an experimental feature.  See the API docs for more detail at
 * http://named-data.net/doc/ndn-ccl-api/key-chain.html .
 */
class KeyChain {
public:
  /**
   * Create a new KeyChain with the given IdentityManager and PolicyManager.
   * @param identityManager An object of a subclass of IdentityManager.
   * @param policyManager An object of a subclass of PolicyManager.
   */
  KeyChain
    (const ptr_lib::shared_ptr<IdentityManager>& identityManager, 
     const ptr_lib::shared_ptr<PolicyManager>& policyManager);

  /**
   * Create a new KeyChain with the given IdentityManager and a 
   * NoVerifyPolicyManager.
   * @param identityManager An object of a subclass of IdentityManager.
   */
  KeyChain(const ptr_lib::shared_ptr<IdentityManager>& identityManager);

  /**
   * Create a new KeyChain with the the default IdentityManager and a 
   * NoVerifyPolicyManager.
   */
  KeyChain();

  /*****************************************
   *          Identity Management          *
   *****************************************/

  /**
   * Create an identity by creating a pair of Key-Signing-Key (KSK) for this identity and a self-signed certificate of the KSK.
   * @param identityName The name of the identity.
   * @return The key name of the auto-generated KSK of the identity.
   */
  Name
  createIdentity(const Name& identityName)
  {
    return identityManager_->createIdentity(identityName);
  }

  /**
   * Get the default identity.
   * @return The name of default identity.
   * @throw SecurityException if the default identity is not set.
   */
  Name
  getDefaultIdentity()
  {
    return identityManager_->getDefaultIdentity();
  }
    
  /**
   * Get the default certificate name of the default identity.
   * @return The requested certificate name.
   * @throw SecurityException if the default identity is not set or the default 
   * key name for the identity is not set or the default certificate name for 
   * the key name is not set.
   */
  Name
  getDefaultCertificateName()
  {
    return identityManager_->getDefaultCertificateName();
  }
  
  /**
   * Generate a pair of RSA keys for the specified identity.
   * @param identityName The name of the identity.
   * @param isKsk (optional) true for generating a Key-Signing-Key (KSK), false 
   * for a Data-Signing-Key (DSK). If omitted, generate a Data-Signing-Key.
   * @param keySize (optional) The size of the key. If omitted, use a default
   * secure key size.
   * @return The generated key name.
   */
  Name
  generateRSAKeyPair(const Name& identityName, bool isKsk = false, int keySize = 2048)
  {
    return identityManager_->generateRSAKeyPair(identityName, isKsk, keySize);
  }

  /**
   * Set a key as the default key of an identity.
   * @param keyName The name of the key.
   * @param identityName (optional) the name of the identity. If not specified, 
   * the identity name is inferred from the keyName.
   */
  void
  setDefaultKeyForIdentity(const Name& keyName, const Name& identityName = Name())
  {
    return identityManager_->setDefaultKeyForIdentity(keyName, identityName);
  }

  /**
   * Generate a pair of RSA keys for the specified identity and set it as default key for the identity.
   * @param identityName The name of the identity.
   * @param isKsk (optional) true for generating a Key-Signing-Key (KSK), false 
   * for a Data-Signing-Key (DSK). If omitted, generate a Data-Signing-Key.
   * @param keySize (optional) The size of the key. If omitted, use a default
   * secure key size.
   * @return The generated key name.
   */
  Name
  generateRSAKeyPairAsDefault(const Name& identityName, bool isKsk = false, int keySize = 2048)
  {
    return identityManager_->generateRSAKeyPairAsDefault(identityName, isKsk, keySize);
  }

  /**
   * Create a public key signing request.
   * @param keyName The name of the key.
   * @returns The signing request data.
   */
  Blob
  createSigningRequest(const Name& keyName)
  {
    return identityManager_->getPublicKey(keyName)->getKeyDer();
  }

  /**
   * Install an identity certificate into the public key identity storage.
   * @param certificate The certificate to to added.
   */
  void
  installIdentityCertificate(const IdentityCertificate& certificate)
  {
    identityManager_->addCertificate(certificate);
  }

  /**
   * Set the certificate as the default for its corresponding key.
   * @param certificate The certificate.
   */
  void
  setDefaultCertificateForKey(const IdentityCertificate& certificate)
  {
    identityManager_->setDefaultCertificateForKey(certificate);
  }

  /**
   * Get a certificate with the specified name.
   * @param certificateName The name of the requested certificate.
   * @return The requested certificate which is valid.
   */
  ptr_lib::shared_ptr<Certificate>
  getCertificate(const Name& certificateName)
  {
    return identityManager_->getCertificate(certificateName);
  }

  /**
   * Get a certificate even if the certificate is not valid anymore.
   * @param certificateName The name of the requested certificate.
   * @return The requested certificate.
   */
  ptr_lib::shared_ptr<Certificate>
  getAnyCertificate(const Name& certificateName)
  {
    return identityManager_->getAnyCertificate(certificateName);
  }

  /**
   * Get an identity certificate with the specified name.
   * @param certificateName The name of the requested certificate.
   * @return The requested certificate which is valid.
   */
  ptr_lib::shared_ptr<IdentityCertificate>
  getIdentityCertificate(const Name& certificateName)
  {
    return identityManager_->getCertificate(certificateName);
  }

  /**
   * Get an identity certificate even if the certificate is not valid anymore.
   * @param certificateName The name of the requested certificate.
   * @return The requested certificate.
   */
  ptr_lib::shared_ptr<IdentityCertificate>
  getAnyIdentityCertificate(const Name& certificateName)
  {
    return identityManager_->getAnyCertificate(certificateName);
  }

  /**
   * Revoke a key.
   * @param keyName The name of the key that will be revoked.
   */
  void 
  revokeKey(const Name & keyName)
  {
    //TODO: Implement
  }

  /**
   * Revoke a certificate.
   * @param certificateName The name of the certificate that will be revoked.
   */
  void 
  revokeCertificate(const Name & certificateName)
  {
    //TODO: Implement
  }

  /**
   * Get the identity manager given to or created by the constructor.
   * @return The identity manager.
   */
  ptr_lib::shared_ptr<IdentityManager>
  getIdentityManager() { return identityManager_; }
  
  /*****************************************
   *           Policy Management           *
   *****************************************/

  /**
   * Get the policy manager given to or created by the constructor.
   * @return The policy manager.
   */
  const ptr_lib::shared_ptr<PolicyManager>&
  getPolicyManager() { return policyManager_; }
  
  /*****************************************
   *              Sign/Verify              *
   *****************************************/

  /**
   * Wire encode the Data object, sign it and set its signature.
   * @param data The Data object to be signed.  This updates its signature and key locator field and wireEncoding.
   * @param certificateName The certificate name of the key to use for signing.
   * @param wireFormat (optional) A WireFormat object used to encode the input. If omitted, use WireFormat getDefaultWireFormat().
   */
  void 
  sign(Data& data, const Name& certificateName, WireFormat& wireFormat = *WireFormat::getDefaultWireFormat());

  /**
   * Append a SignatureInfo to the Interest name, sign the name components and
   * append a final name component with the signature bits.
   * @param interest The Interest object to be signed. This appends name
   * components of SignatureInfo and the signature bits.
   * @param certificateName The certificate name of the key to use for signing.
   * @param wireFormat (optional) A WireFormat object used to encode the input. If omitted, 
   * use WireFormat getDefaultWireFormat().
   */
  void 
  sign
    (Interest& interest, const Name& certificateName, 
     WireFormat& wireFormat = *WireFormat::getDefaultWireFormat());
  
  /**
   * Sign the byte array using a certificate name and return a Signature object.
   * @param buffer The byte array to be signed.
   * @param bufferLength the length of buffer.
   * @param certificateName The certificate name used to get the signing key and which will be put into KeyLocator.
   * @return The Signature.
   */
  ptr_lib::shared_ptr<Signature> 
  sign(const uint8_t* buffer, size_t bufferLength, const Name& certificateName);

  /**
   * Sign the byte array using a certificate name and return a Signature object.
   * @param buffer The byte array to be signed.
   * @param certificateName The certificate name used to get the signing key and which will be put into KeyLocator.
   * @return The Signature.
   */
  ptr_lib::shared_ptr<Signature> 
  sign(const std::vector<uint8_t>& buffer, const Name& certificateName)
  {
    return sign(&buffer[0], buffer.size(), certificateName);
  }

  /**
   * Wire encode the Data object, sign it and set its signature.
   * @param data The Data object to be signed.  This updates its signature and key locator field and wireEncoding.
   * @param identityName (optional) The identity name for the key to use for signing.  If omitted, infer the signing identity from the data packet name.
   * @param wireFormat (optional) A WireFormat object used to encode the input. If omitted, use WireFormat getDefaultWireFormat().
   */
  void 
  signByIdentity(Data& data, const Name& identityName = Name(), WireFormat& wireFormat = *WireFormat::getDefaultWireFormat());

  /**
   * Sign the byte array using an identity name and return a Signature object.
   * @param buffer The byte array to be signed.
   * @param bufferLength the length of buffer.
   * @param identityName The identity name.
   * @return The Signature.
   */
  ptr_lib::shared_ptr<Signature> 
  signByIdentity(const uint8_t* buffer, size_t bufferLength, const Name& identityName);

  /**
   * Sign the byte array using an identity name and return a Signature object.
   * @param buffer The byte array to be signed.
   * @param identityName The identity name.
   * @return The Signature.
   */
  ptr_lib::shared_ptr<Signature> 
  signByIdentity(const std::vector<uint8_t>& buffer, const Name& identityName)
  {
    return signByIdentity(&buffer[0], buffer.size(), identityName);
  }

  /**
   * Check the signature on the Data object and call either onVerify or onVerifyFailed. 
   * We use callback functions because verify may fetch information to check the signature.
   * @param data The Data object with the signature to check. It is an error if data does not have a wireEncoding. 
   * To set the wireEncoding, you can call data.wireDecode.
   * @param onVerified If the signature is verified, this calls onVerified(data).
   * @param onVerifyFailed If the signature check fails, this calls onVerifyFailed(data).
   */
  void
  verifyData
    (const ptr_lib::shared_ptr<Data>& data, const OnVerified& onVerified, const OnVerifyFailed& onVerifyFailed, int stepCount = 0);

  /*****************************************
   *           Encrypt/Decrypt             *
   *****************************************/

  /**
   * Generate a symmetric key.
   * @param keyName The name of the generated key.
   * @param keyType The type of the key, e.g. KEY_TYPE_AES
   */
  void 
  generateSymmetricKey(const Name& keyName, KeyType keyType)
  {
    encryptionManager_->createSymmetricKey(keyName, keyType);
  }

  /**
   * Encrypt a byte array.
   * @param keyName The name of the encrypting key.
   * @param data The byte array that will be encrypted.
   * @param dataLength The length of data.
   * @param useSymmetric (optional) If true then symmetric encryption is used, 
   * otherwise asymmetric encryption is used. If omitted, use symmetric 
   * encryption.
   * @param encryptMode (optional) The encryption mode. If omitted, use
   * ENCRYPT_MODE_DEFAULT.
   * @return The encrypted data as an immutable Blob.
   */
  Blob
  encrypt(const Name &keyName, const uint8_t* data, size_t dataLength, bool useSymmetric = true, 
          EncryptMode encryptMode = ENCRYPT_MODE_DEFAULT)
  {
    return encryptionManager_->encrypt(keyName, data, dataLength, useSymmetric, encryptMode);
  }

  /**
   * Decrypt a byte array.
   * @param keyName The name of the decrypting key.
   * @param data The byte array that will be decrypted.
   * @param dataLength The length of data.
   * @param useSymmetric (optional) If true then symmetric encryption is used, 
   * otherwise asymmetric encryption is used. If omitted, use symmetric 
   * encryption.
   * @param encryptMode (optional) The encryption mode. If omitted, use
   * @return The decrypted data as an immutable Blob.
   */
  Blob
  decrypt(const Name &keyName, const uint8_t* data, size_t dataLength, bool useSymmetric = true, 
          EncryptMode encryptMode = ENCRYPT_MODE_DEFAULT)
  {
     return encryptionManager_->decrypt(keyName, data, dataLength, useSymmetric, encryptMode);
  }
  
  /**
   * Set the Face which will be used to fetch required certificates.
   * @param face A pointer to the Face object.
   */
  void
  setFace(Face* face) { face_ = face; }

private:
  void
  onCertificateData
    (const ptr_lib::shared_ptr<const Interest> &interest, const ptr_lib::shared_ptr<Data> &data, ptr_lib::shared_ptr<ValidationRequest> nextStep);
  
  void
  onCertificateInterestTimeout
    (const ptr_lib::shared_ptr<const Interest> &interest, int retry, const OnVerifyFailed& onVerifyFailed, 
     const ptr_lib::shared_ptr<Data> &data, ptr_lib::shared_ptr<ValidationRequest> nextStep);

  ptr_lib::shared_ptr<IdentityManager> identityManager_;
  ptr_lib::shared_ptr<PolicyManager> policyManager_;
  ptr_lib::shared_ptr<EncryptionManager> encryptionManager_;
  Face* face_;
  const int maxSteps_;
};

}

#endif
