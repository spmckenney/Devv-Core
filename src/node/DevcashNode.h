/*
 * DevcashNode.h manages this node in the devcash network.
 *
 *  Created on: Mar 20, 2018
 *      Author: Nick Williams
 */
#pragma once

#include <string>
#include <vector>

#include "concurrency/ValidatorController.h"
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

  DevcashNode(ValidatorController& devcash, DevcashContext& context);

  /** Initialize devcoin core: Basic context setup.
   *  @note Do not call Shutdown() if this function fails.
   *  @pre Parameters should be parsed and config file should be read.
   */
  bool init();

  /**
   * Initialization sanity checks: ecc init, sanity checks, dir lock.
   * @note Do not call Shutdown() if this function fails.
   * @pre Parameters should be parsed and config file should be read.
   */
  bool SanityChecks();

  /**
   * Devcash core main initialization.
   * @note Call Shutdown() if this function fails.
   */
  void RunNode();

private:
  ValidatorController& control_;
  DevcashContext& app_context_;
};

} //end namespace Devcash
