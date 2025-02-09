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

  /*!
   * Additional flags that can be specified for YAML printing.
   */
  enum class PrintFlags : uint32_t {
    NONE = 0x00,
    /*!
     * Don't print entries which are considered "empty" (e.g. empty optional values, empty containers, etc.)
     */
    HIDE_EMPTY = 0x01,
    /*!
     * Don't print entries which have a "default" value.
     */
    HIDE_DEFAULT = 0x02,
    /*!
     * Don't print some more detailed information.
     *
     * It is up to the type being printed to decide what "details" to omit.
     */
    HIDE_DETAILS = 0x04,
  };

  constexpr PrintFlags operator|(PrintFlags one, PrintFlags other) noexcept {
    return static_cast<PrintFlags>(static_cast<std::underlying_type_t<PrintFlags>>(one) |
                                   static_cast<std::underlying_type_t<PrintFlags>>(other));
  }

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
    bool prefixSpace = false;
    PrintFlags flags = PrintFlags::NONE;

    /**
     * Copies the options for the next indentation level.
     */
    constexpr Options nextLevel(bool isSequence) const noexcept {
      return Options{truncateSequenceLimit, depth + 1, isSequence, true, flags};
    }

    /**
     * Returns the prefix string to be added for the current indentation level.
     */
    std::string indentation(bool firstMember) const;

    constexpr bool hasFlags(PrintFlags flag) const noexcept {
      return (static_cast<std::underlying_type_t<PrintFlags>>(flags) &
              static_cast<std::underlying_type_t<PrintFlags>>(flag)) ==
             static_cast<std::underlying_type_t<PrintFlags>>(flag);
    }
  };

  /*!
   * Traits type to be specialized for custom types, enabling special handling:
   *
   * The presence of a static member function with the following signature enables printing as YAML value:
   *
   * static <any> print(std::ostream&, const Options &, const T&)
   *
   * The presence of a static member function with the following signature enables support for the
   * PrintFlags#HIDE_EMPTY flag:
   *
   * static bool isEmpty(const T&)
   *
   * The presence of a static member function with the following signature enables support for the
   * PrintFlags#HIDE_DEFAULT flag:
   *
   * static bool isDefault(const T&)
   *
   * When set to true, the following static constant enables printing in short (one-lined) lists:
   *
   * static constexpr bool SIMPLE_LIST;
   */
  template <typename T>
  struct YAMLTraits;

  inline namespace concepts {
    /**
     * Concept checking whether the type has a member function with following signature:
     *
     * <any> printYAML(std::ostream&, const Options &) const
     */
    template <typename Type>
    concept HasPrintYAMLMember =
        requires(const Type &obj) { obj.printYAML(std::declval<std::ostream &>(), std::declval<const Options &>()); };

    /**
     * Concept checking whether the type has an associated bml::yaml::YAMLTraits type with a static member function with
     * following signature:
     *
     * static <any> print(std::ostream&, const Options &, const T&)
     */
    template <typename Type>
    concept HasYAMLTraitsPrint = !HasPrintYAMLMember<Type> && requires(const Type &obj) {
      bml::yaml::YAMLTraits<Type>::print(std::declval<std::ostream &>(), std::declval<const Options &>(), obj);
    };

    /**
     * Concept checking whether the type has an associated bml::yaml::YAMLTraits type with a static member function with
     * following signature:
     *
     * static bool isEmpty(const T&)
     */
    template <typename Type>
    concept HasYAMLTraitsEmptyCheck = requires(const Type &obj) {
      { bml::yaml::YAMLTraits<Type>::isEmpty(obj) } -> std::convertible_to<bool>;
    };

    /**
     * Concept checking whether the type has an associated bml::yaml::YAMLTraits type with a static member function with
     * following signature:
     *
     * static bool isDefault(const T&)
     */
    template <typename Type>
    concept HasYAMLTraitsDefaultCheck = requires(const Type &obj) {
      { bml::yaml::YAMLTraits<Type>::isDefault(obj) } -> std::convertible_to<bool>;
    };

    /**
     * Concept checking whether the type has an associated bml::yaml::YAMLTraits type with a static member constant with
     * following signature:
     *
     * static constexpr bool SIMPLE_LIST;
     */
    template <typename Type>
    concept HasYAMLTraitsSimpleList = requires(const Type &obj) {
      { bml::yaml::YAMLTraits<Type>::SIMPLE_LIST } -> std::convertible_to<bool>;
      bml::yaml::YAMLTraits<Type>::SIMPLE_LIST == true;
    };

    /*!
     * Concept checking whether sequences of the given member type are listed as simple lists (e.g. "[a, b, d, c]") or
     * more complex multi-line lists.
     */
    template <typename Type>
    concept SimpleListMember = std::integral<Type> || std::floating_point<Type> || std::same_as<Type, std::byte> ||
                               HasYAMLTraitsSimpleList<Type> || (HasPrintYAMLMember<Type> && requires() {
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

  template <bml::concepts::Enum T>
  std::ostream &print(std::ostream &os, const Options &options, T val) {
    return print(os, options, static_cast<std::underlying_type_t<T>>(val));
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

  template <HasYAMLTraitsPrint T>
  std::ostream &print(std::ostream &os, const Options &options, const T &val) {
    bml::yaml::YAMLTraits<T>::print(os, options, val);
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
      return os << (options.prefixSpace ? " []" : "[]");
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
    struct HideEmpty {
      bool operator()(const Options &, const T &) noexcept { return false; }
    };

    template <HasYAMLTraitsEmptyCheck T>
    struct HideEmpty<T> {
      bool operator()(const Options &options, const T &val) {
        return options.hasFlags(PrintFlags::HIDE_EMPTY) && bml::yaml::YAMLTraits<T>::isEmpty(val);
      }
    };

    template <typename T>
    struct HideEmpty<std::optional<T>> {
      bool operator()(const Options &options, const std::optional<T> &val) noexcept {
        return options.hasFlags(PrintFlags::HIDE_EMPTY) && !val;
      }
    };

    template <typename T>
    struct HideEmpty<std::span<const T>> {
      bool operator()(const Options &options, std::span<const T> val) noexcept {
        return options.hasFlags(PrintFlags::HIDE_EMPTY) && val.empty();
      }
    };

    template <typename T>
    struct HideEmpty<std::vector<T>> {
      bool operator()(const Options &options, const std::vector<T> &val) noexcept {
        return options.hasFlags(PrintFlags::HIDE_EMPTY) && val.empty();
      }
    };

    template <typename T>
    struct HideDefault {
      bool operator()(const Options &, const T &) noexcept { return false; }
    };

    template <HasYAMLTraitsDefaultCheck T>
    struct HideDefault<T> {
      bool operator()(const Options &options, const T &val) noexcept {
        return options.hasFlags(PrintFlags::HIDE_DEFAULT) && bml::yaml::YAMLTraits<T>::isDefault(val);
      }
    };

    template <typename T>
    void printMember(std::ostream &os, const Options &options, bool firstMember, std::string_view name, T &&last) {
      if (!HideEmpty<std::decay_t<T>>{}(options, last) && !HideDefault<std::decay_t<T>>{}(options, last)) {
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
      if (HideEmpty<std::decay_t<T>>{}(options, head) || HideDefault<std::decay_t<T>>{}(options, head)) {
        nextIsFirstMember = firstMember;
      } else {
        os << options.indentation(firstMember) << names.substr(0, nameLength) << ':';
        bml::yaml::print(os, options.nextLevel(false /* no sequence */), std::forward<T>(head));
      }
      printMember(os, options, nextIsFirstMember, names.substr(nameEnd + 2), std::forward<Tail>(tail)...);
    }
  } // namespace detail
} // namespace bml::yaml

namespace bml {
  struct ByteRange;
} // namespace bml

namespace bml::yaml {
  template <>
  struct YAMLTraits<bml::ByteRange> {
    static std::ostream &print(std::ostream &os, const Options &options, const bml::ByteRange &val);
    static bool isEmpty(const bml::ByteRange &val);
  };
} // namespace bml::yaml
