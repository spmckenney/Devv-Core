/*
 * primitives/block.h
 *
 *  Created on: Jun 6, 2018
 *      Author: Shawn McKenney
 */
#pragma once

#include <cstdint>

#include "common/devcash_types.h"
#include "primitives/Summary.h"
#include "primitives/Transaction.h"
#include "primitives/Validation.h"

namespace Devcash {

/**
 * Holds block-related data
 */
struct block_info {
  /// Version of the block
  uint8_t version = 0;
  /// Number of bytes in the block
  uint64_t num_bytes;
  /// Hash of previous block
  Hash prev_hash;
  /// Size of Transactions in this block
  uint64_t tx_size;
  /// Size of the Summary
  uint64_t summary_size;
  /// Number of Validations
  uint32_t validation_count;
  /// vector of TransactionPtrs
  std::vector<TransactionPtr> transaction_vector;
  /// Summary
  Summary summary;
  /// Validation
  Validation vals;
  /// ChainState
  ChainState block_state;
};

} // namespace Devcash