/*
 * BlockchainModule.h manages this node in the devcash network.
 *
 *  Created on: Mar 20, 2018
 *      Author: Nick Williams
 */
#pragma once

#include <string>
#include <vector>

#include "concurrency/ConsensusController.h"
#include "concurrency/InternetworkController.h"
#include "concurrency/ValidatorController.h"
#include "concurrency/ThreadGroup.h"
#include "io/message_service.h"
#include "modules/ThreadedController.h"
#include "modules/ModuleInterface.h"
#include "io/message_service.h"

namespace Devcash {

class BlockchainModule : public ModuleInterface {
  typedef std::unique_ptr<ThreadedController<ConsensusController>> ThreadedConsensusPtr;
  typedef std::unique_ptr<ThreadedController<InternetworkController>> ThreadedInternetworkPtr;
  typedef std::unique_ptr<ThreadedController<ValidatorController>> ThreadedValidatorPtr;

 public:
  BlockchainModule(io::TransactionServer &server,
                  io::TransactionClient &client,
                  const KeyRing &keys,
                  const ChainState &prior,
                  eAppMode mode,
                  DevcashContext &context,
                  size_t max_tx_per_block);

  /**
   * Move constructor
   * @param other
   */
  BlockchainModule(BlockchainModule&& other) = default;

  /**
   * Default move-assignment operator
   * @param other
   * @return
   */
  BlockchainModule& operator=(BlockchainModule&& other) = default;

  virtual ~BlockchainModule() {}

  /**
   *
   */
  static std::unique_ptr<BlockchainModule> Create(io::TransactionServer &server,
                                io::TransactionClient &client,
                                const KeyRing &keys,
                                const ChainState &prior,
                                eAppMode mode,
                                Blockchain final_chain,
                                UnrecordedTransactionPool utx_pool,
                                DevcashContext &context,
                                size_t max_tx_per_block);

  /** Initialize devcoin core: Basic context setup.
   *  @note Do not call Shutdown() if this function fails.
   *  @pre Parameters should be parsed and config file should be read.
   */
  void init() override;

  /**
   * Initialization sanity checks: ecc init, sanity checks, dir lock.
   * @note Do not call Shutdown() if this function fails.
   * @pre Parameters should be parsed and config file should be read.
   */
  void performSanityChecks() override;

  /**
   * Stop any running threads and shutdown the module
   */
  void shutdown();

  /**
   * Devcash core main initialization.
   * @note Call Shutdown() if this function fails.
   */
  void start();

  void handleMessage(DevcashMessageUniquePtr message);

 private:
  io::TransactionServer &server_;
  io::TransactionClient &client_;

  const KeyRing &keys_;
  const ChainState &prior_;
  eAppMode mode_;

  ThreadedConsensusPtr consensus_ = nullptr;
  ThreadedInternetworkPtr internetwork_ = nullptr;
  ThreadedValidatorPtr validator_ = nullptr;

  Blockchain final_chain_;
  UnrecordedTransactionPool utx_pool_;

  DevcashContext &app_context_;
};

} //end namespace Devcash
