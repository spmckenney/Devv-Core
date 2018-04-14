/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#include <primitives/Transfer.h>

/**
 * FinalBlock
 */
template <type StorageManager, typename StorageStrategy>
class FinalBlock {
public:

  const uint64_t& hash_merkle_root() const {
    return(final_block_.hash_merkle_root());
  }

  void set_hash_merkle_root(const uint64_t& merkle_root) const {
    final_block_.set_hash_merkle_root(merkle_root));
  }

 bool sign_block(EC_KEY* ecKey, std::string my_addr) {}

private:
  StorageManager::FinalBlockS final_block_;
};
