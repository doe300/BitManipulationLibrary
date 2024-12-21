#include "io.hpp"

#include "common.hpp"

namespace bml {
  void copyBits(BitReader &reader, BitWriter &writer, BitCount numBits) {
    auto bitsLeft = numBits;
    while (bitsLeft) {
      auto currentBits = std::min(CACHE_SIZE / 2, bitsLeft);
      writer.write(reader.read(currentBits), currentBits);
      bitsLeft -= currentBits;
    }
  }
} // namespace bml
