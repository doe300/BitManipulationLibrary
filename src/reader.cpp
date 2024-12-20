
#include "reader.hpp"

#include "common.hpp"
#include "helper.hpp"
#include "source_sink.hpp"

#include <stdexcept>

namespace bml {

  bool BitReader::hasMoreBytes() noexcept {
    makeAvailable(BYTE_SIZE, false);
    return cacheSize >= BYTE_SIZE;
  }

  BitCount BitReader::skipToAligment(BitCount bitAlignment) {
    auto numBits = position() % bitAlignment ? (bitAlignment - position() % bitAlignment) : 0_bits;
    skip(numBits);
    return numBits;
  }

  void BitReader::assertAlignment(BitCount bitAlignment) {
    if (skipToAligment(bitAlignment)) [[unlikely]] {
      throw std::invalid_argument("Input bit stream is not properly aligned");
    }
  }

  bool BitReader::read() {
    makeAvailable(1_bits);
    auto bit = (cache >> (CACHE_SIZE - 1_bits)) & 1u;
    --cacheSize;
    cache <<= 1u;
    return bit;
  }

  std::uintmax_t BitReader::peek(BitCount numBits) {
    makeAvailable(numBits);
    Cache tmp{cache, cacheSize};
    return readFromCache(tmp, numBits);
  }

  std::uintmax_t BitReader::read(BitCount numBits) {
    if (cacheSize && numBits > cacheSize && (numBits + cacheSize) > (CACHE_SIZE - BYTE_SIZE)) {
      // if we fill a non-empty cache with too many bits, the last byte might no longer fit (e.g. when having 1 bit in
      // cache and loading 64 bits from the source), since only full bytes can be loaded. In this case split the
      // reading.
      auto upper = read(numBits / 2);
      auto lower = read(numBits - (numBits / 2));
      return upper << (numBits - (numBits / 2)) | lower;
    }
    makeAvailable(numBits);
    Cache tmp{cache, cacheSize};
    auto bits = readFromCache(tmp, numBits);
    cache = tmp.value;
    cacheSize = tmp.size;
    return bits;
  }

  std::uintmax_t BitReader::readBytes(ByteCount numBytes) {
    assertAlignment(BYTE_SIZE);
    return read(numBytes);
  }

  void BitReader::readBytesInto(std::span<std::byte> outBytes) {
    assertAlignment(BYTE_SIZE);
    if (cacheSize) {
      throw std::invalid_argument("Input bit stream is not properly aligned");
    }
    if (!extractNextBytes(source, outBytes)) {
      throw std::out_of_range("Cannot read more bytes, end of input reached");
    } else {
      bytesRead += outBytes.size() * 1_bytes;
    }
  }

  std::uintmax_t BitReader::readExpGolomb() {
    auto exponent = readLeadingZeroes();
    return decodeExpGolomb(read(exponent + 1_bits /* marker 1-bit */));
  }

  std::intmax_t BitReader::readSignedExpGolomb() {
    auto exponent = readLeadingZeroes();
    return decodeSignedExpGolomb(read(exponent + 1_bits /* marker 1-bit */));
  }

  char32_t BitReader::readUtf8CodePoint() {
    auto b = peek(1_bytes);
    if ((b & 0x80) == 0x00) {
      // simple 7-bit ASCII
      return read<uint32_t>(1_bytes);
    } else if ((b & 0xE0) == 0xC0) {
      uint32_t codePoint = (read<uint32_t>(1_bytes) & 0x1F) << 6U;
      return codePoint | (read<uint32_t>(1_bytes) & 0x3F);
    } else if ((b & 0xF0) == 0xE0) {
      uint32_t codePoint = (read<uint32_t>(1_bytes) & 0x0F) << 12;
      codePoint |= (read<uint32_t>(1_bytes) & 0x3F) << 6;
      return codePoint | (read<uint32_t>(1_bytes) & 0x3F);
    } else if ((b & 0xF8) == 0xF0) {
      uint32_t codePoint = (read<uint32_t>(1_bytes) & 0x07) << 18;
      codePoint |= (read<uint32_t>(1_bytes) & 0x3F) << 12;
      codePoint |= (read<uint32_t>(1_bytes) & 0x3F) << 6;
      return codePoint | (read<uint32_t>(1_bytes) & 0x3F);
    } else if ((b & 0xC0) == 0x80) {
      // start within the previous UTF-8 character, cannot decode, so skip remainder
      while ((peek(1_bytes) & 0xC0) == 0x80) {
        readByte();
      }
    }
    return 0;
  }

