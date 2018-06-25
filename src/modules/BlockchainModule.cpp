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
#include <string>
#include <iostream>
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
  try {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    isCryptoInit = true;
    return true;
  } catch (const std::exception& e) {
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
      app_context_(context),
      final_chain_("final_chain_"),
      utx_pool_(prior, mode, max_tx_per_block),
      consensus_controller_(keys_, app_context_, prior_, final_chain_, utx_pool_, mode_),
      internetwork_controller_(keys_, app_context_, prior_, final_chain_, utx_pool_, mode_),
      validator_controller_(keys_, app_context_, prior_, final_chain_, utx_pool_, mode_)
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

  /// Register the outgoing callback to send over zmq
  auto outgoing_callback =
      [&server](DevcashMessageUniquePtr p) { server.queueMessage(std::move(p)); };

  /// Shorten the name of the controllers
  auto& vc = blockchain_module_ptr->validator_controller_;
  auto& cc = blockchain_module_ptr->consensus_controller_;
  auto& ic = blockchain_module_ptr->internetwork_controller_;

  vc.registerOutgoingCallback(outgoing_callback);
  /// The controllers contain the the algorithms, the ParallelExecutor parallelizes them

  blockchain_module_ptr->validator_executor_ =
      std::make_unique<ParallelExecutor<ValidatorController>>(vc, context, 1);
  /// Attach a callback to be run in the threads

  blockchain_module_ptr->validator_executor_->attachCallback(
      [&](DevcashMessageUniquePtr p) { vc.validatorCallback(std::move(p));
  });


  /// Register the outgoing callback to send over zmq
  cc.registerOutgoingCallback(outgoing_callback);

  /// The controllers contain the the algorithms, the ParallelExecutor parallelizes them
  blockchain_module_ptr->consensus_executor_ =
      std::make_unique<ParallelExecutor<ConsensusController>>(cc, context, 1);

  /// Attach a callback to be run in the threads
  blockchain_module_ptr->consensus_executor_->attachCallback([&](DevcashMessageUniquePtr p) {
    cc.consensusCallback(std::move(p));
  });


  /// Register the outgoing callback to send over zmq
  ic.registerOutgoingCallback(outgoing_callback);

  /// The controllers contain the the algorithms, the ParallelExecutor parallelizes them
  blockchain_module_ptr->internetwork_executor_ =
      std::make_unique<ParallelExecutor<InternetworkController>>(ic, context, 1);

  /// Attach a callback to be run in the threads
  blockchain_module_ptr->internetwork_executor_->attachCallback([&](DevcashMessageUniquePtr p) {
    ic.messageCallback(std::move(p));
  });

  return (blockchain_module_ptr);
}

void BlockchainModule::handleMessage(DevcashMessageUniquePtr message) {
  LogDevcashMessageSummary(*message, "BlockchainModule::handleMessage()", 6);
  switch(message->message_type) {
    case eMessageType::TRANSACTION_ANNOUNCEMENT:
      LOG_DEBUG << "BlockchainModule::handleMessage(): push(TX_ANNOUNCEMNT)";
      validator_executor_->pushMessage(std::move(message));
      break;
    case eMessageType::GET_BLOCKS_SINCE:
      LOG_DEBUG << "BlockchainModule::handleMessage(): push(GET_BLOCKS_SINCE)";
      internetwork_executor_->pushMessage(std::move(message));
      break;
    case eMessageType::BLOCKS_SINCE:
      LOG_DEBUG << "BlockchainModule::handleMessage(): push(BLOCKS_SINCE)";
      internetwork_executor_->pushMessage(std::move(message));
      break;
    case eMessageType::REQUEST_BLOCK:
      LOG_DEBUG << "BlockchainModule::handleMessage(): push(REQUEST_BLOCK)";
      internetwork_executor_->pushMessage(std::move(message));
      break;
    case eMessageType::FINAL_BLOCK:
      LOG_DEBUG << "BlockchainModule::handleMessage(): push(FINAL_BLOCK)";
      consensus_executor_->pushMessage(std::move(message));
      break;
    case eMessageType::PROPOSAL_BLOCK:
      LOG_DEBUG << "BlockchainModule::handleMessage(): push(PROPOSAL_BLOCK)";
      consensus_executor_->pushMessage(std::move(message));
      break;
    case eMessageType::VALID:
      LOG_DEBUG << "BlockchainModule::handleMessage(): push(VALID)";
      consensus_executor_->pushMessage(std::move(message));
      break;
    default:
      throw DevcashMessageError("Unknown message type:"+std::to_string(message->message_type));
  }
  LOG_DEBUG << "BlockchainModule::handleMessage(): message handled!!";
}

void BlockchainModule::init()
{
  LOG_INFO << "Start BlockchainModule::init()";

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
}

void BlockchainModule::start()
{
  LOG_INFO << "Start BlockchainModule";
  init();

  try {
    consensus_executor_->start();
    internetwork_executor_->start();
    validator_executor_->start();
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
  //consensus_controller_.shutdown();
  //internetwork_.shutdown();
  //validator_.shutdown();

}

}
