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

std::atomic<bool> request_shutdown(false); /** has a shutdown been requested? */
bool isCryptoInit = false;

std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

void DevcashNode::StartShutdown()
{
  Shutdown();
}

bool DevcashNode::ShutdownRequested()
{
  return request_shutdown;
}

void DevcashNode::Shutdown()
{
  request_shutdown = true;
  /// @todo (mckenney): how to stop zmq?
  LOG_INFO << "Shutting down DevCash";
  control_.StopAll();
}

DevcashNode::DevcashNode(ValidatorController& control, DevcashContext& context)
    : control_(control), app_context_(context)
{
  LOG_INFO << "Hello from node: " << app_context_.get_uri() << "!!";

  std::signal(SIGINT, signal_handler);
  std::signal(SIGABRT, signal_handler);
  std::signal(SIGTERM, signal_handler);
  shutdown_handler = [&](int signal) {
      LOG_INFO << "Received signal ("+std::to_string(signal)+").";
      Shutdown();
  };

  DevcashMessageCallbacks callbacks;
  callbacks.blocks_since_cb = HandleBlocksSince;
  callbacks.blocks_since_request_cb = HandleBlocksSinceRequest;
  callbacks.final_block_cb = HandleFinalBlock;
  callbacks.proposal_block_cb = HandleProposalBlock;
  callbacks.transaction_announcement_cb = CreateNextProposal;
  callbacks.validation_block_cb = HandleValidationBlock;

  control_.setMessageCallbacks(callbacks);
}

bool InitCrypto()
{
  CASH_TRY {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    isCryptoInit = true;
    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashNode.InitCrypto");
  }
  return(false);
}

bool DevcashNode::init()
{
  /// Initialize OpenSSL
  return InitCrypto();
}

bool DevcashNode::SanityChecks()
{
  if (!isCryptoInit) InitCrypto();
  EVP_MD_CTX *ctx;
  if (!(ctx = EVP_MD_CTX_create())) {
    LOG_FATAL << "Could not create signature context!";
    return false;
  }

  std::vector<byte> msg = {'h', 'e', 'l', 'l', 'o'};
  Hash test_hash(DevcashHash(msg));
  std::string sDer;

  EC_KEY *loadkey = LoadEcKey(app_context_.kADDRs[1],
                              app_context_.kADDR_KEYs[1]);

  Signature sig;
  SignBinary(loadkey, test_hash, sig);

  if (!VerifyByteSig(loadkey, test_hash, sig)) {
    return false;
  }
  return true;
}

void DevcashNode::RunNode()
{
  try {
    LOG_INFO << "Start controller.";
    control_.Start();
    LOG_INFO << "Controller stopped.";
  } catch (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "DevcashNode.RunScanner");
  }
}

}
