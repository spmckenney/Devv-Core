/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#include <primitives/Transfer.h>

/**
 * TransactionSummaryBlock
 */
template <typename StorageManager, typename StorageStrategy>
class TransactionSummaryBlock {
public:

  const uint256_t& hash_merkle_root() const {
    return(transaction_summary_block_.merkle_root());
  }

  void set_hash_merkle_root(const uint256_t& merkle_root) const {
    transaction_summary_block_.set_hash_merkle_root(merkle_root));
  }


private:
  StorageManager::TransactionSummaryBlock transaction_summary_block_;
};
