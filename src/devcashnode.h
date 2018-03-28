/*
 * devnode.h manages this node in the devcash network.
 *
 *  Created on: Mar 20, 2018
 *      Author: Nick Williams
 */

#ifndef DEVCASHNODE_H_
#define DEVCASHNODE_H_

#ifndef DEVCASH_INIT_H
#define DEVCASH_INIT_H

#include <string>
#include <vector>

#include "common/devcash_context.h"
#include "concurrency/DevcashController.h"
#include "concurrency/WorkerTypes.h"
#include "io/message_service.h"

namespace Devcash
{

class DevcashNode {
 public:
  /** Begin stopping threads and shutting down. */
  void StartShutdown();

  /** Check if system is shutting down.
   *  @return true iff shutdown process has begun
   *  @return false otherwise
  */
  bool ShutdownRequested();

  /** Shut down immediately. */
  void Shutdown();

  DevcashNode(eAppMode mode
              , int node_index
              , ConsensusWorker& consensus
              , ValidatorWorker& validator
              , io::TransactionClient& client
              , io::TransactionServer& server);

  DevcashNode(DevcashController& devcash, DevcashContext& context);

  /** Initialize devcoin core: Basic context setup.
   *  @note Do not call Shutdown() if this function fails.
   *  @pre Parameters should be parsed and config file should be read.
   */
  bool Init();

  /**
   * Initialization sanity checks: ecc init, sanity checks, dir lock.
   * @note Do not call Shutdown() if this function fails.
   * @pre Parameters should be parsed and config file should be read.
   */
  bool SanityChecks();

  /**
   * Runs the scanner over the input.
   * @pre Parameters should be parsed and config file should be read.
   */
  std::string RunScanner(std::string inStr);

  /**
   * Devcash core main initialization.
   * @note Call Shutdown() if this function fails.
   */
  std::string RunNode(std::string& inStr);

  std::string RunNetworkTest(unsigned int node_index);

private:
  DevcashController& control_;
  DevcashContext& app_context_;
};

} //end namespace Devcash

#endif // DEVCASH_INIT_H


#endif /* DEVCASHNODE_H_ */
