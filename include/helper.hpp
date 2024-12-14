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

  /**
   * Returns the fixed number of binary data bits for the specified integral or enum type.
   */
  template <typename T>
  constexpr std::size_t bits() noexcept
    requires(std::integral<T> || std::is_enum_v<T>)
  {
    return std::numeric_limits<std::make_unsigned_t<T>>::digits;
  }

  /**
   * Returns the fixed number of binary data bits for the bool type.
   */
  template <>
  constexpr std::size_t bits<bool>() noexcept {
    return 1U;
  }

  /**
   * Inverts the lower given number of bits within the given value.
   *
   * E.g. converts 0b1011 to 0b1101.
   */
  std::uintmax_t invertBits(std::uintmax_t value, BitCount numBits);

  template <std::unsigned_integral T>
  T invertBits(T value, BitCount numBits = BitCount{bits<T>()}) {
    return static_cast<T>(invertBits(static_cast<std::uintmax_t>(value), numBits));
  }

  /**
   * Helper type to determine the best unsigned integral type to store the specified number of bits.
   */
  template <std::size_t N>
  using best_type =
      std::conditional_t<N <= bits<uint8_t>(), uint8_t,
                         std::conditional_t<N <= bits<uint16_t>(), uint16_t,
                                            std::conditional_t<N <= bits<uint32_t>(), uint32_t, uint64_t>>>;

  template <typename T>
  struct EncodedValue {
    T value;
    /** Number of valid lower bits */
    BitCount numBits;
  };

  /**
   * Encodes the given unsigned integral value with the Exponential-Golomb coding and returns the encoded value.
   */
  template <std::unsigned_integral T>
  constexpr EncodedValue<best_type<2 * bits<T>() + 1>> encodeExpGolomb(T value) noexcept {
    ++value;
    auto numBits = std::bit_width(value) - 1;
    return {best_type<2 * bits<T>() + 1>{value}, static_cast<BitCount>(numBits * 2 + 1)};
  }

  /**
   * Decodes the given Exponential-Golomb encoded value into its underlying unsigned integral value.
   */
  template <std::unsigned_integral T>
  constexpr T decodeExpGolomb(T value) noexcept {
    return value - 1U;
  }

  /**
   * Returns a string containing a hexadecimal representation of the given value with the given number of bytes.
   */
  std::string toHexString(std::uintmax_t value, ByteCount typeSize, bool withPrefix = true) noexcept;

  /**
   * Returns a string containing a hexadecimal representation of the given value with the given number of bytes.
   */
  template <std::unsigned_integral T>
  std::string toHexString(T value, bool withPrefix = true) noexcept {
    return toHexString(static_cast<std::uintmax_t>(value), ByteCount{sizeof(T)}, withPrefix);
  }

  /**
   * Encodes the given signed integral value with the Exponential-Golomb coding and returns the encoded value.
   */
  template <std::signed_integral T>
  constexpr EncodedValue<best_type<2 * bits<T>() + 1>> encodeSignedExpGolomb(T value) noexcept {
    auto tmp = value < 0 ? (-2 * value) : value > 0 ? (2 * value - 1) : 0;
    return encodeExpGolomb(std::bit_cast<std::make_unsigned_t<T>>(tmp));
  }

  /**
   * Decodes the given Exponential-Golomb encoded value into its underlying signed integral value.
   */
  template <std::unsigned_integral T>
  constexpr std::intmax_t decodeSignedExpGolomb(T value) noexcept {
    auto tmp = decodeExpGolomb(value);
    auto sign = (tmp + 1U) & 0x1 ? -1 : 1;
    auto val = static_cast<std::intmax_t>(tmp / 2 + (tmp & 0x1));
    return sign * val;
  }

  /**
   * Encodes the given unsigned integral value with the Fibonacci coding and returns the encoded value.
   */
  EncodedValue<std::uintmax_t> encodeFibonacci(uint32_t value) noexcept;

  /**
   * Decodes the given Fibonacci encoded value into its underlying unsigned integral value.
   */
  uint32_t decodeFibonacci(std::uintmax_t value) noexcept;

  /**
   * Encodes the given signed integral value with the Negafibonacci coding and returns the encoded value.
   */
  EncodedValue<std::uintmax_t> encodeNegaFibonacci(int32_t value);

  /**
   * Decodes the given Negafibonacci encoded value into its underlying signed integral value.
   */
  int32_t decodeNegaFibonacci(std::uintmax_t value);

  void writeBits(std::ostream &os, std::uintmax_t value, BitCount numBits);
  void writeBits(std::ostream &os, std::intmax_t value, BitCount numBits);

  /**
   * Writes a string representation of the given value and number of valid bits to the given output stream.
   *
   * Bit widths of less than 8 are written as binary literal (with "0b" prefix) while any larger bit widths are written
   * as hexadecimal literal (with "0x" prefix).
   */
  template <typename T>
  void writeBits(std::ostream &os, T value, BitCount numBits = BitCount{bits<T>()})
    requires(std::integral<T> || std::is_enum_v<T>)
  {
    auto tmp = static_cast<std::conditional_t<std::is_signed_v<T>, std::intmax_t, std::uintmax_t>>(value);
    writeBits(os, tmp, numBits);
  }
} // namespace bml
