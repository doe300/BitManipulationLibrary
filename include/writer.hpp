#pragma once

#include "sizes.hpp"

#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <variant>

namespace bml {
  class BitWriter {
  public:
    using ByteConsumer = std::function<bool(std::byte)>;
    struct ByteRange {
      std::byte *begin;
      std::byte *end;
    };

    using ByteSink = std::variant<std::monostate, ByteConsumer, ByteRange>;

    static constexpr ByteCount BYTE_SIZE{1};
    static constexpr BitCount CACHE_SIZE{static_cast<std::size_t>(std::numeric_limits<std::uintmax_t>::digits)};

    BitWriter() noexcept = default;
    BitWriter(ByteConsumer &&consumer) noexcept : sink(std::move(consumer)) {}
    BitWriter(uint8_t *begin, uint8_t *end) noexcept
        : sink(ByteRange{reinterpret_cast<std::byte *>(begin), reinterpret_cast<std::byte *>(end)}) {}
    BitWriter(std::byte *begin, std::byte *end) noexcept : sink(ByteRange{begin, end}) {}

    BitWriter(std::span<uint8_t> range) noexcept
        : sink(ByteRange{reinterpret_cast<std::byte *>(range.data()),
                         reinterpret_cast<std::byte *>(range.data() + range.size())}) {}
    BitWriter(std::span<std::byte> range) noexcept : sink(ByteRange{range.data(), range.data() + range.size()}) {}

    BitCount position() const noexcept { return bytesWritten + cacheSize; }

    BitCount fillToAligment(BitCount bitAlignment, bool bit);
    void assertAlignment(BitCount bitAlignment);

    void write(bool bit) { write(bit ? 1u : 0u, 1_bits); }

    void write(std::uintmax_t value, BitCount numBits);

    void writeBytes(std::uintmax_t value, ByteCount numBytes);
    void writeByte(std::byte byte) { writeBytes(static_cast<uint8_t>(byte), BYTE_SIZE); }
    void writeBytes(std::span<const std::byte> bytes);

    void writeExpGolomb(std::uintmax_t value);

    void writeSignedExpGolomb(std::intmax_t value);

  private:
    void flushFullBytes();

  private:
    ByteCount bytesWritten{};
    BitCount cacheSize{};
    /** is always left-adjusted to write the highest bits first */
    std::uintmax_t cache = 0;
    ByteSink sink;
  };

} // namespace bml
