#pragma once

#include "sizes.hpp"

#include <bit>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <type_traits>

namespace bml {

  /**
   * Returns the fixed number of binary data bits for the specified integral or enum type.
   */
  template <typename T>
  constexpr std::size_t bits() noexcept
    requires((std::integral<T> || std::is_enum_v<T>) && !std::is_same_v<T, bool>)
  {
    return std::numeric_limits<std::make_unsigned_t<T>>::digits;
  }

  /**
   * Returns the fixed number of binary data bits for the bool type.
   */
  template <typename T>
  constexpr std::size_t bits() noexcept
    requires(std::is_same_v<T, bool>)
  {
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
    auto numBits = static_cast<uint32_t>(std::bit_width(value) - 1);
    return {best_type<2 * bits<T>() + 1>{value}, BitCount{numBits * 2U + 1U}};
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
    std::uintmax_t tmp;
    if constexpr (std::is_same_v<T, bool>) {
      tmp = static_cast<std::uintmax_t>(value);
    } else if constexpr (std::is_enum_v<T>) {
      tmp = static_cast<std::uintmax_t>(std::bit_cast<std::make_unsigned_t<std::underlying_type_t<T>>>(value));
    } else {
      tmp = static_cast<std::uintmax_t>(std::bit_cast<std::make_unsigned_t<T>>(value));
    }
    writeBits(os, tmp, numBits);
  }

  /**
   * Representation of a subrange of some byte buffer.
   */
  struct ByteRange {
    /**
     * Offset of the range data relative to the start of the underlying buffer.
     */
    ByteCount offset;

    /**
     * Number of data bytes in the range.
     */
    ByteCount size;

    constexpr auto operator<=>(const ByteRange &) const noexcept = default;
    constexpr explicit operator bool() const noexcept { return static_cast<bool>(size); }
    constexpr bool empty() const noexcept { return static_cast<bool>(!size); }

    std::string toString() const;
    friend std::ostream &operator<<(std::ostream &os, const ByteRange &range) { return os << range.toString(); }

    /**
     * Returns the sub-range with the given offset (relative to this range start) and (optional) size.
     *
     * If the sub-range offset lies outside of this range, an empty range is returned.
     * If the sub-range reached out of this range, it it truncated to close with this range's end.
     *
     * NOTE: The resulting range's offset is relative to the underlying data start, not this range!
     */
    ByteRange subRange(ByteCount subOffset,
                       ByteCount subSize = ByteCount{std::numeric_limits<std::size_t>::max()}) const noexcept;

    /**
     * Applies this range to the given input span by producing a subspan limited to the byte range represented by this
     * object.
     *
     * NOTE: If this byte range does not fit into the given input span, an empty span is returned.
     */
    std::span<std::byte> applyTo(std::span<std::byte> source) const noexcept;
    std::span<const std::byte> applyTo(std::span<const std::byte> source) const noexcept;
  };
} // namespace bml
