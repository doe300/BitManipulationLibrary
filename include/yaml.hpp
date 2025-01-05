#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace bml::yaml {

  /**
   * Container for user-defined and internal options for YAML representation generation.
   */
  class Options {
  public:
    constexpr Options(uint32_t level = 0) noexcept : depth(level) {}

    /**
     * Copies the options for the next indentation level.
     */
    Options nextLevel() const noexcept { return Options{depth + 1}; }

    /**
     * Returns the prefix string to be added for the current indentation level.
     */
    std::string indentation() const;

    /**
     * Returns the maximum number of elements for some sequences to list explicitly before only printing the element
     * count.
     */
    std::size_t truncateSequenceLimit() const noexcept;

  private:
    uint32_t depth = 0;
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

  template <typename Clock>
  std::ostream &print(std::ostream &os, const Options & /* options */, const std::chrono::time_point<Clock> &val) {
    return os << val;
  }

  template <HasPrintYAMLMember T>
  std::ostream &print(std::ostream &os, const Options &options, const T &val) {
    val.printYAML(os, options);
    return os;
  }

  template <typename T>
  std::ostream &print(std::ostream &os, const Options &options, std::span<const T> val)
    requires(std::integral<T> || std::floating_point<T> || std::same_as<T, std::byte>)
  {
    if (val.size() > options.truncateSequenceLimit()) {
      return os << "(" << val.size() << " entries)";
    }
    os << "[";
    for (auto elem : val) {
      print(os, options, elem) << ", ";
    }
    return os << "]";
  }

  template <typename T>
  std::ostream &print(std::ostream &os, const Options &options, std::span<const T> val)
    requires(!std::integral<T> && !std::floating_point<T> && !std::same_as<T, std::byte>)
  {
    if (val.empty()) {
      return os << "[]";
    }
    os << "\n";
    for (const auto &elem : val) {
      os << options.indentation() << "- ";
      print(os, options.nextLevel(), elem);
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
    void printMember(std::ostream &os, const Options &options, bool firstMember, std::string_view name, T &&last) {
      if (!firstMember) {
        os << options.indentation();
      }
      os << name << ": ";
      bml::yaml::print(os, options, std::forward<T>(last)) << '\n';
    }

    template <typename T, typename... Tail>
    void printMember(std::ostream &os, const Options &options, bool firstMember, std::string_view names, T &&head,
                     Tail &&...tail) {
      auto nameEnd = names.find(',');
      auto nameLength = nameEnd != std::string_view::npos ? nameEnd : names.size();
      if (!firstMember) {
        os << options.indentation();
      }
      os << names.substr(0, nameLength) << ": ";
      bml::yaml::print(os, options, std::forward<T>(head)) << '\n';
      printMember(os, options, false, names.substr(nameEnd + 2), std::forward<Tail>(tail)...);
    }
  } // namespace detail
} // namespace bml::yaml
