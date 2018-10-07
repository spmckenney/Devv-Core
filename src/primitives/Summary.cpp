//
// Created by mckenney on 6/17/18.
//
#include "Summary.h"
#include "primitives/json_interface.h"

namespace Devv {

bool Summary::isSane() const {
  if (summary_.empty()) { return false; }
  uint64_t coin_total = 0;
  for (auto summary : summary_) {
    auto summary_pair = summary.second;
    auto delayed_map = summary_pair.first;
    auto coin_map = summary_pair.second;
    for (auto delayed_coin : delayed_map) {
      coin_total += delayed_coin.second.delta;
    }
    for (auto coin : coin_map) {
      coin_total += coin.second;
    }
  }
  if (coin_total != 0) {
    LOG_WARNING << "Summary state invalid: " + GetJSON(*this);
    return false;
  }
  return true;
}

}
