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
  auto it = state_map_.find(coin.addr_);
  if (it != state_map_.end()) {
    it->second[coin.coin_] += coin.amount_;
  }
  return(true);
}

bool ChainState::addCoins(const std::map<Address, SmartCoin>& coin_map) {
 for (auto iter = coin_map.begin(); iter != coin_map.end(); ++iter) {
   auto loc = state_map_.find(iter->first);
   if (loc != state_map_.end()) {
     loc->second[iter->second.coin_] += iter->second.amount_;
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
