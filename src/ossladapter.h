/*
 * ossladapter.h
 *
 *  Created on: Jan 10, 2018
 *      Author: Nick Williams
 */

#ifndef SRC_OSSLADAPTER_H_
#define SRC_OSSLADAPTER_H_

#include <string.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/crypto.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "util.h"

#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && not defined(DEVCASH_NOEXCEPTION)
    #define CASH_THROW(exception) throw exception
    #define CASH_TRY try
    #define CASH_CATCH(exception) catch(exception)
#else
    #define CASH_THROW(exception) std::abort()
    #define CASH_TRY if(true)
    #define CASH_CATCH(exception) if(false)
#endif

static const char alpha[] = "0123456789ABCDEF";
static const char* pwd = "password";  //password for aes pem

/*static std::string toHex(char* input) {
	int len = sizeof(input);
	std::stringstream ss;
	for (int j=0; j<len; j++) {
		int c = (int) input[j];
		ss.put(alpha[(c>>4)&0xF]);
		ss.put(alpha[c&0xF]);
	}
	return(ss.str());
}*/

static std::string toHex(char* input, int len) {
	std::stringstream ss;
	for (int j=0; j<len; j++) {
		int c = (int) input[j];
		ss.put(alpha[(c>>4)&0xF]);
		ss.put(alpha[c&0xF]);
	}
	return(ss.str());
}

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
	CASH_THROW("Invalid hex string");
	return(-1);
}

static char *hex2Bytes(std::string hex, char* buffer) {
	int len = hex.length();
	for (int i=0;i<len/2;i++) {
		buffer[i] = char2int(hex.at(i*2))*16+char2int(hex.at(1+i*2));
	}
	buffer[len/2] = '\0';
	return(buffer);
}

static EC_GROUP* getEcGroup() {
	return(EC_GROUP_new_by_curve_name(NID_secp256k1));
}

static EC_GROUP* getInnGroup() {
	return(EC_GROUP_new_by_curve_name(NID_secp384r1));
}

static EC_KEY* genEcKey(EVP_MD_CTX *ctx, std::string &publicKey, std::string &pkHex) {
	EC_GROUP* ecGroup = getEcGroup();
	if (NULL == ecGroup) {
		CASH_THROW("Failed to generate EC group.");
	}

	EC_KEY *eckey=EC_KEY_new();
	if (NULL == eckey) {
		CASH_THROW("Failed to allocate EC key.");
	}
	EC_GROUP_set_asn1_flag(ecGroup, OPENSSL_EC_NAMED_CURVE);
	int state = EC_KEY_set_group(eckey, ecGroup);
	if (1 != state) CASH_THROW("Failed to set EC group status.");
	state = EC_KEY_generate_key(eckey);
	if (1 != state) CASH_THROW("Failed to generate EC key.");

	OpenSSL_add_all_algorithms();
	EVP_PKEY* pkey = EVP_PKEY_new();
	if (!EVP_PKEY_set1_EC_KEY(pkey, eckey)) {
		CASH_THROW("Could not export private key");
	}
	const EVP_CIPHER* cipher = EVP_aes_128_cbc();
	//BIO* fileOut = BIO_new_file("pk.key", "w");
	BIO* fOut = BIO_new(BIO_s_mem());
	int result = PEM_write_bio_PKCS8PrivateKey(fOut, pkey, cipher, NULL, 0, NULL,
			const_cast<char*>(pwd));
	if (result != 1) CASH_THROW("Failed to generate PEM private key file");
	std::string pkPem("");
	char buffer[1024];
	while (BIO_read(fOut, buffer, 1024) > 0) {
		pkPem += buffer;
	}
	BIO_free(fOut);
	EVP_cleanup();

	if (1 != EC_GROUP_check(EC_KEY_get0_group(eckey), NULL)) {
		CASH_THROW("group is invalid!");
	}

	if (1 != EC_KEY_check_key(eckey)) {
		std::cout << "key is invalid!" << std::endl;
	}

	const EC_POINT *pubKey = EC_KEY_get0_public_key(eckey);
	publicKey = EC_POINT_point2hex(ecGroup, pubKey, POINT_CONVERSION_COMPRESSED, NULL);

	return(eckey);
}

static EC_KEY* loadEcKey(EVP_MD_CTX *ctx, std::string &publicKey, std::string &privKey) {
	EC_GROUP* ecGroup = getEcGroup();
	if (NULL == ecGroup) {
		CASH_THROW("Failed to generate EC group.");
	}

	EC_KEY *eckey=EC_KEY_new();
	if (NULL == eckey) {
		CASH_THROW("Failed to allocate EC key.");
	}
	EC_GROUP_set_asn1_flag(ecGroup, OPENSSL_EC_NAMED_CURVE);
	int state = EC_KEY_set_group(eckey, ecGroup);
	if (1 != state) CASH_THROW("Failed to set EC group status.");

	EC_POINT* tempPoint = NULL;
	const char* pubKeyBuffer = &publicKey[0u];
	const EC_POINT* pubKeyPoint = EC_POINT_hex2point(EC_KEY_get0_group(eckey), pubKeyBuffer, tempPoint, NULL);
	if (eckey == NULL) CASH_THROW("Invalid public key point.");

	OpenSSL_add_all_algorithms();
	EVP_PKEY* pkey = EVP_PKEY_new();
	char buffer[privKey.size()+1];
	strcpy(buffer, privKey.c_str());
	BIO* fIn = BIO_new(BIO_s_mem());
	BIO_write(fIn, buffer, privKey.size()+1);
	//BIO* fileIn = BIO_new_file("pk.key", "r");
	//if (fileIn == NULL) CASH_THROW("Could not open PEM file.");
	pkey = PEM_read_bio_PrivateKey(fIn, NULL, NULL, const_cast<char*>(pwd));
	eckey = EVP_PKEY_get1_EC_KEY(pkey);
	EC_KEY_set_public_key(eckey, pubKeyPoint);
	EVP_cleanup();

	if (1 != EC_GROUP_check(EC_KEY_get0_group(eckey), NULL)) {
		CASH_THROW("group is invalid!");
	}

	if (1 != EC_KEY_check_key(eckey)) {
		std::cout << "key is invalid!" << std::endl;
	}
	return(eckey);
}

