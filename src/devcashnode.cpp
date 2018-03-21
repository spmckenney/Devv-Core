/*
 * devcashnode.cpp handles orchestration of modules, startup, shutdown,
 * and response to signals.
 *
 *  Created on: Mar 20, 2018
 *  Author: Nick Williams
 */

#include "devcashnode.h"

#include <atomic>
#include <functional>
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

#include "concurrency/WorkerTypes.h"
#include "consensus/chainstate.h"
#include "common/json.hpp"
#include "common/logger.h"
#include "common/ossladapter.h"
#include "common/util.h"
#include "concurrency/DevcashWorkerPool.h"
#include "io/zhelpers.hpp"
#include "oracles/api.h"
#include "oracles/data.h"
#include "oracles/dcash.h"
#include "oracles/dnero.h"
#include "oracles/dneroavailable.h"
#include "oracles/dnerowallet.h"
#include "oracles/id.h"
#include "oracles/vote.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "types/DevcashMessage.h"

using namespace Devcash;
using json = nlohmann::json;

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

namespace Devcash {

std::atomic<bool> fRequestShutdown(false); /** has a shutdown been requested? */
bool isCryptoInit = false;
DevcashContext appContext;

zmq::context_t zmqContext(zmq_ctx_new());
io::TransactionClient client(zmqContext);
io::TransactionServer server(zmqContext, "self");

DCState* chainState = new DCState();
ConsensusWorker consensus(chainState, &server, kCONSENSUS_THREADS);
ValidatorWorker validator(chainState, &consensus, kVALIDATOR_THREADS);


void DevcashNode::StartShutdown()
{
  Shutdown();
}
bool DevcashNode::ShutdownRequested()
{
  return fRequestShutdown;
}

void DevcashNode::Shutdown()
{
  fRequestShutdown = true;
  //TODO: how to stop zmq?
  consensus.stopAll();
  validator.stopAll();
  LOG_INFO << "Shutting down DevCash";
}

DevcashNode::DevcashNode(std::string mode, int nodeIndex) : appContext() {
  eAppMode appMode;
  if (mode == "T1") {
    appMode = T1;
    LOG_INFO << "Configuring T1 node.\n";
  } else if (mode == "T2") {
    appMode = T2;
    LOG_INFO << "Configuring T2 node.\n";
  } else if (mode == "scan") {
    appMode = scan;
    LOG_INFO << "Configuring scanner.\n";
  } else {
    LOG_FATAL << "Invalid mode";
    CASH_THROW("Invalid mode: "+mode+"!\n");
  }
  appContext.currentNode=nodeIndex;
  appContext.appMode=appMode;
}

bool initCrypto()
{
  CASH_TRY {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    isCryptoInit = true;
    return true;
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "DevcashNode.initCrypto");
  }
  return(false);
}

bool DevcashNode::Init()
{
  if (appContext.currentNode < 0
      || appContext.currentNode >= appContext.kNODE_KEYs.size()
      || appContext.currentNode >= appContext.kNODE_ADDRs.size()) {
    LOG_FATAL << "Invalid node index: "+
      std::to_string(appContext.currentNode)+"\n";
    return false;
  }
  return initCrypto();
}

bool DevcashNode::SanityChecks()
{
  bool retVal = false;
  CASH_TRY {
    if (!isCryptoInit) initCrypto();
    EVP_MD_CTX *ctx;
    if(!(ctx = EVP_MD_CTX_create())) {
      LOG_FATAL << "Could not create signature context!";
      return false;
    }

    std::string msg("hello");
    std::string hash("");
    hash = strHash(msg);

    EC_KEY* loadkey = loadEcKey(ctx,
        appContext.kADDRs[1],
        appContext.kADDR_KEYs[1]);

    std::string sDer = sign(loadkey, hash);
    return(verifySig(loadkey, hash, sDer));
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "DevcashNode.initCrypto");
  }
  return(retVal);
}

std::string DevcashNode::RunScanner(std::string inStr) {
  LOG_INFO << "Scanner Mode";
  std::string out("");
  CASH_TRY {
    std::vector<uint8_t> buffer = hex2CBOR(out);
    json j = json::from_cbor(buffer);
    out = j.dump();
    //remove escape chars
    out.erase(remove(out.begin(), out.end(), '\\'), out.end());
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "DevcashNode.RunScanner");
  }
  return out;
}

std::string DevcashNode::RunNode(std::string inStr)
{
  std::string out("");
  CASH_TRY {
    client.AttachCallback([this](std::unique_ptr<DevcashMessage> ptr) {
      if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
          validator.push(std::move(ptr));
        } else {
          consensus.push(std::move(ptr));
        }
    });

    validator.start();
    consensus.start();
    std::vector<uint8_t> data(100);
    auto startMsg = std::unique_ptr<DevcashMessage>(
        new DevcashMessage("self", TRANSACTION_ANNOUNCEMENT, data));
    server.QueueMessage(std::move(startMsg));
    //client.Run();
  } CASH_CATCH (const std::exception& e) {
    FormatException(&e, "DevcashNode.RunScanner");
  }

  return out;
}

} //end namespace Devcash
