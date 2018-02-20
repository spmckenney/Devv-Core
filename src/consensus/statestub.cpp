/*
 * statestub.cpp
 *
 *  Created on: Jan 12, 2018
 *  Author: Nick Williams
 */

#include "statestub.h"
#include "../primitives/SmartCoin.h"
#include <stdint.h>
#include <map>

std::map<std::string, std::map<std::string, long>> stateMap;

bool addCoin(SmartCoin& coin) {
  std::map<std::string, std::map<std::string, long>>::iterator it = stateMap.find(coin.type);
  if (it != stateMap.end()) {
  std::map<std::string, long> elt = it->second;
  auto amtIt = elt.find(coin.addr);
  if (amtIt != elt.end()) {
  long amt = amtIt->second;
  elt[coin.addr] = coin.amount+((long) amt);
  } else {
  elt[coin.addr] = coin.amount;
  }
  } else {
  stateMap[coin.type][coin.addr] = coin.amount;
  }
  return(true);
}

long getAmount(std::string type, std::string addr) {
  std::map<std::string, std::map<std::string, long>>::iterator it = stateMap.find(type);
  if (it != stateMap.end()) {
  std::map<std::string, long> elt = it->second;
  auto amtIt = elt.find(addr);
  if (amtIt != elt.end()) {
  return((long) amtIt->second);
  }
  }
  return(0);
}

bool moveCoin(SmartCoin& start, SmartCoin& end) {
  if (start.type != end.type) return(false);
  if (start.amount != end.amount) return(false);

  std::map<std::string, std::map<std::string, long>>::iterator it = stateMap.find(start.type);
  if (it != stateMap.end()) {
  std::map<std::string, long> sElt = it->second;
  auto amtIt = sElt.find(start.addr);
  if (amtIt != sElt.end()) {
  long amt = amtIt->second;
  if (amt >= start.amount) {
  sElt[start.addr] = start.amount-amt;
  if(addCoin(end)) return(true);
  sElt[start.addr] = start.amount+amt;
  return(false);
  }
  }
  }
  return(false);
}

bool delCoin(SmartCoin& coin) {
  std::map<std::string, std::map<std::string, long>>::iterator it = stateMap.find(coin.type);
  if (it != stateMap.end()) {
  std::map<std::string, long> elt = it->second;
  auto amtIt = elt.find(coin.addr);
  if (amtIt != elt.end()) {
  long amt = amtIt->second;
  if (amt >= coin.amount) {
  elt[coin.addr] = coin.amount-amt;
  return(true);
  }
  }
  }
  return(false);
}
