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
  auto it = stateMap_.find(coin.getAddress());
  if (it != stateMap_.end()) {
    it->second[coin.getCoin()] += coin.getAmount();
  }
  return(true);
}

bool ChainState::addCoins(const std::map<Address, SmartCoin>& coin_map) {
 for (auto& coin : coin_map) {
   auto loc = stateMap_.find(coin.first);
   if (loc != stateMap_.end()) {
     loc->second[coin.second.getCoin()] += coin.second.getAmount();
   }
 }
 return(true);
}

long ChainState::getAmount(uint64_t type, const Address& addr) const {
  auto it = stateMap_.find(addr);
  if (it != stateMap_.end()) {
    int64_t amount = it->second.at(type);
    return amount;
  }
  return(0);
}

bool ChainState::moveCoin(const SmartCoin& start, const SmartCoin& end) {
  if (start.getCoin() != end.getCoin()) return(false);
  if (start.getAmount() != end.getAmount()) return(false);

  uint64_t start_balance = stateMap_[start.getAddress()][start.getCoin()];
  if (start_balance >= start.getAmount()) {
    stateMap_[start.getAddress()][start.getCoin()] = start_balance-start.getAmount();
    uint64_t end_balance = stateMap_[end.getAddress()][start.getCoin()];
    stateMap_[end.getAddress()][start.getCoin()] = end_balance+start.getAmount();
    return true;
  }
  return false;
}

bool ChainState::delCoin(SmartCoin& coin) {
  auto it = stateMap_.find(coin.getAddress());
  if (it != stateMap_.end()) {
    it->second[coin.getCoin()] -= coin.getAmount();
    return true;
  }
  return(false);
}

} //end namespace Devcash
