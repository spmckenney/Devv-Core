/**
 * blockchain.h
 *  Classes to manage blockchains
 *
 *  Created on: Apr 8, 2018
 *      Author: Shawn McKenney <shawn.mckenney@emmion.com
 */
#pragma once

#include <atomic>
#include <vector>

#include "consensus/proposedblock.h"
#include "consensus/finalblock.h"

namespace Devcash {

// default implementation
template <typename T>
struct TypeName
{
    static const char* Get()
    {
        return typeid(T).name();
    }
};

// a specialization for each type of those you want to support
// and don't like the string returned by typeid
template <>
struct TypeName<int>
{
    static const char* Get()
    {
        return "int";
    }
};

// usage:
//const char* name = TypeName<MyType>::Get();

template <typename BlockType>
class Blockchain {
public:
  typedef std::shared_ptr<BlockType> BlockSharedPtr;

  Blockchain(const std::string& name)
    : name_(name), chain_size_(0)
  {
  }

  ~Blockchain()
  {
  }

  void push_back(BlockSharedPtr block) {
    chain_.push_back(block);
    chain_size_++;
    LOG_TRACE << name_ << ": push_back(); new size(" << chain_size_ << ")";
  }

  BlockSharedPtr& back() {
    LOG_TRACE << name_ << ": back(); size(" << chain_size_ << ")";
    return chain_.back();
  }

  const BlockSharedPtr& back() const {
    LOG_TRACE << name_ << ": back() const; size(" << chain_size_ << ")";
    return chain_.back();
  }

  size_t size() const {
    LOG_TRACE << name_ << ": size(" << chain_size_ << ")";
    return chain_size_;
  }

  BlockSharedPtr& at(size_t where) {
    LOG_TRACE << name_ << ": at(" << where << "); size(" << chain_size_ << ")";
    return chain_.at(where);
  }

  const BlockSharedPtr& at(size_t where) const {
    LOG_TRACE << name_ << ": at(" << where << ") const; size(" << chain_size_ << ")";
    return chain_.at(where);
  }

  uint256_t getHighestMerkleRoot() const {
    if (chain_size_ < 1) return 0;
    return back().get()->getMerkleRoot();
  }

private:
  std::vector<BlockSharedPtr> chain_;
  const std::string name_;
  std::atomic<int> chain_size_;
};

typedef Blockchain<FinalBlock> FinalBlockchain;
typedef Blockchain<ProposedBlock> ProposedBlockchain;

} // namespace Devcash
