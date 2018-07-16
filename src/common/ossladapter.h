/*
 * ossladapter.h header only C-Style OpenSSL adapter for hashing and ECDSA.
 *
 *  Created on: Jan 10, 2018
 *  Author: Nick Williams
 */

#ifndef SRC_COMMON_OSSLADAPTER_H_
#define SRC_COMMON_OSSLADAPTER_H_

#include <string.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/crypto.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "common/binary_converters.h"
#include "common/util.h"

/** Gets the EC_GROUP for normal transactions.
 *  @return a pointer to the EC_GROUP
 */
static EC_GROUP* getWalletGroup() {
  return(EC_GROUP_new_by_curve_name(NID_secp256k1));
}

/** Gets the EC_GROUP for internal transactions.
 *  @return a pointer to the EC_GROUP
 */
static EC_GROUP* getNodeGroup() {
  return(EC_GROUP_new_by_curve_name(NID_secp384r1));
}

/** Generates a new EC_KEY.
 *  @pre an OpenSSL context must have been initialized
 *  @param publicKey [out] reference with ASCII armor
 *  @param pkHex [out] references private key with ASCII armor, pem, aes
 *  @param aes_password [in] the password for aes
 *  @return if success, a pointer to the EC_KEY object
 *  @return if error, a NULLPTR
 */
static EC_KEY* GenerateEcKey(std::string& publicKey, std::string& pk
                           , const std::string& aes_password) {
  CASH_TRY {
    EC_GROUP* ecGroup = getWalletGroup();
    if (NULL == ecGroup) {
      LOG_ERROR << "Failed to generate EC group.";
    }

    EC_KEY *eckey=EC_KEY_new();
    if (NULL == eckey) {
      LOG_ERROR << "Failed to allocate EC key.";
    }

    EC_GROUP_set_asn1_flag(ecGroup, OPENSSL_EC_NAMED_CURVE);
    int state = EC_KEY_set_group(eckey, ecGroup);
    if (1 != state) { LOG_ERROR << "Failed to set EC group status."; }
    state = EC_KEY_generate_key(eckey);
    if (1 != state) { LOG_ERROR << "Failed to generate EC key."; }

    OpenSSL_add_all_algorithms();
    EVP_PKEY* pkey = EVP_PKEY_new();
    if (!EVP_PKEY_set1_EC_KEY(pkey, eckey)) {
      LOG_ERROR << "Could not export private key";
    }

    if (1 != EC_GROUP_check(EC_KEY_get0_group(eckey), NULL)) {
      CASH_THROW("EC group is invalid!");
    }

    if (1 != EC_KEY_check_key(eckey)) {
      LOG_ERROR << "key is invalid!";
    }

    const EVP_CIPHER* cipher = EVP_aes_128_cbc();
    BIO* fOut = BIO_new(BIO_s_mem());
    int result = PEM_write_bio_PKCS8PrivateKey(fOut, pkey, cipher, NULL, 0, NULL,
      const_cast<char*>(aes_password.c_str()));
    if (result != 1) { LOG_ERROR << "Failed to generate PEM private key file"; }
    char buffer[1024];
    while (BIO_read(fOut, buffer, 1024) > 0) {
      pk += buffer;
    }
    BIO_free(fOut);
    EVP_cleanup();

    const EC_POINT *pubKey = EC_KEY_get0_public_key(eckey);
    publicKey = EC_POINT_point2hex(ecGroup, pubKey, POINT_CONVERSION_COMPRESSED, NULL);

    return eckey;
  } CASH_CATCH(const std::exception& e) {
    LOG_WARNING << Devcash::FormatException(&e, "Crypto");
  }
  return nullptr;
}

/** Generates new EC_KEYs and writes the key pairs to a file.
 *  @pre an OpenSSL context must have been initialized
 *  @param path [in] the path of the file to write
 *  @param num_keys [in] the number of key pairs to generate
 *  @param aes_password [in] the password for aes
 *  @return if success, true (file with key pairs should exist)
 *  @return if error, false
 */
