/*
 * DevcashController.h controls worker threads for the Devcash protocol.
 *
 *  Created on: Mar 21, 2018
 *      Author: Nick Williams
 */
#ifndef CONCURRENCY_DEVCASHCONTROLLER_H_
#define CONCURRENCY_DEVCASHCONTROLLER_H_

#include <condition_variable>
#include <mutex>

#include "consensus/KeyRing.h"
#include "consensus/UnrecordedTransactionPool.h"
#include "consensus/blockchain.h"
#include "consensus/tier2_message_handlers.h"
#include "consensus/chainstate.h"
#include "io/message_service.h"

namespace Devcash {

class DevcashControllerWorker;

class DevcashController {
 public:
  DevcashController(io::TransactionServer& server,
                    io::TransactionClient& peer_client,
                    io::TransactionClient& loopback_client,
                    int validatorCount,
                    int consensusCount,
                    int generateCount,
                    int batchSize,
                    size_t transaction_limit,
                    const KeyRing& keys,
                    DevcashContext& context,
                    const ChainState& prior,
                    eAppMode mode, std::string scan_dir);

  ~DevcashController();

  std::vector<std::vector<byte>> generateTransactions();
  std::vector<std::vector<byte>> loadTransactions();

  /**
   * Start the workers and comm threads
   */
  std::vector<byte> Start();

  /** Stops all threads used by this controller.
   * @note This function may block.
   */
  void stopAll();

  /**
   * Push a message to the consensus workers.
   */
  void pushConsensus(std::unique_ptr<DevcashMessage> ptr);

  /**
   * Push a message to the validator workers.
   */
  void pushValidator(std::unique_ptr<DevcashMessage> ptr);

  /**
   * Push a message to the inter-shard communication workers.
   */
  void pushShardComms(std::unique_ptr<DevcashMessage> ptr);

  /**
   * Process a consensus worker message.
   */
  void consensusCallback(std::unique_ptr<DevcashMessage> ptr);

  /**
   * Process a validator worker message.
   */
  void validatorCallback(std::unique_ptr<DevcashMessage> ptr);

  /**
   * Process a inter-shard communciation worker message.
   */
  void shardCommsCallback(std::unique_ptr<DevcashMessage> ptr);

  /**
   * Process a consensus toy worker message.
   */
  void consensusToyCallback(std::unique_ptr<DevcashMessage> ptr);

  /**
   * Process a validator toy worker message.
   */
  void validatorToyCallback(std::unique_ptr<DevcashMessage> ptr);

 private:
  io::TransactionServer& server_;
  io::TransactionClient& peer_client_;
  io::TransactionClient& loopback_client_;
  const int validator_count_;
  const int consensus_count_;
  const size_t generate_count_;
  const size_t batch_size_;
  const size_t transaction_limit_;
  size_t shutdown_counter_ = 0;
  const KeyRing& keys_;
  DevcashContext& context_;
  Blockchain final_chain_;
  UnrecordedTransactionPool utx_pool_;
  eAppMode mode_;
  std::string scan_dir_;

  // Pointer because incomplete type
  DevcashControllerWorker* workers_ = nullptr;
  bool validator_flipper_ = true;
  bool consensus_flipper_ = true;
  bool shutdown_ = false;
  uint64_t waiting_ = 0;
  mutable std::mutex mutex_;
  uint64_t remote_blocks_ = 0;
  size_t input_blocks_ = 0;
};

} /* namespace Devcash */

#endif /* CONCURRENCY_DEVCASHCONTROLLER_H_ */
