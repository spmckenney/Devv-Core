/*
 * DevcashController.h controls worker threads for the Devcash protocol.
 *
 *  Created on: Mar 21, 2018
 *      Author: Nick Williams
 */

#ifndef CONCURRENCY_DEVCASHCONTROLLER_H_
#define CONCURRENCY_DEVCASHCONTROLLER_H_

#include "consensus/chainstate.h"
#include "consensus/finalblock.h"
#include "consensus/proposedblock.h"
#include "consensus/KeyRing.h"
#include "io/message_service.h"
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <thread>

namespace Devcash {

class DevcashControllerWorker;

class DevcashController {
 public:
  DevcashController(std::unique_ptr<io::TransactionServer> serverPtr,
      std::unique_ptr<io::TransactionClient> clientPtr,
        const int validatorCount, const int consensusWorkerCount,
        KeyRing& keys, DevcashContext& context,
        ProposedBlock& nextBlock, ProposedBlock& futureBlock);
  virtual ~DevcashController() {};

  void seedTransactions(std::string txs);
  void startToy();
  void start();
  void stopAll();
  void pushConsensus(std::unique_ptr<DevcashMessage> ptr);
  void pushValidator(std::unique_ptr<DevcashMessage> ptr);

  void ConsensusCallback(std::unique_ptr<DevcashMessage> ptr);
  void ValidatorCallback(std::unique_ptr<DevcashMessage> ptr);

 protected:
  io::TransactionServer& server_;
  io::TransactionClient& client_;
  const int validator_count_;
  const int consensus_count_;
  KeyRing& keys_;
  DevcashContext& context_;
  std::vector<FinalBlock*> final_chain_;
  ProposedBlock& next_block_;
  ProposedBlock& future_block_;
  std::vector<std::string> seeds_;
  unsigned int seeds_at_;
  DevcashControllerWorker* workers_;
  bool validator_flipper = true;
  bool consensus_flipper = true;

 private:
  void rotateBlocks();
  void postTransactions();

};

} /* namespace Devcash */

#endif /* CONCURRENCY_DEVCASHCONTROLLER_H_ */
