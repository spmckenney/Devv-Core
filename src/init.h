/*
 * init.h
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

void StartShutdown();
bool ShutdownRequested();
void Shutdown();
//!Initialize error logging infrastructure
void InitError(const std::string &str);

/** Initialize devcoin core: Basic context setup.
 *  @note This can be done before daemonization. Do not call Shutdown() if this function fails.
 *  @pre Parameters should be parsed and config file should be read.
 */
bool AppInitBasicSetup(ArgsManager &args);
/**
 * Initialization sanity checks: ecc init, sanity checks, dir lock.
 * @note This can be done before daemonization. Do not call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read, AppInitParameterInteraction should have been called.
 */
bool AppInitSanityChecks();
/**
 * Devcoin core main initialization.
 * @note This should only be done after daemonization. Call Shutdown() if this function fails.
 * @pre Parameters should be parsed and config file should be read, AppInitLockDataDirectory should have been called.
 */
std::string AppInitMain(std::string inStr, std::string mode);

#endif // DEVCASH_INIT_H

