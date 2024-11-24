#pragma once

#include "sizes.hpp"

#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

namespace bml {

  template <typename T>
  constexpr std::size_t bits() noexcept
    requires(std::integral<T> || std::is_enum_v<T>)
  {
    return std::numeric_limits<std::make_unsigned_t<T>>::digits;
  }

  template <>
  constexpr std::size_t bits<bool>() noexcept {
    return 1U;
  }

  constexpr uintmax_t mask(BitCount numBits) noexcept {
    if (numBits.value() == std::numeric_limits<uintmax_t>::digits)
      return static_cast<uintmax_t>(-1);
    return (uintmax_t{1} << numBits) - 1U;
  }

  template <std::size_t N>
  using best_type =
      std::conditional_t<N <= bits<uint8_t>(), uint8_t,
                         std::conditional_t<N <= bits<uint16_t>(), uint16_t,
                                            std::conditional_t<N <= bits<uint32_t>(), uint32_t, uint64_t>>>;

  template <std::unsigned_integral T>
  constexpr std::pair<best_type<2 * bits<T>() + 1>, BitCount> encodeExpGolomb(T value) noexcept {
    ++value;
    auto numBits = std::bit_width(value) - 1;
    return std::make_pair(best_type<2 * bits<T>() + 1>{value}, static_cast<BitCount>(numBits * 2 + 1));
  }

  template <std::unsigned_integral T>
  constexpr T decodeExpGolomb(T value) noexcept {
    return value - 1U;
  }

  std::string toHexString(std::uintmax_t value, ByteCount typeSize, bool withPrefix = true) noexcept;

  template <std::unsigned_integral T>
  std::string toHexString(T value, bool withPrefix = true) noexcept {
    return toHexString(static_cast<std::uintmax_t>(value), ByteCount{sizeof(T)}, withPrefix);
  }

  template <std::signed_integral T>
  constexpr std::pair<best_type<2 * bits<T>() + 1>, BitCount> encodeSignedExpGolomb(T value) noexcept {
    auto tmp = value < 0 ? (-2 * value) : value > 0 ? (2 * value - 1) : 0;
    return encodeExpGolomb(std::bit_cast<std::make_unsigned_t<T>>(tmp));
  }

  template <std::unsigned_integral T>
  constexpr std::intmax_t decodeSignedExpGolomb(T value) noexcept {
    auto tmp = decodeExpGolomb(value);
    auto sign = (tmp + 1U) & 0x1 ? -1 : 1;
    auto val = static_cast<std::intmax_t>(tmp / 2 + (tmp & 0x1));
    return sign * val;
  }

  void writeBits(std::ostream &os, std::uintmax_t value, BitCount numBits);
  void writeBits(std::ostream &os, std::intmax_t value, BitCount numBits);

  template <typename T>
  void writeBits(std::ostream &os, T value, BitCount numBits = BitCount{bits<T>()})
    requires(std::integral<T> || std::is_enum_v<T>)
  {
    auto tmp = static_cast<std::conditional_t<std::is_signed_v<T>, std::intmax_t, std::uintmax_t>>(value);
    writeBits(os, tmp, numBits);
  }
} // namespace bml
