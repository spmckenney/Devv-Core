/**
 * blockchain.h
 *  Classes to manage blockchains
 *
 *  Created on: Apr 8, 2018
 *      Author: Shawn McKenney <shawn.mckenney@emmion.com
 */
#pragma once

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
    : name_(name)
  {
  }

  ~Blockchain()
  {
  }

  void push_back(BlockSharedPtr block) {
    chain_.push_back(block);
    LOG_TRACE << name_ << ": push_back(); new size(" << chain_.size() << ")";
  }

  BlockSharedPtr& back() {
    LOG_TRACE << name_ << ": back(); size(" << chain_.size() << ")";
    return chain_.back();
  }

  const BlockSharedPtr& back() const {
    LOG_TRACE << name_ << ": back() const; size(" << chain_.size() << ")";
    return chain_.back();
  }

  size_t size() const {
    LOG_TRACE << name_ << ": size(" << chain_.size() << ")";
    return chain_.size();
  }

  BlockSharedPtr& at(size_t where) {
    LOG_TRACE << name_ << ": at(" << where << "); size(" << chain_.size() << ")";
    return chain_.at(where);
  }

  const BlockSharedPtr& at(size_t where) const {
    LOG_TRACE << name_ << ": at(" << where << ") const; size(" << chain_.size() << ")";
    return chain_.at(where);
  }

private:
  std::vector<BlockSharedPtr> chain_;
  const std::string name_;
};

typedef Blockchain<FinalBlock> FinalBlockchain;
typedef Blockchain<ProposedBlock> ProposedBlockchain;

  /*
class FinalBlockchain {
  FinalBlockchain();
  ~FinalBlockchain();

private:
  std::vector<FinalPtr> chain_;
};
  */

} // namespace Devcash