static bool GenerateAndWriteKeyfile(const std::string& path, size_t num_keys
                                    , const std::string& aes_password) {
  std::string output;
  output.reserve(num_keys*(Devcash::kFILE_KEY_SIZE+(Devcash::kWALLET_ADDR_SIZE*2)));
  for (size_t i=0; i<num_keys; ++i) {
    std::string addr;
    std::string key;
    GenerateEcKey(addr, key, aes_password);
    output += addr+key;
  }
  std::ofstream out_file(path);
  if (out_file.is_open()) {
    out_file << output;
    out_file.close();
  } else {
    LOG_WARNING << "Failed write keys to file '" << path << "'.";
    return false;
  }
  return true;
}

/** Loads an existing EC_KEY based on the provided key pair.
 *  @pre an OpenSSL context must have been initialized
 *  @param publicKey [in] reference compressed as hex
 *  @param privKey [in] references private key with ASCII armor, pem, aes
 *  @param aes_password [in] the password for aes
 *  @return if success, a pointer to the EC_KEY object
 *  @return if error, a NULLPTR
 */
static EC_KEY* LoadEcKey(const std::string& publicKey
                       , const std::string& privKey
                       , const std::string& aes_password) {
  try {
    EC_GROUP* ecGroup;
    if (publicKey.size() == kWALLET_ADDR_SIZE*2) {
      ecGroup = getWalletGroup();
    } else if (publicKey.size() == kNODE_ADDR_SIZE*2) {
      ecGroup = getNodeGroup();
    } else {
      throw std::runtime_error("Invalid public key!");
	}
    if (NULL == ecGroup) {
      throw std::runtime_error("Failed to generate EC group.");
    }

    EC_KEY *eckey=EC_KEY_new();
    if (NULL == eckey) {
      LOG_ERROR << "Failed to allocate EC key.";
    }

    EC_GROUP_set_asn1_flag(ecGroup, OPENSSL_EC_NAMED_CURVE);
    if (1 != EC_KEY_set_group(eckey, ecGroup)) {
      LOG_ERROR << "Failed to set EC group status.";
    }

    EC_POINT* tempPoint = NULL;
    const char* pubKeyBuffer = &publicKey[0u];
    const EC_POINT* pubKeyPoint = EC_POINT_hex2point(EC_KEY_get0_group(eckey), pubKeyBuffer, tempPoint, NULL);
    if (eckey == NULL) { LOG_ERROR << "Invalid public key point."; }

    OpenSSL_add_all_algorithms();
    EVP_PKEY* pkey = EVP_PKEY_new();
    char buffer[privKey.size()+1];
    strcpy(buffer, privKey.c_str());
    BIO* fIn = BIO_new(BIO_s_mem());
    BIO_write(fIn, buffer, privKey.size()+1);
    pkey = PEM_read_bio_PrivateKey(fIn, NULL, NULL
           , const_cast<char*>(aes_password.c_str()));
    eckey = EVP_PKEY_get1_EC_KEY(pkey);
    EC_KEY_set_public_key(eckey, pubKeyPoint);
    EVP_cleanup();

    if (1 != EC_GROUP_check(EC_KEY_get0_group(eckey), NULL)) {
      LOG_ERROR << "group is invalid!";
    }

    if (1 != EC_KEY_check_key(eckey)) {
      LOG_ERROR << "key is invalid!";
    }
    return(eckey);
  } catch(const std::exception& e) {
    LOG_WARNING << Devcash::FormatException(&e, "Crypto");
  }
  return nullptr;
}

/** Loads an existing public EC_KEY based on the provided public key.
 *  @pre an OpenSSL context must have been initialized
 *  @param public_key [in] reference compressed as Address
 *  @return if success, a pointer to the EC_KEY object
 *  @return if error, a NULLPTR
 */
