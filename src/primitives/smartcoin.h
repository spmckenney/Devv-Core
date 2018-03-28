/*
 * smartcoin.h defines the structure of an abstract 'Tier 1' coin
 *
 *  Created on: Jan 12, 2018
 *  Author: Nick Williams
 */

#ifndef SRC_PRIMITIVES_SMARTCOIN_H_
#define SRC_PRIMITIVES_SMARTCOIN_H_

#include <string>
#include <vector>
#include <stdint.h>

namespace Devcash
{

static const long kCOIN = 100000000;
static const long kMAX_MONEY = 21000000 * kCOIN;  //2.1 quintillion
inline bool MoneyRange(const long& nValue) {
  return (nValue >= 0 && nValue <= kMAX_MONEY);
}

class SmartCoin {
 public:
  int type_;
  std::string addr_;
  long amount_ = 0;

/** Constructor */
  SmartCoin(int type, std::string addr, long amount=0) {
    this->type_=type;
    this->addr_=addr;
    this->amount_ = amount;
  }
};

} //end namespace Devcash

#endif /* SRC_PRIMITIVES_SMARTCOIN_H_ */
