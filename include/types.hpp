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
   * Byte-aligned number of full bytes.
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

  template <typename T = uint8_t, std::size_t ByteAlignment = 1>
  using Byte = Bytes<1, T, ByteAlignment>;

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
   * Byte-aligned fixed-value entry.
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

  template <auto Value, std::size_t ByteAlignment = 1, bool IgnoreInvalidValue = false>
  using FixedByte = FixedBytes<1, Value, ByteAlignment, IgnoreInvalidValue>;

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

  template <typename T = std::uintmax_t>
  class ExpGolombBits {
    static_assert(std::is_unsigned_v<T>);

  public:
    using value_type = T;
    static constexpr BitCount minNumBits() { return 1_bits; };
    static constexpr BitCount maxNumBits() { return 2_bits * bits<T>() + 1_bits; };

    constexpr BitCount numBits() const noexcept { return encodeExpGolomb(value).second; }
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

  template <typename T = std::intmax_t>
  class SignedExpGolombBits {
    static_assert(std::is_signed_v<T>);

  public:
    using value_type = T;
    static constexpr BitCount minNumBits() { return 1_bits; };
    static constexpr BitCount maxNumBits() { return 2_bits * bits<T>() + 1_bits; };

    constexpr BitCount numBits() const noexcept { return encodeSignedExpGolomb(value).second; }
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
   * A list with a preceding number of entries.
   *
   * For a length value of N, this container stores N entries of the specified type.
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
   * A list with a preceding length in bytes.
   *
   * For a length value of N, this container stores the number of entries fitting into N bytes.
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
        throw std::invalid_argument("Value '" + toHexString(value) + "' does not match the alignment padding value '" +
                                    toHexString(Value) + "'");
    }
  };
} // namespace bml