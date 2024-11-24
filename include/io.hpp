#pragma once

#include "destructure.hpp"
#include "helper.hpp"
#include "reader.hpp"
#include "sizes.hpp"
#include "writer.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <memory>
#include <numeric>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>

namespace bml {

  namespace detail {
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
    concept HasSizeMembers = HasMinMaxSizeMembers<Type> && requires(const Type &obj) {
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
  } // namespace detail

  ////
  // READ
  ////

  template <typename Type>
  void read(BitReader &reader, Type &value)
    requires(detail::HasReadMember<Type>)
  {
    value.read(reader);
  }

  template <typename Type>
  void read(BitReader &reader, Type &value)
    requires(std::is_same_v<Type, bool>)
  {
    value = reader.read();
  }

  template <typename Type>
  void read(BitReader &reader, Type &value)
    requires(std::is_unsigned_v<Type> && !std::is_same_v<Type, bool>)
  {
    value = reader.read<Type>(sizeof(Type) * 1_bytes);
  }

  template <typename Type>
  void read(BitReader &reader, Type &value)
    requires(std::is_signed_v<Type>)
  {
    value = std::bit_cast<Type>(reader.read<std::make_unsigned_t<Type>>(sizeof(Type) * 1_bytes));
  }

  template <typename Type>
  void read(BitReader &reader, Type &value)
    requires(std::is_enum_v<Type>)
  {
    value = static_cast<Type>(reader.read<std::underlying_type_t<Type>>(sizeof(Type) * 1_bytes));
  }

  template <typename Type>
  void read(BitReader &reader, Type &object)
    requires(!detail::HasReadMember<Type> && detail::AllowedDestructurable<Type>)
  {
    detail::readMembers(reader, object);
  }

  template <typename Type, std::size_t N>
  void read(BitReader &reader, std::array<Type, N> &value)
    requires requires(Type obj, BitReader reader) { bml::read(reader, obj); }
  {
    for (auto &elem : value) {
      read(reader, elem);
    }
  }

  template <typename... Types>
  void read(BitReader &reader, std::tuple<Types...> &value)
    requires(requires(Types obj, BitReader reader) { bml::read(reader, obj); } && ...)
  {
    std::apply([&reader](auto &...args) { (read(reader, args), ...); }, value);
  }

  template <typename Type>
  std::optional<Type> readOptional(BitReader &reader, bool condition) {
    if (condition) {
      Type tmp{};
      read(reader, tmp);
      return tmp;
    }
    return std::optional<Type>{};
  }

  template <typename T>
  T read(BitReader &reader) {
    T tmp{};
    read(reader, tmp);
    return tmp;
  }

  ////
  // WRITE
  ////

  template <typename Type>
  void write(BitWriter &writer, const Type &value)
    requires(detail::HasWriteMember<Type>)
  {
    value.write(writer);
  }

  template <typename Type>
  void write(BitWriter &writer, const Type &value)
    requires(std::is_same_v<Type, bool>)
  {
    writer.write(value);
  }

  template <typename Type>
  void write(BitWriter &writer, const Type &value)
    requires(std::is_unsigned_v<Type> && !std::is_same_v<Type, bool>)
  {
    writer.write(value, sizeof(Type) * 1_bytes);
  }

  template <typename Type>
  void write(BitWriter &writer, const Type &value)
    requires(std::is_signed_v<Type>)
  {
    writer.write(std::bit_cast<std::make_unsigned_t<Type>>(value), sizeof(Type) * 1_bytes);
  }

  template <typename Type>
  void write(BitWriter &writer, const Type &value)
    requires(std::is_enum_v<Type>)
  {
    writer.write(static_cast<std::underlying_type_t<Type>>(value), sizeof(Type) * 1_bytes);
  }

  inline void write(BitWriter &writer, const std::byte &value) {
    writer.write(static_cast<uint8_t>(value), ByteCount{1});
  }

  template <typename Type>
  void write(BitWriter &writer, const Type &object)
    requires(!detail::HasWriteMember<Type> && detail::AllowedDestructurable<Type>)
  {
    detail::writeMembers(writer, object);
  }

