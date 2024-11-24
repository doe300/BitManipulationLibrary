#pragma once

#include "sizes.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>
#include <variant>

namespace bml {

  class BitReader {
  public:
    using ByteGenerator = std::function<bool(std::byte &)>;
    struct ByteRange {
      const std::byte *begin;
      const std::byte *end;
    };

    using ByteSource = std::variant<std::monostate, ByteGenerator, ByteRange>;

    static constexpr ByteCount BYTE_SIZE{1};
    static constexpr BitCount CACHE_SIZE{static_cast<std::size_t>(std::numeric_limits<std::uintmax_t>::digits)};

    BitReader() noexcept = default;
    BitReader(ByteGenerator &&generator) noexcept : source(std::move(generator)) {}
    BitReader(const uint8_t *begin, const uint8_t *end) noexcept
        : source(ByteRange{reinterpret_cast<const std::byte *>(begin), reinterpret_cast<const std::byte *>(end)}) {}
    BitReader(const std::byte *begin, const std::byte *end) noexcept : source(ByteRange{begin, end}) {}

    BitReader(std::span<const uint8_t> range) noexcept
        : source(ByteRange{reinterpret_cast<const std::byte *>(range.data()),
                           reinterpret_cast<const std::byte *>(range.data() + range.size())}) {}
    BitReader(std::span<const std::byte> range) noexcept
        : source(ByteRange{range.data(), range.data() + range.size()}) {}

    /**
     * Returns the number of bits already read.
     *
     * if the start of the bit source is properly aligned, this can also be used to determine the current alignment.
     */
    BitCount position() const noexcept { return bytesRead - cacheSize; }
    bool hasMoreBytes() noexcept;

    BitCount skipToAligment(BitCount bitAlignment);
    void assertAlignment(BitCount bitAlignment);

    bool read();

    std::uintmax_t peek(BitCount numBits);
    std::uintmax_t read(BitCount numBits);

    template <typename T>
    T read(BitCount numBits)
      requires(std::is_unsigned_v<T> || std::is_enum_v<T>)
    {
      return static_cast<T>(read(numBits));
    }

    std::uintmax_t readBytes(ByteCount numBytes);
    std::byte readByte() { return static_cast<std::byte>(readBytes(1_bytes)); }

    template <typename T>
    T readBytes(ByteCount numBytes)
      requires(std::is_unsigned_v<T> || std::is_enum_v<T>)
    {
      return static_cast<T>(readBytes(numBytes));
    }

    void readBytesInto(std::span<std::byte> outBytes);

    std::uintmax_t readExpGolomb();

    template <typename T>
    T readExpGolomb()
      requires(std::is_unsigned_v<T>)
    {
      return static_cast<T>(readExpGolomb());
    }

    std::intmax_t readSignedExpGolomb();

    template <typename T>
    T readSignedExpGolomb()
      requires(std::is_signed_v<T>)
    {
      return static_cast<T>(readSignedExpGolomb());
    }

    void skip(BitCount numBits);

  private:
    void makeAvailable(BitCount numBits, bool throwOnEos = true);

    /** read until the the uppermost bit in the cache is set return the number of leading zero bits */
    BitCount readLeadingZeroes();

  private:
    ByteCount bytesRead{};
    BitCount cacheSize{};
    /** is always left-adjusted to read the highest bits first */
    std::uintmax_t cache = 0;
    ByteSource source;
  };

} // namespace bml
