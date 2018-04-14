/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#include <primitives/Transfer.h>

/**
 * TSVBlock
 */
template <type StorageManager, typename StorageStrategy>
class TSVBlock {
public:

  const uint64_t& hash_merkle_root() const {
    return(tsv_block_.hash_merkle_root());
  }

  void set_hash_merkle_root(const uint64_t& merkle_root) const {
    tsv_block_.set_hash_merkle_root(merkle_root));
  }

private:
  StorageManager::TSVBlockS tsv_block_;
};
