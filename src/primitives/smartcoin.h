/*
 * smartcoin.h
 *
 *  Created on: Jan 12, 2018
 *  Author: Nick Williams
 */

#ifndef SRC_PRIMITIVES_SMARTCOIN_H_
#define SRC_PRIMITIVES_SMARTCOIN_H_

#include <string>
#include <vector>
#include <stdint.h>

static const long COIN = 100000000;
static const long MAX_MONEY = 21000000 * COIN;  //2.1 quintillion
inline bool MoneyRange(const long& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }

class SmartCoin {
public:
  std::string type;
  std::string addr;
  long amount = 0;

  SmartCoin(std::string type, std::string addr, long amount=0) {
  this->type=type;
  this->addr=addr;
  this->amount = amount;
  }
  virtual ~SmartCoin();
};

#endif /* SRC_PRIMITIVES_SMARTCOIN_H_ */
