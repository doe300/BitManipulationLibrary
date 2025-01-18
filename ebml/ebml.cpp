
#include "ebml.hpp"

#include "debug.hpp"

#include <span>

namespace bml::ebml {
  namespace detail {

    extern const Date DATE_EPOCH = [] {
      using namespace std::chrono_literals;

      return Date::clock::from_sys(static_cast<std::chrono::sys_days>(2001y / std::chrono::January / 1d));
    }();

    ByteCount requiredBytes(intmax_t value) noexcept {
      if (value >= 0) {
        return requiredBytes(static_cast<uintmax_t>(value));
      }
      BitCount numLeadingOnes{static_cast<uint32_t>(std::countl_one(std::bit_cast<uintmax_t>(value)))};
      return ByteCount{sizeof(value)} - ByteCount{numLeadingOnes / 1_bytes};
    }

    /**
     * Variable-size integer consists of 3 parts:
     *
     * - N consecutive 0 bits
     * - 1 single 1 bit
     * - (N+1) 7-bit blocks of actual integer data
     */
    static uintmax_t peekVariableSizeInteger(BitReader &reader, bool includePrefix) {
      static_assert(sizeof(uintmax_t) <= 8, "The current implementation only works for up to 8 bytes read");
      auto prefix = static_cast<uint8_t>(reader.peek(1_bytes));
      auto numBytes = static_cast<uint32_t>(std::countl_zero(prefix) + 1 /* 1 bit */);
      auto val = reader.peek(ByteCount{numBytes});
      if (includePrefix) {
        return val;
      }
      return val & (ByteCount{numBytes} - BitCount{numBytes}).mask();
    }

    static void writeVariableSizeInteger(BitWriter &writer, uintmax_t value) {
      const BitCount numBits{static_cast<uint32_t>(std::bit_width(value))};
      BitCount coveredBits = 7_bits;
      for (; coveredBits < numBits; coveredBits += 7_bits) {
        writer.write(false);
      }
      if (value == coveredBits.mask()) {
        // All bits set = "unknown size", so need to pad with zeroes
        writer.write(false);
        coveredBits += 7_bits;
      }
      writer.write(true);
      writer.write(value, coveredBits);
    }

    template <typename Func = void (*)(BitWriter &)>
    static void writeElement(BitWriter &writer, ElementId id, Func &&writeContent) {
      std::vector<std::byte> buffer{};
      BitWriter bufWriter{[&](std::byte b) {
        buffer.push_back(b);
        return true;
      }};
      writeContent(bufWriter);
      bufWriter.flush();

      detail::writeElementHeader(writer, id, ByteCount{buffer.size()});
      writer.writeBytes(buffer);
    }

    template <>
    void BaseSimpleElement<bool>::readValue(BitReader &reader, ElementId id, const bool &defaultValue) {
      auto numBytes = readElementHeader(reader, id);
      value = numBytes ? (reader.read(numBytes) != 0x00) : defaultValue;
    }

    template <>
    void BaseSimpleElement<bool>::writeValue(BitWriter &writer, ElementId id, const bool &defaultValue) const {
      if (value == defaultValue) {
        return;
      }
      detail::writeElementHeader(writer, id, 1_bytes);
      writer.writeBytes(value ? 0x01U : 0x00U, 1_bytes);
    }

    template <>
    void BaseSimpleElement<intmax_t>::readValue(BitReader &reader, ElementId id, const intmax_t &defaultValue) {
      auto numBytes = detail::readElementHeader(reader, id);
      if (!numBytes) {
        value = defaultValue;
        return;
      }
      auto tmp = reader.read(numBytes);
      tmp <<= ByteCount{sizeof(tmp)} - numBytes;
      value = std::bit_cast<intmax_t>(tmp) >> (ByteCount{sizeof(tmp)} - numBytes);
    }

    template <>
    void BaseSimpleElement<intmax_t>::writeValue(BitWriter &writer, ElementId id, const intmax_t &defaultValue) const {
      if (value == defaultValue) {
        return;
      }
      writeElement(writer, id, [value{value}](BitWriter &bufWriter) {
        bufWriter.write(std::bit_cast<uintmax_t>(value), detail::requiredBytes(value));
      });
    }

    template <>
    void BaseSimpleElement<uintmax_t>::readValue(BitReader &reader, ElementId id, const uintmax_t &defaultValue) {
      auto numBytes = detail::readElementHeader(reader, id);
      value = numBytes ? reader.read(numBytes) : defaultValue;
    }

    template <>
    void BaseSimpleElement<uintmax_t>::writeValue(BitWriter &writer, ElementId id,
                                                  const uintmax_t &defaultValue) const {
      if (value == defaultValue) {
        return;
      }
      writeElement(writer, id,
                   [value{value}](BitWriter &bufWriter) { bufWriter.write(value, detail::requiredBytes(value)); });
    }