/*static EC_KEY* loadEcKey(EVP_MD_CTX *ctx, std::string &publicKey, std::string &privKey) {
	EC_GROUP *ecGroup = getEcGroup();
	if (NULL == ecGroup) {
		CASH_THROW("Failed to generate EC group.");
	}
	EC_GROUP_set_asn1_flag(ecGroup, OPENSSL_EC_NAMED_CURVE);
	EC_KEY *eckey = EC_KEY_new_by_curve_name(NID_secp256k1);

	//size_t pubKeyLen = publicKey.length()/2;
	//char* pubKeyBuffer = (char*) malloc(pubKeyLen+1);
	const char* pubKeyBuffer = &publicKey[0u];
	//hex2Bytes(publicKey, pubKeyBuffer);
	//const unsigned char* pubKeyPtr = reinterpret_cast<const unsigned char*>(pubKeyBuffer);

	if (1 != EC_KEY_set_group(eckey, ecGroup)) CASH_THROW("Failed to set EC group status.");
	//if (1 != EC_KEY_generate_key(eckey)) CASH_THROW("Failed to generate EC key.");
	//val_out, *val_out, (*val_out)->group non null
	EC_POINT* tempPoint = NULL;
	const EC_POINT* pubKeyPoint = EC_POINT_hex2point(EC_KEY_get0_group(eckey), pubKeyBuffer, tempPoint, NULL);
	if (eckey == NULL) CASH_THROW("Invalid public key point.");
	EC_KEY_set_public_key(eckey, pubKeyPoint);
	//eckey = o2i_ECPublicKey(&eckey, &pubKeyPtr, pubKeyLen);
	if (eckey == NULL) CASH_THROW("Invalid public key.");

	//free(pubKeyBuffer);
	if (privKey.length() > 1) {
		size_t pkLen = privKey.length()/2+1;
		char* pkPtr = (char*) malloc(pkLen);
		hex2Bytes(privKey, pkPtr);
		BIGNUM *inKey = BN_new();
		BN_bin2bn((const unsigned char*) pkPtr, pkLen, inKey);
		EC_KEY_set_private_key(eckey, inKey);
		free(pkPtr);
	}
	const BIGNUM *pk = EC_KEY_get0_private_key(eckey);
	unsigned char pkArray[BN_num_bits(pk)];
	BN_bn2bin(pk, pkArray);
	std::string pkHex = toHex(reinterpret_cast<char *>(pkArray));

	const EC_POINT *pubKey = EC_KEY_get0_public_key(eckey);
	std::string newKey = EC_POINT_point2hex(ecGroup, pubKey, POINT_CONVERSION_COMPRESSED, NULL);
	return(eckey);
}*/

static std::string strHash(std::string msg) {
	unsigned char md[SHA256_DIGEST_LENGTH];
	char temp[msg.length()+1];
	strcpy(temp, msg.c_str());
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, temp, strlen(temp));
	SHA256_Final(md, &sha256);
	return(toHex((char*) md, SHA256_DIGEST_LENGTH));
}

static void dcHash(std::string msg, char* hash) {
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
}

static bool verifySig(EC_KEY *eckey, std::string msg, std::string strSig) {
	CASH_TRY {
		EVP_MD_CTX *ctx;
		if(!(ctx = EVP_MD_CTX_create())) LogPrintStr("Could not create signature context!");
		int len = strSig.length();
		if (len%2==1) {
			CASH_THROW("Invalid signature hex!");
			return(false);
		}
		char temp[msg.length()+1];
		strcpy(temp, msg.c_str());

		unsigned char *newSig = (unsigned char*) malloc(len/2+1);
		hex2Bytes(strSig, (char*) newSig);
		ECDSA_SIG *signature = d2i_ECDSA_SIG(NULL, (const unsigned char**) &newSig, len/2);
		int state = ECDSA_do_verify((const unsigned char*) temp, strlen(temp), signature, eckey);
		return(1 == state);
	} CASH_CATCH (const std::exception* e) {
		FormatException(e, "verifySig", DCLog::CRYPTO);
	}
	return(false);
}

static std::string sign(EC_KEY *eckey, std::string msg) {
	CASH_TRY {
		char temp[msg.length()+1];
		strcpy(temp, msg.c_str());

		ECDSA_SIG *signature = ECDSA_do_sign((const unsigned char*) temp, strlen(temp), eckey);
		int state = ECDSA_do_verify((const unsigned char*) temp, strlen(temp), signature, eckey);
		if (1 != state) CASH_THROW("Signature did not validate("+std::to_string(state)+")");
		//0 -> invalid, -1 -> openssl error

		int len = i2d_ECDSA_SIG(signature, NULL);
		unsigned char *sigBytes = (unsigned char*) malloc(len);
		unsigned char *ptr;
		memset(sigBytes, 6, len);
		ptr=sigBytes;
		len = i2d_ECDSA_SIG(signature, &ptr);
		std::string out = toHex((char*) sigBytes, len);

		free(sigBytes);
		return(out);
	} CASH_CATCH (const std::exception* e) {
		FormatException(e, "verifySig", DCLog::CRYPTO);
	}
	CASH_THROW("Failed to sign message!");
}

#endif /* SRC_OSSLADAPTER_H_ */
