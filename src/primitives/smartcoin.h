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

#include "common/ossladapter.h"
#include "common/util.h"

namespace Devcash
{

//keep coins within uint64, divisible to 10^-8
static const uint64_t kCOIN = 100000000;
static const uint64_t kMAX_COIN = 184000000000 * kCOIN;
inline bool MoneyRange(const uint64_t& nValue) {
  return (nValue <= kMAX_COIN);
}

class SmartCoin {
 public:
  Address addr_;
  uint64_t coin_;
  uint64_t amount_ = 0;

/** Constructor */
  SmartCoin(const Address& addr, uint64_t coin, uint64_t amount=0)
    : addr_(addr), coin_(coin), amount_(amount) {
  }
};

} //end namespace Devcash

#endif /* SRC_PRIMITIVES_SMARTCOIN_H_ */
