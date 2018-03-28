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

#include "util.h"
#include "logger.h"

//exception toggling capability
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) \
  && not defined(DEVCASH_NOEXCEPTION)
    #define CASH_THROW(exception) throw exception
    #define CASH_TRY try
    #define CASH_CATCH(exception) catch(exception)
#else
    #define CASH_THROW(exception) std::abort()
    #define CASH_TRY if(true)
    #define CASH_CATCH(exception) if(false)
#endif

namespace Devcash
{

static const char alpha[] = "0123456789ABCDEF";  /** chars for hex conversions */
static const char* pwd = "password";  /** password for aes pem */

/** Maps a hex digit to an int value.
 *  @param hex digit to get the int value for
 *  @return int value of this hex digit
*/
static int char2int(char in) {
  if (in >= '0' && in <= '9') {
    return(in - '0');
  }
  if (in >= 'A' && in <= 'F') {
    return(in - 'A' + 10);
  }
  if (in >= 'a' && in <= 'f') {
  return(in - 'a' + 10);
  }
  return(-1);
}

/** Maps a hex string into a byte vector for CBOR parsing.
 *  @param hex a string of hex digits encoding a CBOR message.
 *  @return a byte vector of the same data in binary form.
*/
static std::vector<uint8_t> hex2CBOR(std::string hex) {
  int len = hex.length();
  std::vector<uint8_t> buf(len/2+1);
  for (int i=0;i<len/2;i++) {
    buf.at(i) = (uint8_t) char2int(hex.at(i*2))*16+char2int(hex.at(1+i*2));
  }
  buf.pop_back(); //remove null terminator
  return(buf);
}

/** Change binary data into a hex string.
 *  @pre input must be allocated to a length of len
 *  @param input pointer to the binary data
 *  @param len length of the binary data
 *  @return string containing these data as hex numbers
 */
static std::string toHex(char* input, int len) {
  std::stringstream ss;
  for (int j=0; j<len; j++) {
    int c = (int) input[j];
    ss.put(alpha[(c>>4)&0xF]);
    ss.put(alpha[c&0xF]);
  }
  return(ss.str());
}

/** Maps a hex string into a buffer as binary data.
 *  @pre the buffer should have memory allocated
 *  @pre the buffer size should be half the length of the hex number plus one
 *  @param hex the number to change into binary data
 *  @param buffer an allocated buffer where this data will be written
 *  @return pointer to the buffer
 */
static char* hex2Bytes(std::string hex, char* buffer) {
  int len = hex.length();
  for (int i=0;i<len/2;i++) {
    buffer[i] = char2int(hex.at(i*2))*16+char2int(hex.at(1+i*2));
  }
  buffer[len/2] = '\0';
  return(buffer);
}

/** Gets the EC_GROUP for normal transactions.
 *  @return a pointer to the EC_GROUP
 */
static EC_GROUP* getEcGroup() {
  return(EC_GROUP_new_by_curve_name(NID_secp256k1));
}

/** Gets the EC_GROUP for INN transactions.
 *  @return a pointer to the EC_GROUP
 */
/*static EC_GROUP* getInnGroup() {
  return(EC_GROUP_new_by_curve_name(NID_secp384r1));
}*/

/** Generates a new EC_KEY.
 *  @pre an OpenSSL context must have been initialized
 *  @param ctx pointer to the OpenSSL context
 *  @param publicKey [out] reference with ASCII armor
 *  @param pkHex [out] references private key with ASCII armor, pem, aes
 *  @return if success, a pointer to the EC_KEY object
 *  @return if error, a NULLPTR
 */
/*static EC_KEY* genEcKey(EVP_MD_CTX* ctx, std::string& publicKey, std::string& pk) {
  CASH_TRY {
    EC_GROUP* ecGroup = getEcGroup();
    if (NULL == ecGroup) {
      LOG_ERROR << "Failed to generate EC group.";
    }

    EC_KEY *eckey=EC_KEY_new();
    if (NULL == eckey) {
      LOG_ERROR << "Failed to allocate EC key.";
    }

    EC_GROUP_set_asn1_flag(ecGroup, OPENSSL_EC_NAMED_CURVE);
    int state = EC_KEY_set_group(eckey, ecGroup);
    if (1 != state) LOG_ERROR << "Failed to set EC group status.";
    state = EC_KEY_generate_key(eckey);
    if (1 != state) LOG_ERROR << "Failed to generate EC key.";

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
      const_cast<char*>(pwd));
    if (result != 1) LOG_ERROR << "Failed to generate PEM private key file";
    char buffer[1024];
    while (BIO_read(fOut, buffer, 1024) > 0) {
      pk += buffer;
    }
    BIO_free(fOut);
    EVP_cleanup();

    //BIO* pOut = BIO_new(BIO_s_mem());
    //result = PEM_write_bio_PUBKEY(pOut, pkey);
    //if (result != 1) LOG_ERROR << "Failed to generate PEM public key file";
    //std::string pubPem("");
    //char buffer2[1024];
    //while (BIO_read(pOut, buffer2, 1024) > 0) {
    //  publicKey += buffer2;
    //}
    //BIO_free(pOut);
    //EVP_cleanup();

    const EC_POINT *pubKey = EC_KEY_get0_public_key(eckey);
    publicKey = EC_POINT_point2hex(ecGroup, pubKey, POINT_CONVERSION_COMPRESSED, NULL);

    return eckey;
  } CASH_CATCH(const std::exception& e) {
    LOG_WARNING << FormatException(&e, "Crypto");
  }
  return nullptr;
}*/

/** Loads an existing EC_KEY based on the provided key pair.
 *  @pre an OpenSSL context must have been initialized
 *  @param ctx pointer to the OpenSSL context
 *  @param publicKey [in] reference with ASCII armor
 *  @param pkHex [in] references private key with ASCII armor, pem, aes
 *  @return if success, a pointer to the EC_KEY object
 *  @return if error, a NULLPTR
 */
static EC_KEY* loadEcKey(EVP_MD_CTX* ctx, const std::string& publicKey, const std::string& privKey) {
  CASH_TRY {
    EC_GROUP* ecGroup = getEcGroup();
    if (NULL == ecGroup) {
      LOG_ERROR << "Failed to generate EC group.";
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
    if (eckey == NULL) LOG_ERROR << "Invalid public key point.";

    OpenSSL_add_all_algorithms();
    EVP_PKEY* pkey = EVP_PKEY_new();
    char buffer[privKey.size()+1];
    strcpy(buffer, privKey.c_str());
    BIO* fIn = BIO_new(BIO_s_mem());
    BIO_write(fIn, buffer, privKey.size()+1);
    pkey = PEM_read_bio_PrivateKey(fIn, NULL, NULL, const_cast<char*>(pwd));
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
  } CASH_CATCH(const std::exception& e) {
    LOG_WARNING << FormatException(&e, "Crypto");
  }
  return 0;
}

/** Hashes a string with SHA-256.
 *  @param msg the string to hash
 *  @return a hex string of the hashed value
 */
static std::string strHash(const std::string& msg) {
  unsigned char md[SHA256_DIGEST_LENGTH];
  char temp[msg.length()+1];
  strcpy(temp, msg.c_str());
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, temp, strlen(temp));
  SHA256_Final(md, &sha256);
  std::string out = toHex((char*) md, SHA256_DIGEST_LENGTH);
  return(out);
}

/*static void dcHash(std::string msg, char* hash) {
  unsigned char md[SHA256_DIGEST_LENGTH];
  char temp[msg.length()+1];
  strcpy(temp, msg.c_str());
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, temp, strlen(temp));
  SHA256_Final(md, &sha256);
  int i = 0;
  for (i=0;i<SHA256_DIGEST_LENGTH;i++)
  sprintf(hash + (i*2), "%02x", md[i]);
  hash[SHA256_DIGEST_LENGTH*2] = 0;
}*/

/** Verifies the signature corresponding to a given string
 *  @param ecKey pointer to the public key that will check this signature
 *  @param msg the message digest to verify
 *  @param strSig a hex string of the signature to verify
 *  @return true iff the signature verifies with the provided key and context
 *  @return false otherwise
 */
static bool verifySig(EC_KEY* ecKey, const std::string msg, const std::string strSig) {
  CASH_TRY {
    EVP_MD_CTX *ctx;
    if(!(ctx = EVP_MD_CTX_create()))
      LOG_FATAL << "Could not create signature context!";
    int len = strSig.length();
    if (len%2==1) {
      LOG_ERROR << "Invalid signature hex!";
      return(false);
    }

    char temp[msg.length()+1];
    strcpy(temp, msg.c_str());

    unsigned char *newSig = (unsigned char*) malloc(len/2+1);
    hex2Bytes(strSig, (char*) newSig);
    ECDSA_SIG *signature = d2i_ECDSA_SIG(NULL, (const unsigned char**) &newSig, len/2);
    int state = ECDSA_do_verify((const unsigned char*) temp, strlen(temp), signature, ecKey);

    return(1 == state);
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "Crypto.verifySignature");
  }
  return(false);
}