static EC_KEY* LoadPublicKey(const Devcash::Address& public_key) {
  try {
    EC_GROUP* ecGroup;
    if (public_key.isWalletAddress()) {
      ecGroup = getWalletGroup();
    } else if (public_key.isNodeAddress()) {
      ecGroup = getNodeGroup();
    } else {
      throw std::runtime_error("Invalid public key!");
	}
    if (NULL == ecGroup) {
      throw std::runtime_error("Failed to generate EC group.");
    }

    EC_KEY *eckey=EC_KEY_new();
    if (NULL == eckey) {
      LOG_ERROR << "Failed to allocate EC key.";
    }

    EC_GROUP_set_asn1_flag(ecGroup, OPENSSL_EC_NAMED_CURVE);
    if (1 != EC_KEY_set_group(eckey, ecGroup)) {
      LOG_ERROR << "Failed to set EC group status.";
    }

    std::string publicKey(public_key.getJSON());
    publicKey.erase(0,1);
    EC_POINT* tempPoint = NULL;
    const char* pubKeyBuffer = &publicKey[0u];
    const EC_POINT* pubKeyPoint = EC_POINT_hex2point(EC_KEY_get0_group(eckey), pubKeyBuffer, tempPoint, NULL);
    if (eckey == NULL) { LOG_ERROR << "Invalid public key point."; }

    EC_KEY_set_public_key(eckey, pubKeyPoint);
    EVP_cleanup();

    if (1 != EC_GROUP_check(EC_KEY_get0_group(eckey), NULL)) {
      LOG_ERROR << "group is invalid!";
    }

    if (1 != EC_KEY_check_key(eckey)) {
      LOG_ERROR << "key is invalid!";
    }
    return(eckey);
  } catch (const std::exception& e) {
    LOG_WARNING << Devcash::FormatException(&e, "Crypto");
  }
  return nullptr;
}

/** Hashes a bytestring with SHA-256.
 *  @note none of the parameters of this function are const.
 *  Parameters may be altered by this function.
 *  @param[in] msg the bytestring to hash
 *  @return the calculated hash
 */
static Devcash::Hash DevcashHash(const std::vector<Devcash::byte>& msg) {
  Devcash::Hash md;
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, &msg[0], msg.size());
  SHA256_Final(&md[0], &sha256);
  return md;
}

/** Verifies the signature corresponding to a given string
 *  @param ecKey pointer to the public key that will check this signature
 *  @param msg pointer to the message digest to verify
 *  @param msg_size the size of the message
 *  @param sig pointer to the binary signature to verify
 *  @param sig_size size of the signature, should be 72 bytes
 *  @return true iff the signature verifies with the provided key and context
 *  @return false otherwise
 */
static bool VerifyByteSig(EC_KEY* ecKey, const Devcash::Hash& msg
    , const Devcash::Signature& sig) {
  CASH_TRY {
    EVP_MD_CTX *ctx;
    if(!(ctx = EVP_MD_CTX_create())) {
      LOG_FATAL << "Could not create signature context!";
    }
    Devcash::Hash temp = msg;
    unsigned char* copy_sig = (unsigned char*) malloc(Devcash::kSIG_SIZE+1);
    for (size_t i=0; i<Devcash::kSIG_SIZE; ++i) {
      copy_sig[i] = sig[i];
    }
    ECDSA_SIG *signature = d2i_ECDSA_SIG(NULL
        , (const unsigned char**) &copy_sig
        , Devcash::kSIG_SIZE);
    int state = ECDSA_do_verify((const unsigned char*) &temp[0]
        , SHA256_DIGEST_LENGTH, signature, ecKey);

    return(1 == state);
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << Devcash::FormatException(&e, "Crypto.verifySignature");
  }
  return(false);
}

/** Generates the signature for a given string and key pair
 *  @pre the EC_KEY must include a private key
 *  @pre the OpenSSL context must be intialized
 *  @param ec_key pointer to the public key that will check this signature
 *  @param msg pointer to the message digest to sign
 *  @param len length of binary message to sign
 *  @param sig target buffer where signature is put, must be allocted
 *  @throws std::exception on error
 */
static void SignBinary(EC_KEY* ec_key, const Devcash::Hash& msg, Devcash::Signature& sig) {
  CASH_TRY {
    Devcash::Hash temp = msg;
    ECDSA_SIG *signature = ECDSA_do_sign((const unsigned char*) &temp[0],
        SHA256_DIGEST_LENGTH, ec_key);
    int state = ECDSA_do_verify((const unsigned char*) &temp[0],
        SHA256_DIGEST_LENGTH, signature, ec_key);

    //0 -> invalid, -1 -> openssl error
    if (1 != state) {
      LOG_ERROR << "Signature did not validate("+std::to_string(state)+")";
    }

    int len = i2d_ECDSA_SIG(signature, NULL);
    unsigned char* ptr = &sig[0];
    memset(&sig[0], 6, len);
    len = i2d_ECDSA_SIG(signature, &ptr);
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << Devcash::FormatException(&e, "Crypto.sign");
  }
}

#endif /* SRC_COMMON_OSSLADAPTER_H_ */
