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
  consensus_.stopAll();
  validator_.stopAll();
  LOG_INFO << "Shutting down DevCash\n";
}

DevcashNode::DevcashNode(eAppMode mode
                         , int node_index
                         , ConsensusWorker& consensus
                         , ValidatorWorker& validator
                         , io::TransactionClient& client
                         , io::TransactionServer& server)
  : app_context_()
  , consensus_(consensus)
  , validator_(validator)
  , client_(client)
  , server_(server)
{
  app_context_.current_node_ = node_index;
  app_context_.app_mode_ = mode;
}

bool initCrypto()
{
  CASH_TRY {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    isCryptoInit = true;
    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashNode.initCrypto");
  }
  return(false);
}

bool DevcashNode::Init()
{
  if (app_context_.current_node_ >= app_context_.kNODE_KEYs.size() ||
      app_context_.current_node_ >= app_context_.kNODE_ADDRs.size()) {
    LOG_FATAL << "Invalid node index: " <<
      std::to_string(app_context_.current_node_);
    return false;
  }
  return initCrypto();
}

bool DevcashNode::SanityChecks()
{
  CASH_TRY {
    if (!isCryptoInit) initCrypto();
    EVP_MD_CTX *ctx;
    if(!(ctx = EVP_MD_CTX_create())) {
      LOG_FATAL << "Could not create signature context!";
      return false;
    }

    std::string msg("hello");
    std::string hash(strHash(msg));

    EC_KEY* loadkey = loadEcKey(ctx,
        app_context_.kADDRs[1],
        app_context_.kADDR_KEYs[1]);

    std::string sDer = sign(loadkey, hash);
    return(verifySig(loadkey, hash, sDer));
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashNode.sanityChecks");
  }
  return false;
}

std::string DevcashNode::RunScanner(std::string) {
  LOG_INFO << "Scanner Mode";
  std::string out("");
  CASH_TRY {
    std::vector<uint8_t> buffer = hex2CBOR(out);
    json j = json::from_cbor(buffer);
    out = j.dump();
    //remove escape chars
    out.erase(remove(out.begin(), out.end(), '\\'), out.end());
  } CASH_CATCH (const std::exception& e) {
    LOG_ERROR << FormatException(&e, "DevcashNode.RunScanner");
  }
  return out;
}

std::string DevcashNode::RunNode()
{
  // Start the TransactionServer running in a thread
  server_.StartServer();

  std::string out("");
  CASH_TRY {
    client_.AttachCallback([this](std::unique_ptr<DevcashMessage> ptr) {
      if (ptr->message_type == TRANSACTION_ANNOUNCEMENT) {
          validator_.push(std::move(ptr));
        } else {
          consensus_.push(std::move(ptr));
        }
    });

    validator_.start();
    consensus_.start();

    client_.StartClient();

    std::vector<uint8_t> data(100);
    auto startMsg = std::unique_ptr<DevcashMessage>(
        new DevcashMessage("self", TRANSACTION_ANNOUNCEMENT, data));
    server_.QueueMessage(std::move(startMsg));
    //client.Run();

	//make sure child threads are actually initialized before shutting down
    boost::this_thread::sleep_for(boost::chrono::milliseconds(20000));
    StartShutdown();
    LOG_INFO << "end of RunNode\n";
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashNode.RunScanner");
  }

  return out;
}

} //end namespace Devcash
