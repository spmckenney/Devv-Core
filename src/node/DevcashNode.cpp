/*
 * DevcashNode.cpp handles orchestration of modules, startup, shutdown,
 * and response to signals.
 *
 *  Created on: Mar 20, 2018
 *  Author: Nick Williams
 */

#include "DevcashNode.h"

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
#include <boost/thread/thread.hpp>

#ifndef WIN32
#include <signal.h>
#endif

#include "consensus/chainstate.h"
#include "consensus/tier2_message_handlers.h"
#include "io/zhelpers.hpp"
#include "oracles/api.h"
#include "oracles/data.h"
#include "oracles/dcash.h"
#include "oracles/dnero.h"
#include "oracles/dneroavailable.h"
#include "oracles/dnerowallet.h"
#include "oracles/id.h"
#include "oracles/vote.h"
#include "primitives/Transaction.h"
#include "types/DevcashMessage.h"

namespace Devcash
{

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
  /// @todo (mckenney): how to stop zmq?
  LOG_INFO << "Shutting down DevCash";
  //control_.stopAll();
}

DevcashNode::DevcashNode(DevcashController& control, DevcashContext& context)
    : control_(control), app_context_(context)
{
  LOG_INFO << "Hello from node: " << app_context_.get_uri() << "!!";

  DevcashMessageCallbacks callbacks;
  callbacks.blocks_since_cb = HandleBlocksSince;
  callbacks.blocks_since_request_cb = HandleBlocksSinceRequest;
  callbacks.final_block_cb = HandleFinalBlock;
  callbacks.proposal_block_cb = HandleProposalBlock;
  callbacks.transaction_announcement_cb = CreateNextProposal;
  callbacks.validation_block_cb = HandleValidationBlock;

  control_.setMessageCallbacks(callbacks);
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
  /*if (app_context_.get_current_node() >= app_context_.kNODE_KEYs.size() ||
      app_context_.get_current_node() >= app_context_.kNODE_ADDRs.size()) {
    LOG_FATAL << "Invalid node index: " <<
      std::to_string(app_context_.get_current_node());
    return false;
  }*/
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

    std::vector<byte> msg = {'h', 'e', 'l', 'l', 'o'};
    Hash test_hash(DevcashHash(msg));
    std::string sDer;

    EC_KEY* loadkey = LoadEcKey(app_context_.kADDRs[1],
        app_context_.kADDR_KEYs[1]);

    Signature sig;
    SignBinary(loadkey, test_hash, sig);

    if (!VerifyByteSig(loadkey, test_hash, sig)) {
      return false;
    }

    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashNode.sanityChecks");
  }
  return false;
}

std::string DevcashNode::RunScanner() {
  LOG_INFO << "Scanner Mode";
  std::string out("");
  CASH_TRY {
    /*std::vector<uint8_t> buffer = Hex2Bin(inStr);
    json j = json::from_cbor(buffer);
    out = j.dump();*/
    //remove escape chars
    out.erase(remove(out.begin(), out.end(), '\\'), out.end());
  } CASH_CATCH (const std::exception& e) {
    LOG_ERROR << FormatException(&e, "DevcashNode.RunScanner");
  }
  return out;
}

std::vector<byte> DevcashNode::RunNode()
{
  std::vector<byte> out;
  CASH_TRY {
    LOG_INFO << "Start controller.";
    return control_.Start();

  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashNode.RunScanner");
  }

  return out;
}

std::string DevcashNode::RunNetworkTest(unsigned int)
{
  std::string out("");
  CASH_TRY {
    //control_.StartToy(node_index);

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

}
