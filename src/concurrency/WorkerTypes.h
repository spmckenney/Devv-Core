/*
 * ValidationWorker.hpp manages the work of validating transactions.
 * Invalid transactions are ignored.
 *
 *  Created on: Mar 14, 2018
 *  Author: Nick Williams
 */

#ifndef CONCURRENCY_WORKERTYPES_H_
#define CONCURRENCY_WORKERTYPES_H_

#include "DevcashWorkerPool.h"
#include "common/logger.h"
#include "consensus/chainstate.h"
#include "io/message_service.h"
#include "types/DevcashMessage.h"

namespace Devcash
{

using namespace Devcash;

class ConsensusWorker {
 public:
  ConsensusWorker(DCState& chain
                  , io::TransactionServer& server
                  , int num_workers) :
    chain_state_(chain), server_(server),
    internalPool_([this](std::unique_ptr<DevcashMessage> ptr) {
    if (flipper) {
      server_.QueueMessage(std::move(ptr));
    }
    flipper = !flipper;
  }, num_workers) {}

  void push(std::unique_ptr<DevcashMessage> ptr) {internalPool_.push(std::move(ptr));}
  void start() {internalPool_.start();}
  void stopAll() {internalPool_.stopAll();}

  DCState& chain_state_;
  io::TransactionServer& server_;
  bool flipper = false;

private:
  DevcashWorkerPool internalPool_;
};

class ValidatorWorker {
 public:
  ValidatorWorker(DCState& chain, ConsensusWorker& consensus, int num_workers) :
    chain_state_(chain), consensus_(consensus),
    internalPool_([this](std::unique_ptr<DevcashMessage> ptr) {
    if (flipper) {
      consensus_.push(std::move(ptr));
    }
    flipper = !flipper;
  }, num_workers) {}

  bool flipper = false;
  DCState& chain_state_;
  ConsensusWorker& consensus_;

  void push(std::unique_ptr<DevcashMessage> ptr) {internalPool_.push(std::move(ptr));}
  void start() {internalPool_.start();}
  void stopAll() {internalPool_.stopAll();}

private:
  DevcashWorkerPool internalPool_;
};

} //end namespace Devcash

#endif /* CONCURRENCY_WORKERTYPES_H_ */
