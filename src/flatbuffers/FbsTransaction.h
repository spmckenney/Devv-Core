/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

#include "flatbuffers/fbs_strategy.h"
#include "flatbuffers/FbsTransfer.h"

template <typename FlatBufferManager, typename InPlaceStrategy>
class FbsTransaction {
public:
  typedef flatbuffers::Vector<flatbuffers::Offset<fbs::Transfer>> TransferListType

  const StorageManager::TransferType& GetTransferAt(size_t transfer_index) const {
    return(

  FlatBufferManager& AddTransfer();

private:
   TransferListType transfers_
};
