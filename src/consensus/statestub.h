/*
 * statestub.h
 *
 *  Created on: Jan 12, 2018
 *  Author: Nick Williams
 */

#ifndef SRC_CONSENSUS_STATESTUB_H_
#define SRC_CONSENSUS_STATESTUB_H_

#include "../primitives/smartcoin.h"
#include <map>

class DCState {
public:
  std::map<std::string, std::map<std::string, long>> stateMap;

  DCState();
  virtual ~DCState();

  bool addCoin(SmartCoin& coin);
  long getAmount(std::string type, std::string addr);
  bool moveCoin(SmartCoin& start, SmartCoin& end);
  bool delCoin(SmartCoin& coin);
  bool clear();
};

#endif /* SRC_CONSENSUS_STATESTUB_H_ */
