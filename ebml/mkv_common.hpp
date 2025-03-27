#pragma once

#include "ebml.hpp"

#include <array>
#include <cmath>

/**
 * Types for reading/writing the container structure of the Matroska (MKV) media container format.
 *
 * See: https://www.matroska.org/technical/elements.html
 */
namespace bml::ebml::mkv {

  /**
   * Block header fields.
   */
  struct BlockHeader {
    static constexpr ByteCount minNumBits() { return VariableSizeInteger::minNumBits() + 3_bytes; };
    static constexpr ByteCount maxNumBits() { return VariableSizeInteger::maxNumBits() + 3_bytes; }

    enum class Lacing : uint8_t {
      NONE = 0b00,
      XIPH = 0b01,
      EBML = 0b11,
      FIXED_SIZE = 0b10,
    };

    VariableSizeInteger trackNumber;
    SignedBytes<int16_t> timestampOffset;
    Bit keyframe;
    FixedBits<3, 0b0> reserved;
    Bit invisible;
    Bits<2, Lacing> lacing;
    Bit discardable;

    constexpr BlockHeader() noexcept = default;
    constexpr auto operator<=>(const BlockHeader &) const noexcept = default;
    constexpr ByteCount numBits() const noexcept { return trackNumber.numBits() + 3_bytes; }

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(BitWriter &writer) const;

    BML_DEFINE_PRINT(BlockHeader, trackNumber, timestampOffset, keyframe, reserved, invisible, lacing, discardable)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  namespace detail {
    /**
     * Base type for all Binary Elements storing an UUID.
     */
    struct BaseUUIDElement : public bml::ebml::detail::BaseSimpleElement<std::array<std::byte, 16>> {
      static constexpr auto NUM_BYTES = 16_bytes;

      using BaseSimpleElement::BaseSimpleElement;

      friend std::ostream &operator<<(std::ostream &os, const BaseUUIDElement &element);
      std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;

    protected:
      void readValue(BitReader &reader, ElementId id, const ReadOptions &options);
      void writeValue(BitWriter &writer, ElementId id) const;
    };

    /**
     * Base type for all Block Element types to provide common read/write functionality.
     */
    struct BaseBlockElement : MasterElement {
      /**
       * The block header data.
       */
      BlockHeader header;

      /**
       * Ranges of the contained frame data.
       *
       * NOTE: If the frame data ranges are BORROWED, the underlying in-memory raw bytes need to remain valid as long as
       * these frame data ranges are accessed.
       *
       * NOTE: This member can only be written back if the contained data ranges do directly reference their represented
       * raw bytes (i.e. for BORROWED or OWNED data range modes).
       */
      std::vector<DataRange> frameDataRanges;

      constexpr auto operator<=>(const BaseBlockElement &) const noexcept = default;

      BML_DEFINE_PRINT(BaseBlockElement, crc32, header, frameDataRanges, voidElements)
      std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;

    protected:
      void readValue(bml::BitReader &reader, ElementId id, const ReadOptions &options);
      void writeValue(bml::BitWriter &writer, ElementId id) const;
    };

    struct MatroskaClock {
      static std::ostream &print(std::ostream &os, uintmax_t val) { return os << val << " ns"; }
    };

    struct SegmentClock {
      using parent_clock = MatroskaClock;

      template <typename T>
      static std::ostream &print(std::ostream &os, T val) {
        return os << val << " segment ticks";
      }
    };

    struct TrackClock {
      using parent_clock = SegmentClock;

      template <typename T>
      static std::ostream &print(std::ostream &os, T val) {
        return os << val << " track ticks";
      }
    };

    /**
     * Common type for all timestamps based on the different clocks as specified for Matroska.
     */
    template <typename Clock, typename T = uintmax_t>
    struct Timestamp {
      using value_type = T;

      constexpr auto operator<=>(const Timestamp &) const noexcept = default;

      /**
       * "For such elements, the timestamp value is stored directly in nanoseconds."
       */
      operator std::chrono::nanoseconds() const noexcept
        requires(std::is_same_v<Clock, MatroskaClock>)
      {
        return std::chrono::nanoseconds{value};
      }

      std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const {
        return bml::yaml::print(os, options, value);
      }

      friend std::ostream &operator<<(std::ostream &os, const Timestamp &timestamp) {
        return Clock::print(os, timestamp.value);
      }

      value_type value;
    };

    /**
     * Common type for all timescales to convert between the different clocks as specified for Matroska.
     */
    template <typename Clock, typename T = uintmax_t>
    struct Timescale {
      using value_type = T;

      constexpr auto operator<=>(const Timescale &) const noexcept = default;

      template <typename U>
      friend Timestamp<typename Clock::parent_clock> operator*(const Timestamp<Clock, U> &timestamp,
                                                               const Timescale &timescale) noexcept {
        using common_type = std::common_type_t<U, T>;
        auto tmp = static_cast<common_type>(timestamp.value) * static_cast<common_type>(timescale.value);
        return {static_cast<Timestamp<typename Clock::parent_clock>::value_type>(std::round(tmp))};
      }

      friend Timestamp<Clock> operator/(const Timestamp<typename Clock::parent_clock> &timestamp,
                                        const Timescale &timescale) noexcept {
        using common_type = std::common_type_t<typename Timestamp<typename Clock::parent_clock>::value_type, T>;
        auto tmp = static_cast<common_type>(timestamp.value) / static_cast<common_type>(timescale.value);
        return {static_cast<Timestamp<Clock>::value_type>(std::round(tmp))};
      }

      std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const {
        return bml::yaml::print(os, options, value);
      }

      friend std::ostream &operator<<(std::ostream &os, const Timescale &timescale) {
        return Clock::parent_clock::print(os, timescale.value);
      }

      value_type value;
    };

    template <ElementId Id, typename T, T::value_type Default = {}>
    struct TimeElement : public bml::ebml::detail::BaseSimpleElement<T> {
      using Base = bml::ebml::detail::BaseSimpleElement<T>;

      static constexpr ElementId ID = Id;
      static constexpr T DEFAULT{Default};
      static constexpr ByteCount maxNumBits() { return bml::ebml::detail::calcElementSize(ID, ByteCount{sizeof(T)}); }

      constexpr TimeElement() noexcept : Base(DEFAULT) {}
      explicit constexpr TimeElement(T val) noexcept : Base(val) {}

      constexpr ByteCount numBits() const noexcept {
        return bml::ebml::detail::calcElementSize(ID, bml::ebml::detail::requiredBytes(this->value.value),
                                                  this->value == DEFAULT);
      }

      TimeElement &operator=(T val) {
        this->value = val;
        return *this;
      }

      void read(bml::BitReader &reader, const ReadOptions & = {}) { Base::readValue(reader, ID, DEFAULT); }
      void write(BitWriter &writer) const { Base::writeValue(writer, ID, DEFAULT); }
    };
  } // namespace detail

