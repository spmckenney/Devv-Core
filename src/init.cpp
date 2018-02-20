/*
 * init.cpp
 *
 *  Created on: Dec 10, 2017
 *  Author: Nick Williams
 */

#include "init.h"
#include <atomic>
#include <stdio.h>
#include <string>
#include <iostream>
#include <inttypes.h>

#include "common/json.hpp"
using namespace std;
using json = nlohmann::json;

#include "consensus/statestub.h"
#include "consensus/validation.h"
#include <stdint.h>
#include <stdio.h>
#include <memory>
#include <csignal>

#ifndef WIN32
#include <signal.h>
#endif

#include "common/ctpl.h"

const int N_THREADS = 64;

/*#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/bind.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/thread.hpp>*/

bool fFeeEstimatesInitialized = false;
DCState myState();

#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && not defined(DEVCASH_NOEXCEPTION)
    #define CASH_THROW(exception) throw exception
    #define CASH_TRY try
    #define CASH_CATCH(exception) catch(exception)
#else
    #define CASH_THROW(exception) std::abort()
    #define CASH_TRY if(true)
    #define CASH_CATCH(exception) if(false)
#endif

const char *DevCashContext::DEFAULT_PUBKEY = "-----BEGIN PUBLIC KEY-----MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4KLYaBof/CkaSGQrkmbbGr1C5pLqMXepNJFlqdS2mb3ZnB8Ut07VkFVyYSDS2Dj4AIWZE/JSKwsgTbFgipZ5NANffbOCzSRh+uWTklg6vtZm9jAZYypPVVfUYqxa+Q0dNtLotUW+IefHZ/D6AxuIRsRoZjvx/8Ir/BKoTZm/KuQBwSONWrM5aM6NPWJjUverMtLjwJTXo8VYBFTHfyxFJqjS9PMsjNKva3+nb/SCKoCR2xNxsQHheUSRv33t6XWb9UPfyFmKYjAfiOOZHS9/pABiY0/GsexrMsgDHKXa2mW3EK/z2IYq9sEqFf9YO8E8rWAXjQ58ZKf1yOnaDXcPMQIDAQAB-----END PUBLIC KEY-----";
const char *DevCashContext::DEFAULT_PK = "-----BEGIN PRIVATE KEY-----MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDgothoGh/8KRpIZCuSZtsavULmkuoxd6k0kWWp1LaZvdmcHxS3TtWQVXJhINLYOPgAhZkT8lIrCyBNsWCKlnk0A199s4LNJGH65ZOSWDq+1mb2MBljKk9VV9RirFr5DR020ui1Rb4h58dn8PoDG4hGxGhmO/H/wiv8EqhNmb8q5AHBI41aszlozo09YmNS96sy0uPAlNejxVgEVMd/LEUmqNL08yyM0q9rf6dv9IIqgJHbE3GxAeF5RJG/fe3pdZv1Q9/IWYpiMB+I45kdL3+kAGJjT8ax7GsyyAMcpdraZbcQr/PYhir2wSoV/1g7wTytYBeNDnxkp/XI6doNdw8xAgMBAAECggEAArYUdJU0I5//YDZNTFQPevAj2ZKWXwh5s1e56WXW2l4vPTIm1tuNulM9sSxrPw7Y93ClW1dGZJyaxDVK3AFa7yTHR0YeYwl4YUXaFR8ZfmoqDfigpdDB6l7IAnTgGDdvTdUX1/BCjjg08O04p0byyx/dvrYkgpi+XSmAfIdJhmP6U4Q5iBXiiBMwVDCG/sqC4QWX40LKCcd6NGJW5/RHJInoSAw2rhZOf9ZiCdHkHDvkjsKXkkMJ+LyO062lI1pXykhWmMe+qqaK/3xk+za9MhW7NZEsOiqtCk3PE9c27kybmFqXXRqhRLPZ4vr3M0zWUx4B6zn5ihssyyawPBQsAQKBgQDwI3NzdUf7VUhjzLhfGL4iKXfJR29zs3h2CVH7ZcGpHhTyE+R+a/vfkR2LXR/n/HynQlYZhZ8mm68Ynjpg9UqlA/B4Z/Ap6gVGTEPC3zU1jJox2qSle5aAndO2V/IyUM+n4vCeQ8jZzE0WYtAv73H5se+jtAwl0BCMNeKbWVKAcQKBgQDveUMEwlS2KPmSLWSykR7Hpn4V/UvOjmTWX8+RbNs0hxrlGduiDmVZB7smsCSQbhhfixkmsx4K4KYewfXvqjjVupbCmuFNkf2nECiV9t4ACNvmUtIIgXOf08oaZM895K24IbZdH5VjgXljXsMAulXbG7Ij1SjtXvWf1ykaNSTawQKBgCn9yP5zj7a/Xv00mzjl1rmajru/phmRVIsvbgqL7KVqATejit0gfNbHRWdNTXr/h7ynuO6VkxLpPmELqiGyQu9AFRi49CIgLfPw+hhld6R5ha0aEphtWA/9iTvlfRCXWPh+kpzaNZEATKqRdN4s/L0xBDqYDVe/XmVmNs37fJXBAoGASF1AX0PKDXG8WOvWrg8kWfh5yXNNYRGubwls0+ktJGZfPjPeJs5q2ch4SWyY3/wk6VpDM2qU/Xx9NnYuN0oc+pjzzcK3qpUfLUi4uvhqhWAn8yW7yk4z/mwlemxUI8Piqu2lCebtYbBSWjDchG/KWfe4kRNs1q4HU1HVXdIJXQECgYEAqqhFUHuJT7x2pIYiRsi4LTlJSKRUjN0pMqd2Hh6xlgS7VgiF32l5E+1SLTlKrcyXCZjcIaJrWWK9TQHXLCa+vIi2thcA8BGrdNqVwnXSPVX/o038cefP/Aj/iBGEMazp/olOidDG35EJp1gy00QBjCPDf5/svY3ed1ACw9fZ7k4=-----END PRIVATE KEY-----";
std::string pubKeyStr = "";
std::string pkStr = "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAgMnJzrxK/HEAICCAAw\nHQYJYIZIAWUDBAECBBD5kJEZMeZ2JBaiUB1RIdFRBIGQO8Z8P7ktVRBzGR5bUX8I\nXHG3dTLg+WNvU+C8ecFhSOStgpvhAzUPmX/KPJ1vrMQ07Qw0EGxR+TZ1bqUnMMRl\n6WhU/cVHBrThOwrk2Wx80CSc7uzl9UsZKtXwsR7/OXan5cyMxnHo3OVfA/0Vrrhk\n1CBBQjC22Pxw6lVlUijtPrR0sxhHruFSCZoMMu0NF0SK\n-----END ENCRYPTED PRIVATE KEY-----";

