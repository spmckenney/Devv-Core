/*
 * chainstate.h holds state for a validating fork
 *
 *  Created on: Jan 12, 2018
 *  Author: Nick Williams
 */

#include "chainstate.h"

#include <stdint.h>
#include <map>

#include "../primitives/SmartCoin.h"

namespace Devcash
{

using namespace Devcash;

std::map<std::string, std::map<std::string, long>> stateMap_;

bool addCoin(SmartCoin& coin) {
  std::map<std::string, std::map<std::string, long>>::iterator it =
      stateMap_.find(coin.type_);
  if (it != stateMap_.end()) {
    std::map<std::string, long> elt = it->second;
    auto amtIt = elt.find(coin.addr_);
    if (amtIt != elt.end()) {
      long amt = amtIt->second;
      elt[coin.addr_] = coin.amount_+((long) amt);
    } else {
      elt[coin.addr_] = coin.amount_;
    }
  } else {
    stateMap_[coin.type_][coin.addr_] = coin.amount_;
  }
  return(true);
}

long getAmount(std::string type, std::string addr) {
  std::map<std::string, std::map<std::string, long>>::iterator it =
      stateMap_.find(type);
  if (it != stateMap_.end()) {
    std::map<std::string, long> elt = it->second;
    auto amtIt = elt.find(addr);
    if (amtIt != elt.end()) return((long) amtIt->second);
  }
  return(0);
}

bool moveCoin(SmartCoin& start, SmartCoin& end) {
  if (start.type_ != end.type_) return(false);
  if (start.amount_ != end.amount_) return(false);

  std::map<std::string, std::map<std::string, long>>::iterator it =
      stateMap_.find(start.type_);
  if (it != stateMap_.end()) {
    std::map<std::string, long> sElt = it->second;
    auto amtIt = sElt.find(start.addr_);
    if (amtIt != sElt.end()) {
      long amt = amtIt->second;
      if (amt >= start.amount_) {
        sElt[start.addr_] = start.amount_-amt;
        if(addCoin(end)) return(true);
        sElt[start.addr_] = start.amount_+amt;
        return(false);
      } //endif enough coins available
    } //endif any coins here
  } //endif any coins of this type
  return(false);
}

bool delCoin(SmartCoin& coin) {
  std::map<std::string, std::map<std::string, long>>::iterator it =
      stateMap_.find(coin.type_);
  if (it != stateMap_.end()) {
    std::map<std::string, long> elt = it->second;
    auto amtIt = elt.find(coin.addr_);
    if (amtIt != elt.end()) {
      long amt = amtIt->second;
      if (amt >= coin.amount_) {
        elt[coin.addr_] = coin.amount_-amt;
        return(true);
      }
    }
  }
  return(false);
}

} //end namespace Devcash
