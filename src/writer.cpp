
#include "writer.hpp"

#include "common.hpp"
#include "helper.hpp"
#include "source_sink.hpp"

#include <stdexcept>

namespace bml {

  BitCount BitWriter::fillToAligment(BitCount bitAlignment, bool bit) {
    auto numBits = position() % bitAlignment ? (bitAlignment - position() % bitAlignment) : 0_bits;
    auto value = bit ? std::numeric_limits<std::uintmax_t>::max() : 0U;
    write(value, numBits);
    return numBits;
  }

  void BitWriter::assertAlignment(BitCount bitAlignment) {
    if (position() % bitAlignment) {
      throw std::invalid_argument("Output bit stream is not properly aligned");
    }
  }

  void BitWriter::write(std::uintmax_t value, BitCount numBits) {
    if ((numBits + cacheSize) > CACHE_SIZE) {
      // split up the bits to write into multiple calls
      auto upper = value >> (CACHE_SIZE / 2u);
      write(upper, numBits - CACHE_SIZE / 2u);
      auto lower = value & ((std::uintmax_t{1} << (CACHE_SIZE / 2u)) - 1u);
      write(lower, CACHE_SIZE / 2u);
      return;
    }
    cacheSize += numBits;
    cache |= value << (CACHE_SIZE - cacheSize);
    flushFullBytes();
  }

  void BitWriter::writeBytes(std::uintmax_t value, ByteCount numBytes) {
    assertAlignment(BYTE_SIZE);
    write(value, numBytes);
  }

  void BitWriter::writeBytes(std::span<const std::byte> bytes) {
    assertAlignment(BYTE_SIZE);
    flushFullBytes();
    if (cacheSize) {
      throw std::invalid_argument("Output bit stream is not properly aligned");
    }
    if (!writeNextBytes(sink, bytes)) {
      throw std::out_of_range("Cannot write more bytes, end of output reached");
    }
    bytesWritten += ByteCount{bytes.size()};
  }

  void BitWriter::writeExpGolomb(std::uintmax_t value) {
    auto [encodedValue, numBits] = encodeExpGolomb(value);
    write(encodedValue, numBits);
  }

  void BitWriter::writeSignedExpGolomb(std::intmax_t value) {
    auto [encodedValue, numBits] = encodeSignedExpGolomb(value);
    write(encodedValue, numBits);
  }

  void BitWriter::writeUtf8CodePoint(char32_t codePoint) {
    auto utf8String = toUtf8String(codePoint);
    writeBytes(std::as_bytes(std::span{utf8String}));
  }

  void BitWriter::writeUtf16CodePoint(char32_t codePoint) {
    auto value = static_cast<uint32_t>(codePoint);
    if (value < 0x10000) {
      write(value, 2_bytes);
    } else {
      value -= 0x10000;
      write(0xD800 + ((value >> 10) & 0x3FF), 2_bytes);
      write(0xDC00 + (value & 0x3FF), 2_bytes);
    }
  }

  void BitWriter::writeFibonacci(uint32_t value) {
    auto [encodedValue, numBits] = encodeFibonacci(value);
    write(invertBits(encodedValue, numBits), numBits);
  }

  void BitWriter::writeSignedFibonacci(int32_t value) {
    auto [encodedValue, numBits] = encodeNegaFibonacci(value);
    write(invertBits(encodedValue, numBits), numBits);
  }

  void BitWriter::flushFullBytes() {
    Cache tmp{cache, cacheSize};
    bytesWritten += flushFullCacheBytes(
        tmp, [this](std::byte nextByte) { return writeNextByte(sink, nextByte); },
        []() { throw std::out_of_range("Cannot write more bytes, end of output reached"); });
    cache = tmp.value;
    cacheSize = tmp.size;
  }

} // namespace bml
