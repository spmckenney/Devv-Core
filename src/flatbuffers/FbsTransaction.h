/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

#include "flatbuffers/fbs_strategy.h"
#include "flatbuffers/FbsTransfer.h"

template <typename FlatBufferManager, typename InPlace>
class FbsTransaction {
public:
  typedef flatbuffers::Offset<fbs::Transfer> TransferType;
  typedef flatbuffers::Vector<TransferType> TransferListType

  const TransferType& GetTransferAt(size_t transfer_index) const {
    return(transaction_.xfers()->at(transfer_index));
  }

private:
  FlatBufferManager::TransactionType transaction_;
};
