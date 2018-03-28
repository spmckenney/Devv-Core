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
#include "common/devcash_context.h"
#include "common/json.hpp"
#include "common/logger.h"
#include "common/ossladapter.h"
#include "common/util.h"
#include "concurrency/DevcashController.h"
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
  control_.stopAll();
  LOG_INFO << "Shutting down DevCash";
}

/*DevcashNode::DevcashNode(eAppMode mode
                         , int node_index
                         , ConsensusWorker& consensus
                         , ValidatorWorker& validator
                         , io::TransactionClient& client
                         , io::TransactionServer& server)
  : app_context_(node_index, mode)
  , consensus_(consensus)
  , validator_(validator)
  , client_(client)
  , server_(server)
{
}

DevcashNode::DevcashNode(DevcashContext& nodeContext
                         , ConsensusWorker& consensus
                         , ValidatorWorker& validator
                         , io::TransactionClient& client
                         , io::TransactionServer& server)
  : app_context_(nodeContext)
  , consensus_(consensus)
  , validator_(validator)
  , client_(client)
  , server_(server)
{
}*/

DevcashNode::DevcashNode(DevcashController& control, DevcashContext& context)
    : control_(control), app_context_(context)
{
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
        app_context_.kINN_ADDR,
        app_context_.kINN_KEY);

    std::string sDer = sign(loadkey, hash);
    if (!verifySig(loadkey, hash, sDer)) return false;

    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashNode.sanityChecks");
  }
  return false;
}

std::string DevcashNode::RunScanner(std::string inStr) {
  LOG_INFO << "Scanner Mode";
  std::string out("");
  CASH_TRY {
    std::vector<uint8_t> buffer = hex2CBOR(inStr);
    json j = json::from_cbor(buffer);
    out = j.dump();
    //remove escape chars
    out.erase(remove(out.begin(), out.end(), '\\'), out.end());
  } CASH_CATCH (const std::exception& e) {
    LOG_ERROR << FormatException(&e, "DevcashNode.RunScanner");
  }
  return out;
}

std::string DevcashNode::RunNode(std::string& inStr)
{
  std::string out("");
  CASH_TRY {
    LOG_INFO << "Start controller.";
    control_.start();

    control_.seedTransactions(inStr);
    //TODO: block here until all input is processed

  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashNode.RunScanner");
  }

  return out;
}

std::string DevcashNode::RunNetworkTest(unsigned int node_index)
{
  std::string out("");
  CASH_TRY {
    control_.StartToy(node_index);

    //TODO: add messages for each node in concurrency/DevcashController.cpp

	//let the test run before shutting down (<10 sec)
    auto ms = 10000;
    LOG_INFO << "Sleeping for " << ms;
    boost::this_thread::sleep_for(boost::chrono::milliseconds(ms));
    LOG_INFO << "Starting shutdown";
    StartShutdown();
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashNode.RunScanner");
  }

  return out;
}

} //end namespace Devcash
