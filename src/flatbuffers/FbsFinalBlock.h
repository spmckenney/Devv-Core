/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

#include <flabuffers/fbs_blockchain.h>

/**
 * FbsFinalBlock
 */
template <type FastBufferManager, typename ModifyStrategy>
class FbsFinalBlock {
public:

  Validation GetValidationAt(const KeyRing& keys);

  Transaction& SetTransaction();

  const uint64_t& hash_merkle_root() const {
    return(final_block_.hash_merkle_root());
  }

  void set_hash_merkle_root(const uint64_t& merkle_root) const {
    final_block_.set_hash_merkle_root(merkle_root));
  }

 bool sign_block(EC_KEY* ecKey, std::string my_addr) {}

private:
  FastBufferManager::FbsFinalBlockS final_block_;

};


//std::unique_ptr<FBSFinalBlock> CreateFinalBlock(uint64_t

std::unique_ptr<FBSFinalBlock> CreateFinalBlock(std::vector<uint8_t>* buffer);
