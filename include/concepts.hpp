#pragma once

#include "io.hpp"
#include "print.hpp"

#include <concepts>
#include <cstdint>
#include <iostream>

namespace bml {

  /**
   * Concept checking whether the type is readable via the BitReader.
   */
  template <typename Type>
  concept Readable = requires(Type obj, BitReader reader) { bml::read(reader, obj); };

  /**
   * Concept checking whether the type is writeable via the BitWriter.
   */
  template <typename Type>
  concept Writeable = requires(const Type &obj, BitWriter writer) { bml::write(writer, obj); };

  /**
   * Concept checking whether the type is skipable via the BitReader.
   */
  template <typename Type>
  concept Skipable = requires(BitReader reader) { bml::skip<Type>(reader); };

  /**
   * Concept checking whether the type is copyable via the BitReader and BitWriter.
   */
  template <typename Type>
  concept Copyable = requires(BitReader reader, BitWriter writer) { bml::copy<Type>(reader, writer); };

  /**
   * Concept checking whether the type is printable via a std::ostream.
   */
  template <typename Type>
  concept Printable = requires(const Type &obj, std::ostream &out) {
    { out << bml::printView(obj) } -> std::convertible_to<std::ostream &>;
  };

  /**
   * Concept checking whether the type provides information about the minimum/maximum/actual size of the binary
   * representation of an object of this type.
   */
  template <typename Type>
  concept Sized = requires(const Type &obj) {
    { bml::minNumBits<Type>() } -> std::convertible_to<BitCount>;
    { bml::maxNumBits<Type>() } -> std::convertible_to<BitCount>;
    { bml::numBits(obj) } -> std::convertible_to<BitCount>;
  };

  /**
   * Concept checking whether the type is a bit-field.
   *
   * This is a base concept allowing all matching types to be handled by any functionality provided by this library.
   *
   * Custom bit-mapping types should strive to fulfill this concept to be generically usable.
   */
  template <typename Type>
  concept BitField =
      Readable<Type> && Writeable<Type> && Printable<Type> && Sized<Type> && Skipable<Type> && Copyable<Type>;

  /**
   * Concept checking whether the type is value-mapping bit-field.
   *
   * This concept checks additionally to the BitField concept whether the mapped value of the value type can be accessed
   * within the bit-mapping object.
   */
  template <typename Type, typename V>
  concept ValueBitField = BitField<Type> && requires(Type obj, V value) {
    typename Type::value_type;
    // conversion from and to value
    { obj } -> std::convertible_to<V>;
    requires std::assignable_from<Type &, V>;
    // get/set value
    { obj.get() } -> std::convertible_to<V>;
    obj.set(value);
  };

} // namespace bml
