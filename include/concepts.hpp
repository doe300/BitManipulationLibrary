#pragma once

#include "io.hpp"
#include "print.hpp"

#include <concepts>
#include <cstdint>
#include <iostream>

namespace bml {
  inline namespace concepts {
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
     * This concept checks additionally to the BitField concept whether the mapped value of the value type can be
     * accessed within the bit-mapping object.
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

    /**
     * Concept checking whether the type is a direct value mapper.
     *
     * This concept checks whether instances of the given type can be used to map values of its #type nested member type
     * from a BitReader/to a BitWriter.
     */
    template <typename T>
    concept DirectMapper = requires(T obj) {
      typename T::type;

      { obj(std::declval<BitReader &>()) } -> std::convertible_to<typename T::type>;
      obj(std::declval<BitWriter &>(), std::declval<const typename T::type &>());
    };

    /**
     * Concept checking whether the type is a member value mapper.
     *
     * This concept checks whether instances of the given type can be used to map a member (of its #member_type) from
     * its #object_type nested member type from a BitReader/to a BitWriter.
     */
    template <typename T>
    concept MemberMapper = requires(T obj) {
      typename T::object_type;
      typename T::member_type;

      obj(std::declval<BitReader &>(), std::declval<typename T::object_type &>());
      obj(std::declval<BitWriter &>(), std::declval<const typename T::object_type &>());
    };

    /**
     * Concept checking whether the given type is a homogeneous container which can be default-constructed and provides
     * following member accesses:
     * - operator[size_t] -> value_type&
     * - resize(size_t)
     */
    template <typename T>
    concept DefaultConstructibleResizeableContainer = requires(T obj) {
      typename T::value_type;
      T{};
      { obj[std::declval<std::size_t>()] } -> std::same_as<typename T::value_type &>;
      obj.resize(std::declval<std::size_t>());
    };

    /**
     * Checks whether the given type is an enumeration type.
     */
    template <typename T>
    concept Enum = std::is_enum_v<T>;

    /**
     * Checks whether the given type is a value mapper with a fixed binary representation size.
     *
     * This is determined by the presence of a #fixedSize() member function returning a BitCount (or ByteCount).
     */
    template <typename T>
    concept FixedSizeMapper = requires(T obj) {
      { obj.fixedSize() } -> std::convertible_to<BitCount>;
    };
  } // namespace concepts
} // namespace bml
