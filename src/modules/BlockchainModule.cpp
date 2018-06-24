/*
 * BlockchainModule.cpp handles orchestration of modules, startup, shutdown,
 * and response to signals.
 *
 *  Created on: Mar 20, 2018
 *  Author: Nick Williams
 *          Shawn McKenney
 */

#include "BlockchainModule.h"

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
#include "common/devcash_exceptions.h"

namespace Devcash
{

std::atomic<bool> request_shutdown(false); /** has a shutdown been requested? */
bool isCryptoInit = false;

std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

bool InitCrypto()
{
  CASH_TRY {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    isCryptoInit = true;
    return true;
  } CASH_CATCH (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "BlockchainModule.InitCrypto");
  }
  return(false);
}

BlockchainModule::BlockchainModule(io::TransactionServer& server,
                                 io::TransactionClient& client,
                                 const KeyRing& keys,
                                 const ChainState &prior,
                                 eAppMode mode,
                                 DevcashContext &context,
                                 size_t max_tx_per_block)
    : server_(server),
      client_(client),
      keys_(keys),
      prior_(prior),
      mode_(mode),
      final_chain_("final_chain_"),
      utx_pool_(prior, mode, max_tx_per_block),
      app_context_(context)
{
  LOG_INFO << "Hello from node: " << app_context_.get_uri() << "!!";
}

std::unique_ptr<BlockchainModule> BlockchainModule::Create(io::TransactionServer &server,
                                        io::TransactionClient &client,
                                        const KeyRing &keys,
                                        const ChainState &prior,
                                        eAppMode mode,
                                        DevcashContext &context,
                                        size_t max_tx_per_block) {

  /// Create the ValidatorModule which holds all of the controllers
  auto blockchain_module_ptr = std::make_unique<BlockchainModule>(server,
                                     client,
                                     keys,
                                     prior,
                                     mode,
                                     context,
                                     max_tx_per_block);

  /**
   * Initialize ValidatorController
   *
   * Create the ValidatorController to handle validation messages
   */
  ValidatorController validator_controller(keys,
                                           context,
                                           prior,
                                           blockchain_module_ptr->getFinalChain(),
                                           blockchain_module_ptr->getTransactionPool(),
                                           mode);

  /// Register the outgoing callback to send over zmq
  auto outgoing_callback =
      [&](DevcashMessageUniquePtr p) { blockchain_module_ptr->server_.queueMessage(std::move(p)); };

  validator_controller.registerOutgoingCallback(outgoing_callback);

  /// The controllers contain the the algorithms, the ParallelExecutor parallelizes them
  blockchain_module_ptr->validator_ =
      std::make_unique<ParallelExecutor<ValidatorController>>(validator_controller, context, 1);

  /// Attach a callback to be run in the threads
  blockchain_module_ptr->validator_->attachCallback(
      [&](DevcashMessageUniquePtr p) { validator_controller.validatorCallback(std::move(p));
  });


  /**
   * Initialize ConsensusController
   */
  // Create the ConsensusController to handle consensus messages
  ConsensusController consensus_controller(keys,
                                           context,
                                           prior,
                                           blockchain_module_ptr->getFinalChain(),
                                           blockchain_module_ptr->getTransactionPool(),
                                           mode);

  /// Register the outgoing callback to send over zmq
  consensus_controller.registerOutgoingCallback(outgoing_callback);

  /// The controllers contain the the algorithms, the ParallelExecutor parallelizes them
  blockchain_module_ptr->consensus_ =
      std::make_unique<ParallelExecutor<ConsensusController>>(consensus_controller, context, 1);

  /// Attach a callback to be run in the threads
  blockchain_module_ptr->consensus_->attachCallback([&](DevcashMessageUniquePtr p) {
    consensus_controller.consensusCallback(std::move(p));
  });

  /**
   * Initialize InternetworkController
   */
  // Create the InternetworkController to handle Internetwork messages
  InternetworkController internetwork_controller(keys,
                                                 context,
                                                 prior,
                                                 blockchain_module_ptr->getFinalChain(),
                                                 blockchain_module_ptr->getTransactionPool(),
                                                 mode);

  /// Register the outgoing callback to send over zmq
  internetwork_controller.registerOutgoingCallback(outgoing_callback);

  /// The controllers contain the the algorithms, the ParallelExecutor parallelizes them
  blockchain_module_ptr->internetwork_ =
      std::make_unique<ParallelExecutor<InternetworkController>>(internetwork_controller, context, 1);

  /// Attach a callback to be run in the threads
  blockchain_module_ptr->internetwork_->attachCallback([&](DevcashMessageUniquePtr p) {
    internetwork_controller.messageCallback(std::move(p));
  });

  return (blockchain_module_ptr);
}

