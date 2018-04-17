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
  typedef flatbuffers::Vector<int8_t> Buffer;
  typedef const flatbuffers::Vector<int8_t> ConstBuffer;
};

/**
 * Returns a pointer to an object given the Offset<object>
 */
template <typename Type>
Type* GetMutablePointer(flatbuffers::FlatBufferBuilder& builder, const flatbuffers::Offset<Type>& object) {
  return (reinterpret_cast<Type *>(builder.GetCurrentBufferPointer() + builder.GetSize() - object.o));
}

template <typename Type>
Type* GetPointer(flatbuffers::FlatBufferBuilder& builder, flatbuffers::Offset<Type>& object) {
  return (reinterpret_cast<Type *>(builder.GetCurrentBufferPointer() + builder.GetSize() - object.o));
}

template <typename Type>
const Type* GetPointer(flatbuffers::FlatBufferBuilder& builder, const flatbuffers::Offset<Type>& object) {
  return (reinterpret_cast<const Type *>(builder.GetCurrentBufferPointer() + builder.GetSize() - object.o));
}

} // namespace Devcash
