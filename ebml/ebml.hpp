#pragma once

#include "bml.hpp"
#include "yaml.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

/**
 * Types for reading/writing the basic Elements of the Extensible Binary Meta Language (EBML, RFC 8794) container
 * format.
 *
 * See: https://www.rfc-editor.org/rfc/rfc8794.html
 */
namespace bml::ebml {

  /** Marker enum for element IDs as defined in the associated specifications */
  enum class ElementId : uint64_t {};

  using Date = std::chrono::utc_time<std::chrono::nanoseconds>;

  struct ReadOptions {
    /**
     * If set to true, will validate CRC-32 Elements in Master Elements (if present) and throw an error if the
     * successive Elements inside the Master Element do not match the CRC-32.
     */
    bool validateCRC32 = false;

    /**
     * If set to true, will read the binary blob of Elements containing media data (e.g. Matroska Blocks). Otherwise
     * will only read the media data sizes.
     *
     * NOTE: Consider the memory footprint when reading the whole media data into RAM!
     *
     * NOTE: When media data elements are read without their contents, they cannot be written back. These objects can
     * only be used for displaying or container analysis.
     */
    bool readMediaData = false;
  };

  inline namespace literals {
    constexpr ElementId operator""_id(unsigned long long int value) noexcept { return ElementId{value}; }
  } // namespace literals

  inline namespace concepts {
    template <typename Type>
    concept ReadableWithOptions =
        requires(Type obj, BitReader reader, const ReadOptions &options) { obj.read(reader, options); };

    template <typename Type>
    concept HasEBMLID = requires() {
      { Type::ID } -> std::convertible_to<ElementId>;
    };

    /**
     * Concept checking whether the given type is a fully supported EBML Element type.
     */
    template <typename Type>
    concept EBMLElement = ReadableWithOptions<Type> && Writeable<Type> && Printable<Type> && Skipable<Type> &&
                          Copyable<Type> && HasEBMLID<Type>;
  } // namespace concepts

  namespace detail {
    /**
     * The maximum possible value for a Variable-Size Integer with at most 8 bytes.
     */
    static constexpr ByteCount VINTMAX{(1ULL << 56ULL) - 2ULL};

    /**
     * The marker value for a Variable-Size Integer indicating a Master Element with unknown size.
     */
    static constexpr ByteCount UNKNOWN_SIZE{std::numeric_limits<std::size_t>::max()};

    /**
     * The Epoch of the Date Element, specified as 2001-01-01T00:00:00.000000000 UTC.
     */
    extern const Date DATE_EPOCH;

    template <std::unsigned_integral T>
    constexpr ByteCount requiredBytes(T value) noexcept {
      BitCount numBits{static_cast<uint32_t>(std::bit_width(value))};
      numBits = !numBits ? 1_bytes : numBits;
      return ByteCount{numBits / 1_bytes} + (numBits % 1_bytes ? 1_bytes : 0_bytes);
    }

    ByteCount requiredBytes(intmax_t value) noexcept;

    constexpr ByteCount calcElementSize(ElementId id, ByteCount contentSize, bool isDefaultValue = false) noexcept {
      if (isDefaultValue) {
        contentSize = 0_bytes;
      }
      auto elementSize = requiredBytes(contentSize.num * 8U / 7U);
      if (contentSize.num == (elementSize - 1_bits).mask()) {
        // All bits set = "unknown size", so need to pad with zeroes
        elementSize += 1_bytes;
      }
      return requiredBytes(static_cast<std::underlying_type_t<ElementId>>(id)) + elementSize + contentSize;
    }

    /**
     * Base type for common functionality of "simple" (non-Master) Element types.
     */
    template <typename T>
    class BaseSimpleElement {
    public:
      using value_type = T;
      using yaml_print_type = T;
      static constexpr BitCount minNumBits() { return 0_bytes /* default value */; };

      constexpr BaseSimpleElement() noexcept = default;
      constexpr BaseSimpleElement(T val) : value(std::move(val)) {}

      constexpr auto operator<=>(const BaseSimpleElement &) const noexcept = default;

