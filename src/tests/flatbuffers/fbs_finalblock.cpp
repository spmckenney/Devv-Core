/**
 *
 */
#include "flatbuffers/devcash_primitives_generated.h"  // Already includes "flatbuffers/flatbuffers.h".

using namespace Devcash::Primitives;

// Example how to use FlatBuffers to create and read binary buffers.

int main(int /*argc*/, const char * /*argv*/ []) {
  // Build up a serialized buffer algorithmically:
  flatbuffers::FlatBufferBuilder builder;

  flatbuffers::Offset<flatbuffers::Vector<int8_t>> addr(33);

  auto transfer = CreateTransfer(builder
                                 , addr
                                 , 1
                                 , 2
                                 , 0);

  flatbuffers::Offset<flatbuffers::Vector<int8_t>> nonce(20);
  flatbuffers::Offset<flatbuffers::Vector<int8_t>> sig(72);

}
