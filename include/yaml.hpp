#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace bml::yaml {

  /**
   * Container for user-defined and internal options for YAML representation generation.
   */
  struct Options {
    /**
     * The default maximum number of elements for some sequences to list explicitly before only printing the element
     * count.
     */
    static constexpr uint32_t DEFAULT_SEQUENCE_LIMIT = 16U;

    uint32_t truncateSequenceLimit = DEFAULT_SEQUENCE_LIMIT;
    uint32_t depth = 0;
    bool inSequence = false;
    bool hideEmpty = false;
    bool prefixSpace = false;

    /**
     * Copies the options for the next indentation level.
     */
    constexpr Options nextLevel(bool isSequence) const noexcept {
      return Options{truncateSequenceLimit, depth + 1, isSequence, hideEmpty, true};
    }

    /**
     * Returns the prefix string to be added for the current indentation level.
     */
    std::string indentation(bool firstMember) const;
  };

  inline namespace concepts {
    /**
     * Concept checking whether the type has a member function with following signature:
     *
     * <any> printYAML(std::ostream&, const Options &) const
     */
    template <typename Type>
    concept HasPrintYAMLMember =
        requires(const Type &obj) { obj.printYAML(std::declval<std::ostream &>(), std::declval<const Options &>()); };

    /*!
     * Concept checking whether sequences of the given member type are listed as simple lists (e.g. "[a, b, d, c]") or
     * more complex multi-line lists.
     */
    template <typename Type>
    concept SimpleListMember = std::integral<Type> || std::floating_point<Type> || std::same_as<Type, std::byte> ||
                               (HasPrintYAMLMember<Type> && requires() {
                                 typename Type::yaml_print_type;
                                 requires(std::integral<typename Type::yaml_print_type> ||
                                          std::floating_point<typename Type::yaml_print_type> ||
                                          std::same_as<typename Type::yaml_print_type, std::byte>);
                               });

  } // namespace concepts

  std::ostream &print(std::ostream &os, const Options &options, std::byte val);
  std::ostream &print(std::ostream &os, const Options &options, bool val);
  std::ostream &print(std::ostream &os, const Options &options, uintmax_t val);
  std::ostream &print(std::ostream &os, const Options &options, intmax_t val);
  std::ostream &print(std::ostream &os, const Options &options, float val);
  std::ostream &print(std::ostream &os, const Options &options, double val);
  std::ostream &print(std::ostream &os, const Options &options, std::string_view val);
  std::ostream &print(std::ostream &os, const Options &options, std::u8string_view val);

  template <std::unsigned_integral T>
  std::ostream &print(std::ostream &os, const Options &options, T val) {
    return print(os, options, static_cast<uintmax_t>(val));
  }

  template <std::signed_integral T>
  std::ostream &print(std::ostream &os, const Options &options, T val) {
    return print(os, options, static_cast<intmax_t>(val));
  }

  template <typename Clock, typename Duration>
  std::ostream &print(std::ostream &os, const Options &options, const std::chrono::time_point<Clock, Duration> &val) {
    if (options.prefixSpace) {
      os << ' ';
    }
    return os << val;
  }

  template <HasPrintYAMLMember T>
  std::ostream &print(std::ostream &os, const Options &options, const T &val) {
    val.printYAML(os, options);
    return os;
  }

  template <typename T>
  std::ostream &print(std::ostream &os, const Options &options, const std::optional<T> &val) {
    if (val) {
      return print(os, options, val.value());
    }
    return os << "null";
  }

  template <typename T>
  std::ostream &print(std::ostream &os, const Options &options, std::span<const T> val)
    requires(SimpleListMember<T>)
  {
    if (options.prefixSpace) {
      os << ' ';
    }
    if (val.size() > options.truncateSequenceLimit) {
      return os << "(" << val.size() << " entries)";
    }
    os << "[";
    auto subOptions = options;
    subOptions.prefixSpace = false;
    for (auto elem : val) {
      print(os, subOptions, elem) << ", ";
    }
    return os << "]";
  }

  template <typename T>
  std::ostream &print(std::ostream &os, const Options &options, std::span<const T> val)
    requires(!SimpleListMember<T>)
  {
    if (val.empty()) {
      return os << "[]";
    }
    for (const auto &elem : val) {
      print(os, options.nextLevel(true /* sequence */), elem);
    }
    return os;
  }

  template <typename T>
  std::ostream &print(std::ostream &os, const Options &options, const std::vector<T> &val) {
    return print(os, options, std::span{val});
  }

/**
 * Defines the implementation of a member function to print the object as YAML mapping node with following signature:
 *
 * std::ostream& printYAML(std::ostream&, const Options &) const
 */
#define BML_YAML_DEFINE_PRINT(Type, ...)                                                                               \
  std::ostream &Type ::printYAML(std::ostream &os, const bml::yaml::Options &options) const {                          \
    static const auto NAMES = #__VA_ARGS__; /* are stringified as single string with commas */                         \
    bml::yaml::detail::printMember(os, options, true /* first member */, NAMES, __VA_ARGS__);                          \
    return os;                                                                                                         \
  }

  namespace detail {

    template <typename T>
    static bool hideMember(const Options & /* options */, const T &) noexcept {
      return false;
    }

    template <typename T>
    static bool hideMember(const Options &options, const std::optional<T> &val) noexcept {
      return options.hideEmpty && !val;
    }

    template <typename T>
    static bool hideMember(const Options &options, std::span<const T> val) noexcept {
      return options.hideEmpty && val.empty();
    }

    template <typename T>
    static bool hideMember(const Options &options, const std::vector<T> &val) noexcept {
      return options.hideEmpty && val.empty();
    }

    template <typename T>
    void printMember(std::ostream &os, const Options &options, bool firstMember, std::string_view name, T &&last) {
      if (!hideMember(options, last)) {
        os << options.indentation(firstMember) << name << ':';
        bml::yaml::print(os, options.nextLevel(false /* no sequence */), std::forward<T>(last));
      }
    }

    template <typename T, typename... Tail>
    void printMember(std::ostream &os, const Options &options, bool firstMember, std::string_view names, T &&head,
                     Tail &&...tail) {
      auto nameEnd = names.find(',');
      auto nameLength = nameEnd != std::string_view::npos ? nameEnd : names.size();
      bool nextIsFirstMember = false;
      if (hideMember(options, head)) {
        nextIsFirstMember = firstMember;
      } else {
        os << options.indentation(firstMember) << names.substr(0, nameLength) << ':';
        bml::yaml::print(os, options.nextLevel(false /* no sequence */), std::forward<T>(head));
      }
      printMember(os, options, nextIsFirstMember, names.substr(nameEnd + 2), std::forward<Tail>(tail)...);
    }
  } // namespace detail
} // namespace bml::yaml
