/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

#include <flatbuffers/fbs_strategy.h>

typedef <>
class FBSTransfer<FlatBufferManager, InPlace> {
public:
  void FBSTransfer(FlatBufferManager::Buffer address,
                int64_t amount,
                int64_t coin_index,
                int64_t delay);

  /**
   * Assignment operator
   */
  Transfer* operator=(DCTransfer&& other)
  {
    if (this != &other) {
      this->set_address(other.address());
      this->set_amount(other.amount());
      this->set_delay(other.delay());
      this->set_coin_index(other.coin_index());
    }
    return this;
  }

  /**
   * Assignment move operator
   */
  DCTransfer* operator=(const DCTransfer& other)
  {
    if (this != &other) {
      this->set_address(other.address());
      this->set_amount(other.amount());
      this->set_delay(other.delay());
      this->set_coin_index(other.coin_index());
    }
    return this;
  }

  ConstBuffer address() {
    return(transfer_.addr());
  }

  void set_address(ConstBuffer addr) {
    *transfer_.mutaable_addr() = addr;
  }

  int64_t amount() {
    return(transfer_.amount());
  }

  void set_amount(int64_t amount) {
    transfer_.mutate_amount(amount);
  }

  int64_t coin_index() {
    return(transfer_.coin_index);
  }

  void set_coin_index(int64_t coin_index) {
    transfer_.mutate_coin_index(coin_index);
  }

  int64_t delay() {
    return(transfer_.delay());
  }

  void set_delay(int64_t delay) {
    transfer_.mutate_delay(delay);
  }

  void SetNull {

private:
    FlatBufferManager::FBSTransferType transfer_;
};
