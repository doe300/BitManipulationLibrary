#pragma once

#include "destructure.hpp"
#include "io_internal.hpp"
#include "reader.hpp"
#include "sizes.hpp"
#include "writer.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <numeric>
#include <optional>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace bml {

  ////
  // READ
  ////

  /**
   * Reads from the given BitReader into the given output object using its member read() function.
   */
  template <typename Type>
  void read(BitReader &reader, Type &value)
    requires(detail::HasReadMember<Type>)
  {
    value.read(reader);
  }

  /**
   * Reads a single bit from the given BitReader into the given output value.
   */
  template <typename Type>
  void read(BitReader &reader, Type &value)
    requires(std::is_same_v<Type, bool>)
  {
    value = reader.read();
  }

  /**
   * Reads a fixed-size unsigned integral value from the given BitReader into the given output value.
   */
  template <typename Type>
  void read(BitReader &reader, Type &value)
    requires(std::is_unsigned_v<Type> && !std::is_same_v<Type, bool>)
  {
    value = reader.read<Type>(sizeof(Type) * 1_bytes);
  }

  /**
   * Reads a fixed-size signed integral value from the given BitReader into the given output value.
   */
  template <typename Type>
  void read(BitReader &reader, Type &value)
    requires(std::is_signed_v<Type>)
  {
    value = std::bit_cast<Type>(reader.read<std::make_unsigned_t<Type>>(sizeof(Type) * 1_bytes));
  }

  /**
   * Reads a fixed-size numerical value from the given BitReader into the given output object enum value.
   */
  template <typename Type>
  void read(BitReader &reader, Type &value)
    requires(std::is_enum_v<Type>)
  {
    value = static_cast<Type>(reader.read<std::underlying_type_t<Type>>(sizeof(Type) * 1_bytes));
  }

  /**
   * Reads from the given BitReader into the given output object using member-wise read.
   */
  template <typename Type>
  void read(BitReader &reader, Type &object)
    requires(!detail::HasReadMember<Type> && detail::AllowedDestructurable<Type>)
  {
    detail::readMembers(reader, object);
  }

  /**
   * Reads all elements from the given BitReader into the given output array.
   */
  template <typename Type, std::size_t N>
  void read(BitReader &reader, std::array<Type, N> &value)
    requires requires(Type obj, BitReader r) { bml::read(r, obj); }
  {
    for (auto &elem : value) {
      read(reader, elem);
    }
  }

  /**
   * Reads all elements from the given BitReader into the given output object tuple.
   */
  template <typename... Types>
  void read(BitReader &reader, std::tuple<Types...> &value)
    requires(requires(Types obj, BitReader r) { bml::read(r, obj); } && ...)
  {
    std::apply([&reader](auto &...args) { (read(reader, args), ...); }, value);
  }

  /**
   * Reads an object of the specific type from the given BitReader if the given condition is true.
   */
  template <typename Type>
  std::optional<Type> readOptional(BitReader &reader, bool condition) {
    if (condition) {
      Type tmp{};
      read(reader, tmp);
      return tmp;
    }
    return std::optional<Type>{};
  }

  /**
   * Reads an object of the specific type from the given BitReader.
   */
  template <typename T>
  T read(BitReader &reader) {
    T tmp{};
    read(reader, tmp);
    return tmp;
  }

  ////
  // WRITE
  ////

  inline void write(BitWriter &, const std::monostate &) { /* no-op */ }

  /**
   * Writes the given object to the given BitWriter using its member write() function.
   */
  template <typename Type>
  void write(BitWriter &writer, const Type &value)
    requires(detail::HasWriteMember<Type>)
  {
    value.write(writer);
  }

  /**
   * Writes the given boolean value to the given BitWriter as a single bit.
   */
  template <typename Type>
  void write(BitWriter &writer, const Type &value)
    requires(std::is_same_v<Type, bool>)
  {
    writer.write(value);
  }

  /**
   * Writes the given fixed-size unsigned integral value to the given BitWriter.
   */
  template <typename Type>
  void write(BitWriter &writer, const Type &value)
    requires(std::is_unsigned_v<Type> && !std::is_same_v<Type, bool>)
  {
    writer.write(value, sizeof(Type) * 1_bytes);
  }

  /**
   * Writes the given fixed-size signed integral value to the given BitWriter.
   */
  template <typename Type>
  void write(BitWriter &writer, const Type &value)
    requires(std::is_signed_v<Type>)
  {
    writer.write(std::bit_cast<std::make_unsigned_t<Type>>(value), sizeof(Type) * 1_bytes);
  }

  /**
   * Writes the given fixed-size enum value to the given BitWriter.
   */
  template <typename Type>
  void write(BitWriter &writer, const Type &value)
    requires(std::is_enum_v<Type>)
  {
    writer.write(static_cast<std::underlying_type_t<Type>>(value), sizeof(Type) * 1_bytes);
  }

  /**
   * Writes the given byte to the given BitWriter.
   */
  inline void write(BitWriter &writer, const std::byte &value) {
    writer.write(static_cast<uint8_t>(value), ByteCount{1});
  }

  /**
   * Writes the given object to the given BitWriter using member-wise write.
   */
  template <typename Type>
  void write(BitWriter &writer, const Type &object)
    requires(!detail::HasWriteMember<Type> && detail::AllowedDestructurable<Type>)
  {
    detail::writeMembers(writer, object);
  }

  /**
   * Writes the given pointed-to value to the given BitWriter if present.
   */
  template <typename Type>
  void write(BitWriter &writer, const std::unique_ptr<Type> &value)
    requires requires(const Type &obj, BitWriter w) { bml::write(w, obj); }
  {
    if (value) {
      bml::write(writer, *value);
    }
  }

  /**
   * Writes the given list of objects to the given BitWriter by writing all elements.
   */
  template <typename Type>
  void write(BitWriter &writer, const std::vector<Type> &value)
    requires requires(const Type &obj, BitWriter w) { bml::write(w, obj); }
  {
    for (const auto &element : value) {
      bml::write(writer, element);
    }
  }

  /**
   * Writes the given array of objects to the given BitWriter by writing all elements.
   */
  template <typename Type, std::size_t N>
  void write(BitWriter &writer, const std::array<Type, N> &value)
    requires requires(const Type &obj, BitWriter w) { bml::write(w, obj); }
  {
    for (const auto &element : value) {
      bml::write(writer, element);
    }
  }

  /**
   * Writes the given tuple to the given BitWriter by writing all elements.
   */
  template <typename... Types>
  void write(BitWriter &writer, const std::tuple<Types...> &value)
    requires(requires(const Types &obj, BitWriter w) { bml::write(w, obj); } && ...)
  {
    std::apply([&writer](auto &...args) { (bml::write(writer, args), ...); }, value);
  }

  /**
   * Writes the given variant to the given BitWriter by writing the active element.
   */
  template <typename... Types>
  void write(BitWriter &writer, const std::variant<Types...> &value)
    requires(requires(const Types &obj, BitWriter w) { bml::write(w, obj); } && ...)
  {
    return std::visit([&writer](const auto &entry) { return bml::write(writer, entry); }, value);
  }

  /**
   * Writes the given optional object to the given BitWriter if present.
   */
  template <typename Type>
  void write(BitWriter &writer, const std::optional<Type> &value)
    requires requires(const Type &obj, BitWriter w) { bml::write(w, obj); }
  {
    if (value) {
      bml::write(writer, *value);
    }
  }

  ////
  // SIZE
  ////

  /**
   * Returns the minimum binary data size of the specified type.
   */
  template <typename Type>
  constexpr BitCount minNumBits() noexcept
    requires requires(const Type &obj) {
      { detail::SizeHelper<Type>::calcMinNumBits() } -> std::convertible_to<BitCount>;
    }
  {
    return detail::SizeHelper<Type>::calcMinNumBits();
  }

  /**
   * Returns the maximum binary data size of the specified type.
   */
  template <typename Type>
  constexpr BitCount maxNumBits() noexcept
    requires requires(const Type &obj) {
      { detail::SizeHelper<Type>::calcMaxNumBits() } -> std::convertible_to<BitCount>;
    }
  {
    return detail::SizeHelper<Type>::calcMaxNumBits();
  }

  /**
   * Returns the effective binary data size of the given object using its member numBits() function.
   */
  template <typename Type>
  constexpr BitCount numBits(const Type &obj) noexcept
    requires(detail::HasSizeMember<Type>)
  {
    return obj.numBits();
  }

  /**
   * Returns the effective binary data size of the given fixed-size object.
   */
  template <typename Type>
  constexpr BitCount numBits(const Type & /*obj */) noexcept
    requires(!detail::HasSizeMember<Type> && detail::SizeHelper<Type>::isFixedSize())
  {
    return bml::minNumBits<Type>();
  }

  /**
   * Returns the effective binary data size of the given object by accumulating its member binary data sizes.
   */
  template <typename Type>
  constexpr BitCount numBits(const Type &object) noexcept
    requires(!detail::HasSizeMember<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             detail::AllowedDestructurable<Type>)
  {
    return detail::numBitsMembers(object);
  }

  /**
   * Returns the effective binary data size of the given pointed-to object if present.
   */
  template <typename Type>
  constexpr BitCount numBits(const std::unique_ptr<Type> &object) noexcept
    requires requires(const Type &obj) {
      { bml::numBits(obj) } -> std::convertible_to<BitCount>;
    }
  {
    return object ? bml::numBits(*object) : 0_bits;
  }

  /**
   * Returns the effective binary data size of the given vector by accumulating its element binary data sizes.
   */
  template <typename Type>
  constexpr BitCount numBits(const std::vector<Type> &object) noexcept
    requires requires(const Type &obj) {
      { bml::numBits(obj) } -> std::convertible_to<BitCount>;
    }
  {
    return std::accumulate(object.begin(), object.end(), BitCount{},
                           [](BitCount sum, const Type &elem) { return sum + bml::numBits(elem); });
  }

  /**
   * Returns the effective binary data size of the given array by accumulating its element binary data sizes.
   */
  template <typename Type, std::size_t N>
  constexpr BitCount numBits(const std::array<Type, N> &object) noexcept
    requires requires(const Type &obj) {
      { bml::numBits(obj) } -> std::convertible_to<BitCount>;
    }
  {
    return std::accumulate(object.begin(), object.end(), BitCount{},
                           [](BitCount sum, const Type &elem) { return sum + bml::numBits(elem); });
  }

  /**
   * Returns the effective binary data size of the given tuple by accumulating its element binary data sizes.
   */
  template <typename... Types>
  constexpr BitCount numBits(const std::tuple<Types...> &object) noexcept
    requires(requires(const Types &obj) {
      { bml::numBits(obj) } -> std::convertible_to<BitCount>;
    } && ...)
  {
    return std::apply([](const auto &...elem) { return (bml::numBits(elem) + ...); }, object);
  }

  /**
   * Returns the effective binary data size of the given variant by calculating the binary data size of the active
   * element.
   */
  template <typename... Types>
  constexpr BitCount numBits(const std::variant<Types...> &object) noexcept
    requires(requires(const Types &obj) {
      { bml::numBits(obj) } -> std::convertible_to<BitCount>;
    } && ...)
  {
    return std::visit([](const auto &entry) { return bml::numBits(entry); }, object);
  }

  /**
   * Returns the effective binary data size of the given optional value if present.
   */
  template <typename Type>
  constexpr BitCount numBits(const std::optional<Type> &object) noexcept
    requires requires(const Type &obj) {
      { bml::numBits(obj) } -> std::convertible_to<BitCount>;
    }
  {
    return object ? bml::numBits(*object) : 0_bits;
  }

  ////
  // COPY
  ////

  /**
   * Copies the binary data representing the specified type from the given BitReader to the given BitWriter by using its
   * member copy() function.
   */
  template <typename Type>
  void copy(BitReader &reader, BitWriter &writer)
    requires(detail::HasCopyMember<Type>)
  {
    Type::copy(reader, writer);
  }

  /**
   * Copies the given number of raw bits from the given BitReader to the given BitWriter.
   */
  void copyBits(BitReader &reader, BitWriter &writer, BitCount numBits);

  /**
   * Copies the binary data representing the specified type from the given BitReader to the given BitWriter by copying
   * its fixed number of bits.
   */
  template <typename Type>
  void copy(BitReader &reader, BitWriter &writer)
    requires(!detail::HasCopyMember<Type> && detail::SizeHelper<Type>::isFixedSize())
  {
    copyBits(reader, writer, minNumBits<Type>());
  }

  /**
   * Copies the binary data representing the specified type from the given BitReader to the given BitWriter by using its
   * member read() and write() functions on a temporary instance.
   */
  template <typename Type>
  void copy(BitReader &reader, BitWriter &writer)
    requires(!detail::HasCopyMember<Type> && !detail::SizeHelper<Type>::isFixedSize() && detail::HasReadMember<Type> &&
             detail::HasWriteMember<Type>)
  {
    // prefer custom member read/write over member-wise copy
    std::remove_cvref_t<Type> tmp{};
    tmp.read(reader);
    tmp.write(writer);
  }

  /**
   * Copies the binary data representing the specified type from the given BitReader to the given BitWriter by copying
   * each member object.
   */
  template <typename Type>
  void copy(BitReader &reader, BitWriter &writer)
    requires(!detail::HasCopyMember<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             !(detail::HasReadMember<Type> && detail::HasWriteMember<Type>) && detail::AllowedDestructurable<Type>)
  {
    detail::copyMembers<decltype(detail::destructureReference(std::declval<Type>()))>(reader, writer);
  }

  /**
   * Copies the binary data representing the specified tuple type from the given BitReader to the given BitWriter by
   * copying each element.
   */
  template <typename Type>
  void copy(BitReader &reader, BitWriter &writer)
    requires(detail::IsTuple<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             requires(BitReader &r, BitWriter &w) { detail::TupleHelper<Type>::copy(r, w); })
  {
    detail::TupleHelper<Type>::copy(reader, writer);
  }

  /**
   * Copies the binary data representing the specified array type from the given BitReader to the given BitWriter by
   * copying all elements.
   */
  template <typename Type>
  void copy(BitReader &reader, BitWriter &writer)
    requires(detail::IsArray<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             requires(BitReader &r, BitWriter &w) { bml::copy<typename detail::ArrayHelper<Type>::value_type>(r, w); })
  {
    for (std::size_t i = 0; i < std::tuple_size_v<Type>; ++i) {
      copy<typename detail::ArrayHelper<Type>::value_type>(reader, writer);
    }
  }

  ////
  // SKIP
  ////

  /**
   * Skips the binary data representing the specified type for reading from the given BitReader using its member skip()
   * function.
   */
  template <typename Type>
  void skip(BitReader &reader)
    requires(detail::HasSkipMember<Type>)
  {
    Type::skip(reader);
  }

  /**
   * Skips the binary data representing the specified fixed-size type for reading from the given BitReader by skipping
   * the fixed number of bytes.
   */
  template <typename Type>
  void skip(BitReader &reader)
    requires(!detail::HasSkipMember<Type> && detail::SizeHelper<Type>::isFixedSize())
  {
    reader.skip(minNumBits<Type>());
  }

  /**
   * Skips the binary data representing the specified type for reading from the given BitReader using its member read()
   * function on a temporary instance.
   */
  template <typename Type>
  void skip(BitReader &reader)
    requires(!detail::HasSkipMember<Type> && !detail::SizeHelper<Type>::isFixedSize() && detail::HasReadMember<Type>)
  {
    // prefer custom member read over member-wise skip
    Type tmp{};
    tmp.read(reader);
  }

  /**
   * Skips the binary data representing the specified type for reading from the given BitReader by skipping the binary
   * data representations of all member objects.
   */
  template <typename Type>
  void skip(BitReader &reader)
    requires(!detail::HasSkipMember<Type> && !detail::SizeHelper<Type>::isFixedSize() && !detail::HasReadMember<Type> &&
             detail::AllowedDestructurable<Type>)
  {
    detail::skipMembers<decltype(detail::destructure(std::declval<Type>()))>(reader);
  }

  /**
   * Skips the binary data representing the specified tuple type for reading from the given BitReader by skipping the
   * binary data representations of all elements.
   */
  template <typename Type>
  void skip(BitReader &reader)
    requires(detail::IsTuple<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             requires(BitReader &r) { detail::TupleHelper<Type>::skip(r); })
  {
    detail::TupleHelper<Type>::skip(reader);
  }

  /**
   * Skips the binary data representing the specified array type for reading from the given BitReader by skipping the
   * binary data representations of all elements.
   */
  template <typename Type>
  void skip(BitReader &reader)
    requires(detail::IsArray<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             requires(BitReader &r) { bml::skip<typename detail::ArrayHelper<Type>::value_type>(r); })
  {
    for (std::size_t i = 0; i < std::tuple_size_v<Type>; ++i) {
      skip<typename detail::ArrayHelper<Type>::value_type>(reader);
    }
  }

  namespace detail {
    // Define some forward-declared symbols after all the symbols that might be called have been declared

    template <typename Type>
    void readMembers(BitReader &reader, Type &object) {
      std::apply([&reader](auto &...args) { (bml::read(reader, args), ...); }, destructureReference(object));
    }

    template <typename Type>
    void writeMembers(BitWriter &writer, const Type &object) {
      std::apply([&writer](auto &...args) { (bml::write(writer, args), ...); }, destructureReference(object));
    }

    template <typename Type>
    constexpr BitCount numBitsMembers(const Type &object) noexcept {
      return std::apply([](const auto &...args) { return (bml::numBits(args) + ...); }, destructureReference(object));
    }

    template <typename... Types>
    void copyMembers(BitReader &reader, BitWriter &writer) {
      (bml::copy<Types>(reader, writer), ...);
    }

    template <typename... Types>
    void skipMembers(BitReader &reader) {
      (bml::skip<Types>(reader), ...);
    }

    template <typename... Types>
    void TupleHelper<std::tuple<Types...>>::skip(BitReader &reader) {
      (bml::skip<Types>(reader), ...);
    }

    template <typename... Types>
    void TupleHelper<std::tuple<Types...>>::copy(BitReader &reader, BitWriter &writer) {
      (bml::copy<Types>(reader, writer), ...);
    }
  } // namespace detail
} // namespace bml
