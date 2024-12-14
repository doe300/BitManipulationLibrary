#pragma once

#include "concepts.hpp"
#include "debug.hpp"
#include "helper.hpp"
#include "io.hpp"
#include "print.hpp"
#include "reader.hpp"
#include "writer.hpp"

#include <algorithm>
#include <array>
#include <compare>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

namespace bml {

  /**
   * Mapping type for a single bit represented as a bool value.
   */
  class Bit {
    static_assert(sizeof(bool) == 1);

  public:
    using value_type = bool;
    static constexpr BitCount minNumBits() { return 1_bits; };
    static constexpr BitCount maxNumBits() { return 1_bits; };

    constexpr BitCount numBits() const noexcept { return 1_bits; }
    constexpr auto operator<=>(const Bit &) const noexcept = default;

    constexpr bool get() const noexcept { return value; }
    constexpr operator bool() const noexcept { return value; }

    void set(bool val) noexcept { value = val; }

    Bit &operator=(bool val) noexcept {
      value = val;
      return *this;
    }

    void read(BitReader &reader) { value = reader.read(); }
    void write(BitWriter &writer) const { writer.write(value); }

    friend std::ostream &operator<<(std::ostream &os, const Bit &bit) {
      writeBits(os, bit.value, 1_bits);
      return os;
    }

  private:
    bool value;
  };

  /**
   * Mapping type for a fixed number of bits, represented as a numerical value.
   *
   * Reading and assignment of values has built-in bounds checks.
   */
  template <std::size_t N, typename T = best_type<N>>
  class Bits {
  public:
    using value_type = T;
    static constexpr BitCount minNumBits() { return BitCount{N}; };
    static constexpr BitCount maxNumBits() { return BitCount{N}; };

    constexpr BitCount numBits() const noexcept { return BitCount{N}; }
    constexpr auto operator<=>(const Bits &) const noexcept = default;

    constexpr value_type get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }
    void set(value_type val) { value = checkValue(val); }

    Bits &operator=(value_type val) {
      value = checkValue(val);
      return *this;
    }

    void read(BitReader &reader) { value = checkValue(reader.read<T>(BitCount{N})); }

    void write(BitWriter &writer) const {
      if constexpr (std::is_enum_v<T>) {
        writer.write(static_cast<std::underlying_type_t<T>>(value), BitCount{N});
      } else {
        writer.write(value, BitCount{N});
      }
    }

    friend std::ostream &operator<<(std::ostream &os, const Bits &bits) {
      writeBits(os, bits.value, BitCount{N});
      return os;
    }

  protected:
    template <typename V = value_type>
    V checkValue(V val)
      requires std::is_enum_v<V>
    {
      auto valueBits = static_cast<std::underlying_type_t<V>>(val);
      if ((valueBits & BitCount{N}.mask()) != valueBits)
        throw std::invalid_argument("Value '" + toHexString(valueBits) + "' does not fit into a " + std::to_string(N) +
                                    "-Bit type");
      return val;
    }

    template <typename V = value_type>
    V checkValue(V val) {
      if ((val & BitCount{N}.mask()) != val)
        throw std::invalid_argument("Value '" + toHexString(val) + "' does not fit into a " + std::to_string(N) +
                                    "-Bit type");
      return val;
    }

