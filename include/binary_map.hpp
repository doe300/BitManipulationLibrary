#pragma once

#include "concepts.hpp"
#include "print.hpp"
#include "reader.hpp"
#include "sizes.hpp"
#include "writer.hpp"

#include <array>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace bml {

  ////
  // Forward declarations
  ////

  template <typename T>
  struct MapElementary {
    static T read(BitReader &reader, BitCount numBits);
    static void write(BitWriter &writer, const T &value, BitCount numBits);
    static T readExpGolomb(BitReader &reader);
    static void writeExpGolomb(BitWriter &writer, const T &value);
    static T readFibonacci(BitReader &reader);
    static void writeFibonacci(BitWriter &writer, const T &value);
  };

  template <typename T, typename M>
  struct MapContainer {
    static T read(BitReader &reader, const M &readElement, std::size_t numElements);
    static void write(BitWriter &writer, const T &value, const M &writeElement, std::size_t numElements);
  };

  template <typename T>
  [[nodiscard]] constexpr auto mapBits(BitCount numBits) noexcept;
  template <typename T, BitCount NumBits>
  [[nodiscard]] constexpr auto mapBits() noexcept;
  template <typename T>
  [[nodiscard]] constexpr auto mapBits() noexcept;
  template <std::integral T>
  [[nodiscard]] constexpr auto mapBits() noexcept;
  template <Enum T>
  [[nodiscard]] constexpr auto mapBits() noexcept;

  template <typename T>
  [[nodiscard]] constexpr auto mapBytes(ByteCount numBytes) noexcept;
  template <typename T, ByteCount NumBytes>
  [[nodiscard]] constexpr auto mapBytes() noexcept;
  template <typename T>
  [[nodiscard]] constexpr auto mapBytes() noexcept;
  template <std::integral T>
  [[nodiscard]] constexpr auto mapBytes() noexcept;
  template <Enum T>
  [[nodiscard]] constexpr auto mapBytes() noexcept;

  template <typename T>
  [[nodiscard]] constexpr auto mapExpGolombBits() noexcept;

  template <typename T>
  [[nodiscard]] constexpr auto mapUtf8Bytes() noexcept;

  template <typename T>
  [[nodiscard]] constexpr auto mapFibonacciBits() noexcept;

  template <typename T>
  [[nodiscard]] constexpr auto mapUncheckedFixedBits(T fixedValue, BitCount numBits);
  template <auto FixedValue, BitCount NumBits>
  [[nodiscard]] constexpr auto mapUncheckedFixedBits() noexcept;

  template <typename T, typename Ex = std::invalid_argument>
  [[nodiscard]] constexpr auto mapCheckedFixedBits(T fixedValue, BitCount numBits);
  template <auto FixedValue, BitCount NumBits, typename Ex = std::invalid_argument>
  [[nodiscard]] constexpr auto mapCheckedFixedBits() noexcept;

  template <typename T>
  [[nodiscard]] constexpr auto mapUncheckedFixedBytes(T fixedValue, ByteCount numBytes);
  template <auto FixedValue, ByteCount NumBytes>
  [[nodiscard]] constexpr auto mapUncheckedFixedBytes() noexcept;
  template <typename T, typename Ex = std::invalid_argument>
  [[nodiscard]] constexpr auto mapCheckedFixedBytes(T fixedValue, ByteCount numBytes);
  template <auto FixedValue, ByteCount NumBytes, typename Ex = std::invalid_argument>
  [[nodiscard]] constexpr auto mapCheckedFixedBytes() noexcept;

  template <typename T, MemberMapper... Mappers>
  [[nodiscard]] constexpr auto mapCompound(Mappers... memberMappers) noexcept;

  template <typename O, typename T, DirectMapper M>
  [[nodiscard]] constexpr auto mapMember(T O::*member, M mapValue) noexcept;
  template <auto Member, auto MapValue>
  [[nodiscard]] constexpr auto mapMember() noexcept;
  template <BitCount NumBits, typename O, typename T>
  [[nodiscard]] constexpr auto mapMemberBits(T O::*member) noexcept;
  template <auto Member, BitCount NumBits>
  [[nodiscard]] constexpr auto mapMemberBits() noexcept;
  template <typename O, std::integral T>
  [[nodiscard]] constexpr auto mapMemberBits(T O::*member) noexcept;
  template <auto Member>
  [[nodiscard]] constexpr auto mapMemberBits() noexcept;
  template <typename G, typename S, DirectMapper M>
  [[nodiscard]] constexpr auto mapMemberProperty(G memberGetter, S memberSetter, M mapValue) noexcept;
  template <auto MemberGetter, auto MemberSetter, auto MapValue>
  [[nodiscard]] constexpr auto mapMemberProperty() noexcept
    requires(!std::is_same_v<decltype(MapValue), BitCount>);
  template <BitCount NumBits, typename G, typename S>
  [[nodiscard]] constexpr auto mapMemberProperty(G memberGetter, S memberSetter) noexcept;
  template <auto MemberGetter, auto MemberSetter, BitCount NumBits>
  [[nodiscard]] constexpr auto mapMemberProperty() noexcept;
  template <typename G, typename S>
  [[nodiscard]] constexpr auto mapMemberProperty(G memberGetter, S memberSetter) noexcept;
  template <auto MemberGetter, auto MemberSetter>
  [[nodiscard]] constexpr auto mapMemberProperty() noexcept;

  template <typename O, DefaultConstructibleResizeableContainer T, DirectMapper M, typename S>
  [[nodiscard]] constexpr auto mapMemberContainer(T O::*member, M mapElement, S O::*sizeMember) noexcept;
  template <auto Member, auto MapValue, auto SizeMember>
  [[nodiscard]] constexpr auto mapMemberContainer() noexcept;

  template <typename O, typename T, DirectMapper M, std::size_t N>
  [[nodiscard]] constexpr auto mapMemberArray(std::array<T, N> O::*member, M mapElement) noexcept;
  template <auto Member, auto MapValue>
  [[nodiscard]] constexpr auto mapMemberArray() noexcept;

  [[nodiscard]] constexpr auto assertByteAligned() noexcept;

  namespace detail {

    /**
     * Marker type for a typed compile-time constant value.
     */
    template <auto Value>
    struct CompileTimeValueHolder {
      using type = std::remove_cvref_t<decltype(Value)>;

      constexpr auto value() const noexcept { return Value; }
      constexpr operator decltype(Value)() const noexcept { return Value; }
      constexpr operator BitCount() const noexcept
        requires(std::is_convertible_v<decltype(Value), BitCount>)
      {
        return Value;
      }
      constexpr operator ByteCount() const noexcept
        requires(std::is_same_v<decltype(Value), BitCount> && Value % 8 == 0)
      {
        return ByteCount{Value / 1_bytes};
      }

      template <auto Other>
      friend constexpr CompileTimeValueHolder<Value + Other>
      operator+(CompileTimeValueHolder<Value> /* unused */, CompileTimeValueHolder<Other> /* unused */) noexcept {
        return CompileTimeValueHolder<Value + Other>{};
      }
    };

    /**
     * Marker type for a typed run-time constant value.
     */
    template <typename T>
    struct RunTimeValueHolder {
      using type = T;

      [[no_unique_address]] T storedValue;

      constexpr auto value() const noexcept { return storedValue; }
      constexpr operator T() const noexcept { return storedValue; }
      constexpr operator BitCount() const noexcept
        requires(std::is_convertible_v<T, BitCount>)
      {
        return storedValue;
      }

      friend constexpr RunTimeValueHolder operator+(const RunTimeValueHolder &one,
                                                    const RunTimeValueHolder &other) noexcept {
        return RunTimeValueHolder{one.storedValue + other.storedValue};
      }

      template <auto Value>
      friend constexpr RunTimeValueHolder operator+(const RunTimeValueHolder &one,
                                                    CompileTimeValueHolder<Value> /* unused */) noexcept {
        return RunTimeValueHolder{one.storedValue + Value};
      }

      template <auto Value>
      friend constexpr RunTimeValueHolder operator+(CompileTimeValueHolder<Value> /* unused */,
                                                    const RunTimeValueHolder &other) noexcept {
        return RunTimeValueHolder{Value + other.storedValue};
      }
    };

    /**
     * Marker type for a typed run-time value which is calculated on-the-fly.
     */
    struct DynamicValueHolder {

      template <typename T>
      friend constexpr DynamicValueHolder operator+(DynamicValueHolder /* unused */, T /* unused */) noexcept {
        return DynamicValueHolder{};
      }

      template <typename T>
      friend constexpr DynamicValueHolder operator+(T /* unused */, DynamicValueHolder /* unused */) noexcept
        requires(!std::is_same_v<T, DynamicValueHolder>)
      {
        return DynamicValueHolder{};
      }
    };

    template <typename Mapper>
    constexpr auto getFixedSize(const Mapper & /* unused */) noexcept {
      return DynamicValueHolder{};
    }

    template <FixedSizeMapper Mapper>
    constexpr auto getFixedSize(const Mapper &mapper) noexcept {
      return mapper.fixedSize();
    }

    /**
     * Marker type representing the product of a marker type with a constant.
     */
    template <typename T, std::size_t N>
    struct Multiply {
      [[no_unique_address]] T value;

      constexpr T operator()() const noexcept { return T{N * value}; }
    };

    /**
     * Marker type representing the product of a compile-time constant marker type with a constant.
     */
    template <auto Value, std::size_t N>
    struct Multiply<CompileTimeValueHolder<Value>, N> {
      [[no_unique_address]] CompileTimeValueHolder<Value> dummy;

      constexpr auto operator()() const noexcept { return CompileTimeValueHolder<N * Value>{}; }
    };

    /**
     * Base mapper type for a simple value mapper using read and write functors.
     *
     * Extending the reader and writer functors allows empty-base-class-optimization to reduce the object size if they
     * do not have any members (e.g. for function pointers, captureless lambdas).
     */
    template <typename NumBitsHolder, typename Reader, typename Writer>
    struct MapSimple : Reader, Writer {

      [[no_unique_address]] NumBitsHolder numBits;

      using Reader::operator();
      using Writer::operator();

      constexpr auto fixedSize() const noexcept { return numBits; }
    };

    template <typename NumBitsHolder, typename Reader, typename Writer>
    MapSimple(Reader &&, Writer &&, NumBitsHolder &&) -> MapSimple<NumBitsHolder, Reader, Writer>;

    /**
     * Base mapper type for a simple value mapper of a specified type using read and write functors.
     *
     * Extending the reader and writer functors allows empty-base-class-optimization to reduce the object size if they
     * do not have any members (e.g. for function pointers, captureless lambdas).
     */
    template <typename NumBitsHolder, typename Reader, typename Writer>
    struct MapTyped : NumBitsHolder, Reader, Writer {
      using type = std::decay_t<std::invoke_result_t<Reader, BitReader &>>;
      using Reader::operator();
      using Writer::operator();

      constexpr NumBitsHolder fixedSize() const noexcept
        requires(requires(NumBitsHolder b) { b.value(); })
      {
        // slice on purpose
        return static_cast<NumBitsHolder>(*this);
      }
    };

    template <typename NumBitsHolder, typename Reader, typename Writer>
    MapTyped(NumBitsHolder &&, Reader &&, Writer &&) -> MapTyped<NumBitsHolder, Reader, Writer>;

    /**
     * Base mapper type for a simple value mapper of an elementary type.
     */
    template <typename T, typename NumBitsHolder, bool AssertByteAligned = false>
    struct MapTypedElementary {
      using type = T;

      [[no_unique_address]] NumBitsHolder numBits;

      auto operator()(BitReader &reader) const {
        if constexpr (AssertByteAligned) {
          reader.assertAlignment(1_bytes);
        }

        return MapElementary<T>::read(reader, numBits.value());
      }

      auto operator()(BitWriter &writer, const T &value) const {
        if constexpr (AssertByteAligned) {
          writer.assertAlignment(1_bytes);
        }
        return MapElementary<type>::write(writer, value, numBits.value());
      }

      constexpr auto fixedSize() const noexcept { return numBits; }
    };

    /**
     * Base mapper type for a simple mapper of a fixed value including value match checks.
     */
    template <typename FixedValueHolder, typename NumBitsHolder, bool Checked, bool AssertByteAligned,
              typename Ex = std::invalid_argument>
    struct MapFixedBits {
      using type = FixedValueHolder::type;

      [[no_unique_address]] FixedValueHolder fixedValue;
      [[no_unique_address]] NumBitsHolder numBits;

      auto operator()(BitReader &reader) const {
        if constexpr (AssertByteAligned) {
          reader.assertAlignment(1_bytes);
        }
        if constexpr (Checked) {
          auto actualValue = MapElementary<type>::read(reader, numBits.value());
          if (actualValue != fixedValue.value()) {
            std::stringstream ss;
            ss << "Invalid value in input, got " << printView(actualValue) << ", expected "
               << printView(fixedValue.value()) << "!";
            throw Ex{ss.str()};
          }
        } else {
          reader.skip(numBits.value());
        }
        return fixedValue.value();
      }

      auto operator()(BitWriter &writer, const type & /* ignored */) const {
        if constexpr (AssertByteAligned) {
          writer.assertAlignment(1_bytes);
        }
        return MapElementary<type>::write(writer, fixedValue.value(), numBits.value());
      }

      constexpr auto fixedSize() const noexcept { return numBits; }
    };

    template <typename T>
    struct ObjectTypeImpl;

    template <typename Object, typename Member>
    struct ObjectTypeImpl<Member Object::*> {
      using type = Object;
    };

    template <typename T>
    using ObjectType = typename ObjectTypeImpl<T>::type;

    template <typename T>
    struct MemberTypeImpl;

    template <typename Object, typename Member>
    struct MemberTypeImpl<Member Object::*> {
      using type = std::remove_cvref_t<Member>;
    };

    template <typename Object, typename Member>
    struct MemberTypeImpl<Member (Object::*)() const> {
      using type = std::remove_cvref_t<Member>;
    };

    template <typename T>
    using MemberType = typename MemberTypeImpl<T>::type;

    /**
     * Base mapper type for a member value mapper using a simple mapper to read/write the member value.
     */
    template <typename MemberHolder, typename MapperHolder>
    struct MapSimpleMember {
      using object_type = detail::ObjectType<typename MemberHolder::type>;
      using member_type = detail::MemberType<typename MemberHolder::type>;

      [[no_unique_address]] MemberHolder member;
      [[no_unique_address]] MapperHolder mapValue;

      auto operator()(BitReader &reader, object_type &object) const {
        return object.*(member.value()) = (mapValue.value())(reader);
      }
      auto operator()(BitWriter &writer, const object_type &object) const {
        return (mapValue.value())(writer, object.*(member.value()));
      }

      constexpr auto fixedSize() const noexcept
        requires(FixedSizeMapper<typename MapperHolder::type>)
      {
        return mapValue.value().fixedSize();
      }
    };

    /**
     * Base mapper type for a member value mapper using an elementary mapper to read/write the member value.
     */
    template <typename MemberHolder, typename NumBitsHolder>
    struct MapMemberBits {
      using object_type = detail::ObjectType<typename MemberHolder::type>;
      using member_type = detail::MemberType<typename MemberHolder::type>;

      [[no_unique_address]] MemberHolder member;
      [[no_unique_address]] NumBitsHolder numBits;

      auto operator()(BitReader &reader, object_type &object) const {
        return object.*(member.value()) = MapElementary<member_type>::read(reader, BitCount{numBits.value()});
      }

      auto operator()(BitWriter &writer, const object_type &object) const {
        return MapElementary<member_type>::write(writer, object.*(member.value()), BitCount{numBits.value()});
      }

      constexpr auto fixedSize() const noexcept { return numBits; }
    };

    /**
     * Base mapper type for a member value mapper using a simple mapper to read/write and the associated getter/setter
     * to access the member value.
     */
    template <typename GetterHolder, typename SetterHolder, typename MapperHolder>
    struct MapMemberProperty {
      using object_type = ObjectType<typename GetterHolder::type>;
      using member_type = MemberType<typename GetterHolder::type>;

      [[no_unique_address]] GetterHolder getter;
      [[no_unique_address]] SetterHolder setter;
      [[no_unique_address]] MapperHolder mapValue;

      auto operator()(BitReader &reader, object_type &object) const {
        return (object.*(setter.value()))((mapValue.value())(reader));
      }

      auto operator()(BitWriter &writer, const object_type &object) const {
        return (mapValue.value())(writer, (object.*(getter.value()))());
      }

      constexpr auto fixedSize() const noexcept
        requires(FixedSizeMapper<typename MapperHolder::type>)
      {
        return mapValue.value().fixedSize();
      }
    };

    /**
     * Base mapper type for a member value mapper using an elementary mapper to read/write and the associated
     * getter/setter to access the member value.
     */
    template <typename GetterHolder, typename SetterHolder, typename NumBitsHolder>
    struct MapMemberPropertyBits {
      using object_type = ObjectType<typename GetterHolder::type>;
      using member_type = MemberType<typename GetterHolder::type>;

      [[no_unique_address]] GetterHolder getter;
      [[no_unique_address]] SetterHolder setter;
      [[no_unique_address]] NumBitsHolder numBits;

      auto operator()(BitReader &reader, object_type &object) const {
        return (object.*(setter.value()))(MapElementary<member_type>::read(reader, BitCount{numBits.value()}));
      }

      auto operator()(BitWriter &writer, const object_type &object) const {
        return MapElementary<member_type>::write(writer, (object.*(getter.value()))(), BitCount{numBits.value()});
      }

      constexpr auto fixedSize() const noexcept { return numBits; }
    };

    /**
     * Base mapper type for a member container value mapper using a resizeable-container mapper to read/write the member
     * value.
     */
    template <typename MemberHolder, typename MapperHolder, typename SizeMemberHolder>
    struct MapMemberResizeableContainer {
      using object_type = detail::ObjectType<typename MemberHolder::type>;
      using member_type = detail::MemberType<typename MemberHolder::type>;

      [[no_unique_address]] MemberHolder member;
      [[no_unique_address]] MapperHolder mapElement;
      [[no_unique_address]] SizeMemberHolder sizeMember;

      auto operator()(BitReader &reader, object_type &object) const {
        return object.*(member.value()) = MapContainer<member_type, typename MapperHolder::type>::read(
                   reader, mapElement.value(), object.*(sizeMember.value()));
      }

      auto operator()(BitWriter &writer, const object_type &object) const {
        return MapContainer<member_type, typename MapperHolder::type>::write(
            writer, object.*(member.value()), mapElement.value(), object.*(sizeMember.value()));
      }
    };

    /**
     * Base mapper type for a member array value mapper using a simple mapper to read/write the member value.
     */
    template <typename MemberHolder, typename MapperHolder>
    struct MapMemberArray {
      using object_type = detail::ObjectType<typename MemberHolder::type>;
      using member_type = detail::MemberType<typename MemberHolder::type>;
      static constexpr std::size_t N = std::tuple_size_v<member_type>;

      [[no_unique_address]] MemberHolder member;
      [[no_unique_address]] MapperHolder mapElement;

      auto operator()(BitReader &reader, object_type &object) const {
        return object.*(member.value()) =
                   MapContainer<member_type, typename MapperHolder::type>::read(reader, mapElement.value(), N);
      }

      auto operator()(BitWriter &writer, const object_type &object) const {
        return MapContainer<member_type, typename MapperHolder::type>::write(writer, object.*(member.value()),
                                                                             mapElement.value(), N);
      }

      constexpr auto fixedSize() const noexcept
        requires(FixedSizeMapper<typename MapperHolder::type>)
      {
        return detail::Multiply<decltype(mapElement.value().fixedSize()), N>{mapElement.value().fixedSize()}();
      }
    };

    template <typename>
    struct TypeBits {};

    template <std::unsigned_integral T>
    struct TypeBits<T> : std::integral_constant<std::size_t, std::numeric_limits<T>::digits> {};

    // std::numeric_limits<>::digits for signed integers is 1 less, but we want the full bit-width
    template <std::signed_integral T>
    struct TypeBits<T> : TypeBits<std::make_unsigned_t<T>> {};

    template <Enum T>
    struct TypeBits<T> : TypeBits<std::underlying_type_t<T>> {};
  } // namespace detail

  /**
   * Elementary value mapper functions for unsigned integral types.
   */
  template <std::unsigned_integral T>
  struct MapElementary<T> {
    static T read(BitReader &reader, BitCount numBits) { return reader.read<T>(numBits); }
    static void write(BitWriter &writer, const T &value, BitCount numBits) { writer.write(value, numBits); }
    static T readExpGolomb(BitReader &reader) { return reader.readExpGolomb<T>(); }
    static void writeExpGolomb(BitWriter &writer, const T &value) { writer.writeExpGolomb(value); }
    static T readFibonacci(BitReader &reader) { return reader.readFibonacci<T>(); }
    static void writeFibonacci(BitWriter &writer, const T &value) { writer.writeFibonacci(value); }
  };

  /**
   * Elementary value mapper functions for signed integral types.
   */
  template <std::signed_integral T>
  struct MapElementary<T> {
    static T read(BitReader &reader, BitCount numBits) {
      return std::bit_cast<T>(reader.read<std::make_unsigned_t<T>>(numBits));
    }
    static void write(BitWriter &writer, const T &value, BitCount numBits) {
      writer.write(std::bit_cast<std::make_unsigned_t<T>>(value), numBits);
    }
    static T readExpGolomb(BitReader &reader) { return reader.readSignedExpGolomb<T>(); }
    static void writeExpGolomb(BitWriter &writer, const T &value) { writer.writeSignedExpGolomb(value); }
    static T readFibonacci(BitReader &reader) { return reader.readSignedFibonacci<T>(); }
    static void writeFibonacci(BitWriter &writer, const T &value) { writer.writeSignedFibonacci(value); }
  };

  /**
   * Elementary value mapper functions for enum types.
   */
  template <Enum T>
  struct MapElementary<T> {
    static T read(BitReader &reader, BitCount numBits) { return reader.read<T>(numBits); }
    static void write(BitWriter &writer, T value, BitCount numBits) {
      return MapElementary<std::underlying_type_t<T>>::write(writer, std::bit_cast<std::underlying_type_t<T>>(value),
                                                             numBits);
    }
  };

  /**
   * Container value mapper functions for resizeable container types.
   */
  template <DefaultConstructibleResizeableContainer T, typename M>
  struct MapContainer<T, M> {
    static T read(BitReader &reader, const M &readElement, std::size_t numElements) {
      T container{};
      container.resize(numElements);
      for (std::size_t i = 0; i < numElements; ++i) {
        container[i] = readElement(reader);
      }
      return container;
    }

    static void write(BitWriter &writer, const T &value, const M &writeElement, std::size_t numElements) {
      for (std::size_t i = 0; i < numElements; ++i) {
        writeElement(writer, value[i]);
      }
    }
  };

  /**
   * Container value mapper functions for array types.
   */
  template <typename T, std::size_t N, typename M>
  struct MapContainer<std::array<T, N>, M> {
    static std::array<T, N> read(BitReader &reader, const M &readElement, std::size_t numElements) {
      std::array<T, N> container{};
      for (std::size_t i = 0; i < numElements; ++i) {
        container[i] = readElement(reader);
      }
      return container;
    }

    static void write(BitWriter &writer, const std::array<T, N> &value, const M &writeElement,
                      std::size_t numElements) {
      for (std::size_t i = 0; i < numElements; ++i) {
        writeElement(writer, value[i]);
      }
    }
  };

  /**
   * Maps the given number of bits as value of the given elementary type.
   */
  template <typename T>
  constexpr auto mapBits(BitCount numBits) noexcept {
    return detail::MapTypedElementary<T, detail::RunTimeValueHolder<BitCount>>{numBits};
  }

  /**
   * Maps the given number of bits as value of the given elementary type.
   */
  template <typename T, BitCount NumBits>
  constexpr auto mapBits() noexcept {
    return detail::MapTypedElementary<T, detail::CompileTimeValueHolder<NumBits>>{};
  }

  /**
   * Maps the given elementary type.
   *
   * This mapper reads/writes the number of bits matching the elementary type size.
   */
  template <std::integral T>
  constexpr auto mapBits() noexcept {
    constexpr BitCount NUM_BITS{detail::TypeBits<T>::value};
    return mapBits<T, NUM_BITS>();
  }

  /**
   * Maps the given enumeration type.
   *
   * This mapper reads/writes the number of bits matching the enumeration type size.
   */
  template <Enum T>
  constexpr auto mapBits() noexcept {
    constexpr BitCount NUM_BITS{detail::TypeBits<T>::value};
    return mapBits<T, NUM_BITS>();
  }

  /**
   * Maps the given number of aligned bytes as value of the given elementary type.
   */
  template <typename T>
  constexpr auto mapBytes(ByteCount numBytes) noexcept {
    return detail::MapTypedElementary<T, detail::RunTimeValueHolder<ByteCount>, true>{numBytes};
  }

  /**
   * Maps the given number of aligned bytes as value of the given elementary type.
   */
  template <typename T, ByteCount NumBytes>
  constexpr auto mapBytes() noexcept {
    return detail::MapTypedElementary<T, detail::CompileTimeValueHolder<NumBytes>, true>{};
  }

  /**
   * Maps the given elementary type.
   *
   * This mapper reads/writes the number of aligned bytes matching the elementary type size.
   */
  template <std::integral T>
  constexpr auto mapBytes() noexcept {
    constexpr ByteCount NUM_BYTES{sizeof(T)};
    return mapBytes<T, NUM_BYTES>();
  }

  /**
   * Maps the given enumeration type.
   *
   * This mapper reads/writes the number of aligned bytes matching the enumeration type size.
   */
  template <Enum T>
  constexpr auto mapBytes() noexcept {
    constexpr ByteCount NUM_BYTES{sizeof(std::underlying_type_t<T>)};
    return mapBytes<T, NUM_BYTES>();
  }

  /**
   * Maps the given type as Exponentional-Golomb encoded value.
   */
  template <typename T>
  constexpr auto mapExpGolombBits() noexcept {
    return detail::MapTyped{detail::DynamicValueHolder{},
                            [](BitReader &reader) { return MapElementary<T>::readExpGolomb(reader); },
                            [](BitWriter &writer, const T &value) { MapElementary<T>::writeExpGolomb(writer, value); }};
  }

  /**
   * Maps the given type as UTF-8 encoded value.
   *
   * This mapper requires the input/output byte stream to be byte aligned.
   */
  template <typename T = char32_t>
  constexpr auto mapUtf8Bytes() noexcept {
    return detail::MapTyped{
        detail::DynamicValueHolder{}, [](BitReader &reader) { return static_cast<T>(reader.readUtf8CodePoint()); },
        [](BitWriter &writer, const T &value) { writer.writeUtf8CodePoint(static_cast<uint32_t>(value)); }};
  }

  /**
   * Maps the given type as (Nega-)Fibonacci encoded value.
   */
  template <typename T>
  constexpr auto mapFibonacciBits() noexcept {
    return detail::MapTyped{detail::DynamicValueHolder{},
                            [](BitReader &reader) { return MapElementary<T>::readFibonacci(reader); },
                            [](BitWriter &writer, const T &value) { MapElementary<T>::writeFibonacci(writer, value); }};
  }

  /**
   * Maps the given number of bits as fixed value of the given type without error-checking.
   */
  template <typename T>
  constexpr auto mapUncheckedFixedBits(T fixedValue, BitCount numBits) {
    return detail::MapFixedBits<detail::RunTimeValueHolder<T>, detail::RunTimeValueHolder<BitCount>, false, false>{
        {fixedValue}, {numBits}};
  }

  /**
   * Maps the given number of bits as fixed value of the given type without error-checking.
   */
  template <auto FixedValue, BitCount NumBits>
  constexpr auto mapUncheckedFixedBits() noexcept {
    return detail::MapFixedBits<detail::CompileTimeValueHolder<FixedValue>, detail::CompileTimeValueHolder<NumBits>,
                                false, false>{};
  }

  /**
   * Maps the given number of bits as fixed value of the given type with error-checking.
   */
  template <typename T, typename Ex>
  constexpr auto mapCheckedFixedBits(T fixedValue, BitCount numBits) {
    return detail::MapFixedBits<detail::RunTimeValueHolder<T>, detail::RunTimeValueHolder<BitCount>, true, false, Ex>{
        {fixedValue}, {numBits}};
  }

  /**
   * Maps the given number of bits as fixed value of the given type with error-checking.
   */
  template <auto FixedValue, BitCount NumBits, typename Ex>
  constexpr auto mapCheckedFixedBits() noexcept {
    return detail::MapFixedBits<detail::CompileTimeValueHolder<FixedValue>, detail::CompileTimeValueHolder<NumBits>,
                                true, false, Ex>{};
  }

  /**
   * Maps the given number of aligned bytes as fixed value of the given type without error-checking.
   */
  template <typename T>
  constexpr auto mapUncheckedFixedBytes(T fixedValue, ByteCount numBytes) {
    return detail::MapFixedBits<detail::RunTimeValueHolder<T>, detail::RunTimeValueHolder<BitCount>, false, true>{
        {fixedValue}, {numBytes}};
  }

  /**
   * Maps the given number of aligned bytes as fixed value of the given type without error-checking.
   */
  template <auto FixedValue, ByteCount NumBytes>
  constexpr auto mapUncheckedFixedBytes() noexcept {
    return detail::MapFixedBits<detail::CompileTimeValueHolder<FixedValue>, detail::CompileTimeValueHolder<NumBytes>,
                                false, true>{};
  }

  /**
   * Maps the given number of aligned bytes as fixed value of the given type with error-checking.
   */
  template <typename T, typename Ex>
  constexpr auto mapCheckedFixedBytes(T fixedValue, ByteCount numBytes) {
    return detail::MapFixedBits<detail::RunTimeValueHolder<T>, detail::RunTimeValueHolder<BitCount>, true, true, Ex>{
        {fixedValue}, {numBytes}};
  }

  /**
   * Maps the given number of aligned bytes as fixed value of the given type with error-checking.
   */
  template <auto FixedValue, ByteCount NumBytes, typename Ex>
  constexpr auto mapCheckedFixedBytes() noexcept {
    return detail::MapFixedBits<detail::CompileTimeValueHolder<FixedValue>, detail::CompileTimeValueHolder<NumBytes>,
                                true, true, Ex>{};
  }

  /**
   * Maps the given compound type by applying all given mappers in-order.
   */
  template <typename T, MemberMapper... Mappers>
  constexpr auto mapCompound(Mappers... memberMappers) noexcept {
    return detail::MapTyped{(detail::getFixedSize(memberMappers) + ...),
                            [... mappers = std::forward<Mappers>(memberMappers)](BitReader &reader) {
                              T obj{};
                              (..., mappers(reader, obj));
                              return obj;
                            },
                            [... mappers = std::forward<Mappers>(memberMappers)](BitWriter &writer, const T &obj) {
                              (..., mappers(writer, obj));
                            }};
  }

  /**
   * Maps the given member by applying the given simple value mapper and assigning the member object.
   */
  template <typename O, typename T, DirectMapper M>
  constexpr auto mapMember(T O::*member, M mapValue) noexcept {
    return detail::MapSimpleMember<detail::RunTimeValueHolder<T O::*>, detail::RunTimeValueHolder<M>>{{member},
                                                                                                      {mapValue}};
  }

  /**
   * Maps the given member by applying the given simple value mapper and assigning the member object.
   */
  template <auto Member, auto MapValue>
  constexpr auto mapMember() noexcept {
    return detail::MapSimpleMember<detail::CompileTimeValueHolder<Member>, detail::CompileTimeValueHolder<MapValue>>{};
  }

  /**
   * Maps the given member by applying the matching elementary value mapper and assigning the member object.
   */
  template <BitCount NumBits, typename O, typename T>
  constexpr auto mapMemberBits(T O::*member) noexcept {
    static_assert(BitCount{NumBits} <= ByteCount{sizeof(T)});
    if constexpr (std::is_integral_v<T>) {
      static_assert(NumBits <= BitCount{detail::TypeBits<T>::value});
    }
    return detail::MapMemberBits<detail::RunTimeValueHolder<T O::*>, detail::CompileTimeValueHolder<NumBits>>{{member},
                                                                                                              {}};
  }

  /**
   * Maps the given member by applying the matching elementary value mapper for the given number of bits and assigning
   * the member object.
   */
  template <auto Member, BitCount NumBits>
  constexpr auto mapMemberBits() noexcept {
    using T = detail::MemberType<decltype(Member)>;
    static_assert(BitCount{NumBits} <= ByteCount{sizeof(T)});
    if constexpr (std::is_integral_v<T>) {
      static_assert(NumBits <= BitCount{detail::TypeBits<T>::value});
    }
    return detail::MapMemberBits<detail::CompileTimeValueHolder<Member>, detail::CompileTimeValueHolder<NumBits>>{};
  }

  /**
   * Maps the given integral member by applying the matching elementary value mapper and assigning the member object.
   */
  template <typename O, std::integral T>
  constexpr auto mapMemberBits(T O::*member) noexcept {
    constexpr BitCount NUM_BITS{detail::TypeBits<T>::value};
    return mapMemberBits<NUM_BITS>(member);
  }

  /**
   * Maps the given enumeration member by applying the matching elementary value mapper and assigning the member object.
   */
  template <auto Member>
  constexpr auto mapMemberBits() noexcept {
    static_assert(std::integral<detail::MemberType<decltype(Member)>> ||
                  std::is_enum_v<detail::MemberType<decltype(Member)>>);
    constexpr BitCount NUM_BITS{detail::TypeBits<detail::MemberType<decltype(Member)>>::value};
    return mapMemberBits<Member, NUM_BITS>();
  }

  /**
   * Maps the given member by applying the given simple value mapper and getter/setter functions.
   */
  template <typename G, typename S, DirectMapper M>
  constexpr auto mapMemberProperty(G memberGetter, S memberSetter, M mapValue) noexcept {
    return detail::MapMemberProperty{detail::RunTimeValueHolder{memberGetter}, detail::RunTimeValueHolder{memberSetter},
                                     detail::RunTimeValueHolder{mapValue}};
  }

  /**
   * Maps the given member by applying the given simple value mapper and getter/setter functions.
   */
  template <auto MemberGetter, auto MemberSetter, auto MapValue>
  constexpr auto mapMemberProperty() noexcept
    requires(!std::is_same_v<decltype(MapValue), BitCount>)
  {
    return detail::MapMemberProperty{detail::CompileTimeValueHolder<MemberGetter>{},
                                     detail::CompileTimeValueHolder<MemberSetter>{},
                                     detail::CompileTimeValueHolder<MapValue>{}};
  }

  /**
   * Maps the given member by applying the elementary value mapper for the given number of bits and using the given
   * getter/setter functions.
   */
  template <BitCount NumBits, typename G, typename S>
  constexpr auto mapMemberProperty(G memberGetter, S memberSetter) noexcept {
    using Type = detail::MemberType<G>;
    static_assert(BitCount{NumBits} <= ByteCount{sizeof(Type)});
    if constexpr (std::is_integral_v<Type>) {
      static_assert(NumBits <= BitCount{detail::TypeBits<Type>::value});
    }
    return detail::MapMemberPropertyBits<detail::RunTimeValueHolder<G>, detail::RunTimeValueHolder<S>,
                                         detail::CompileTimeValueHolder<NumBits>>{memberGetter, memberSetter, {}};
  }

  /**
   * Maps the given member by applying the elementary value mapper for the given number of bits and using the given
   * getter/setter functions.
   */
  template <auto MemberGetter, auto MemberSetter, BitCount NumBits>
  constexpr auto mapMemberProperty() noexcept {
    using Type = detail::MemberType<decltype(MemberGetter)>;
    static_assert(BitCount{NumBits} <= ByteCount{sizeof(Type)});
    if constexpr (std::is_integral_v<Type>) {
      static_assert(NumBits <= BitCount{detail::TypeBits<Type>::value});
    }
    return detail::MapMemberPropertyBits<detail::CompileTimeValueHolder<MemberGetter>,
                                         detail::CompileTimeValueHolder<MemberSetter>,
                                         detail::CompileTimeValueHolder<NumBits>>{};
  }

  /**
   * Maps the given integral member by applying the elementary value mapper and using the given getter/setter functions.
   */
  template <typename G, typename S>
  constexpr auto mapMemberProperty(G memberGetter, S memberSetter) noexcept
    requires(std::integral<detail::MemberType<G>>)
  {
    constexpr BitCount NUM_BITS{detail::TypeBits<detail::MemberType<G>>::value};
    return mapMemberProperty<NUM_BITS>(memberGetter, memberSetter);
  }

  /**
   * Maps the given member by applying the deduced value mapper and using the given getter/setter functions.
   */
  template <auto MemberGetter, auto MemberSetter>
  constexpr auto mapMemberProperty() noexcept {
    constexpr BitCount NUM_BITS{detail::TypeBits<detail::MemberType<decltype(MemberGetter)>>::value};
    return mapMemberProperty<MemberGetter, MemberSetter, NUM_BITS>();
  }

  /**
   * Maps the given default-constructible and resizeable member container by using the given simple size and element
   * mappers.
   */
  template <typename O, DefaultConstructibleResizeableContainer T, DirectMapper M, typename S>
  constexpr auto mapMemberContainer(T O::*member, M mapElement, S O::*sizeMember) noexcept {
    return detail::MapMemberResizeableContainer{detail::RunTimeValueHolder<T O::*>{member},
                                                detail::RunTimeValueHolder<M>{mapElement},
                                                detail::RunTimeValueHolder<S O::*>{sizeMember}};
  }

  /**
   * Maps the given member container by using the given simple size and element mappers.
   */
  template <auto Member, auto MapValue, auto SizeMember>
  constexpr auto mapMemberContainer() noexcept {
    return detail::MapMemberResizeableContainer<detail::CompileTimeValueHolder<Member>,
                                                detail::CompileTimeValueHolder<MapValue>,
                                                detail::CompileTimeValueHolder<SizeMember>>{};
  }

  /**
   * Maps the given array member container by using the given simple element mapper.
   */
  template <typename O, typename T, DirectMapper M, std::size_t N>
  constexpr auto mapMemberArray(std::array<T, N> O::*member, M mapElement) noexcept {
    return detail::MapMemberArray{detail::RunTimeValueHolder<std::array<T, N> O::*>{member},
                                  detail::RunTimeValueHolder<M>{mapElement}};
  }

  /**
   * Maps the given array member container by using the given simple element mapper.
   */
  template <auto Member, auto MapValue>
  constexpr auto mapMemberArray() noexcept {
    return detail::MapMemberArray<detail::CompileTimeValueHolder<Member>, detail::CompileTimeValueHolder<MapValue>>{};
  }

  /**
   * "Mapper" which does not read or write anything, just asserts that the current read/write position is byte-aligned.
   */
  constexpr auto assertByteAligned() noexcept {
    return detail::MapSimple{[](BitReader &reader) { reader.assertAlignment(1_bytes); },
                             [](BitWriter &writer) { writer.assertAlignment(1_bytes); },
                             detail::CompileTimeValueHolder<0_bits>{}};
  }

} // namespace bml
