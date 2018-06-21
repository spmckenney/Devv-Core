/*
 * Tier2Transaction.cpp
 *
 *  Created on: Jun 20, 2018
 *      Author: Shawn McKenney
 */
#include "primitives/Tier2Transaction.h"
#include "common/devcash_exceptions.h"

namespace Devcash {

void Tier2Transaction::Fill(Tier2Transaction& tx,
                    InputBuffer &buffer,
                    const KeyRing &keys,
                    bool calculate_soundness) {
  MTR_SCOPE_FUNC();
  int trace_int = 124;
  MTR_START("Transaction", "Transaction", &trace_int);
  MTR_STEP("Transaction", "Transaction", &trace_int, "step1");

  if (buffer.size() < buffer.getOffset() + MinSize()) {
    throw DeserializationError("Invalid serialized T2 transaction, too small!");
  }
  /// Don't increment the buffer, we want to copy it all to canonical_
  tx.xfer_count_ = buffer.getNextUint64(false);
  size_t tx_size = MinSize() + (Transfer::Size() * tx.xfer_count_);
  if (buffer.size() < buffer.getOffset() + tx_size) {
    std::stringstream ss;
    std::vector<byte> prefix(buffer.getCurrentIterator(), buffer.getCurrentIterator() + 8);
    ss << "Invalid serialized T2 transaction, wrong size (" + std::to_string(tx_size) + ")!";
    ss << "Transaction prefix: " + ToHex(prefix);
    ss << "Bytes offset: " + std::to_string(buffer.getOffset());
    throw DeserializationError(ss.str());
  }
  MTR_STEP("Transaction", "Transaction", &trace_int, "step2");
  buffer.copy(std::back_inserter(tx.canonical_), tx_size);

  if (tx.getOperation() > 3) {
    LOG_WARNING << "Invalid serialized T2 transaction, invalid operation! (" << tx.getOperation() << ")";
    return;
  }
  MTR_STEP("Transaction", "Transaction", &trace_int, "sound");

  if (calculate_soundness) {
    tx.is_sound_ = tx.isSound(keys);
    if (!tx.is_sound_) {
      LOG_WARNING << "Invalid serialized T2 transaction, not sound!";
    }
  }
  MTR_FINISH("Transaction", "Transaction", &trace_int);
}

Tier2Transaction Tier2Transaction::Create(InputBuffer& buffer,
                                          const KeyRing& keys,
                                          bool calculate_soundness) {
  Tier2Transaction tx;
  Tier2Transaction::Fill(tx, buffer, keys, calculate_soundness);
  return tx;
}

Tier2TransactionPtr Tier2Transaction::CreateUniquePtr(InputBuffer& buffer,
                                                      const KeyRing& keys,
                                                      bool calculate_soundness) {
  auto tx = std::make_unique<Tier2Transaction>();
  Tier2Transaction::Fill(*tx, buffer, keys, calculate_soundness);
  return tx;
}

} // namespace Devcash