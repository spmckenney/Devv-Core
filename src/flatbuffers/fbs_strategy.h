/**
 * Copyright 2018 Devv.io
 * Author: Shawn McKenney <shawn.mckenney@emmion.com>
 */
#pragma once

#include <primitives/strategy.h>

namespace Devcash
{

struct Flatbuffer<InPlace> {
  typedef fbs::Transfer TransferType;
  typedef fbs::Transaction TransactionType;
  typedef flatbuffers::Vector<flatbuffers::Offset<fbs::Transfer>> TransferListType;
}

} // namespace Devcash
