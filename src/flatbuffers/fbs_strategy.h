/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

#include "flatbuffers/devcash_primitives_generated.h"
#include "primitives/strategy.h"

namespace Devcash {

struct FlatBufferStrategy {
  typedef fbs::Transfer TransferType;
  typedef fbs::Transaction TransactionType;
  typedef flatbuffers::Vector<flatbuffers::Offset<fbs::Transfer>> TransferListType;
  typedef flatbuffers::Offset<flatbuffers::Vector<int8_t>> Buffer;
  typedef flatbuffers::Offset<const flatbuffers::Vector<int8_t>> ConstBuffer;
};

} // namespace Devcash
