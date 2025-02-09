
#include "ebml.hpp"

#include "debug.hpp"

#include <array>
#include <iomanip>
#include <span>
#include <sstream>

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

    static ElementId peekElementId(BitReader &reader) {
      static_assert(sizeof(uintmax_t) <= 8, "The current implementation only works for up to 8 bytes read");
      auto prefix = static_cast<uint8_t>(reader.peek(1_bytes));
      auto numBytes = static_cast<uint32_t>(std::countl_zero(prefix) + 1 /* 1 bit */);
      return static_cast<ElementId>(reader.peek(ByteCount{numBytes}));
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

    /**
     * Variable-size integer consists of 3 parts:
     *
     * - N consecutive 0 bits
     * - 1 single 1 bit
     * - (N+1) 7-bit blocks of actual integer data
     */
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

    ByteCount skipElement(BitReader &reader, const std::set<ElementId> &terminatingElementIds) {
      auto id = readVariableSizeInteger(reader, true /* don't care */);
      auto size = readVariableSizeInteger(reader, false /* without prefix */);
      if (size.first == size.second.mask()) {
        ByteCount numBytesSkipped = id.second.divide<8>() + ByteCount{size.second.num / 7U};
        while (reader.hasMoreBytes() && !terminatingElementIds.contains(peekElementId(reader))) {
          numBytesSkipped += skipElement(reader, terminatingElementIds);
        }
        return numBytesSkipped;
      }
      reader.skip(ByteCount{size.first});
      return id.second.divide<8>() + ByteCount{size.second.num / 7U} + ByteCount{size.first};
    }

    ByteCount copyElement(BitReader &reader, BitWriter &writer, const std::set<ElementId> &terminatingElementIds) {
      auto id = readVariableSizeInteger(reader, true /* don't care */);
      auto size = readVariableSizeInteger(reader, false /* without prefix */);
      if (size.first == size.second.mask()) {
        writer.write(id.first, id.second);
        writer.write(UNKNOWN_SIZE.num, ByteCount{size.second.num / 7U});
        ByteCount numBytesCopied = id.second.divide<8>() + ByteCount{size.second.num / 7U};
        while (reader.hasMoreBytes() && !terminatingElementIds.contains(peekElementId(reader))) {
          numBytesCopied += copyElement(reader, writer, terminatingElementIds);
        }
        return numBytesCopied;
      }
      writeElementHeader(writer, ElementId{id.first}, ByteCount{size.first});
      bml::copyBits(reader, writer, ByteCount{size.first});
      return id.second.divide<8>() + ByteCount{size.second.num / 7U} + ByteCount{size.first};
    }

    /**
     * RAII scope for calculating and validating the CRC-32 value of a Master Element while reading from a BitReader.
     */
    class CRCScope {
    public:
      explicit CRCScope(BitReader &reader, bool calculateCRC) {
        // Top-level CRC reader scope, initialize base underlying reader
        if (!underlyingReader) {
          underlyingReader = &reader;
        }
        ++numOpenScopes;

        if (calculateCRC) {
          wrappingReader = BitReader{[&, this](std::byte &out) {
            // peek in the base underlying reader to not fill the cache of the parent CRC-calculating reader.
            if (!underlyingReader || !underlyingReader->hasMoreBytes()) {
              return false;
            }
            // read from the direct parent reader to transitively read from all parent CRC-calculating readers.
            out = reader.readByte();
            processNextByte(out);
            return true;
          }};
        } else {
          // Simply wrap the base reader
          wrappingReader = BitReader{[](std::byte &out) {
            if (!underlyingReader || !underlyingReader->hasMoreBytes()) {
              return false;
            }
            out = underlyingReader->readByte();
            return true;
          }};
        }
      }

      CRCScope(const CRCScope &) = delete;
      CRCScope(CRCScope &&) noexcept = delete;

      ~CRCScope() noexcept {
        --numOpenScopes;
        if (numOpenScopes == 0) {
          // Top-level CRC reader scope, clean up base underlying reader
          underlyingReader = nullptr;
        }
      }

      CRCScope &operator=(const CRCScope &) = delete;
      CRCScope &operator=(CRCScope &&) noexcept = delete;

      BitReader &peeker() { return *underlyingReader; }
      BitReader &reader() { return wrappingReader; }

      void validateCRC(const MasterElement &master) const {
        auto finalCRC32 = ~crc32;
        if (master.crc32 && finalCRC32 != master.crc32->get()) {
          std::stringstream ss;
          CRC32 dummy{finalCRC32};
          ss << "CRC-32 Element with value '" << *master.crc32 << "' does not match calculated CRC-32: " << dummy;
          throw std::runtime_error{ss.str()};
        }
      }

    private:
      void processNextByte(std::byte b) {
        auto val = static_cast<uint32_t>(b);
        crc32 = CRC_TABLE[(crc32 ^ val) & 0xFF] ^ (crc32 >> 8);
      }

    private:
      uint32_t crc32 = 0xFFFFFFFF;
      BitReader wrappingReader;

      // The original non CRC-calculating BitReader, used for peeking
      static thread_local BitReader *underlyingReader;
      static thread_local uint64_t numOpenScopes;
      friend BitReader &getUnderlyingReader(BitReader &);

      // Adapted from https://rosettacode.org/wiki/CRC-32#C++
      static constexpr auto CRC_TABLE = [] {
        std::array<std::uint32_t, 256> table;
        for (uint32_t i = 0; i < table.size(); ++i) {
          uint32_t c = i;
          for (uint8_t k = 0; k < 8; ++k) {
            if (c & 1) {
              // IEEE-CRC-32 algorithm, specified in ISO 3309 or ITU-T V.42
              // This is the "reversed" (little-endian) form as of
              // https://en.wikipedia.org/wiki/Cyclic_redundancy_check
              c = 0xEDB88320U ^ (c >> 1);
            } else {
              c >>= 1;
            }
          }
          table[i] = c;
        }
        return table;
      }();

      // See https://github.com/Matroska-Org/libebml/blob/master/src/EbmlCrc32.cpp
      static_assert(CRC_TABLE[7] == 0x9e6495a3L);
    };

    thread_local uint64_t CRCScope::numOpenScopes = 0;
    thread_local BitReader *CRCScope::underlyingReader = nullptr;

    BitReader &getUnderlyingReader(BitReader &reader) {
      if (CRCScope::underlyingReader) {
        return *CRCScope::underlyingReader;
      }
      return reader;
    }

    void readMasterElement(BitReader &reader, ElementId id, MasterElement &master, const ReadOptions &options,
                           const std::map<ElementId, std::function<void(BitReader &, const ReadOptions &)>> &members,
                           const std::set<ElementId> &terminatingElementIds) {
      auto masterSize = readElementHeader(reader, id);
      CRCScope crcScope{reader, options.validateCRC32};

      const auto hasMoreMembers = [masterSize, startPos{crcScope.peeker().position()}](BitReader &r) {
        bool pendingBytes = (masterSize == UNKNOWN_SIZE && r.hasMoreBytes()) || (r.position() < startPos + masterSize);
        return pendingBytes;
      };

      // CRC-32 Element (if present) MUST BE the first Element
      if (options.validateCRC32 && hasMoreMembers(crcScope.peeker())) {
        auto crcId = peekElementId(crcScope.peeker());
        if (auto it = members.find(crcId); crcId == CRC32::ID && it != members.end()) {
          // don't read with CRC scope, since the CRC-32 itself is not part of its calculated value
          // but read with given BitReader, which includes the CRC scopes of any parent Element.
          it->second(reader, options);
        }
      }

      while (hasMoreMembers(crcScope.peeker())) {
        auto memberId = peekElementId(crcScope.peeker());
        if (auto it = members.find(memberId); it != members.end()) {
          it->second(crcScope.reader(), options);
        } else if (masterSize == UNKNOWN_SIZE && terminatingElementIds.contains(memberId)) {
          // end of element with unknown size
          break;
        } else {
          std::ignore /* skip ID */ = readVariableSizeInteger(crcScope.reader(), true /* don't care */);
          ByteCount elementSize{readVariableSizeInteger(crcScope.reader(), false /* without prefix */).first};
          Debug::error("No member element with ID '" + toHexString(static_cast<uintmax_t>(memberId)) +
                       "' for current master element '" + toHexString(static_cast<uintmax_t>(id)) +
                       "', skipping unknown element of size " + elementSize.toString());
          crcScope.reader().skip(elementSize);
        }
      }

      if (options.validateCRC32) {
        crcScope.validateCRC(master);
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

  void VariableSizeInteger::read(bml::BitReader &reader, const ReadOptions &) {
    value = detail::readVariableSizeInteger(reader, false).first;
  }

  void VariableSizeInteger::write(BitWriter &writer) const { detail::writeVariableSizeInteger(writer, value); }

  // Contrary to the rest of the EBML values, the CRC-32 is stored in little endian!
  void CRC32::read(bml::BitReader &reader, const ReadOptions &) {
    auto numBytes = detail::readElementHeader(reader, ID);
    if (!numBytes) {
      return;
    }
    value = 0;
    for (auto b = 0_bytes; b < numBytes; ++b) {
      auto tmp = static_cast<uint32_t>(reader.readByte());
      value = value | (tmp << b);
    }
  }

  void CRC32::write(BitWriter &writer) const {
    detail::writeElementHeader(writer, ID, 4_bytes);
    const auto BYTE = 1_bytes;
    auto tmp = value;
    writer.writeByte(static_cast<std::byte>(tmp & BYTE.mask()));
    tmp >>= BYTE;
    writer.writeByte(static_cast<std::byte>(tmp & BYTE.mask()));
    tmp >>= BYTE;
    writer.writeByte(static_cast<std::byte>(tmp & BYTE.mask()));
    tmp >>= BYTE;
    writer.writeByte(static_cast<std::byte>(tmp & BYTE.mask()));
  }

  std::ostream &operator<<(std::ostream &os, const CRC32 &element) {
    return os << "0x" << std::hex << std::setfill('0') << std::setw(8) << element.value << std::dec;
  }

  std::ostream &CRC32::printYAML(std::ostream &os, const bml::yaml::Options &options) const {
    if (options.prefixSpace) {
      os << ' ';
    }
    return os << *this;
  }

  void Void::read(bml::BitReader &reader, const ReadOptions &) {
    skipBytes = detail::readElementHeader(reader, ID);
    reader.skip(skipBytes);
  }

  void Void::write(bml::BitWriter &writer) const {
    detail::writeElementHeader(writer, ID, skipBytes);
    writer.fillBytes(std::byte{0x00}, skipBytes);
  }

  void Void::skip(BitReader &reader) { detail::skipElement(reader); }
  void Void::copy(BitReader &reader, BitWriter &writer) { detail::copyElement(reader, writer); }

  std::ostream &Void::printYAML(std::ostream &os, const bml::yaml::Options &options) const {
    os << options.indentation(true /* first member */);
    return os << "skipBytes: " << skipBytes.num;
  }

  void MasterElement::skip(BitReader &reader) { detail::skipElement(reader); }
  void MasterElement::copy(BitReader &reader, BitWriter &writer) { detail::copyElement(reader, writer); }

  BML_EBML_DEFINE_IO(DocTypeExtension, crc32, docTypeExtensionName, docTypeExtensionVersion, voidElements)
  BML_YAML_DEFINE_PRINT(DocTypeExtension, crc32, docTypeExtensionName, docTypeExtensionVersion, voidElements)

  BML_EBML_DEFINE_IO(EBMLHeader, crc32, version, readVersion, maxIDLength, maxSizeLength, docType, docTypeVersion,
                     docTypeReadVersion, docTypeExtensions, voidElements)
  BML_YAML_DEFINE_PRINT(EBMLHeader, crc32, version, readVersion, maxIDLength, maxSizeLength, docType, docTypeVersion,
                        docTypeReadVersion, docTypeExtensions, voidElements)
} // namespace bml::ebml
