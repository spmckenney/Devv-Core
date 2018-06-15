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
  auto it = state_map_.find(coin.getAddress());
  if (it != state_map_.end()) {
    it->second[coin.getCoin()] += coin.getAmount();
  }
  return(true);
}

bool ChainState::addCoins(const std::map<Address, SmartCoin>& coin_map) {
 for (auto& coin : coin_map) {
   auto loc = state_map_.find(coin.first);
   if (loc != state_map_.end()) {
     loc->second[coin.second.getCoin()] += coin.second.getAmount();
   }
 }
 return(true);
}

long ChainState::getAmount(uint64_t type, const Address& addr) const {
  auto it = state_map_.find(addr);
  if (it != state_map_.end()) {
    int64_t amount = it->second.at(type);
    return amount;
  }
  return(0);
}

} //end namespace Devcash