  template <typename Type>
  void write(BitWriter &writer, const std::unique_ptr<Type> &value)
    requires requires(const Type &obj, BitWriter writer) { bml::write(writer, obj); }
  {
    if (value) {
      bml::write(writer, *value);
    }
  }

  template <typename Type>
  void write(BitWriter &writer, const std::vector<Type> &value)
    requires requires(const Type &obj, BitWriter writer) { bml::write(writer, obj); }
  {
    for (const auto &element : value) {
      bml::write(writer, element);
    }
  }

  template <typename Type, std::size_t N>
  void write(BitWriter &writer, const std::array<Type, N> &value)
    requires requires(const Type &obj, BitWriter writer) { bml::write(writer, obj); }
  {
    for (const auto &element : value) {
      bml::write(writer, element);
    }
  }

  template <typename... Types>
  void write(BitWriter &writer, const std::tuple<Types...> &value)
    requires(requires(const Types &obj, BitWriter writer) { bml::write(writer, obj); } && ...)
  {
    std::apply([&writer](auto &...args) { (bml::write(writer, args), ...); }, value);
  }

  template <typename... Types>
  void write(BitWriter &writer, const std::variant<Types...> &value)
    requires(requires(const Types &obj, BitWriter writer) { bml::write(writer, obj); } && ...)
  {
    return std::visit([&writer](const auto &entry) { return bml::write(writer, entry); }, value);
  }

  template <typename Type>
  void write(BitWriter &writer, const std::optional<Type> &value)
    requires requires(const Type &obj, BitWriter writer) { bml::write(writer, obj); }
  {
    if (value) {
      bml::write(writer, *value);
    }
  }

  ////
  // SIZE
  ////

  template <typename Type>
  constexpr BitCount minNumBits() noexcept
    requires requires(const Type &obj) {
      { detail::SizeHelper<Type>::calcMinNumBits() } -> std::convertible_to<BitCount>;
    }
  {
    return detail::SizeHelper<Type>::calcMinNumBits();
  }

  template <typename Type>
  constexpr BitCount maxNumBits() noexcept
    requires requires(const Type &obj) {
      { detail::SizeHelper<Type>::calcMaxNumBits() } -> std::convertible_to<BitCount>;
    }
  {
    return detail::SizeHelper<Type>::calcMaxNumBits();
  }

  template <typename Type>
  constexpr BitCount numBits(const Type &obj) noexcept
    requires(detail::HasSizeMembers<Type>)
  {
    return obj.numBits();
  }

  template <typename Type>
  constexpr BitCount numBits(const Type & /*obj */) noexcept
    requires(!detail::HasSizeMembers<Type> && detail::SizeHelper<Type>::isFixedSize())
  {
    return bml::minNumBits<Type>();
  }

  template <typename Type>
  constexpr BitCount numBits(const Type &object) noexcept
    requires(!detail::HasSizeMembers<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             detail::AllowedDestructurable<Type>)
  {
    return detail::numBitsMembers(object);
  }

  template <typename Type>
  constexpr BitCount numBits(const std::unique_ptr<Type> &object) noexcept
    requires requires(const Type &obj) {
      { bml::numBits(obj) } -> std::convertible_to<BitCount>;
    }
  {
    return object ? bml::numBits(*object) : 0_bits;
  }

  template <typename Type>
  constexpr BitCount numBits(const std::vector<Type> &object) noexcept
    requires requires(const Type &obj) {
      { bml::numBits(obj) } -> std::convertible_to<BitCount>;
    }
  {
    return std::accumulate(object.begin(), object.end(), BitCount{},
                           [](BitCount sum, const Type &elem) { return sum + bml::numBits(elem); });
  }

  template <typename Type, std::size_t N>
  constexpr BitCount numBits(const std::array<Type, N> &object) noexcept
    requires requires(const Type &obj) {
      { bml::numBits(obj) } -> std::convertible_to<BitCount>;
    }
  {
    return std::accumulate(object.begin(), object.end(), BitCount{},
                           [](BitCount sum, const Type &elem) { return sum + bml::numBits(elem); });
  }