    template <>
    void BaseSimpleElement<double>::readValue(BitReader &reader, ElementId id, const double &defaultValue) {
      auto numBytes = detail::readElementHeader(reader, id);
      auto tmp = reader.read(numBytes);
      switch (numBytes.num) {
      case 4:
        value = std::bit_cast<float>(static_cast<uint32_t>(tmp));
        break;
      case 8:
        value = std::bit_cast<double>(static_cast<uint64_t>(tmp));
        break;
      default:
        value = defaultValue;
      }
    }

    template <>
    void BaseSimpleElement<double>::writeValue(BitWriter &writer, ElementId id, const double &defaultValue) const {
      if (value == defaultValue) {
        return;
      }
      writeElement(writer, id, [value{std::bit_cast<uint64_t>(value)}](BitWriter &bufWriter) {
        bufWriter.write(value, detail::requiredBytes(value));
      });
    }

    template <>
    void BaseSimpleElement<float>::readValue(BitReader &reader, ElementId id, const float &defaultValue) {
      auto numBytes = detail::readElementHeader(reader, id);
      auto tmp = reader.read(numBytes);
      switch (numBytes.num) {
      case 4:
        value = std::bit_cast<float>(static_cast<uint32_t>(tmp));
        break;
      case 8:
        value = static_cast<float>(std::bit_cast<double>(static_cast<uint64_t>(tmp)));
        break;
      default:
        value = defaultValue;
      }
    }

    template <>
    void BaseSimpleElement<float>::writeValue(BitWriter &writer, ElementId id, const float &defaultValue) const {
      if (value == defaultValue) {
        return;
      }
      writeElement(writer, id, [value{std::bit_cast<uint32_t>(value)}](BitWriter &bufWriter) {
        bufWriter.write(value, detail::requiredBytes(value));
      });
    }

    template <>
    void BaseSimpleElement<std::string>::readValue(BitReader &reader, ElementId id, const std::string &defaultValue) {
      auto numChars = detail::readElementHeader(reader, id);
      if (!numChars) {
        value = defaultValue;
        return;
      }
      value = std::string(numChars.value(), '\0');
      reader.readBytesInto(std::as_writable_bytes(std::span{value}));
    }

    template <>
    void BaseSimpleElement<std::string>::writeValue(BitWriter &writer, ElementId id,
                                                    const std::string &defaultValue) const {
      if (value == defaultValue) {
        return;
      }
      detail::writeElementHeader(writer, id, ByteCount{value.size()});
      writer.writeBytes(std::as_bytes(std::span{value}));
    }

    template <>
    void BaseSimpleElement<std::u8string>::readValue(BitReader &reader, ElementId id,
                                                     const std::u8string &defaultValue) {
      auto numChars = detail::readElementHeader(reader, id);
      if (!numChars) {
        value = defaultValue;
        return;
      }
      value = std::u8string(numChars.value(), '\0');
      reader.readBytesInto(std::as_writable_bytes(std::span{value}));
    }

    template <>
    void BaseSimpleElement<std::u8string>::writeValue(BitWriter &writer, ElementId id,
                                                      const std::u8string &defaultValue) const {
      if (value == defaultValue) {
        return;
      }
      detail::writeElementHeader(writer, id, ByteCount{value.size()});
      writer.writeBytes(std::as_bytes(std::span{value}));
    }

    template <>
    void BaseSimpleElement<Date>::readValue(BitReader &reader, ElementId id, const Date &defaultValue) {
      BaseSimpleElement<intmax_t> tmp{};
      tmp.readValue(reader, id, (defaultValue - detail::DATE_EPOCH).count());
      value = DATE_EPOCH + std::chrono::nanoseconds{tmp.get()};
    }

    template <>
    void BaseSimpleElement<Date>::writeValue(BitWriter &writer, ElementId id, const Date &defaultValue) const {
      BaseSimpleElement<intmax_t> tmp{};
      tmp.set((value - DATE_EPOCH).count());
      tmp.writeValue(writer, id, (defaultValue - detail::DATE_EPOCH).count());
    }

    template <>
    void BaseSimpleElement<std::vector<std::byte>>::readValue(BitReader &reader, ElementId id,
                                                              const std::vector<std::byte> &defaultValue) {
      auto numBytes = detail::readElementHeader(reader, id);
      if (!numBytes) {
        value = defaultValue;
        return;
      }
      value.resize(numBytes.value());
      reader.readBytesInto(std::span{value});
    }

    template <>
    void BaseSimpleElement<std::vector<std::byte>>::writeValue(BitWriter &writer, ElementId id,
                                                               const std::vector<std::byte> &defaultValue) const {
      if (value == defaultValue) {
        return;
      }
      detail::writeElementHeader(writer, id, ByteCount{value.size()});
      writer.writeBytes(std::span{value});
    }

