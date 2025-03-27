#pragma once

#include "sizes.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace bml {

  constexpr ByteCount BYTE_SIZE{1};
  constexpr BitCount CACHE_SIZE{static_cast<std::size_t>(std::numeric_limits<std::uintmax_t>::digits)};

  struct Cache {
    // Value is left-adjusted
    std::uintmax_t value{};
    BitCount size{};

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
      if (!produceNextByte(nextByte)) [[unlikely]] {
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

  constexpr std::uintmax_t readFromCache(Cache &cache, BitCount numBits) noexcept {
    if (!numBits) {
      return 0;
    }
    auto result = (cache.value >> (CACHE_SIZE - numBits)) & numBits.mask();
    if (numBits == CACHE_SIZE) [[unlikely]] {
      cache.size = 0_bits;
      cache.value = 0;
    } else {
      cache.size -= numBits;
      cache.value <<= numBits;
    }
    return result;
  }

  template <typename ByteConsumer = bool (*)(std::byte), typename ErrorHandler = void (*)()>
  constexpr ByteCount flushFullCacheBytes(Cache &cache, ByteConsumer consumeNextByte, ErrorHandler handleEos) {
    auto bytesWritten = 0_bytes;
    while (cache.size >= 1_bytes) {
      // flush highest cache byte
      auto byte = static_cast<std::byte>(cache.value >> (CACHE_SIZE - 1_bytes));
      cache.value <<= 1_bytes;
      cache.size -= 1_bytes;
      if (!consumeNextByte(byte)) [[unlikely]] {
        handleEos();
        break;
      }
      ++bytesWritten;
    }
    return bytesWritten;
  }

  constexpr std::u8string toUtf8String(char32_t codePoint) noexcept {
    auto code = std::bit_cast<uint32_t>(codePoint);
    std::u8string outUtf8String{};
    if (code < 0x80U) {
      outUtf8String.push_back(static_cast<char8_t>(code & 0x7FU));
    } else if (code < 0x800U) {
      outUtf8String.push_back(static_cast<char8_t>(0xC0U + ((code >> 6U) & 0x1FU)));
      outUtf8String.push_back(static_cast<char8_t>(0x80U + (code & 0x3FU)));
    } else if (code < 0x10000U) {
      outUtf8String.push_back(static_cast<char8_t>(0xE0U + ((code >> 12U) & 0x0FU)));
      outUtf8String.push_back(static_cast<char8_t>(0x80U + ((code >> 6U) & 0x3FU)));
      outUtf8String.push_back(static_cast<char8_t>(0x80U + (code & 0x3FU)));
    } else if (code < 0x110000U) {
      outUtf8String.push_back(static_cast<char8_t>(0xF0U + ((code >> 18U) & 0x07U)));
      outUtf8String.push_back(static_cast<char8_t>(0x80U + ((code >> 12U) & 0x3FU)));
      outUtf8String.push_back(static_cast<char8_t>(0x80U + ((code >> 6U) & 0x3FU)));
      outUtf8String.push_back(static_cast<char8_t>(0x80U + (code & 0x3FU)));
    }
    return outUtf8String;
  }

} // namespace bml
