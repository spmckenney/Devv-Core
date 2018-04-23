/*
 * chainstate.h holds state for a validating fork
 *
 *  Created on: Jan 12, 2018
 *  Author: Nick Williams
 */

#include "chainstate.h"

#include <stdint.h>
#include <map>
#include <mutex>

#include "chainstate.h"

namespace Devcash
{

using namespace Devcash;

bool ChainState::addCoin(const SmartCoin& coin) {
  //std::lock_guard<std::mutex> lock(lock_);
  auto it = stateMap_.find(coin.addr_);
  if (it != stateMap_.end()) {
    it->second[coin.coin_] += coin.amount_;
  }
  return(true);
}

long ChainState::getAmount(uint64_t type, const Address& addr) {
  auto it = stateMap_.find(addr);
  if (it != stateMap_.end()) {
	int64_t amount = it->second[type];
    return amount;
  }
  return(0);
}

bool ChainState::moveCoin(const SmartCoin& start, const SmartCoin& end) {
  //std::lock_guard<std::mutex> lock(lock_);
  if (start.coin_ != end.coin_) return(false);
  if (start.amount_ != end.amount_) return(false);

  uint64_t start_balance = stateMap_[start.addr_][start.coin_];
  if (start_balance >= start.amount_) {
    stateMap_[start.addr_][start.coin_] = start_balance-start.amount;
    uint64_t end_balance = stateMap_[end.addr_][start.coin_];
    stateMap_[end.addr_][start.coin_] = end_balance+start.amount;
    return true;
  }
  return false;
}

bool ChainState::delCoin(SmartCoin& coin) {
  //std::lock_guard<std::mutex> lock(lock_);
  auto it = stateMap_.find(coin.addr_);
  if (it != stateMap_.end()) {
    it->second[coin.coin_] -= coin.amount_;
    return true;
  }
  return(false);
}

} //end namespace Devcash
