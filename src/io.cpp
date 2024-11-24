#include "io.hpp"

namespace bml {
  void copyBits(BitReader &reader, BitWriter &writer, BitCount numBits) {
    static_assert(BitReader::CACHE_SIZE == BitWriter::CACHE_SIZE);
    auto bitsLeft = numBits;
    while (bitsLeft) {
      auto currentBits = std::min(BitReader::CACHE_SIZE / 2, bitsLeft);
      writer.write(reader.read(currentBits), currentBits);
      bitsLeft -= currentBits;
    }
  }
} // namespace bml
