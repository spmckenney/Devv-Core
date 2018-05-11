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

#include "primitives/FinalBlock.h"

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

class Blockchain {
public:
  typedef std::shared_ptr<FinalBlock> BlockSharedPtr;

  Blockchain(const std::string& name)
    : name_(name), chain_size_(0), num_transactions_(0)
  {
  }

  ~Blockchain()
  {
  }

  void push_back(BlockSharedPtr block) {
    chain_.push_back(block);
    chain_size_++;
    num_transactions_ += block->getNumTransactions();

    LOG_NOTICE << name_ << "Updating Final Blockchain - (size/ntxs)"
                << " (" << chain_size_ << "/" << num_transactions_ << ")";
  }

  size_t getNumTransactions() const {
    return num_transactions_;
  }

  BlockSharedPtr back() {
    LOG_TRACE << name_ << ": back(); size(" << chain_size_ << ")";
    return chain_.back();
  }

  const BlockSharedPtr back() const {
    LOG_TRACE << name_ << ": back() const; size(" << chain_size_ << ")";
    return chain_.back();
  }

  size_t size() const {
    LOG_TRACE << name_ << ": size(" << chain_size_ << ")";
    return chain_size_;
  }

  Hash getHighestMerkleRoot() const {
    if (chain_size_ < 1) {
      Hash genesis;
      return genesis;
    }
    return back()->getMerkleRoot();
  }

  ChainState getHighestChainState() const {
    if (chain_size_ < 1) {
      ChainState state;
      return state;
    }
    return back()->getChainState();
  }

  std::vector<byte> BinaryDump() const {
    std::vector<byte> out;
    for (auto const& item : chain_) {
      std::vector<byte> canonical = item->getCanonical();
      out.insert(out.end(), canonical.begin(), canonical.end());
    }
    return out;
  }

  std::vector<byte> PartialBinaryDump(size_t start) const {
    std::vector<byte> out;
    if (size() > 0) {
      //this interface should not return the top/back block
      for (size_t i=start; i < size()-1; i++) {
        std::vector<byte> canonical = chain_.at(i)->getCanonical();
        out.insert(out.end(), canonical.begin(), canonical.end());
      }
    }
    return out;
  }

  std::string JsonDump() const {
    std::string out("[");
    bool first = true;
    for (auto const& item : chain_) {
      if (first) {
        first = false;
      } else {
        out += ",";
      }
      out += item->getJSON();
    }
    out += "]";
    return out;
  }

private:
  std::vector<BlockSharedPtr> chain_;
  const std::string name_;
  std::atomic<int> chain_size_;
  std::atomic<int> num_transactions_;
};

} // namespace Devcash