  using MatroskaTimestamp = detail::Timestamp<detail::MatroskaClock, uint64_t>;
  template <typename T = uintmax_t>
  using SegmentTimestamp = detail::Timestamp<detail::SegmentClock, T>;
  template <typename T = uintmax_t>
  using TrackTimestamp = detail::Timestamp<detail::TrackClock, T>;

  using SegmentTimescale = detail::Timescale<detail::SegmentClock, uintmax_t>;
  using TrackTimescale = detail::Timescale<detail::TrackClock, float>;

  ////
  // Element Types
  ////

  template <ElementId Id>
  using MatroskaTimestampElement = detail::TimeElement<Id, MatroskaTimestamp>;
  template <ElementId Id, typename T = uintmax_t>
  using SegmentTimestampElement = detail::TimeElement<Id, SegmentTimestamp<T>>;
  template <ElementId Id, typename T = uintmax_t>
  using TrackTimestampElement = detail::TimeElement<Id, TrackTimestamp<T>>;

  template <ElementId Id, uintmax_t Default>
  using SegmentTimescaleElement = detail::TimeElement<Id, SegmentTimescale, Default>;
  template <ElementId Id, float Default>
  using TrackTimescaleElement = detail::TimeElement<Id, TrackTimescale, Default>;

  template <ElementId Id>
  struct UUIDElement : public detail::BaseUUIDElement {
    static constexpr ElementId ID = Id;
    static constexpr ByteCount minNumBits() { return bml::ebml::detail::calcElementSize(ID, NUM_BYTES); };
    static constexpr ByteCount maxNumBits() { return bml::ebml::detail::calcElementSize(ID, NUM_BYTES); }
    constexpr ByteCount numBits() const noexcept { return bml::ebml::detail::calcElementSize(ID, NUM_BYTES); }

    UUIDElement &operator=(std::array<std::byte, 16> val) {
      value = std::move(val);
      return *this;
    }

    void read(BitReader &reader, const ReadOptions &options) { BaseUUIDElement::readValue(reader, ID, options); }
    void write(BitWriter &writer) const { BaseUUIDElement::writeValue(writer, ID); }
  };
} // namespace bml::ebml::mkv
