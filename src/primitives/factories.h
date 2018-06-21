/*
 * FinalBlock.h
 *
 *  Created on: Jun 20, 2018
 *      Author: Shawn McKenney
 */
#pragma once

#include "primitives/Tier1Transaction.h"
#include "primitives/Tier2Transaction.h"
#include "common/devcash_exceptions.h"

namespace Devcash {

static std::unique_ptr<Transaction> CreateTransaction(InputBuffer& buffer,
                                               const KeyRing& keys,
                                               eAppMode mode,
                                               bool calculate_soundness = true) {
  if (mode == eAppMode::T1) {
    auto tx = std::make_unique<Tier1Transaction>(buffer, keys, calculate_soundness);
    return tx;
  } else if (mode == eAppMode::T2) {
    auto tx = Tier2Transaction::CreateUniquePtr(buffer, keys, calculate_soundness);
    return tx;
  }
  throw DeserializationError("CreateTransaction(): unknown eAppMode type");
}

} // namespace Devcash