#pragma once

#include "destructure.hpp"
#include "helper.hpp"
#include "reader.hpp"
#include "sizes.hpp"
#include "writer.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace bml::detail {
  template <typename Type>
  concept HasReadMember = requires(Type &obj, BitReader &reader) { obj.read(reader); };

  template <typename Type>
  concept HasWriteMember = requires(const Type &obj, BitWriter &writer) { obj.write(writer); };

  template <typename Type>
  concept HasSkipMember = requires(BitReader &reader) { Type::skip(reader); };

  template <typename Type>
  concept HasCopyMember = requires(BitReader &reader, BitWriter &writer) { Type::copy(reader, writer); };

  template <BitCount B>
  void checkConstexpr();

  template <typename Type>
  concept HasMinMaxSizeMembers = requires() {
    { Type::minNumBits() } -> std::convertible_to<BitCount>;
    { Type::maxNumBits() } -> std::convertible_to<BitCount>;
    // Check whether the values returned by the functions can be used in a context that requires constant expression
    checkConstexpr<Type::minNumBits()>;
    checkConstexpr<Type::maxNumBits()>;
  };

  template <typename Type>
  concept HasSizeMember = requires(const Type &obj) {
    { obj.numBits() } -> std::convertible_to<BitCount>;
  };

  template <typename Type>
  concept HasFixedSizeMembers = HasMinMaxSizeMembers<Type> && Type::minNumBits() == Type::maxNumBits();

  template <typename Type>
  concept IsEnum = std::is_enum_v<Type>;

  template <typename Type>
  struct ArrayHelper : std::false_type {};

  template <typename Type, std::size_t N>
  struct ArrayHelper<std::array<Type, N>> : std::true_type {
    using value_type = Type;
  };

  template <typename Type>
  concept IsArray = ArrayHelper<Type>::value;

  template <typename Type>
  struct TupleHelper : std::false_type {};

  template <typename... Types>
  struct TupleHelper<std::tuple<Types...>> : std::true_type {
    static void skip(BitReader &reader);
    static void copy(BitReader &reader, BitWriter &writer);
  };

  template <typename Type>
  concept IsTuple = TupleHelper<Type>::value;

  template <typename Type>
  struct AllowedDestructurableHelper {
    // These should be handled via simpler implementations
    static constexpr bool value = !std::is_integral_v<Type> && !std::is_enum_v<Type>;
  };

  // while these are technically "destructurable", using their members would behave wrongly!
  template <typename Type>
  struct AllowedDestructurableHelper<std::unique_ptr<Type>> : std::false_type {};

  template <typename Type>
  struct AllowedDestructurableHelper<std::optional<Type>> : std::false_type {};

  template <typename Type>
  struct AllowedDestructurableHelper<std::vector<Type>> : std::false_type {};

  template <typename Type, std::size_t N>
  struct AllowedDestructurableHelper<std::array<Type, N>> : std::false_type {};

  template <typename... Types>
  struct AllowedDestructurableHelper<std::variant<Types...>> : std::false_type {};

  template <typename Type>
  concept AllowedDestructurable = AllowedDestructurableHelper<Type>::value && Destructurable<Type>;

  template <typename Type>
  struct SizeHelper;

  template <HasMinMaxSizeMembers Type>
  struct SizeHelper<Type> {
    static constexpr BitCount calcMinNumBits() noexcept { return Type::minNumBits(); }
    static constexpr BitCount calcMaxNumBits() noexcept { return Type::maxNumBits(); }
    static constexpr bool isFixedSize() noexcept { return Type::minNumBits() == Type::maxNumBits(); }
  };

  template <>
  struct SizeHelper<std::monostate> {
    static constexpr BitCount calcMinNumBits() noexcept { return 0_bits; }
    static constexpr BitCount calcMaxNumBits() noexcept { return 0_bits; }
    static constexpr bool isFixedSize() noexcept { return true; }
  };

  template <>
  struct SizeHelper<bool> {
    static constexpr BitCount calcMinNumBits() noexcept { return 1_bits; }
    static constexpr BitCount calcMaxNumBits() noexcept { return 1_bits; }
    static constexpr bool isFixedSize() noexcept { return true; }
  };

  template <>
  struct SizeHelper<std::byte> {
    static constexpr BitCount calcMinNumBits() noexcept { return 1_bytes; }
    static constexpr BitCount calcMaxNumBits() noexcept { return 1_bytes; }
    static constexpr bool isFixedSize() noexcept { return true; }
  };

  template <typename Type>
  struct SizeHelper<std::optional<Type>> {
    static constexpr BitCount calcMinNumBits() noexcept { return 0_bits; }
    static constexpr BitCount calcMaxNumBits() noexcept { return SizeHelper<Type>::calcMaxNumBits(); }
    static constexpr bool isFixedSize() noexcept { return false; }
  };

  template <typename Type>
  struct SizeHelper<std::unique_ptr<Type>> {
    static constexpr BitCount calcMinNumBits() noexcept { return 0_bits; }
    static constexpr BitCount calcMaxNumBits() noexcept { return SizeHelper<Type>::calcMaxNumBits(); }
    static constexpr bool isFixedSize() noexcept { return false; }
  };

  template <typename Type>
  struct SizeHelper<std::vector<Type>> {
    static constexpr BitCount calcMinNumBits() noexcept { return 0_bits; }
    static constexpr BitCount calcMaxNumBits() noexcept {
      return BitCount{std::numeric_limits<std::size_t>::max() /
                      32 /* be big enough, but avoid overflow on addition with other sizes */};
    }
    static constexpr bool isFixedSize() noexcept { return false; }
  };

  template <typename Type, std::size_t N>
  struct SizeHelper<std::array<Type, N>> {
    static constexpr BitCount calcMinNumBits() noexcept { return SizeHelper<Type>::calcMinNumBits() * N; }
    static constexpr BitCount calcMaxNumBits() noexcept { return SizeHelper<Type>::calcMaxNumBits() * N; }
    static constexpr bool isFixedSize() noexcept { return SizeHelper<Type>::isFixedSize(); }
  };

  template <typename... Types>
  struct SizeHelper<std::tuple<Types...>> {
    static constexpr BitCount calcMinNumBits() noexcept { return (SizeHelper<Types>::calcMinNumBits() + ...); }
    static constexpr BitCount calcMaxNumBits() noexcept { return (SizeHelper<Types>::calcMaxNumBits() + ...); }
    static constexpr bool isFixedSize() noexcept { return (SizeHelper<Types>::isFixedSize() && ...); }
  };

  template <typename... Types>
  struct SizeHelper<std::variant<Types...>> {
    static constexpr BitCount calcMinNumBits() noexcept { return std::min({SizeHelper<Types>::calcMinNumBits()...}); }
    static constexpr BitCount calcMaxNumBits() noexcept { return std::max({SizeHelper<Types>::calcMaxNumBits()...}); }
    static constexpr bool isFixedSize() noexcept {
      if constexpr ((SizeHelper<Types>::isFixedSize() && ...)) {
        return calcMinNumBits() == calcMaxNumBits();
      }
      return false;
    }
  };

  template <std::integral Type>
  struct SizeHelper<Type> {
    static constexpr BitCount calcMinNumBits() noexcept { return bml::BitCount{bml::bits<Type>()}; }
    static constexpr BitCount calcMaxNumBits() noexcept { return bml::BitCount{bml::bits<Type>()}; }
    static constexpr bool isFixedSize() noexcept { return true; }
  };

  template <std::floating_point Type>
  struct SizeHelper<Type> {
    static constexpr BitCount calcMinNumBits() noexcept {
      return bml::BitCount{bml::bits<best_type<sizeof(Type) * 8>>()};
    }
    static constexpr BitCount calcMaxNumBits() noexcept {
      return bml::BitCount{bml::bits<best_type<sizeof(Type) * 8>>()};
    }
    static constexpr bool isFixedSize() noexcept { return true; }
  };

  template <typename Type>
  struct DestructureSizeHelper {
    static constexpr BitCount calcMinNumBits() noexcept = delete;
    static constexpr BitCount calcMaxNumBits() noexcept = delete;
    static constexpr bool isFixedSize() noexcept { return false; }
  };

  template <AllowedDestructurable Type>
  struct DestructureSizeHelper<Type> {
    static constexpr BitCount calcMinNumBits() noexcept {
      return SizeHelper<decltype(detail::destructure(std::declval<Type>()))>::calcMinNumBits();
    }
    static constexpr BitCount calcMaxNumBits() noexcept {
      return SizeHelper<decltype(detail::destructure(std::declval<Type>()))>::calcMaxNumBits();
    }

    static constexpr bool isFixedSize() noexcept { return calcMinNumBits() == calcMaxNumBits(); }
  };

  template <typename Type>
  struct SizeHelper : DestructureSizeHelper<Type> {};

  template <typename Type>
  struct SizeHelper<Type &> : SizeHelper<Type> {};

  template <IsEnum Type>
  struct SizeHelper<Type> : SizeHelper<std::underlying_type_t<Type>> {};

  template <typename Type>
  void readMembers(BitReader &reader, Type &object);

  template <typename Type>
  void writeMembers(BitWriter &writer, const Type &object);

  template <typename Type>
  constexpr BitCount numBitsMembers(const Type &object) noexcept;

  template <typename... Types>
  void copyMembers(BitReader &reader, BitWriter &writer);

  template <typename... Types>
  void skipMembers(BitReader &reader);
} // namespace bml::detail
