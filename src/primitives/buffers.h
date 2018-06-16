//
// Created by mckenney on 6/15/18.
//
#ifndef DEVCASH_BUFFERS_H
#define DEVCASH_BUFFERS_H

#include <vector>
#include "common/binary_converters.h"
#include "common/devcash_types.h"

namespace Devcash {

class InputBuffer {
 public:
  InputBuffer() = delete;

  InputBuffer(const InputBuffer& buffer) = delete;

  InputBuffer(const std::vector<byte>& input_buffer, size_t offset = 0)
      : buffer_(input_buffer)
      , offset_(offset) {}

  /**
   * Returns the byte at the given position
   *
   * @param loc
   * @return
   */
  byte at(size_t loc) {
    return buffer_.at(loc);
  }

  /**
   * Copies the size length out of the buffer and
   * increment the position by length
   *
   * @tparam Iter
   * @param out_iter
   * @param length
   */
  template <typename Iter>
  void copy(Iter out_iter, size_t length, bool increment_buffer = true)
  {
    std::copy_n(buffer_.begin() + offset_, length, out_iter);
    if (increment_buffer) { offset_ += length; }
  }

  /**
   * Copies the size N out of the buffer and into the array
   * Increments offset_ by N if increment_buffer is true
   *
   * @tparam N Size of the array
   * @param array
   * @param increment_buffer
   */
  template <size_t N>
  void copy(std::array<byte, N>& array, bool increment_buffer = true)
  {
    std::copy_n(buffer_.begin() + offset_, N, array.begin());
    if (increment_buffer) { offset_ += N; }
  }

  /**
   * Returns the byte at the current position
   *
   * @param loc
   * @return
   */
  byte getNextByte(bool increment_buffer = true) {
    byte ret = at(offset_);
    if (increment_buffer) { offset_ += sizeof(ret); }
    return ret;
  }

  /**
   * Returns the next uint32_t and increment the buffer
   * by the size if increment_buffer is true
   *
   * @param increment_buffer
   * @return
   */
  uint32_t getNextUint32(bool increment_buffer = true)
  {
    uint32_t ret(BinToUint32(buffer_, offset_));
    if (increment_buffer) { offset_ += sizeof(ret); }
    return ret;
  }

  /**
   * Returns the next uint64_t and increment the buffer
   * by the size if increment_buffer is true
   *
   * @param increment_buffer
   * @return
   */
  uint64_t getNextUint64(bool increment_buffer = true)
  {
    uint64_t ret(BinToUint64(buffer_, offset_));
    if (increment_buffer) { offset_ += sizeof(ret); }
    return ret;
  }

  const std::vector<byte>& getBuffer() const {
    return buffer_;
  }

  size_t getOffset() const {
    return offset_;
  }

  size_t& getOffsetRef() {
    return offset_;
  }

  void increment(size_t amount) {
    offset_ += amount;
  }

  size_t size() const {
    return buffer_.size();
  }

 private:
  const std::vector<byte>& buffer_;
  size_t offset_;
};

} // namespace Devcash
#endif //DEVCASH_BUFFERS_H
