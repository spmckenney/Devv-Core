/**
 * dynamic_pointer_cast.h
 * Provides access to blockchain structures.
 *
 * @copywrite  2018 Devvio Inc
 */
#pragma once

#include <memory>

// These are reworked from from: https://stackoverflow.com/a/11003103

template <typename T_DEST, typename T_SRC, typename T_DELETER>
std::unique_ptr<T_DEST, T_DELETER>
dynamic_pointer_cast(std::unique_ptr<T_SRC, T_DELETER> & src)
{
  if (!src)
    return std::unique_ptr<T_DEST, T_DELETER>(nullptr);

  T_DEST * dest_ptr = dynamic_cast<T_DEST *>(src.get());
  if (!dest_ptr)
    return std::unique_ptr<T_DEST, T_DELETER>(nullptr);

  std::unique_ptr<T_DEST, T_DELETER> dest_temp(dest_ptr, std::move(src.get_deleter()));

  src.release();

  return dest_temp;
}

template <typename T_SRC, typename T_DEST>
std::unique_ptr<T_DEST>
dynamic_pointer_cast(std::unique_ptr<T_SRC> & src)
{
  if (!src)
    return std::unique_ptr<T_DEST>(nullptr);

  T_DEST * dest_ptr = dynamic_cast<T_DEST *>(src.get());
  if (!dest_ptr)
    return std::unique_ptr<T_DEST>(nullptr);

  std::unique_ptr<T_DEST> dest_temp(dest_ptr);

  src.release();

  return dest_temp;
}

template <typename Tx>
struct TxDeleter
{
  void operator()(Tx* t)
  {
    delete t;
  }
};