  char32_t BitReader::readUtf16CodePoint() {
    auto b = peek(2_bytes);
    if (b < 0xD800 || b >= 0xE000) {
      return read<uint32_t>(2_bytes);
    } else if ((b & 0xDC00) == 0xD800) {
      uint32_t codePoint = (read<uint32_t>(2_bytes) & 0x3FF) << 10;
      return 0x10000U | codePoint | (read<uint32_t>(2_bytes) & 0x3FF);
    } else if ((b & 0xDC00) == 0xDC00) {
      // start within the previous UTF-16 character, cannot decode, so skip remainder
      while ((peek(2_bytes) & 0xDC00) == 0xDC00) {
        read(2_bytes);
      }
    }
    return 0;
  }

  uint32_t BitReader::readFibonacci() {
    auto [value, numBits] = readUntilTwoOnes();
    return decodeFibonacci(invertBits(value, numBits));
  }

  int32_t BitReader::readSignedFibonacci() {
    auto [value, numBits] = readUntilTwoOnes();
    return decodeNegaFibonacci(invertBits(value, numBits));
  }

  void BitReader::skip(BitCount numBits) {
    auto numBitsSkipped = 0_bits;
    while (numBitsSkipped < numBits) {
      BitCount numBitsToRead = std::min(numBits - numBitsSkipped, CACHE_SIZE);
      read(numBitsToRead);
      numBitsSkipped += numBitsToRead;
    }
  }

  void BitReader::makeAvailable(BitCount numBits, bool throwOnEos) {
    Cache tmp{cache, cacheSize};
    bytesRead += fillCache(
        tmp, numBits, [this](std::byte &nextByte) { return extractNextByte(source, nextByte); },
        [throwOnEos]() {
          if (throwOnEos) {
            throw std::out_of_range("Cannot read more bytes, end of input reached");
          }
        });
    cache = tmp.value;
    cacheSize = tmp.size;
  }

  BitCount BitReader::readLeadingZeroes() {
    BitCount numBits{};
    while (cache == 0) {
      if (cacheSize) {
        numBits += cacheSize;
        read(cacheSize);
      }
      makeAvailable(BYTE_SIZE);
    }
    BitCount numRemainingBits{static_cast<uint32_t>(std::countl_zero(cache))};
    read(numRemainingBits);
    numBits += numRemainingBits;
    return numBits;
  }

  EncodedValue<uint64_t> BitReader::readUntilTwoOnes() {
    EncodedValue<uint64_t> result{};
    while (!(cache & (cache >> 1U))) {
      // as long as there are not two consecutive 1 bits, add all bits
      if (cacheSize) {
        result.numBits += cacheSize - 1_bits;
        result.value <<= cacheSize - 1_bits;
        result.value |= read(cacheSize - 1_bits);
      }
      makeAvailable(BYTE_SIZE);
    }
    bool lastBitSet = false;
    bool nextBitSet = false;
    while (true) {
      nextBitSet = read();
      result.numBits += 1_bits;
      result.value <<= 1_bits;
      result.value |= nextBitSet ? 1U : 0U;
      if (nextBitSet && lastBitSet) {
        break;
      }
      lastBitSet = nextBitSet;
    }
    return result;
  }

} // namespace bml
