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

typedef std::shared_ptr<FinalBlock> FinalPtr;
typedef std::shared_ptr<ProposedBlock> ProposedPtr;

class DevcashControllerWorker;

class DevcashController {
 public:
  DevcashController(io::TransactionServer& server,
                    io::TransactionClient& client,
                    const int validatorCount,
                    const int consensusWorkerCount,
                    const int repeatFor,
                    KeyRing& keys,
                    DevcashContext& context);
  virtual ~DevcashController() {};

  void SeedTransactions(std::string txs);
  void StartToy(unsigned int node_index);

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
  io::TransactionClient& client_;
  const int validator_count_;
  const int consensus_count_;
  int repeat_for_ = 1;
  KeyRing& keys_;
  DevcashContext& context_;
  std::vector<FinalPtr> final_chain_;
  std::vector<ProposedPtr> proposed_chain_;
  std::vector<ProposedPtr> upcoming_chain_;
  std::vector<std::string> seeds_;
  unsigned int seeds_at_;
  DevcashControllerWorker* workers_;
  bool validator_flipper_ = true;
  bool consensus_flipper_ = true;
  std::mutex valid_lock_;
  bool accepting_valids_ = false;

  bool PostTransactions();
  bool PostAdvanceTransactions(const std::string& inputTxs);

  bool shutdown_ = false;
  uint64_t waiting_ = 0;
};

DevcashMessageUniquePtr CreateNextProposal(unsigned int block_height,
                                           const ProposedBlock& next_proposal,
                                           const DevcashContext& context,
                                           const KeyRing& keys,
                                           std::vector<ProposedPtr>& proposed_chain,
                                           std::vector<ProposedPtr>& upcoming_chain);

bool HandleFinalBlock(DevcashMessageUniquePtr ptr,
                      const DevcashContext& context,
                      const KeyRing& keys,
                      std::vector<ProposedPtr>& proposed_chain,
                      std::vector<ProposedPtr>& upcoming_chain,
                      std::vector<FinalPtr>& final_chain,
                      std::function<void(DevcashMessageUniquePtr)> callback);

bool HandleProposalBlock(DevcashMessageUniquePtr ptr,
                         const DevcashContext& context,
                         const KeyRing& keys,
                         std::vector<ProposedPtr>& proposed_chain,
                         const std::vector<ProposedPtr>& upcoming_chain,
                         const std::vector<FinalPtr>& final_chain,
                         std::function<void(DevcashMessageUniquePtr)> callback);

bool HandleValidationBlock(DevcashMessageUniquePtr ptr,
                           const DevcashContext& context,
                           const KeyRing& keys,
                           const std::vector<ProposedPtr>& proposed_chain,
                           std::vector<ProposedPtr>& upcoming_chain,
                           std::vector<FinalPtr>& final_chain,
                           std::function<void(DevcashMessageUniquePtr)> callback);

std::string GetHighestMerkleRoot(const std::vector<FinalPtr>& final_chain);

} /* namespace Devcash */

#endif /* CONCURRENCY_DEVCASHCONTROLLER_H_ */
