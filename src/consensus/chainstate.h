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

#include "primitives/SmartCoin.h"

namespace Devcash
{

class ChainState {
public:

/** Constructor */
  ChainState() {}

  ChainState(const ChainState& other)
  : stateMap_(other.state_map_)
  {
  }

  ChainState& operator=(ChainState&& other)
  {
    if (this != &other) {
      this->state_map_ = other.state_map_;
    }
    return *this;
  }

  ChainState& operator=(const ChainState& other)
  {
    if (this != &other) {
      this->state_map_ = other.state_map_;
    }
    return *this;
  }

/** Adds a coin to the state.
 *  @param reference to the coin to add
 *  @return true if the coin was added successfully
 *  @return false otherwise
*/
  bool addCoin(const SmartCoin& coin);
  bool addCoins(const std::map<Address, SmartCoin>& coin_map);

/** Gets the number of coins at a particular location.
 *  @param type the coin type to check
 *  @param the address to check
 *  @return the number of this type of coins at this address
*/
  long getAmount(uint64_t type, const Address& addr) const;

/** Clears this chain state.
 *  @return true if the state cleared successfully
 *  @return false otherwise
*/
  bool clear();

/** Get the map describing this chain state
 *  @return tre map describing this chain state
*/
  std::map<Address, std::map<uint64_t, uint64_t>> getStateMap() {
    return state_map_;
  }

private:
 std::map<Address, std::map<uint64_t, uint64_t>> state_map_;
};

} //namespace Devcash

#endif /* SRC_CONSENSUS_STATESTUB_H_ */