      constexpr T get() const noexcept { return value; }
      constexpr operator T() const noexcept { return value; }
      void set(T val) noexcept { value = std::move(val); }

      friend std::ostream &operator<<(std::ostream &os, const BaseSimpleElement &element) {
        return os << bml::printView(element.value);
      }

      static void skip(BitReader &reader);
      static void copy(BitReader &reader, BitWriter &writer);

    protected:
      void readValue(BitReader &reader, ElementId id, const T &defaultValue);
      void writeValue(BitWriter &writer, ElementId id, const T &defaultValue) const;

    protected:
      T value = {};

      template <typename U>
      friend class BaseSimpleElement;
      template <typename U>
      friend struct bml::yaml::YAMLTraits;
    };

    template <typename T>
    concept SimpleEBMLElement = requires(T val) {
      // Check whether the type T can be converted (and thus extends) to a specialization of BaseSimpleElement
      []<typename X>(BaseSimpleElement<X> &) {}(val);
    };

    /**
     * Helper type to allow for compile-time constant (template parameter) string literals.
     */
    template <size_t N>
    struct StringLiteral {
      constexpr StringLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }

      char value[N];
    };

  } // namespace detail

  ////
  // Basic Element Types
  ////

  /**
   * Specialization of the Unsigned Integer Element with an allowed range of 0-1 mapping to a C++ boolean value.
   */
  template <ElementId Id, bool Default = false>
  struct BoolElement : public detail::BaseSimpleElement<bool> {
    static constexpr ElementId ID = Id;
    static constexpr bool DEFAULT = Default;
    static constexpr BitCount maxNumBits() { return detail::calcElementSize(ID, 1_bytes); }

    constexpr BoolElement() noexcept : detail::BaseSimpleElement<bool>(DEFAULT) {}
    explicit constexpr BoolElement(bool val) noexcept : detail::BaseSimpleElement<bool>(val) {}

    constexpr BitCount numBits() const noexcept { return detail::calcElementSize(ID, 1_bytes, value == DEFAULT); }

    BoolElement &operator=(bool val) {
      value = val;
      return *this;
    }

    void read(bml::BitReader &reader, const ReadOptions & = {}) { readValue(reader, ID, DEFAULT); }
    void write(BitWriter &writer) const { writeValue(writer, ID, DEFAULT); }
  };

  template <ElementId Id, intmax_t Default = 0>
  struct SignedIntElement : public detail::BaseSimpleElement<intmax_t> {
    static constexpr ElementId ID = Id;
    static constexpr intmax_t DEFAULT = Default;
    static constexpr BitCount maxNumBits() { return detail::calcElementSize(ID, ByteCount{sizeof(intmax_t)}); }

    constexpr SignedIntElement() noexcept : detail::BaseSimpleElement<intmax_t>(DEFAULT) {}
    explicit constexpr SignedIntElement(intmax_t val) noexcept : detail::BaseSimpleElement<intmax_t>(val) {}

    constexpr BitCount numBits() const noexcept {
      return detail::calcElementSize(ID, detail::requiredBytes(this->value), value == DEFAULT);
    }

    SignedIntElement &operator=(intmax_t val) {
      this->value = val;
      return *this;
    }

    void read(bml::BitReader &reader, const ReadOptions & = {}) { this->readValue(reader, ID, DEFAULT); }
    void write(BitWriter &writer) const { this->writeValue(writer, ID, DEFAULT); }
  };

  template <ElementId Id, uintmax_t Default = 0>
  struct UnsignedIntElement : public detail::BaseSimpleElement<uintmax_t> {
    static constexpr ElementId ID = Id;
    static constexpr uintmax_t DEFAULT = Default;
    static constexpr BitCount maxNumBits() { return detail::calcElementSize(ID, ByteCount{sizeof(uintmax_t)}); }

    constexpr UnsignedIntElement() noexcept : detail::BaseSimpleElement<uintmax_t>(DEFAULT) {}
    explicit constexpr UnsignedIntElement(uintmax_t val) noexcept : detail::BaseSimpleElement<uintmax_t>(val) {}

    constexpr BitCount numBits() const noexcept {
      return detail::calcElementSize(ID, detail::requiredBytes(this->value), value == DEFAULT);
    }

    UnsignedIntElement &operator=(uintmax_t val) {
      this->value = val;
      return *this;
    }

    void read(bml::BitReader &reader, const ReadOptions & = {}) { this->readValue(reader, ID, DEFAULT); }
    void write(BitWriter &writer) const { this->writeValue(writer, ID, DEFAULT); }
  };

  template <ElementId Id, typename T = float, T Default = T{0.0}>
  struct FloatElement : public detail::BaseSimpleElement<T> {
    static constexpr ElementId ID = Id;
    static constexpr T DEFAULT = Default;
    static constexpr BitCount maxNumBits() { return detail::calcElementSize(ID, ByteCount{sizeof(T)}); }

    constexpr FloatElement() noexcept : detail::BaseSimpleElement<T>(DEFAULT) {}
    explicit constexpr FloatElement(T val) noexcept : detail::BaseSimpleElement<T>(val) {}

    constexpr BitCount numBits() const noexcept {
      return detail::calcElementSize(ID, std::is_same_v<T, double> ? 8_bytes : 4_bytes, this->value == DEFAULT);
    }

    FloatElement &operator=(T val) {
      this->value = val;
      return *this;
    }

    void read(bml::BitReader &reader, const ReadOptions & = {}) { this->readValue(reader, ID, DEFAULT); }
    void write(BitWriter &writer) const { this->writeValue(writer, ID, DEFAULT); }
  };

  template <ElementId Id>
  struct DateElement : public detail::BaseSimpleElement<Date> {
    static constexpr ElementId ID = Id;
    static constexpr BitCount maxNumBits() { return detail::calcElementSize(ID, ByteCount{sizeof(intmax_t)}); }

    constexpr DateElement() noexcept : detail::BaseSimpleElement<Date>(detail::DATE_EPOCH) {}
    explicit constexpr DateElement(Date val) noexcept : detail::BaseSimpleElement<Date>(val) {}

    constexpr BitCount numBits() const noexcept {
      return detail::calcElementSize(ID, detail::requiredBytes((value - detail::DATE_EPOCH).count()),
                                     value == detail::DATE_EPOCH);
    }

    DateElement &operator=(Date val) {
      value = val;
      return *this;
    }

    void read(bml::BitReader &reader, const ReadOptions & = {}) { readValue(reader, ID, detail::DATE_EPOCH); }
    void write(BitWriter &writer) const { writeValue(writer, ID, detail::DATE_EPOCH); }
  };

  template <ElementId Id, detail::StringLiteral Default = "">
  struct StringElement : public detail::BaseSimpleElement<std::string> {
    static constexpr ElementId ID = Id;
    static constexpr std::string DEFAULT = Default.value;
    static constexpr BitCount maxNumBits() { return detail::calcElementSize(ID, detail::VINTMAX); }

    constexpr StringElement() noexcept : detail::BaseSimpleElement<std::string>(DEFAULT) {}
    explicit constexpr StringElement(std::string val) noexcept
        : detail::BaseSimpleElement<std::string>(std::move(val)) {}

    constexpr BitCount numBits() const noexcept {
      return detail::calcElementSize(ID, ByteCount{value.size()}, value == DEFAULT);
    }

    StringElement &operator=(std::string val) {
      value = std::move(val);
      return *this;
    }

    void read(bml::BitReader &reader, const ReadOptions & = {}) { readValue(reader, ID, DEFAULT); }
    void write(BitWriter &writer) const { writeValue(writer, ID, DEFAULT); }
  };

  template <ElementId Id, typename T = std::u8string>
  struct Utf8StringElement : public detail::BaseSimpleElement<T> {
    static constexpr ElementId ID = Id;
    static constexpr T DEFAULT{};
    static constexpr BitCount maxNumBits() { return detail::calcElementSize(ID, detail::VINTMAX); }

    constexpr Utf8StringElement() noexcept : detail::BaseSimpleElement<T>(DEFAULT) {}
    explicit constexpr Utf8StringElement(T val) noexcept : detail::BaseSimpleElement<T>(std::move(val)) {}

    constexpr BitCount numBits() const noexcept {
      return detail::calcElementSize(ID, ByteCount{this->value.size()}, this->value == DEFAULT);
    }

    Utf8StringElement &operator=(T val) {
      this->value = std::move(val);
      return *this;
    }

    void read(bml::BitReader &reader, const ReadOptions & = {}) { this->readValue(reader, ID, DEFAULT); }
    void write(BitWriter &writer) const { this->writeValue(writer, ID, DEFAULT); }
  };

  template <ElementId Id>
  struct BinaryElement : public detail::BaseSimpleElement<std::vector<std::byte>> {
    static constexpr ElementId ID = Id;
    static constexpr BitCount maxNumBits() { return detail::calcElementSize(ID, detail::VINTMAX); }
    constexpr BitCount numBits() const noexcept {
      return detail::calcElementSize(ID, ByteCount{value.size()}, value.empty());
    }

    BinaryElement &operator=(std::vector<std::byte> val) {
      value = std::move(val);
      return *this;
    }

    void read(bml::BitReader &reader, const ReadOptions & = {}) { readValue(reader, ID, {}); }
    void write(BitWriter &writer) const { writeValue(writer, ID, {}); }
  };

  struct CRC32 : public detail::BaseSimpleElement<uint32_t> {
    static constexpr ElementId ID = 0xBF_id;

    static constexpr BitCount minNumBits() { return detail::calcElementSize(ID, 4_bytes); }
    static constexpr BitCount maxNumBits() { return detail::calcElementSize(ID, 4_bytes); }

    constexpr CRC32() noexcept : detail::BaseSimpleElement<uint32_t>() {}
    explicit constexpr CRC32(uint32_t val) noexcept : detail::BaseSimpleElement<uint32_t>(val) {}

    constexpr BitCount numBits() const noexcept { return detail::calcElementSize(ID, 4_bytes); }

    CRC32 &operator=(uint32_t val) {
      this->value = val;
      return *this;
    }

    void read(bml::BitReader &reader, const ReadOptions & = {});
    void write(BitWriter &writer) const;

    friend std::ostream &operator<<(std::ostream &os, const CRC32 &element);
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  /**
   * Void Element for skipped/ignored data.
   *
   * NOTE: The contents of the Void Element are not actually read, just its length. On write, the skipped number of
   * bytes are filled with zeroes.
   */
  struct Void {
    static constexpr ElementId ID{0xEC};

    ByteCount skipBytes;

    constexpr auto operator<=>(const Void &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions & = {});
    void write(bml::BitWriter &writer) const;

    static void skip(BitReader &reader);
    static void copy(BitReader &reader, BitWriter &writer);

    BML_DEFINE_PRINT(Void, skipBytes)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  /**
   * Base type for all Master Element types.
   *
   * This base class contains the CRC-32 and Void Elements which can occur globally as children of any Master Element.
   */
  struct MasterElement {
    static constexpr BitCount minNumBits() { return 2_bytes /* 1 ID, 1 size */; };
    static constexpr BitCount maxNumBits() { return detail::calcElementSize(0x1A45DFA3_id, detail::VINTMAX); }

    std::optional<CRC32> crc32;
    std::vector<Void> voidElements;

    constexpr auto operator<=>(const MasterElement &) const noexcept = default;

    static void skip(BitReader &reader);
    static void copy(BitReader &reader, BitWriter &writer);
  };

  ////
  // Common EBML Elements
  ////

  struct DocTypeExtension : MasterElement {
    static constexpr ElementId ID{0x4281};

    StringElement<0x4283_id> docTypeExtensionName;
    UnsignedIntElement<0x4284_id> docTypeExtensionVersion;

    constexpr auto operator<=>(const DocTypeExtension &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(DocTypeExtension, crc32, docTypeExtensionName, docTypeExtensionVersion, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct EBMLHeader : MasterElement {
    static constexpr ElementId ID{0x1A45DFA3};

    UnsignedIntElement<0x4286_id> version;
    UnsignedIntElement<0x42F7_id> readVersion;
    UnsignedIntElement<0x42F2_id> maxIDLength;
    UnsignedIntElement<0x42F3_id> maxSizeLength;
    StringElement<0x4282_id> docType;
    UnsignedIntElement<0x4287_id> docTypeVersion;
    UnsignedIntElement<0x4285_id> docTypeReadVersion;
    std::vector<DocTypeExtension> docTypeExtensions;

    constexpr auto operator<=>(const EBMLHeader &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(EBMLHeader, crc32, version, readVersion, maxIDLength, maxSizeLength, docType, docTypeVersion,
                     docTypeReadVersion, docTypeExtensions, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

/**
 * Defines the implementations of the member functions to read/write the Master Element with the following signatures:
 *
 * void printYAML(BitReader &, const ReadOptions &);
 * void write(bml::BitWriter &writer) const;
 */
#define BML_EBML_DEFINE_IO(Type, ...)                                                                                  \
  void Type ::read(bml::BitReader &reader, const ReadOptions &options) {                                               \
    bml::ebml::detail::readMasterElement(reader, ID, *this, options, __VA_ARGS__);                                     \
  }                                                                                                                    \
  void Type ::write(bml::BitWriter &writer) const { bml::ebml::detail::writeMasterElement(writer, ID, __VA_ARGS__); }

  /**
   * Helper functions for implementors of EBML-based container formats.
   */
  namespace detail {
    /**
     * Reads a Variable-Size Integer and returns its read value plus the number of value bits.
     *
     * The includePrefix determines whether the VINT_WIDTH + VINT_MARKER prefix should be included in the returned value
     * (and bit-width) or not.
     */
    std::pair<uintmax_t, BitCount> readVariableSizeInteger(BitReader &reader, bool includePrefix);

    /**
     * Returns Element header and returns the Element Data Size.
     *
     * NOTE: This function throws an error if the Element ID read from the input does not match the expected ID.
     */
    ByteCount readElementHeader(BitReader &reader, ElementId id);

    /**
     * Writes the Element header by writing the given Element ID and Element Data Size values.
     */
    void writeElementHeader(BitWriter &writer, ElementId id, ByteCount contentSize);

    /*!
     * Skips the next Element in the input reader and returns the number of bytes skipped.
     *
     * The optional terminatingElementIds parameter can be used to support Master Elements with Unknown Data Size. Any
     * Element ID present in this list will finish skipping the current Master Element and return from this function.
     * See https://www.rfc-editor.org/rfc/rfc8794.html#section-6.2 for which Element IDs need to be passed into this
     * parameter.
     */
    ByteCount skipElement(BitReader &reader, const std::set<ElementId> &terminatingElementIds = {});

    /*!
     * Copies the next Element from the given input reader to the given output writer and returns the number of bytes
     * copied.
     *
     * The optional terminatingElementIds parameter can be used to support Master Elements with Unknown Data Size. Any
     * Element ID present in this list will finish copying the current Master Element and return from this function. See
     * https://www.rfc-editor.org/rfc/rfc8794.html#section-6.2 for which Element IDs need to be passed into this
     * parameter.
     */
    ByteCount copyElement(BitReader &reader, BitWriter &writer, const std::set<ElementId> &terminatingElementIds = {});

    /**
     * Reads the Master Element with the given ID from the given bit reader and reads all its given members.
     *
     * The optional terminatingElementIds parameter can be used to support Master Elements with Unknown Data Size. Any
     * Element ID present in this list will finish reading into the current Master Element and return from this
     * function. See https://www.rfc-editor.org/rfc/rfc8794.html#section-6.2 for which Element IDs need to be passed
     * into this parameter.
     */
    void readMasterElement(BitReader &reader, ElementId id, MasterElement &master, const ReadOptions &options,
                           const std::map<ElementId, std::function<void(BitReader &, const ReadOptions &)>> &members,
                           const std::set<ElementId> &terminatingElementIds = {});

    /**
     * Writes the Master Element with the given ID to the given bit writer and all its members given.
     *
     * NOTE: The members are written in order of listing in the members parameter.
     */
    void writeMasterElement(BitWriter &writer, ElementId id,
                            const std::vector<std::pair<ElementId, std::function<void(BitWriter &)>>> &members);

    template <typename T>
    std::pair<ElementId, std::function<void(BitReader &, const ReadOptions &)>> wrapMemberReader(T &member)
      requires(ReadableWithOptions<T> && HasEBMLID<T>)
    {
      return std::make_pair(T::ID,
                            [&member](BitReader &reader, const ReadOptions &options) { member.read(reader, options); });
    }

    template <typename T>
    std::pair<ElementId, std::function<void(BitReader &, const ReadOptions &)>>
    wrapMemberReader(std::optional<T> &member)
      requires(ReadableWithOptions<T> && HasEBMLID<T>)
    {
      return std::make_pair(T::ID, [&member](BitReader &reader, const ReadOptions &options) {
        T obj{};
        obj.read(reader, options);
        member = std::move(obj);
      });
    }

    template <typename T>
    std::pair<ElementId, std::function<void(BitReader &, const ReadOptions &)>> wrapMemberReader(std::vector<T> &member)
      requires(ReadableWithOptions<T> && HasEBMLID<T>)
    {
      return std::make_pair(T::ID, [&member](BitReader &reader, const ReadOptions &options) {
        T obj{};
        obj.read(reader, options);
        member.push_back(std::move(obj));
      });
    }

    /**
     * Reads the Master Element with the given ID from the given bit reader and reads all its given members.
     *
     * The variadic template parameters whould be references to the members themselves.
     *
     * This is a convenience function for easier mapping of "simple" Master Elements which do not require any special
     * handling.
     */
    template <typename... Types>
    void readMasterElement(BitReader &reader, ElementId id, MasterElement &master, const ReadOptions &options,
                           Types &...members) {
      return readMasterElement(reader, id, master, options, {wrapMemberReader(members)...});
    }

    template <typename T>
    std::pair<ElementId, std::function<void(BitWriter &)>> wrapMemberWriter(const T &member)
      requires(Writeable<T> && HasEBMLID<T>)
    {
      return std::make_pair(T::ID, [&member](BitWriter &writer) { bml::write(writer, member); });
    }

    template <typename T>
    std::pair<ElementId, std::function<void(BitWriter &)>> wrapMemberWriter(const std::optional<T> &member)
      requires(Writeable<T> && HasEBMLID<T>)
    {
      return std::make_pair(T::ID, [&member](BitWriter &writer) { bml::write(writer, member); });
    }

    template <typename T>
    std::pair<ElementId, std::function<void(BitWriter &)>> wrapMemberWriter(const std::vector<T> &member)
      requires(Writeable<T> && HasEBMLID<T>)
    {
      return std::make_pair(T::ID, [&member](BitWriter &writer) { bml::write(writer, member); });
    }

    /**
     * Writes the Master Element with the given ID to the given bit writer and all its members given.
     *
     * The variadic template parameters whould be references to the members themselves.
     *
     * This is a convenience function for easier mapping of "simple" Master Elements which do not require any special
     * handling.
     */
    template <typename... Types>
    void writeMasterElement(BitWriter &writer, ElementId id, const Types &...members) {
      return writeMasterElement(writer, id, {wrapMemberWriter(members)...});
    }

    template <typename T>
    void BaseSimpleElement<T>::skip(BitReader &reader) {
      skipElement(reader);
    }
    template <typename T>
    void BaseSimpleElement<T>::copy(BitReader &reader, BitWriter &writer) {
      copyElement(reader, writer);
    }
  } // namespace detail
} // namespace bml::ebml

namespace bml::yaml {

  template <bml::ebml::detail::SimpleEBMLElement T>
  struct YAMLTraits<T> {
    static std::ostream &print(std::ostream &os, const Options &options, const T &val) {
      return bml::yaml::print(os, options, val.value);
    }

    static bool isDefault(const T &val)
      requires(requires() { T::DEFAULT; })
    {
      return val.value == T::DEFAULT;
    }

    static constexpr bool SIMPLE_LIST = true;
  };

} // namespace bml::yaml
