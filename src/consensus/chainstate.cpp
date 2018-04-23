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

long ChainState::getAmount(uint64_t type, const Address& addr) const {
  auto it = stateMap_.find(addr);
  if (it != stateMap_.end()) {
    return it->second[type];
  }
  return(0);
}

bool ChainState::moveCoin(const SmartCoin& start, const SmartCoin& end) const {
  //std::lock_guard<std::mutex> lock(lock_);
  if (start.coin_ != end.coin_) return(false);
  if (start.amount_ != end.amount_) return(false);

  auto it = stateMap_.find(start.addr_);
  if (it != stateMap_.end()) {
    uint64_t amt = it->second.at(start.coin_);
    if (amt >= start.amount_) {
      it->second[start.coin_] -= start.amount_;
      it->second[end.coin_] += start.amount_;
      return(true);
    } //endif enough coins available
  } //endif any coins of this type
  return(false);
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