/** Generates the signature for a given string and key pair
 *  @pre the EC_KEY must include a private key
 *  @pre the OpenSSL context must be intialized
 *  @param ecKey pointer to the public key that will check this signature
 *  @param msg the message digest to sign
 *  @return true a hex string of the signature
 *  @throws std::exception on error
 */
static std::string sign(EC_KEY* ecKey, std::string msg) {
  CASH_TRY {
    char temp[msg.length()+1];
    strcpy(temp, msg.c_str());

    ECDSA_SIG *signature = ECDSA_do_sign((const unsigned char*) temp,
        strlen(temp), ecKey);
    int state = ECDSA_do_verify((const unsigned char*) temp,
        strlen(temp), signature, ecKey);

    //0 -> invalid, -1 -> openssl error
    if (1 != state)
      LOG_ERROR << "Signature did not validate("+std::to_string(state)+")";

    int len = i2d_ECDSA_SIG(signature, NULL);
    unsigned char *sigBytes = (unsigned char*) malloc(len);
    unsigned char *ptr;
    memset(sigBytes, 6, len);
    ptr=sigBytes;
    len = i2d_ECDSA_SIG(signature, &ptr);
    std::string out = toHex((char*) sigBytes, len);

    free(sigBytes);
  return(out);
  } CASH_CATCH (const std::exception& e) {
    LOG_WARNING << FormatException(&e, "Crypto.sign");
  }
  return("");
}

} //end namespace Devcash

#endif /* SRC_COMMON_OSSLADAPTER_H_ */
