/*
 * chainstate.h holds the state of the chain for each validating fork
 *
 *  Created on: Jan 12, 2018
 *  Author: Nick Williams
 */

#ifndef SRC_CONSENSUS_STATESTUB_H_
#define SRC_CONSENSUS_STATESTUB_H_

#include <map>
#include <mutex>

#include "primitives/smartcoin.h"

namespace Devcash
{

class DCState {
public:

/** Constructor */
  DCState() {}

  DCState(const DCState& other)
  : stateMap_(other.stateMap_)
  {
  }

  DCState* operator=(DCState&& other)
  {
    if (this != &other) {
      this->stateMap_ = other.stateMap_;
    }
    return this;
  }

  DCState* operator=(const DCState& other)
  {
    if (this != &other) {
      this->stateMap_ = other.stateMap_;
    }
    return this;
  }

/** Adds a coin to the state.
 *  @param reference to the coin to add
 *  @return true if the coin was added successfully
 *  @return false otherwise
*/
  bool addCoin(const SmartCoin& coin);

/** Gets the number of coins at a particular location.
 *  @param type the coin type to check
 *  @param the address to check
 *  @return the number of this type of coins at this address
*/
  long getAmount(int type, const std::string& addr) const;

/** Moves a coin from one address to another
 *  @param start references where the coins will be removed
 *  @param end references where the coins will be added
 *  @return true if the coins were moved successfully
 *  @return false otherwise
*/
  bool moveCoin(SmartCoin& start, SmartCoin& end) const;

/** Deletes a coin from the state.
 *  @param reference to the coin to delete
 *  @return true if the coin was deleted successfully
 *  @return false otherwise
*/
  bool delCoin(SmartCoin& coin);

/** Clears this chain state.
 *  @return true if the state cleared successfully
 *  @return false otherwise
*/
  bool clear();

  std::map<std::string, std::vector<long>> stateMap_;
  mutable std::mutex lock_;
};

} //namespace Devcash

#endif /* SRC_CONSENSUS_STATESTUB_H_ */
