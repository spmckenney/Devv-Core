/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

#include "flatbuffers/fbs_strategy.h"
#include "flatbuffers/FbsManager.h"
#include "primitives/Transfer.h"
#include "common/logger.h"

namespace Devcash {

template <>
class Transfer<FlatBufferStrategy> {
public:
  typedef FlatBufferStrategy FBS;

  Transfer(flatbuffers::FlatBufferBuilder& builder,
           Devcash::Buffer& address,
           int64_t amount,
           int64_t coin_index,
           int64_t delay) :
    transfer_(GetMutablePointer(builder,
                         fbs::CreateTransferDirect(builder,
                                                   &address,
                                                   amount,
                                                   coin_index,
                                                   delay)))
  {
  }

  bool operator==(const Transfer& other)
  {
    if (this != &other) {
      auto a1 = this->transfer_->addr();
      auto a2 = other.transfer_->addr();
      if (a1->size() != a2->size()) {
        return false;
      }
      for (size_t i = 0; i < a1->size(); ++i) {
        if ((*a1)[i] != (*a2)[i]) {
          return false;
        }
      }
      if (this->transfer_->amount() != other.transfer_->amount()) {
        return false;
      }
      if (this->transfer_->coin_index() != other.transfer_->coin_index()) {
        return false;
      }
      if (this->transfer_->delay() != other.transfer_->delay()) {
        return false;
      }
    }
    return true;
  }

  /**
   * Const reference to address
   */
  FBS::ConstBuffer& address() const {
    return(*transfer_->addr());
  }

  void set_address(FBS::ConstBuffer addr) {
    *transfer_->mutable_addr() = addr;
  }

  int64_t amount() {
    return(transfer_->amount());
  }

  void set_amount(int64_t amount) {
    transfer_->mutate_amount(amount);
  }

  int64_t coin_index() {
    return(transfer_->coin_index());
  }

  void set_coin_index(int64_t coin_index) {
    transfer_->mutate_coin_index(coin_index);
  }

  int64_t delay() {
    return(transfer_->delay());
  }

  void set_delay(int64_t delay) {
    transfer_->mutate_delay(delay);
  }

  void SetNull() {
  }

private:
  FBS::TransferType* transfer_;
};

} // namespace Devcash