void BlockchainModule::handleMessage(DevcashMessageUniquePtr message) {
  switch(message->message_type) {
    case eMessageType::TRANSACTION_ANNOUNCEMENT:
      validator_->pushMessage(std::move(message));
      break;
    case eMessageType::GET_BLOCKS_SINCE:
    case eMessageType::BLOCKS_SINCE:
    case eMessageType::REQUEST_BLOCK:
      internetwork_->pushMessage(std::move(message));
      break;
    case eMessageType::FINAL_BLOCK:
    case eMessageType::PROPOSAL_BLOCK:
    case eMessageType::VALID:
      consensus_->pushMessage(std::move(message));
      break;
    default:
      throw DevcashMessageError("Unknown message type:"+std::to_string(message->message_type));
  }
}

void BlockchainModule::init()
{
  /*
  /// Initialize callbacks
  DevcashMessageCallbacks callbacks;
  callbacks.blocks_since_cb = HandleBlocksSince;
  callbacks.blocks_since_request_cb = HandleBlocksSinceRequest;
  callbacks.final_block_cb = HandleFinalBlock;
  callbacks.proposal_block_cb = HandleProposalBlock;
  callbacks.transaction_announcement_cb = CreateNextProposal;
  callbacks.validation_block_cb = HandleValidationBlock;
  validator_.setMessageCallbacks(callbacks);
  */

  client_.attachCallback([&](DevcashMessageUniquePtr p) {
    this->handleMessage(std::move(p));
  });
  client_.listenTo(app_context_.get_shard_uri());
  client_.listenTo(app_context_.get_uri());

  server_.startServer();
  client_.startClient();


  /// Initialize OpenSSL
  InitCrypto();
}

void BlockchainModule::performSanityChecks()
{
  if (!isCryptoInit) InitCrypto();
  EVP_MD_CTX *ctx;
  if (!(ctx = EVP_MD_CTX_create())) {
    throw std::runtime_error("Could not create signature context!");
  }

  std::vector<byte> msg = {'h', 'e', 'l', 'l', 'o'};
  Hash test_hash(DevcashHash(msg));
  std::string sDer;

  EC_KEY *loadkey = LoadEcKey(app_context_.kADDRs[1],
                              app_context_.kADDR_KEYs[1]);

  Signature sig;
  SignBinary(loadkey, test_hash, sig);

  if (!VerifyByteSig(loadkey, test_hash, sig)) {
    throw std::runtime_error("Could not VerifyByteSig!");
  }
  return;
}

void BlockchainModule::start()
{
  try {
    LOG_INFO << "Start BlockchainModule";
    consensus_->start();
    internetwork_->start();
    validator_->start();
    LOG_INFO << "Controllers started.";
  } catch (const std::exception& e) {
    LOG_FATAL << FormatException(&e, "BlockchainModule.RunScanner");
    throw;
  }
}

void BlockchainModule::shutdown()
{
  request_shutdown = true;
  /// Wait for the threads to see the shutdown request
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  client_.stopClient();
  server_.stopServer();

  LOG_INFO << "Shutting down DevCash";
  //consensus_.shutdown();
  //internetwork_.shutdown();
  //validator_.shutdown();

}

}
