#pragma once

#include "sizes.hpp"

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace bml {

  constexpr ByteCount BYTE_SIZE{1};
  constexpr BitCount CACHE_SIZE{static_cast<std::size_t>(std::numeric_limits<std::uintmax_t>::digits)};

  struct Cache {
    // Value is left-adjusted
    std::uintmax_t value;
    BitCount size;

    constexpr auto operator<=>(const Cache &) const noexcept = default;
  };

  template <typename ByteProducer = bool (*)(std::byte &), typename ErrorHandler = void (*)()>
  constexpr ByteCount fillCache(Cache &cache, BitCount numBits, ByteProducer &&produceNextByte,
                                ErrorHandler &&handleEos) {
    auto bytesRead = 0_bytes;
    if (numBits > CACHE_SIZE) [[unlikely]] {
      throw std::out_of_range("Cannot read more bits than fit into uintmax_t");
    }

    while (cache.size < numBits) {
      std::byte nextByte{};
      if (!produceNextByte(nextByte)) {
        handleEos();
        break;
      }
      ++bytesRead;
      auto byte = static_cast<std::uintmax_t>(nextByte);
      byte <<= CACHE_SIZE - cache.size - 1_bytes;
      cache.value |= byte;
      cache.size += 1_bytes;
    }
    return bytesRead;
  }

  constexpr std::uintmax_t readFromCache(Cache &cache, BitCount numBits) {
    if (!numBits)
      return 0;
    auto result = (cache.value >> (CACHE_SIZE - numBits)) & numBits.mask();
    if (numBits == CACHE_SIZE) {
      cache.size = 0_bits;
      cache.value = 0;
    } else {
      cache.size -= numBits;
      cache.value <<= numBits;
    }
    return result;
  }

  template <typename ByteConsumer = bool (*)(std::byte), typename ErrorHandler = void (*)()>
  constexpr ByteCount flushFullCacheBytes(Cache &cache, ByteConsumer &&consumeNextByte, ErrorHandler &&handleEos) {
    auto bytesWritten = 0_bytes;
    while (cache.size >= 1_bytes) {
      // flush highest cache byte
      auto byte = static_cast<std::byte>(cache.value >> (CACHE_SIZE - 1_bytes));
      cache.value <<= 1_bytes;
      cache.size -= 1_bytes;
      if (!consumeNextByte(byte)) {
        handleEos();
        break;
      }
      ++bytesWritten;
    }
    return bytesWritten;
  }

  constexpr std::u8string toUtf8String(char32_t codePoint) noexcept {
    std::u8string outUtf8String{};
    if (codePoint < 0x80) {
      outUtf8String.push_back(static_cast<char8_t>(codePoint & 0x7F));
    } else if (codePoint < 0x800) {
      outUtf8String.push_back(static_cast<char8_t>(0xC0 + ((codePoint >> 6) & 0x1F)));
      outUtf8String.push_back(static_cast<char8_t>(0x80 + (codePoint & 0x3F)));
    } else if (codePoint < 0x10000) {
      outUtf8String.push_back(static_cast<char8_t>(0xE0 + ((codePoint >> 12) & 0x0F)));
      outUtf8String.push_back(static_cast<char8_t>(0x80 + ((codePoint >> 6) & 0x3F)));
      outUtf8String.push_back(static_cast<char8_t>(0x80 + (codePoint & 0x3F)));
    } else if (codePoint < 0x110000) {
      outUtf8String.push_back(static_cast<char8_t>(0xF0 + ((codePoint >> 18) & 0x07)));
      outUtf8String.push_back(static_cast<char8_t>(0x80 + ((codePoint >> 12) & 0x3F)));
      outUtf8String.push_back(static_cast<char8_t>(0x80 + ((codePoint >> 6) & 0x3F)));
      outUtf8String.push_back(static_cast<char8_t>(0x80 + (codePoint & 0x3F)));
    }
    return outUtf8String;
  }

} // namespace bml
