//
// Created by mckenney on 6/15/18.
//
#ifndef DEVCASH_BUFFERS_H
#define DEVCASH_BUFFERS_H

#include <vector>
#include "common/devcash_types.h"

namespace Devcash {

class InputBuffer {
 public:
  InputBuffer() = delete;

  InputBuffer(const std::vector<byte>& input_buffer, size_t offset = 0)
      : buffer_(input_buffer)
      , offset_(offset) {}

  void increment(size_t amount) {
    offset_ += amount;
  }

 private:
  const std::vector<byte>& buffer_;
  size_t offset_;
};

} // namespace Devcash
#endif //DEVCASH_BUFFERS_H
