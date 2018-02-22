/*
 * init.cpp handles orchestration of modules, startup, shutdown,
 * and response to signals.
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
#include <stdint.h>
#include <stdio.h>
#include <memory>
#include <csignal>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/crypto.h>

#ifndef WIN32
#include <signal.h>
#endif

#include "consensus/chainstate.h"
#include "common/ctpl.h"
#include "common/json.hpp"
#include "common/logger.h"
#include "common/ossladapter.h"
#include "primitives/block.h"
#include "primitives/transaction.h"

using namespace Devcash;
using json = nlohmann::json;

const int N_THREADS = 64;  /** Number of threads to use. */

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

#define NOW std::chrono::high_resolution_clock::now()
#define MILLI_SINCE(start) std::chrono::duration_cast<std::chrono::milliseconds>(NOW - start).count()

const char* DevCashContext::DEFAULT_PUBKEY = "-----BEGIN PUBLIC KEY-----MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4KLYaBof/CkaSGQrkmbbGr1C5pLqMXepNJFlqdS2mb3ZnB8Ut07VkFVyYSDS2Dj4AIWZE/JSKwsgTbFgipZ5NANffbOCzSRh+uWTklg6vtZm9jAZYypPVVfUYqxa+Q0dNtLotUW+IefHZ/D6AxuIRsRoZjvx/8Ir/BKoTZm/KuQBwSONWrM5aM6NPWJjUverMtLjwJTXo8VYBFTHfyxFJqjS9PMsjNKva3+nb/SCKoCR2xNxsQHheUSRv33t6XWb9UPfyFmKYjAfiOOZHS9/pABiY0/GsexrMsgDHKXa2mW3EK/z2IYq9sEqFf9YO8E8rWAXjQ58ZKf1yOnaDXcPMQIDAQAB-----END PUBLIC KEY-----";
const char* DevCashContext::DEFAULT_PK = "-----BEGIN ENCRYPTED PRIVATE KEY-----\nMIHeMEkGCSqGSIb3DQEFDTA8MBsGCSqGSIb3DQEFDDAOBAgMnJzrxK/HEAICCAAw\nHQYJYIZIAWUDBAECBBD5kJEZMeZ2JBaiUB1RIdFRBIGQO8Z8P7ktVRBzGR5bUX8I\nXHG3dTLg+WNvU+C8ecFhSOStgpvhAzUPmX/KPJ1vrMQ07Qw0EGxR+TZ1bqUnMMRl\n6WhU/cVHBrThOwrk2Wx80CSc7uzl9UsZKtXwsR7/OXan5cyMxnHo3OVfA/0Vrrhk\n1CBBQjC22Pxw6lVlUijtPrR0sxhHruFSCZoMMu0NF0SK\n-----END ENCRYPTED PRIVATE KEY-----";
std::string pubKeyStr = ""; /** Active public key for benchmarking */
std::string pkStr = ""; /** Active private key for benchmanrking */

std::atomic<bool> fRequestShutdown(false); /** has a shutdown been requested? */

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
  LOG_INFO << "Shutting down DevCash";
}

void InitError(const std::exception* e)
{
  LOG_WARNING << FormatException(e, "init");
}

bool AppInitBasicSetup(ArgsManager &args)
{

  if (!SetupNetworking()) {
    LOG_FATAL << "Initializing networking failed";
    return false;
  }

#ifndef WIN32

  signal(SIGTERM, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGHUP, signalHandler);

  //ignore SIGPIPE
  //otherwise it will bring the daemon down if the client closes unexpectedly
  signal(SIGPIPE, SIG_IGN);
#endif

  CASH_TRY {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    pubKeyStr = args.GetArg("PUBKEY", DevCashContext::DEFAULT_PUBKEY);
    LOG_INFO << "Public Key: "+pubKeyStr;
    //pkStr = args.GetArg("PRIVATE", DevCashContext::DEFAULT_PK);
    LOG_INFO << "Private Key: "+pkStr;
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
    if(!(ctx = EVP_MD_CTX_create())) {
      LOG_FATAL << "Could not create signature context!";
      return false;
    }

    std::string msg("hello");
    std::string hash("");
    hash = strHash(msg);

    EC_KEY* loadkey = loadEcKey(ctx, pubKeyStr, pkStr);

    std::string sDer = sign(loadkey, hash);
    return(verifySig(loadkey, hash, sDer));
  } CASH_CATCH (const std::exception& e) {
    InitError(&e);
  }
  return(retVal);
}

std::string validate_block(DCBlock& newBlock, EC_KEY* eckey)
{
  std::string out = "";
  auto start = NOW;
  auto valid = newBlock.validate(eckey);
  LOG_INFO << "validation() took  : " << MILLI_SINCE(start) << " milliseconds\n";
  if (valid) {
    auto start1 = NOW;
    newBlock.signBlock(eckey);
    LOG_INFO<< "signBlock() took   : " << MILLI_SINCE(start1) << " milliseconds\n";
    start1 = NOW;
    newBlock.finalize();
    LOG_INFO << "finalize() took    : " << MILLI_SINCE(start1) << " milliseconds\n";
    LOG_INFO << "Output block";
    out = newBlock.ToCBOR_str();
  } else {
    LOG_INFO << "No valid transactions";
  }
  LOG_INFO << "Total / block      : " << MILLI_SINCE(start) << " milliseconds\n";
  return(out);
}

std::string AppInitMain(std::string inStr, std::string mode)
{
  std::string out("");
  CASH_TRY {
    if (mode == "mine") {
      LOG_INFO << "Miner Mode\n";
      json outer = json::parse(inStr);
      json j;
      LOG_INFO << std::to_string(outer[kTX_TAG].size())+" transactions\n";
      int txCnt = 0;
      j = outer[kTX_TAG];
      txCnt = j.size();
      std::vector<DCTransaction> txs;
      DCValidationBlock vBlock;
      EVP_MD_CTX *ctx;
      if(!(ctx = EVP_MD_CTX_create()))
        LOG_FATAL << "Could not create signature context!\n";
      if (txCnt < 1) {
        LOG_INFO << "No transactions.  Nothing to do.\n";
        return("");
      } else {
        std::string toPrint(txCnt+" transactions\n");
        LOG_INFO << toPrint;
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
        results[jj] = p.push([&iter, eckey](int){
          return(validate_block(iter, eckey)); });
        jj++;
      }

      std::vector<std::string> outputs;
      for (int i=0; i<jj; i++) {
        outputs.push_back(results[i].get());
      }
      int ttot=MILLI_SINCE(start);
      LOG_INFO << "Total ("<<j.size()*jj<< " / " << ttot << ") - "
        <<j.size()*jj/(float(ttot)/1000.0) << " valids/sec\n";
      out = outputs[0];

    } else if (mode == "scan") {
      LOG_INFO << "Scanner Mode";
      std::vector<uint8_t> buffer = hex2CBOR(out);
      json j = json::from_cbor(buffer);
      out = j.dump();
      //remove escape chars
      out.erase(remove(out.begin(), out.end(), '\\'), out.end());
    }
  } CASH_CATCH (const std::exception& e) {
    InitError(&e);
  }
  return out;
}
