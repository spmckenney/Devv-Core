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

#include "primitives/Summary.h"

namespace Devcash
{

using namespace Devcash;

bool ChainState::addCoin(const SmartCoin& coin) {
  bool no_error = true;
  auto it = state_map_.find(coin.getAddress());
  if (it != state_map_.end()) {
    it->second[coin.getCoin()] += coin.getAmount();
  } else {
    CoinMap inner;
    inner.insert(std::make_pair(coin.getCoin(), coin.getAmount()));
    std::pair<Address, CoinMap> outer(coin.getAddress(), inner);
    auto result = state_map_.insert(outer);
    no_error = result.second && no_error;
  }
  return(no_error);
}

bool ChainState::addCoins(const std::map<Address, SmartCoin>& coin_map) {
  bool no_error = true;
  for (auto& coin : coin_map) {
    no_error = no_error && addCoin(coin.second);
  }
  return(no_error);
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
