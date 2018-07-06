/*
 * ModuleInterface
 *
 *  Created on: 6/22/18
 *      Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

namespace Devcash {

class ModuleInterface {
 public:
  ModuleInterface() {}

  virtual ~ModuleInterface() {}

  /**
   * Initialize the module
   */
  virtual void init() = 0;

  /**
   * Basic tests to ensure the module has been initialized properly
   */
  virtual void performSanityChecks() = 0;

  /**
   * Shut this module down
   */
  virtual void shutdown() = 0;

  /**
   * Starts the module thread(s) if enabled
   */
  virtual void start() = 0;
};

} // namespace Devcash