#ifdef WIN32
// Win32 LevelDB doesn't use filedescriptors, and the ones used for
// accessing block files don't count towards the fd_set size limit
// anyway.
#define MIN_CORE_FILEDESCRIPTORS 0
#else
#define MIN_CORE_FILEDESCRIPTORS 150
#endif

std::atomic<bool> fRequestShutdown(false);
std::atomic<bool> fDumpMempoolLater(false);

void StartShutdown()
{
    fRequestShutdown = true;
}
bool ShutdownRequested()
{
    return fRequestShutdown;
}

void Shutdown()
{
  LogPrintStr("Shutting down DevCash");
}

void InitError(const std::exception* e)
{
  FormatException(e, "main", DCLog::INIT);
}

bool AppInitBasicSetup(ArgsManager &args)
{

    if (!SetupNetworking()) return LogPrintStr("Initializing networking failed");

#ifndef WIN32

    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGHUP, signalHandler);

    // Ignore SIGPIPE, otherwise it will bring the daemon down if the client closes unexpectedly
    signal(SIGPIPE, SIG_IGN);
#endif

    CASH_TRY {
  OpenSSL_add_all_algorithms();
  ERR_load_crypto_strings();
  pubKeyStr = args.GetArg("PUBKEY", DevCashContext::DEFAULT_PUBKEY);
  LogPrintStr("Public Key: "+pubKeyStr);
  //pkStr = args.GetArg("PRIVATE", DevCashContext::DEFAULT_PK);
  LogPrintStr("Private Key: "+pkStr);
  } CASH_CATCH (const std::exception& e) {
  InitError(&e);
  }

    return true;
}

bool AppInitSanityChecks()
{
  bool retVal = false;
  CASH_TRY {
  EVP_MD_CTX *ctx;
  if(!(ctx = EVP_MD_CTX_create())) LogPrintStr("Could not create signature context!");

  std::string msg("hello");
  char hash[SHA256_DIGEST_LENGTH*2+1];
  dcHash(msg, hash);

  EC_KEY* loadkey = loadEcKey(ctx, pubKeyStr, pkStr);

  std::string sDer = sign(loadkey, msg);
  return(verifySig(loadkey, msg, sDer));
  } CASH_CATCH (const std::exception& e) {
  InitError(&e);
  }
    return(retVal);
}

