/*
 * init.h handles orchestration of modules, startup, shutdown,
 * and response to signals.
 *
 *  Created on: Dec 10, 2017
 *  Author: Nick Williams
 */

#ifndef DEVCASH_INIT_H
#define DEVCASH_INIT_H

#include <string>

#include "common/util.h"

class DevCashContext {
public:
  const static char *DEFAULT_PUBKEY;
  const static char *DEFAULT_PK;
};

/** Begin stopping threads and shutting down. */
void StartShutdown();

/** Check if system is shutting down.
 *  @return true iff shutdown process has begun
 *  @return false otherwise
*/
bool ShutdownRequested();

/** Shut down immediately. */
void Shutdown();

/** Initialize devcoin core: Basic context setup.
 *  @note Do not call Shutdown() if this function fails.
 *  @pre Parameters should be parsed and config file should be read.
 */
bool AppInitBasicSetup(Devcash::ArgsManager& args);

/**
 * Initialization sanity checks: ecc init, sanity checks, dir lock.
 * @note Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read.
 */
bool AppInitSanityChecks();

/**
 * Devcash core main initialization.
 * @note Call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read.
 */
std::string AppInitMain(std::string inStr, std::string mode);

#endif // DEVCASH_INIT_H

