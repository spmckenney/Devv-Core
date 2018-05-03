/*
 * DevcashController.h controls worker threads for the Devcash protocol.
 *
 *  Created on: Mar 21, 2018
 *      Author: Nick Williams
 */
#ifndef CONCURRENCY_DEVCASHCONTROLLER_H_
#define CONCURRENCY_DEVCASHCONTROLLER_H_

#include "consensus/chainstate.h"
#include "consensus/blockchain.h"
#include "consensus/KeyRing.h"
#include "consensus/UnrecordedTransactionPool.h"
#include "io/message_service.h"
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <thread>

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
                    const KeyRing& keys,
                    DevcashContext& context,
                    const ChainState& prior);

  ~DevcashController();

  std::vector<std::vector<byte>> GenerateTransactions();

  /**
   * Start the workers and comm threads
   */
  std::string Start();
  void StopAll();
  void PushConsensus(std::unique_ptr<DevcashMessage> ptr);
  void PushValidator(std::unique_ptr<DevcashMessage> ptr);

  void ConsensusCallback(std::unique_ptr<DevcashMessage> ptr);
  void ValidatorCallback(std::unique_ptr<DevcashMessage> ptr);

  void ConsensusToyCallback(std::unique_ptr<DevcashMessage> ptr);
  void ValidatorToyCallback(std::unique_ptr<DevcashMessage> ptr);

private:
  io::TransactionServer& server_;
  io::TransactionClient& peer_client_;
  io::TransactionClient& loopback_client_;
  const int validator_count_;
  const int consensus_count_;
  const size_t generate_count_;
  const size_t batch_size_;
  const KeyRing& keys_;
  DevcashContext& context_;
  Blockchain final_chain_;
  UnrecordedTransactionPool utx_pool_;

  // Pointer because incomplete type
  DevcashControllerWorker* workers_ = nullptr;
  bool validator_flipper_ = true;
  bool consensus_flipper_ = true;
  bool shutdown_ = false;
  uint64_t waiting_ = 0;
  mutable std::mutex mutex_;
};

} /* namespace Devcash */

#endif /* CONCURRENCY_DEVCASHCONTROLLER_H_ */