static std::vector<uint8_t> hex2CBOR(std::string hex) {
  int len = hex.length();
  std::vector<uint8_t> buf(len/2+1);
  for (int i=0;i<len/2;i++) {
  buf.at(i) = (uint8_t) char2int(hex.at(i*2))*16+char2int(hex.at(1+i*2));
  }
  buf.pop_back(); //remove null terminator
  return(buf);
}

/*static std::string CBOR2hex(std::vector<uint8_t> b) {
  int len = b.size();
  std::stringstream ss;
  for (int j=0; j<len; j++) {
  int c = (int) b[j];
  ss.put(alpha[(c>>4)&0xF]);
  ss.put(alpha[c&0xF]);
  }
  return(ss.str());
}*/


#define NOW std::chrono::high_resolution_clock::now()
#define MILLI_SINCE(start) std::chrono::duration_cast<std::chrono::milliseconds>(NOW - start).count()

std::string validate_block(DCBlock& newBlock, EC_KEY* eckey)
{
  std::string out = "";
  auto start = NOW;
  auto valid = newBlock.validate(eckey);
  std::cout << "validation() took  : " << MILLI_SINCE(start) << " milliseconds\n";
  if (valid) {
    auto start1 = NOW;
    newBlock.signBlock(eckey);
    std::cout << "signBlock() took   : " << MILLI_SINCE(start1) << " milliseconds\n";
    start1 = NOW;
    newBlock.finalize();
    std::cout << "finalize() took    : " << MILLI_SINCE(start1) << " milliseconds\n";
    LogPrintStr("Output block");
    out = newBlock.ToCBOR_str();
  } else {
    LogPrintStr("No valid transactions");
  }
  std::cout << "Total / block      : " << MILLI_SINCE(start) << " milliseconds\n";
  return(out);
}

std::string AppInitMain(std::string inStr, std::string mode)
{
  std::string out("");
  CASH_TRY {
  if (mode == "mine") {
  LogPrintStr("Miner Mode");
  json outer = json::parse(inStr);
  json j;
  LogPrintStr(std::to_string(outer[TX_TAG].size())+" transactions");
  int txCnt = 0;
  j = outer[TX_TAG];
  txCnt = j.size();
  std::vector<DCTransaction> txs;
  DCValidationBlock vBlock;
  EVP_MD_CTX *ctx;
  if(!(ctx = EVP_MD_CTX_create())) LogPrintStr("Could not create signature context!");
  if (txCnt < 1) {
  LogPrintStr("No transactions.  Nothing to do.");
  return("");
  } else {
  std::string toPrint(txCnt+" transactions");
  LogPrintStr(toPrint);
  for (auto iter = j.begin(); iter != j.end(); ++iter) {
  std::string tx = iter.value().dump();
  DCTransaction* t = new DCTransaction(tx);
  txs.push_back(*t);
  }
  }
  EC_KEY* eckey = loadEcKey(ctx, pubKeyStr, pkStr);
  std::vector<DCBlock> blocks;
  for (int i = 0; i<128; i++) {
    blocks.push_back(DCBlock(txs, vBlock));
  }

  // Create pool with N threads
  ctpl::thread_pool p(N_THREADS);
  std::vector<std::future<std::string>> results(blocks.size());

  auto start = NOW;
  int jj = 0;
  for (auto iter : blocks) {
    results[jj] = p.push([&iter, eckey](int){ return(validate_block(iter, eckey)); });
    jj++;
  }

  std::vector<std::string> outputs;
  for (int i=0; i<jj; i++) {
    outputs.push_back(results[i].get());
  }
  int ttot=MILLI_SINCE(start);
  std::cout << "Total ("<<j.size()*jj<< " / " << ttot << ") - "
    <<j.size()*jj/(float(ttot)/1000.0) << " valids/sec\n";
  out = outputs[0];

  } else if (mode == "scan") {
  LogPrintStr("Scanner Mode");
  std::vector<uint8_t> buffer = hex2CBOR(out);
  json j = json::from_cbor(buffer);
  out = j.dump();
  out.erase(remove(out.begin(), out.end(), '\\'), out.end()); //remove escape chars
  }
  } CASH_CATCH (const std::exception& e) {
  InitError(&e);
  }
    return out;
}
