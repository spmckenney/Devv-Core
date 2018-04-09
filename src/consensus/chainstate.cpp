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

bool DCState::addCoin(const SmartCoin& coin) {
  std::lock_guard<std::mutex> lock(lock_);
  auto it = stateMap_.find(coin.addr_);
  if (it != stateMap_.end()) {
    it->second[coin.type_] += coin.amount_;
  }
  return(true);
}

long DCState::getAmount(int type, const std::string& addr) const {
  auto it = stateMap_.find(addr);
  if (it != stateMap_.end()) {
    return it->second[type];
  }
  return(0);
}

bool DCState::moveCoin(const SmartCoin& start, const SmartCoin& end) const {
  std::lock_guard<std::mutex> lock(lock_);
  if (start.type_ != end.type_) return(false);
  if (start.amount_ != end.amount_) return(false);

  /*auto it = stateMap_.find(start.addr_);
  if (it != stateMap_.end()) {
    std::map<int, long> sElt = it->second;
    auto amtIt = sElt.find(start.type_);
    if (amtIt != sElt.end()) {
      long amt = amtIt->second;
      if (amt >= start.amount_) {
        sElt[start.addr_] = start.amount_-amt;
        if(addCoin(end)) return(true);
        sElt[start.addr_] = start.amount_+amt;
        return(false);
      } //endif enough coins available
    } //endif any coins here
  } //endif any coins of this type*/
  return(false);
}

bool DCState::delCoin(SmartCoin& coin) {
  std::lock_guard<std::mutex> lock(lock_);
  auto it = stateMap_.find(coin.addr_);
  if (it != stateMap_.end()) {
    it->second[coin.type_] -= coin.amount_;
    return true;
  }
  return(false);
}

} //end namespace Devcash