  protected:
    value_type value;
  };

  /**
   * Mapping type for a (byte-)aligned fixed number of bytes, represented as a numerical value.
   *
   * Reading and assignment of values has built-in bounds checks.
   */
  template <std::size_t N, typename T = best_type<8 * N>, std::size_t ByteAlignment = 1>
  class Bytes : public Bits<8 * N, T> {
    using Base = Bits<8 * N, T>;

  public:
    Bytes &operator=(Base::value_type val) {
      Base::value = Base::checkValue(val);
      return *this;
    }

    void read(BitReader &reader) { Base::value = Base::checkValue(reader.readBytes<T>(ByteCount{N})); }

    void write(BitWriter &writer) const {
      if constexpr (std::is_enum_v<T>) {
        writer.writeBytes(static_cast<std::underlying_type_t<T>>(Base::value), ByteCount{N});
      } else {
        writer.writeBytes(Base::value, ByteCount{N});
      }
    }
  };

  /**
   * Mapping type for a single aligned byte, represented as a numerical value.
   *
   * Reading and assignment of values has built-in bounds checks.
   */
  template <typename T = uint8_t, std::size_t ByteAlignment = 1>
  using Byte = Bytes<1, T, ByteAlignment>;

  /**
   * Mapping type for a fixed number of bits with a fixed numerical value.
   *
   * Reading values has built-in checks against the expected fixed value.
   */
  template <std::size_t N, auto Value, bool IgnoreInvalidValue = false>
  class FixedBits {
  public:
    using value_type = std::conditional_t<std::is_enum_v<decltype(Value)>, decltype(Value), best_type<N>>;
    static constexpr BitCount minNumBits() { return BitCount{N}; };
    static constexpr BitCount maxNumBits() { return BitCount{N}; };

    constexpr BitCount numBits() const noexcept { return BitCount{N}; }
    constexpr auto operator<=>(const FixedBits &) const noexcept = default;

    constexpr value_type get() const noexcept { return Value; }
    constexpr operator value_type() const noexcept { return Value; }

    void read(BitReader &reader) { checkValue(static_cast<value_type>(reader.read<best_type<N>>(BitCount{N}))); }
    void write(BitWriter &writer) const { writer.write(static_cast<best_type<N>>(Value), BitCount{N}); }

    friend std::ostream &operator<<(std::ostream &os, const FixedBits &) {
      writeBits(os, static_cast<value_type>(Value), BitCount{N});
      return os;
    }

    static constexpr bool matches(value_type value) noexcept { return value == static_cast<value_type>(Value); }

  protected:
    void checkValue(value_type value) {
      if (!IgnoreInvalidValue && !matches(value))
        Debug::error("Value '" + toHexString(static_cast<best_type<N>>(value)) + "' does not match the fixed value '" +
                     toHexString(static_cast<best_type<N>>(Value)) + "'");
    }
  };

  /**
   * Mapping type for a (byte-)aligned fixed number of bytes with a fixed numerical value.
   *
   * Reading values has built-in checks against the expected fixed value.
   */
  template <std::size_t N, auto Value, std::size_t ByteAlignment = 1, bool IgnoreInvalidValue = false>
  class FixedBytes : public FixedBits<8 * N, Value, IgnoreInvalidValue> {
    using Base = FixedBits<8 * N, Value, IgnoreInvalidValue>;

  public:
    void read(BitReader &reader) { Base::checkValue(static_cast<Base::value_type>(reader.readBytes(ByteCount{N}))); }

    void write(BitWriter &writer) const {
      if constexpr (std::is_enum_v<decltype(Value)>) {
        writer.writeBytes(static_cast<std::underlying_type_t<decltype(Value)>>(Value), ByteCount{N});
      } else {
        writer.writeBytes(Value, ByteCount{N});
      }
    }
  };

  /**
   * Mapping type for an aligned single byte with a fixed numerical value.
   *
   * Reading values has built-in checks against the expected fixed value.
   */
  template <auto Value, std::size_t ByteAlignment = 1, bool IgnoreInvalidValue = false>
  using FixedByte = FixedBytes<1, Value, ByteAlignment, IgnoreInvalidValue>;

  /**
   * Mapping type for a single aligned byte, represented as an 8-bit character.
   */
  class Char : public Byte<char> {
    using Base = Byte<char>;
    static_assert(sizeof(char) == sizeof(std::byte));

  public:
    void set(value_type val) { Base::value = val; }

    Char &operator=(value_type val) {
      Base::value = val;
      return *this;
    }

    void read(BitReader &reader) { Base::value = static_cast<char>(reader.read<uint8_t>(1_bytes)); }

    friend std::ostream &operator<<(std::ostream &os, const Char &character) { return os << character.value; }
  };

  /**
   * Mapping type for a dynamically sized exponential-Golomb encoded unsigned integral value.
   *
   * The integral template type parameter only used for object-side storage and representation of the value and has no
   * effect on the binary storage.
   *
   * See https://en.wikipedia.org/wiki/Exponential-Golomb_coding
   */
  template <typename T = std::uintmax_t>
  class ExpGolombBits {
    static_assert(std::is_unsigned_v<T>);

  public:
    using value_type = T;
    static constexpr BitCount minNumBits() { return 1_bits; };
    static constexpr BitCount maxNumBits() { return 2_bits * bits<T>() + 1_bits; };

    constexpr BitCount numBits() const noexcept { return encodeExpGolomb(value).numBits; }
    constexpr auto operator<=>(const ExpGolombBits &) const noexcept = default;

    constexpr value_type get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }

    void set(value_type val) { value = val; }

    ExpGolombBits &operator=(value_type val) {
      value = val;
      return *this;
    }

    void read(BitReader &reader) { value = reader.readExpGolomb<value_type>(); }
    void write(BitWriter &writer) const { writer.writeExpGolomb(value); }

    friend std::ostream &operator<<(std::ostream &os, const ExpGolombBits &bits) { return os << bits.value; }

  private:
    value_type value;
  };

  /**
   * Mapping type for a dynamically sized exponential-Golomb encoded signed integral value.
   *
   * The integral template type parameter only used for object-side storage and representation of the value and has no
   * effect on the binary storage.
   *
   * See https://en.wikipedia.org/wiki/Exponential-Golomb_coding#Extension_to_negative_numbers
   */
  template <typename T = std::intmax_t>
  class SignedExpGolombBits {
    static_assert(std::is_signed_v<T>);

  public:
    using value_type = T;
    static constexpr BitCount minNumBits() { return 1_bits; };
    static constexpr BitCount maxNumBits() { return 2_bits * bits<T>() + 1_bits; };

    constexpr BitCount numBits() const noexcept { return encodeSignedExpGolomb(value).numBits; }
    constexpr auto operator<=>(const SignedExpGolombBits &) const noexcept = default;

    constexpr value_type get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }

    void set(value_type val) { value = val; }

    SignedExpGolombBits &operator=(value_type val) {
      value = val;
      return *this;
    }

    void read(BitReader &reader) { value = reader.readSignedExpGolomb<value_type>(); }
    void write(BitWriter &writer) const { writer.writeSignedExpGolomb(value); }

    friend std::ostream &operator<<(std::ostream &os, const SignedExpGolombBits &bits) { return os << bits.value; }

  private:
    value_type value;
  };

  /**
   * Mapping type for a dynamically sized Fibonacci encoded unsigned integral value.
   *
   * The integral template type parameter only used for object-side storage and representation of the value and has no
   * effect on the binary storage.
   *
   * NOTE: Fibonacci-coding cannot represent the value zero!
   *
   * See https://en.wikipedia.org/wiki/Fibonacci_coding
   */
  template <typename T = uint32_t>
  class FibonacciBits {
    static_assert(std::is_unsigned_v<T>);

  public:
    using value_type = T;
    static constexpr BitCount minNumBits() { return 2_bits; };
    static constexpr BitCount maxNumBits() {
      // F(47) is the highest Fibonacci number fitting into 32-bit + trailing 1-bit
      return 48_bits;
    };

    constexpr BitCount numBits() const noexcept { return encodeFibonacci(value).numBits; }
    constexpr auto operator<=>(const FibonacciBits &) const noexcept = default;

    constexpr value_type get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }

    void set(value_type val) { value = val; }

    FibonacciBits &operator=(value_type val) {
      value = val;
      return *this;
    }

    void read(BitReader &reader) { value = reader.readFibonacci<value_type>(); }
    void write(BitWriter &writer) const { writer.writeFibonacci(value); }

    friend std::ostream &operator<<(std::ostream &os, const FibonacciBits &bits) { return os << bits.value; }

  private:
    value_type value;
  };

  /**
   * Mapping type for a dynamically sized Negafibonacci encoded signed integral value.
   *
   * The integral template type parameter only used for object-side storage and representation of the value and has no
   * effect on the binary storage.
   *
   * NOTE: Negafibonacci-coding cannot represent the value zero!
   *
   * See https://en.wikipedia.org/wiki/Negafibonacci_coding
   */
  template <typename T = int32_t>
  class NegaFibonacciBits {
    static_assert(std::is_signed_v<T>);

  public:
    using value_type = T;
    static constexpr BitCount minNumBits() { return 2_bits; };
    static constexpr BitCount maxNumBits() {
      // F(47) is the highest Fibonacci number fitting into 32-bit + trailing 1-bit
      return 48_bits;
    };

    constexpr BitCount numBits() const noexcept { return encodeNegaFibonacci(value).numBits; }
    constexpr auto operator<=>(const NegaFibonacciBits &) const noexcept = default;

    constexpr value_type get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }

    void set(value_type val) { value = val; }

    NegaFibonacciBits &operator=(value_type val) {
      value = val;
      return *this;
    }

    void read(BitReader &reader) { value = reader.readSignedFibonacci<value_type>(); }
    void write(BitWriter &writer) const { writer.writeSignedFibonacci(value); }

    friend std::ostream &operator<<(std::ostream &os, const NegaFibonacciBits &bits) { return os << bits.value; }

  private:
    value_type value;
  };

  /**
   * Mapping type for an optional value represented by the first template type parameter and which present is determined
   * by a single bit.
   *
   * This type can be used for binary data consisting of a single bit determining the presence/absence of the next
   * element directly followed by the bits for this element.
   *
   * The template type parameter can be of any binary-mappable type (e.g. uint32_t, Bit, ExpGolombBits or a user-defined
   * POD or complex type).
   */
  template <typename T, bool OnSet = true>
  class OptionalBits {
  public:
    using value_type = std::optional<T>;
    static constexpr BitCount minNumBits() { return 1_bits; };
    static constexpr BitCount maxNumBits() { return 1_bits + bml::maxNumBits<T>(); };

    constexpr BitCount numBits() const noexcept { return 1_bits + (value ? bml::numBits(*value) : 0_bits); }
    constexpr auto operator<=>(const OptionalBits &) const noexcept = default;

    constexpr explicit operator bool() const noexcept { return value == OnSet; }
    void reset() noexcept { value.reset(); }

    constexpr value_type get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }

    const value_type *operator->() const noexcept { return &value; }
    const value_type &operator*() const noexcept { return value; }

    void set(value_type val) { value = val; }

    OptionalBits &operator=(value_type val) {
      value = val;
      return *this;
    }

    void read(BitReader &reader)
      requires Readable<T>
    {
      if (reader.read() == OnSet) {
        value = bml::read<T>(reader);
      }
    }

    void write(BitWriter &writer) const
      requires Writeable<T>
    {
      writer.write(value.has_value() == OnSet);
      if (value) {
        bml::write(writer, *value);
      }
    }

    static void skip(BitReader &reader)
      requires Skipable<T>
    {
      if (reader.read() == OnSet) {
        bml::skip<T>(reader);
      }
    }

    static void copy(BitReader &reader, BitWriter &writer)
      requires Copyable<T>
    {
      auto flag = reader.read();
      writer.write(flag);
      if (flag == OnSet) {
        bml::copy<T>(reader, writer);
      }
    }

    friend std::ostream &operator<<(std::ostream &os, const OptionalBits &bits)
      requires Printable<T>
    {
      if (bits.value)
        os << printView(*bits.value);
      else
        os << "(empty)";
      return os;
    }

  private:
    value_type value;
  };

  /**
   * Mapping-type for a list of elements with a preceding fixed-size number of elements.
   *
   * This type can be used to represent binary data consisting of a fixed-size number of elements followed directly by
   * the given number of elements.
   *
   * The template type parameter can be of any binary-mappable type (e.g. uint32_t, Bit, ExpGolombBits or a user-defined
   * POD or complex type).
   */
  template <std::size_t LengthBits, typename T>
  class BitList {
  public:
    using value_type = std::vector<T>;
    static constexpr BitCount minNumBits() { return BitCount{LengthBits}; };
    static constexpr BitCount maxNumBits() { return BitCount{LengthBits} + (1U << LengthBits) * bml::maxNumBits<T>(); };

    constexpr BitCount numBits() const noexcept {
      return std::accumulate(value.begin(), value.end(), BitCount{LengthBits},
                             [](BitCount sum, const T &val) { return sum + bml::numBits(val); });
    }
    constexpr auto operator<=>(const BitList &) const noexcept = default;

    constexpr value_type get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }

    const value_type *operator->() const noexcept { return &value; }
    const value_type &operator*() const noexcept { return value; }

    void set(value_type val) { value = val; }

    BitList &operator=(value_type val) {
      value = val;
      return *this;
    }

    void read(BitReader &reader)
      requires Readable<T>
    {
      auto numEntries = reader.read<std::size_t>(BitCount{LengthBits});
      value_type tmp;
      tmp.reserve(numEntries);
      for (std::size_t i = 0; i < numEntries; ++i) {
        bml::read(reader, tmp.emplace_back());
      }
      value = std::move(tmp);
    }

    void write(BitWriter &writer) const
      requires Writeable<T>
    {
      writer.write(value.size(), BitCount{LengthBits});
      for (const auto &entry : value)
        bml::write(writer, entry);
    }

    static void skip(BitReader &reader)
      requires Skipable<T>
    {
      auto numEntries = reader.read<std::size_t>(BitCount{LengthBits});
      for (std::size_t i = 0; i < numEntries; ++i) {
        bml::skip<T>(reader);
      }
    }

    static void copy(BitReader &reader, BitWriter &writer)
      requires Copyable<T>
    {
      auto numEntries = reader.read<std::size_t>(BitCount{LengthBits});
      writer.write(numEntries, BitCount{LengthBits});
      for (std::size_t i = 0; i < numEntries; ++i) {
        bml::copy<T>(reader, writer);
      }
    }

    friend std::ostream &operator<<(std::ostream &os, const BitList &bits)
      requires Printable<T>
    {
      os << bits.value.size() << " [";
      for (auto &entry : bits.value)
        os << printView(entry) << ", ";
      return os << ']';
    }

  protected:
    value_type value;
  };

  /**
   * Mapping-type for a byte-aligned fixed-size number characters (a fixed-size character string).
   *
   * This mapping type provides facilities for easier mapping to standard string types and prints its contained value as
   * character string.
   */
  template <std::size_t N>
  class Chars {
  public:
    using value_type = std::array<char, N>;
    static constexpr BitCount minNumBits() { return N * 1_bytes; };
    static constexpr BitCount maxNumBits() { return N * 1_bytes; };

    constexpr BitCount numBits() const noexcept { return N * 1_bytes; }
    constexpr auto operator<=>(const Chars &) const noexcept = default;

    constexpr value_type get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }

    const value_type *operator->() const noexcept { return &value; }
    const value_type &operator*() const noexcept { return value; }

    operator std::string() const { return std::string{value.data(), value.size()}; }
    operator std::string_view() const { return std::string_view{value.begin(), value.end()}; }

    void set(value_type val) { value = val; }

    Chars &operator=(value_type val) {
      value = val;
      return *this;
    }

    void read(BitReader &reader) {
      static_assert(sizeof(char) == sizeof(std::byte));
      reader.readBytesInto(std::as_writable_bytes(std::span{value}));
    }

    void write(BitWriter &writer) const {
      static_assert(sizeof(char) == sizeof(std::byte));
      writer.writeBytes(std::as_bytes(std::span{value}));
    }

    friend std::ostream &operator<<(std::ostream &os, const Chars &chars) {
      return os << '"' << static_cast<std::string>(chars) << '"';
    }

  private:
    value_type value;
  };

  /**
   * Mapping-type for a list of characters with a preceding fixed-size number of elements (i.e. a "prefix string").
   */
  template <std::size_t LengthBits>
  class PrefixString : public BitList<LengthBits, char> {
  public:
    using value_type = std::string;
    static constexpr BitCount minNumBits() { return BitCount{LengthBits}; };
    static constexpr BitCount maxNumBits() { return BitCount{LengthBits} + (1U << LengthBits) * 1_bytes; };

    constexpr BitCount numBits() const noexcept { return BitCount{LengthBits} + this->value.size() * 1_bytes; }
    constexpr auto operator<=>(const PrefixString &) const noexcept = default;

    constexpr value_type get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }

    const value_type *operator->() const noexcept { return &value; }
    const value_type &operator*() const noexcept { return value; }

    void set(value_type val) { value = val; }

    PrefixString &operator=(value_type val) {
      value = val;
      return *this;
    }

    void read(BitReader &reader) {
      auto numEntries = reader.read<std::size_t>(BitCount{LengthBits});
      value_type tmp;
      tmp.reserve(numEntries);
      for (std::size_t i = 0; i < numEntries; ++i) {
        tmp.push_back(bml::read<char>(reader));
      }
      value = std::move(tmp);
    }

    void write(BitWriter &writer) const {
      writer.write(value.size(), BitCount{LengthBits});
      for (const auto &entry : value)
        bml::write(writer, entry);
    }

    static void skip(BitReader &reader) {
      auto numEntries = reader.read<std::size_t>(BitCount{LengthBits});
      reader.skip(ByteCount{numEntries});
    }

    static void copy(BitReader &reader, BitWriter &writer) {
      auto numEntries = reader.read<std::size_t>(BitCount{LengthBits});
      writer.write(numEntries, BitCount{LengthBits});
      copyBits(reader, writer, ByteCount{numEntries});
    }

    friend std::ostream &operator<<(std::ostream &os, const PrefixString &string) {
      return os << '"' << string.value << '"';
    }

  protected:
    value_type value;
  };

  /**
   * Mapping-type for a list of elements with a preceding fixed-size overall byte-size of all elements.
   *
   * This type can be used to represent binary data consisting of a fixed-size byte-size of the following entries
   * directly followed by the elements up until the specified number of bytes are processed.
   *
   * The template type parameter can be of any binary-mappable type (e.g. uint32_t, Bit, ExpGolombBits or a user-defined
   * POD or complex type).
   */
  template <std::size_t LengthBits, typename T>
  class SizedList {
  public:
    using value_type = std::vector<T>;
    static constexpr BitCount minNumBits() { return BitCount{LengthBits}; };
    static constexpr BitCount maxNumBits() { return BitCount{LengthBits} + (1U << LengthBits) * 1_bytes; };

    constexpr BitCount numBits() const noexcept {
      return std::accumulate(value.begin(), value.end(), BitCount{LengthBits},
                             [](BitCount sum, const T &val) { return sum + bml::numBits(val); });
    }
    constexpr auto operator<=>(const SizedList &) const noexcept = default;

    constexpr value_type get() noexcept { return value; }
    constexpr const value_type &get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }

    const value_type *operator->() const noexcept { return &value; }
    const value_type &operator*() const noexcept { return value; }

    void set(value_type val) { value = val; }

    SizedList &operator=(value_type val) {
      value = val;
      return *this;
    }

    void read(BitReader &reader)
      requires Readable<T>
    {
      auto numBytes = reader.read<std::size_t>(BitCount{LengthBits});
      value_type tmp;
      auto endPosition = reader.position() + numBytes * 1_bytes;
      while (reader.position() < endPosition) {
        bml::read(reader, tmp.emplace_back());
      }
      value = std::move(tmp);
    }

    void write(BitWriter &writer) const
      requires Writeable<T>
    {
      writer.write((this->numBits() - BitCount{LengthBits}) / 1_bytes, BitCount{LengthBits});
      for (const auto &entry : value) {
        bml::write(writer, entry);
      }
    }

    static void skip(BitReader &reader) {
      auto numBytes = reader.read<std::size_t>(BitCount{LengthBits});
      reader.skip(numBytes * 1_bytes);
    }

    static void copy(BitReader &reader, BitWriter &writer) {
      auto numBytes = reader.read<std::size_t>(BitCount{LengthBits});
      writer.write(numBytes, BitCount{LengthBits});
      copyBits(reader, writer, numBytes * 1_bytes);
    }

    friend std::ostream &operator<<(std::ostream &os, const SizedList &list)
      requires Printable<T>
    {
      os << list.value.size() << " [";
      for (auto &entry : list.value)
        os << printView(entry) << ", ";
      return os << ']';
    }

  private:
    value_type value;
  };

  /**
   * Mapping-type for a value which is padded to the specified number of bytes.
   *
   * This type can be used to represent binary data with a fixed total size, but dynamically sized content and trailing
   * padding to fill up to the specified total size.
   *
   * The template type parameter can be of any binary-mappable type (e.g. uint32_t, Bit, ExpGolombBits or a user-defined
   * POD or complex type).
   */
  template <typename T, std::size_t Size, typename PadType, PadType PadValue>
  class PaddedValue {
  public:
    using value_type = T;
    static constexpr BitCount minNumBits() { return BitCount{Size}; };
    static constexpr BitCount maxNumBits() { return BitCount{Size}; };
    constexpr BitCount numBits() const noexcept { return BitCount{Size}; }
    constexpr auto operator<=>(const PaddedValue &) const noexcept = default;

    constexpr value_type get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }
    void set(value_type val) { value = val; }

    PaddedValue &operator=(value_type val) {
      value = val;
      return *this;
    }

    void read(BitReader &reader)
      requires Readable<T>
    {
      auto endPos = reader.position() + BitCount{Size};
      bml::read(reader, value);
      while (reader.position() < endPos) {
        bml::read<PadType>(reader);
      }
    }

    void write(BitWriter &writer) const
      requires Writeable<T>
    {
      auto endPos = writer.position() + BitCount{Size};
      bml::write(writer, value);
      while (writer.position() < endPos) {
        bml::write(writer, PadValue);
      }
    }

    static void skip(BitReader &reader) { reader.skip(BitCount{Size}); }
    static void copy(BitReader &reader, BitWriter &writer) { copyBits(reader, writer, BitCount{Size}); }

    friend std::ostream &operator<<(std::ostream &os, const PaddedValue &value)
      requires Printable<T>
    {
      return os << printView(value.value);
    }

  private:
    value_type value;
  };

  /**
   * Mapping-type for a variant of values with all having the same fixed size.
   *
   * This type can be used to represent binary data of a fixed size which can contain values of different types
   * (depending on some external factor, e.g. a preceding field).
   *
   * The template type parameter can be of any binary-mappable type (e.g. uint32_t, Bit, ExpGolombBits or a user-defined
   * POD or complex type).
   *
   * NOTE: Since the selection of the "active" value within this fixed-size variant is external to this object, it
   * cannot be read by its own.
   */
  template <typename... Types>
  class FixedSizeVariant {
  public:
    using value_type = std::variant<Types...>;

    static constexpr BitCount minNumBits() { return assertSameSize<Types...>(); };
    static constexpr BitCount maxNumBits() { return assertSameSize<Types...>(); };
    constexpr BitCount numBits() const noexcept { return minNumBits(); }
    constexpr auto operator<=>(const FixedSizeVariant &) const noexcept = default;

    constexpr value_type get() noexcept { return value; }
    constexpr const value_type &get() const noexcept { return value; }
    constexpr operator value_type() const noexcept { return value; }

    const value_type *operator->() const noexcept { return &value; }
    const value_type &operator*() const noexcept { return value; }

    void set(value_type val) { value = val; }

    FixedSizeVariant &operator=(const value_type &val) {
      value = val;
      return *this;
    }

    FixedSizeVariant &operator=(value_type &&val) {
      value = std::move(val);
      return *this;
    }

    template <typename U>
    FixedSizeVariant &operator=(U &&val) {
      value = std::move(val);
      return *this;
    }

    void write(BitWriter &writer) const
      requires(Writeable<Types> && ...)
    {
      std::visit([&writer](const auto &val) { bml::write(writer, val); }, value);
    }

    static void skip(BitReader &reader) { reader.skip(minNumBits()); }
    static void copy(BitReader &reader, BitWriter &writer) { copyBits(reader, writer, minNumBits()); }

    friend std::ostream &operator<<(std::ostream &os, const FixedSizeVariant &variant)
      requires(Printable<Types> && ...)
    {
      return std::visit<std::ostream &>([&os](const auto &val) -> std::ostream & { return os << printView(val); },
                                        variant.value);
    }

  private:
    template <typename U>
    using SizeType = std::conditional_t<bml::minNumBits<U>() == bml::maxNumBits<U>(),
                                        std::integral_constant<std::size_t, bml::minNumBits<U>().value()>, void>;

    template <typename FirstArg, typename... OtherArgs>
    static constexpr BitCount assertSameSize() noexcept {
      static_assert((std::is_same_v<SizeType<FirstArg>, SizeType<OtherArgs>> && ...));
      return BitCount{SizeType<FirstArg>::value};
    }

  protected:
    value_type value;
  };

  /**
   * Mapping-type for a padding value to achieve a specified alignment.
   *
   * This type can be used to represent padding up to a specific alignment.
   */
  template <std::size_t Alignment, bool Value, bool IgnoreInvalidValue = false, typename T = best_type<Alignment>>
  class AlignmentBits {
  public:
    using value_type = T;
    static constexpr BitCount minNumBits() { return 0_bits; };
    static constexpr BitCount maxNumBits() { return BitCount{Alignment} - 1_bits; };
    constexpr auto operator<=>(const AlignmentBits &) const noexcept = default;

    void read(BitReader &reader) {
      while (reader.position() % Alignment) {
        checkValue(reader.read());
      }
    }

    void write(BitWriter &writer) const {
      while (writer.position() % Alignment) {
        writer.write(Value);
      }
    }

    static void skip(BitReader &reader) { reader.skipToAligment(BitCount{Alignment}); }

    static void copy(BitReader &reader, BitWriter &writer) {
      AlignmentBits tmp{};
      tmp.read(reader);
      tmp.write(writer);
    }

    friend std::ostream &operator<<(std::ostream &os, const AlignmentBits &) {
      return os << "(fill to " << Alignment << " bits with " << (Value ? '1' : '0') << ")";
    }

  private:
    void checkValue(bool value) {
      if (!IgnoreInvalidValue && value != Value)
        Debug::error("Value '" + toHexString(value) + "' does not match the alignment padding value '" +
                     toHexString(Value) + "'");
    }
  };
} // namespace bml