    std::pair<uintmax_t, BitCount> readVariableSizeInteger(BitReader &reader, bool includePrefix) {
      static_assert(sizeof(uintmax_t) <= 8, "The current implementation only works for up to 8 bytes read");
      auto prefix = static_cast<uint8_t>(reader.peek(1_bytes));
      auto numBytes = static_cast<uint32_t>(std::countl_zero(prefix) + 1 /* 1 bit */);
      auto val = reader.read(ByteCount{numBytes});
      BitCount numBits = ByteCount{numBytes};
      if (!includePrefix) {
        numBits = ByteCount{numBytes} - BitCount{numBytes};
      }
      return std::make_pair(val & numBits.mask(), numBits);
    }

    ByteCount readElementHeader(BitReader &reader, ElementId id) {
      auto elementId = readVariableSizeInteger(reader, true /* with prefix */).first;
      if (elementId != static_cast<uintmax_t>(id)) {
        throw std::invalid_argument("Element ID '" + toHexString(elementId) +
                                    "' does not match ID of read element: " + toHexString(static_cast<uintmax_t>(id)));
      }
      auto elementSize = readVariableSizeInteger(reader, false /* without prefix */);
      if (elementSize.first == elementSize.second.mask()) {
        return UNKNOWN_SIZE;
      }
      return ByteCount{elementSize.first};
    }

    static constexpr ByteCount getIdBytes(ElementId id) noexcept {
      BitCount numBitsId{static_cast<uint32_t>(std::bit_width(static_cast<std::underlying_type_t<ElementId>>(id)))};
      return ByteCount{numBitsId / 1_bytes} + (numBitsId % 1_bytes ? 1_bytes : 0_bytes);
    }

    void writeElementHeader(BitWriter &writer, ElementId id, ByteCount elementSize) {
      writer.write(static_cast<std::underlying_type_t<ElementId>>(id), getIdBytes(id));
      writeVariableSizeInteger(writer, elementSize.value());
    }

    void readMasterElement(BitReader &reader, ElementId id,
                           const std::map<ElementId, std::function<void(BitReader &)>> &members,
                           const std::set<ElementId> &terminatingElementIds) {
      auto masterSize = readElementHeader(reader, id);
      const auto hasMoreMembers = [masterSize, startPos{reader.position()}](BitReader &r) {
        return (masterSize == UNKNOWN_SIZE && r.hasMoreBytes()) || (r.position() < startPos + masterSize);
      };

      while (hasMoreMembers(reader)) {
        auto memberId = static_cast<ElementId>(peekVariableSizeInteger(reader, true /* with prefix */));
        if (auto it = members.find(memberId); it != members.end()) {
          it->second(reader);
        } else if (masterSize == UNKNOWN_SIZE && terminatingElementIds.find(memberId) != terminatingElementIds.end()) {
          // end of element with unknown size
          return;
        } else {
          std::ignore /* skip ID */ = readVariableSizeInteger(reader, true /* don't care */);
          ByteCount elementSize{readVariableSizeInteger(reader, false /* without prefix */).first};
          Debug::error("No member element with ID '" + toHexString(static_cast<uintmax_t>(memberId)) +
                       "' for current master element '" + toHexString(static_cast<uintmax_t>(id)) +
                       "', skipping unknown element of size " + elementSize.toString());
          reader.skip(elementSize);
        }
      }
    }

    void writeMasterElement(BitWriter &writer, ElementId id,
                            const std::vector<std::pair<ElementId, std::function<void(BitWriter &)>>> &members) {

      writeElement(writer, id, [&members](BitWriter &bufWriter) {
        for (const auto &member : members) {
          member.second(bufWriter);
        }
      });
    }
  } // namespace detail

  void Void::read(bml::BitReader &reader) {
    skipBytes = detail::readElementHeader(reader, ID);
    reader.skip(skipBytes);
  }

  void Void::write(bml::BitWriter &writer) const {
    detail::writeElementHeader(writer, ID, skipBytes);
    writer.fillBytes(std::byte{0x00}, skipBytes);
  }

  std::ostream &Void::printYAML(std::ostream &os, const bml::yaml::Options &options) const {
    os << options.indentation(true /* first member */);
    return os << "skipBytes: " << skipBytes.num;
  }

  BML_EBML_DEFINE_IO(DocTypeExtension, crc32, docTypeExtensionName, docTypeExtensionVersion, voidElements)
  BML_YAML_DEFINE_PRINT(DocTypeExtension, crc32, docTypeExtensionName, docTypeExtensionVersion, voidElements)

  BML_EBML_DEFINE_IO(EBMLHeader, crc32, version, readVersion, maxIDLength, maxSizeLength, docType, docTypeVersion,
                     docTypeReadVersion, docTypeExtensions, voidElements)
  BML_YAML_DEFINE_PRINT(EBMLHeader, crc32, version, readVersion, maxIDLength, maxSizeLength, docType, docTypeVersion,
                        docTypeReadVersion, docTypeExtensions, voidElements)
} // namespace bml::ebml
