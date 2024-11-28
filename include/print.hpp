#pragma once

#include <algorithm>
#include <iostream>
#include <span>
#include <string_view>

namespace bml {

  namespace detail {

    template <typename Type>
    struct PrintView;
  } // namespace detail

  /**
   * Wraps the given read-only reference in a PrintView.
   *
   * The wrapped PrintView object can be written to an output stream via the stream output operator (<<).
   *
   * This enables usage of different functions for printing (e.g. member toString(), to_string()) as well as addition of
   * printing support for standard-library types (e.g. tuple, variant).
   */
  template <typename Type>
  detail::PrintView<Type> printView(const Type &val) {
    return detail::PrintView{val};
  }

/**
 * Defines a stream output friend operator (<<) for this type writing all members as key-value pairs using the member
 * name as key.
 *
 * Also defines the DESTRUCTURE_MEMBERS helper constant used to determine the number of members to
 * read/write/copy/skip/etc. if no custom member functions are defined.
 */
#define BML_DEFINE_PRINT(Type, ...)                                                                                    \
  friend std::ostream &operator<<(std::ostream &os, const Type &obj) {                                                 \
    obj.printInner(os);                                                                                                \
    return os;                                                                                                         \
  }                                                                                                                    \
                                                                                                                       \
private:                                                                                                               \
  void printInner(std::ostream &os) const {                                                                            \
    static const auto NAMES = #__VA_ARGS__; /* are stringified as single string with commas */                         \
    os << #Type << '{';                                                                                                \
    detail::printMember(os, NAMES, __VA_ARGS__);                                                                       \
    os << '}';                                                                                                         \
  }                                                                                                                    \
                                                                                                                       \
public:                                                                                                                \
  /* This is used to correctly determine the number of members for I/O of polymorphic types.                           \
   */                                                                                                                  \
  static constexpr std::size_t DESTRUCTURE_MEMBERS = []() {                                                            \
    constexpr auto NAMES = #__VA_ARGS__;                                                                               \
    return std::count(NAMES, NAMES + std::size(#__VA_ARGS__), ',') + 1;                                                \
  }();

  namespace detail {

    template <typename Type>
    concept IsPrintable = requires(const Type &obj, std::ostream &out) {
      { out << PrintView{obj} } -> std::convertible_to<std::ostream &>;
    };

    template <typename Type>
    concept IsDefaultPrintable = requires(const Type &obj, std::ostream &out) {
      { out << obj } -> std::convertible_to<std::ostream &>;
    };

    template <typename Type>
    concept IsByteNumber = !std::is_same_v<Type, char> && std::is_integral_v<Type> && sizeof(Type) == 1;

    template <typename Type>
    struct PrintView {

      friend std::ostream &operator<<(std::ostream &os, const PrintView &view)
        requires(IsDefaultPrintable<Type> && IsByteNumber<Type>)
      {
        return os << static_cast<int32_t>(view.value);
      }

      friend std::ostream &operator<<(std::ostream &os, const PrintView &view)
        requires(IsDefaultPrintable<Type> && !IsByteNumber<Type>)
      {
        return os << view.value;
      }

      friend std::ostream &operator<<(std::ostream &os, const PrintView &view)
        requires(std::is_enum_v<std::remove_reference_t<Type>> && !IsDefaultPrintable<Type>)
      {
        // Prevent 8-bit enum types to be printed as characters
        using IntType = std::common_type_t<std::underlying_type_t<std::remove_reference_t<Type>>, uint16_t>;
        return os << static_cast<IntType>(view.value);
      }

      const Type &value;
    };

    template <typename Type>
    struct PrintView<std::optional<Type>> {
      friend std::ostream &operator<<(std::ostream &os, const PrintView<std::optional<Type>> &view)
        requires IsPrintable<Type>
      {
        return view.value ? (os << PrintView<Type>{*view.value}) : (os << "(none)");
      }

      const std::optional<Type> &value;
    };

    template <typename Type>
    struct PrintView<std::unique_ptr<Type>> {
      friend std::ostream &operator<<(std::ostream &os, const PrintView<std::unique_ptr<Type>> &view)
        requires IsPrintable<Type>
      {
        return view.value ? (os << PrintView<Type>{*view.value}) : (os << "(none)");
      }

      const std::unique_ptr<Type> &value;
    };

    template <typename... Types>
    struct PrintView<std::tuple<Types...>> {
      friend std::ostream &operator<<(std::ostream &os, const PrintView<std::tuple<Types...>> &view)
        requires(IsPrintable<Types> && ...)
      {
        os << '(';
        std::apply([&os](const auto &...val) { ((os << PrintView<decltype(val)>{val} << ", "), ...); }, view.value);
        return os << ')';
      }

      const std::tuple<Types...> &value;
    };

    template <typename... Types>
    struct PrintView<std::variant<Types...>> {
      friend std::ostream &operator<<(std::ostream &os, const PrintView<std::variant<Types...>> &view)
        requires(IsPrintable<Types> && ...)
      {
        return std::visit([&os](const auto &val) -> std::ostream & { return os << PrintView<decltype(val)>{val}; },
                          view.value);
      }

      const std::variant<Types...> &value;
    };

    template <typename Type>
    struct PrintView<std::vector<Type>> {
      friend std::ostream &operator<<(std::ostream &os, const PrintView<std::vector<Type>> &view)
        requires IsPrintable<Type>
      {
        os << view.value.size() << " [";
        for (const auto &entry : view.value)
          os << PrintView<Type>{entry} << ", ";
        return os << ']';
      }

      const std::vector<Type> &value;
    };

    template <typename Type, std::size_t N>
    struct PrintView<std::array<Type, N>> {
      friend std::ostream &operator<<(std::ostream &os, const PrintView<std::array<Type, N>> &view)
        requires IsPrintable<Type>
      {
        os << view.value.size() << " [";
        for (const auto &entry : view.value)
          os << PrintView<Type>{entry} << ", ";
        return os << ']';
      }

      const std::array<Type, N> &value;
    };

    template <>
    struct PrintView<std::byte> {
      friend std::ostream &operator<<(std::ostream &os, const PrintView<std::byte> &view);

      const std::byte &value;
    };

    template <>
    struct PrintView<std::span<const std::byte>> {
      friend std::ostream &operator<<(std::ostream &os, const PrintView<std::span<const std::byte>> &view);

      const std::span<const std::byte> &value;
    };

    template <>
    struct PrintView<std::vector<std::byte>> {
      friend std::ostream &operator<<(std::ostream &os, const PrintView<std::vector<std::byte>> &view) {
        return os << PrintView<std::span<const std::byte>>{view.value};
      }

      const std::vector<std::byte> &value;
    };

    template <std::size_t N>
    struct PrintView<std::array<std::byte, N>> {
      friend std::ostream &operator<<(std::ostream &os, const PrintView<std::array<std::byte, N>> &view) {
        return os << PrintView<std::span<const std::byte>>{view.value};
      }

      const std::array<std::byte, N> &value;
    };

    template <typename T>
    void printMember(std::ostream &os, std::string_view name, T &&last) {
      os << name << " = " << PrintView{last};
    }

    template <typename T, typename... Tail>
    void printMember(std::ostream &os, std::string_view names, T &&head, Tail &&...tail) {
      auto nameEnd = names.find(",");
      auto nameLength = nameEnd != std::string_view::npos ? nameEnd : names.size();
      os << names.substr(0, nameLength) << " = " << PrintView{head} << ", ";
      printMember(os, names.substr(nameEnd + 2), tail...);
    }
  } // namespace detail

} // namespace bml
