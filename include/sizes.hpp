#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

namespace bml {

  namespace detail {

    template <std::size_t Size>
    struct SizeType {
      constexpr SizeType() noexcept = default;
      constexpr explicit SizeType(std::size_t val) noexcept : num(val) {}

      constexpr std::size_t value() const noexcept { return num; };
      constexpr explicit operator std::size_t() const noexcept { return num; }
      constexpr explicit operator bool() const noexcept { return num; }

      template <std::size_t OtherSize>
      constexpr operator SizeType<OtherSize>() const noexcept
        requires(OtherSize < Size && (Size % OtherSize) == 0)
      {
        return SizeType<OtherSize>{num * (Size / OtherSize)};
      }

      constexpr friend SizeType operator+(const SizeType &one, const SizeType &other) noexcept {
        return SizeType{one.num + other.num};
      }
      constexpr friend SizeType operator-(const SizeType &one, const SizeType &other) noexcept {
        return SizeType{one.num - other.num};
      }
      constexpr friend std::size_t operator/(const SizeType &one, const SizeType &other) noexcept {
        return one.num / other.num;
      }
      constexpr friend SizeType operator%(const SizeType &one, const SizeType &other) noexcept {
        return SizeType{one.num % other.num};
      }

      constexpr friend SizeType operator*(const SizeType &one, std::size_t other) noexcept {
        return SizeType{one.num * other};
      }
      constexpr friend SizeType operator*(std::size_t one, const SizeType &other) noexcept {
        return SizeType{one * other.num};
      }
      constexpr friend SizeType operator/(const SizeType &one, std::size_t other) noexcept {
        return SizeType{one.num / other};
      }
      constexpr friend std::size_t operator%(const SizeType &one, std::size_t other) noexcept {
        return one.num % other;
      }

      constexpr friend SizeType &operator+=(SizeType &one, const SizeType &other) noexcept {
        one.num += other.num;
        return one;
      }
      constexpr friend SizeType &operator-=(SizeType &one, const SizeType &other) noexcept {
        one.num -= other.num;
        return one;
      }
      constexpr friend SizeType &operator*=(SizeType &one, std::size_t other) noexcept {
        one.num *= other;
        return one;
      }

      constexpr friend SizeType &operator++(SizeType &value) noexcept {
        ++value.num;
        return value;
      }
      constexpr friend SizeType &operator--(SizeType &value) noexcept {
        --value.num;
        return value;
      }

      template <std::integral T>
      constexpr friend T operator>>(T one, const SizeType &other) noexcept {
        return one >> (other.num * Size);
      }
      template <std::integral T>
      constexpr friend T operator<<(T one, const SizeType &other) noexcept {
        return one << (other.num * Size);
      }

      template <std::integral T>
      constexpr friend T &operator>>=(T &one, const SizeType &other) noexcept {
        return one >>= (other.num * Size);
      }
      template <std::integral T>
      constexpr friend T &operator<<=(T &one, const SizeType &other) noexcept {
        return one <<= (other.num * Size);
      }

      constexpr friend auto operator<=>(const SizeType &one, const SizeType &other) noexcept = default;

      constexpr std::size_t bits() const noexcept { return num * Size; }

      constexpr std::uintmax_t mask() const noexcept {
        if (bits() >= std::numeric_limits<std::uintmax_t>::digits) {
          return std::numeric_limits<std::uintmax_t>::max();
        }
        return (std::uintmax_t{1} << bits()) - 1U;
      }

      template <std::size_t N = Size>
      std::enable_if_t<N == 1, std::string> toString() const;

      template <std::size_t N = Size>
      std::enable_if_t<N == 8, std::string> toString() const;

      template <std::size_t N>
      constexpr SizeType<N> divide() const noexcept {
        return SizeType<N>{bits() / N};
      }

      std::size_t num = 0;
    };

    template <typename Iter>
    constexpr Iter operator+(Iter one, const SizeType<8> &other) noexcept
      requires(std::random_access_iterator<Iter> && sizeof(*std::declval<Iter>()) == 1)
    {
      return one + static_cast<std::iter_difference_t<Iter>>(other.value());
    }

    template <typename Iter>
    constexpr Iter operator-(Iter one, const SizeType<8> &other) noexcept
      requires(std::random_access_iterator<Iter> && sizeof(*std::declval<Iter>()) == 1)
    {
      return one - static_cast<std::iter_difference_t<Iter>>(other.value());
    }
  } // namespace detail

  using BitCount = detail::SizeType<1>;
  using ByteCount = detail::SizeType<8>;

  inline namespace literals {
    constexpr BitCount operator""_bits(unsigned long long int value) noexcept { return BitCount{value}; }
    constexpr ByteCount operator""_bytes(unsigned long long int value) noexcept { return ByteCount{value}; }
  } // namespace literals

} // namespace bml