  template <typename... Types>
  constexpr BitCount numBits(const std::tuple<Types...> &object) noexcept
    requires(requires(const Types &obj) {
      { bml::numBits(obj) } -> std::convertible_to<BitCount>;
    } && ...)
  {
    return std::apply([](const auto &...elem) { return (bml::numBits(elem) + ...); }, object);
  }

  template <typename... Types>
  constexpr BitCount numBits(const std::variant<Types...> &object) noexcept
    requires(requires(const Types &obj) {
      { bml::numBits(obj) } -> std::convertible_to<BitCount>;
    } && ...)
  {
    return std::visit([](const auto &entry) { return bml::numBits(entry); }, object);
  }

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

  template <typename Type>
  void copy(BitReader &reader, BitWriter &writer)
    requires(detail::HasCopyMember<Type>)
  {
    Type::copy(reader, writer);
  }

  void copyBits(BitReader &reader, BitWriter &writer, BitCount numBits);

  template <typename Type>
  void copy(BitReader &reader, BitWriter &writer)
    requires(!detail::HasCopyMember<Type> && detail::SizeHelper<Type>::isFixedSize())
  {
    copyBits(reader, writer, minNumBits<Type>());
  }

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

  template <typename Type>
  void copy(BitReader &reader, BitWriter &writer)
    requires(!detail::HasCopyMember<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             !(detail::HasReadMember<Type> && detail::HasWriteMember<Type>) && detail::AllowedDestructurable<Type>)
  {
    detail::copyMembers<decltype(detail::destructureReference(std::declval<Type>()))>(reader, writer);
  }

  template <typename Type>
  void copy(BitReader &reader, BitWriter &writer)
    requires(detail::IsTuple<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             requires(BitReader &reader, BitWriter &writer) { detail::TupleHelper<Type>::copy(reader, writer); })
  {
    detail::TupleHelper<Type>::copy(reader, writer);
  }

  template <typename Type>
  void copy(BitReader &reader, BitWriter &writer)
    requires(detail::IsArray<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             requires(BitReader &reader, BitWriter &writer) {
               bml::copy<typename detail::ArrayHelper<Type>::value_type>(reader, writer);
             })
  {
    for (std::size_t i = 0; i < std::tuple_size_v<Type>; ++i) {
      copy<typename detail::ArrayHelper<Type>::value_type>(reader, writer);
    }
  }

  ////
  // SKIP
  ////

  template <typename Type>
  void skip(BitReader &reader)
    requires(detail::HasSkipMember<Type>)
  {
    Type::skip(reader);
  }

  template <typename Type>
  void skip(BitReader &reader)
    requires(!detail::HasSkipMember<Type> && detail::SizeHelper<Type>::isFixedSize())
  {
    reader.skip(minNumBits<Type>());
  }

  template <typename Type>
  void skip(BitReader &reader)
    requires(!detail::HasSkipMember<Type> && !detail::SizeHelper<Type>::isFixedSize() && detail::HasReadMember<Type>)
  {
    // prefer custom member read over member-wise skip
    Type tmp{};
    tmp.read(reader);
  }

  template <typename Type>
  void skip(BitReader &reader)
    requires(!detail::HasSkipMember<Type> && !detail::SizeHelper<Type>::isFixedSize() && !detail::HasReadMember<Type> &&
             detail::AllowedDestructurable<Type>)
  {
    detail::skipMembers<decltype(detail::destructure(std::declval<Type>()))>(reader);
  }

  template <typename Type>
  void skip(BitReader &reader)
    requires(detail::IsTuple<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             requires(BitReader &reader) { detail::TupleHelper<Type>::skip(reader); })
  {
    detail::TupleHelper<Type>::skip(reader);
  }

  template <typename Type>
  void skip(BitReader &reader)
    requires(detail::IsArray<Type> && !detail::SizeHelper<Type>::isFixedSize() &&
             requires(BitReader &reader) { bml::skip<typename detail::ArrayHelper<Type>::value_type>(reader); })
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
