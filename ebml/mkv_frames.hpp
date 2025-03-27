#pragma once

#include "ebml.hpp"
#include "mkv_common.hpp"

#include <functional>
#include <optional>
#include <ranges>
#include <span>

/**
 * Types for reading/writing the container structure of the Matroska (MKV) media container format.
 *
 * See: https://www.matroska.org/technical/elements.html
 */
namespace bml::ebml::mkv {

  struct Cluster;

  /**
   * A Frame of data of a single Track, as defined in the Matroska Block specification.
   */
  struct Frame {
    /**
     * The timestamp of this Frame in the associated Track's timescale.
     *
     * For laced Frames, only the first Frame in the Block has its timestamp set.
     */
    std::optional<TrackTimestamp<>> timestamp;

    /**
     * The Frame's data.
     *
     * Whether this data range directly references/holds the underlying raw bytes or just tracks their position relative
     * to the start of the read byte source depends on whether the underlying Block Element was read with data.
     *
     * NOTE: Since this mighty directly references the underlying Block Element's data member, care needs to be taken to
     * not access this member after freeing the underlying Block Element.
     */
    DataRange data;
  };

  /**
   * Read-only view for accessing Frames of a specific Track within a Matroska object.
   *
   * NOTE: Since this object reads from the originating Matroska object, it SHOULD NOT be modified/moved in the
   * meantime!
   */
  class FrameView : public std::ranges::view_base {
    struct Sentinel {};

    class Iterator {
    public:
      using value_type = Frame;
      using difference_type = std::ptrdiff_t;

      explicit Iterator() noexcept = default;
      Iterator(std::span<const Cluster> clusters, uint32_t trackNum, const TrackTimescale &scale,
               const TrackTimestamp<> &start) noexcept;

      Iterator &operator++() noexcept {
        advance();
        return *this;
      }

      Iterator operator++(int) noexcept {
        auto copy = *this;
        advance();
        return copy;
      }

      Frame operator*() const noexcept;

      bool operator==(const Iterator &other) const noexcept;

    private:
      void advance() noexcept;

      friend bool operator==(const Sentinel &, const Iterator &it) noexcept { return it.pendingClusters.empty(); }

    private:
      std::span<const Cluster> pendingClusters;
      const detail::BaseBlockElement *currentBlock;
      TrackTimescale timescale;
      uint32_t trackNumber;
      uint8_t currentLaceIndex;
    };

  public:
    constexpr FrameView(std::span<const Cluster> allClusters, uint32_t track, const TrackTimescale &scale,
                        const TrackTimestamp<> &start) noexcept
        : clusters(allClusters), trackNumber(track), timescale(scale), startTime(start) {}

    Iterator begin() const noexcept { return Iterator{clusters, trackNumber, timescale, startTime}; }
    constexpr Sentinel end() const noexcept { return Sentinel{}; }

  private:
    std::span<const Cluster> clusters;
    uint32_t trackNumber;
    TrackTimescale timescale;
    TrackTimestamp<> startTime;
  };

  /**
   * Returns a range adaptor using the given in-memory buffer to amend the Frame's data.
   */
  constexpr FillDataRange<Frame, std::span<const std::byte>>
  fillFrameData(std::span<const std::byte> allData) noexcept {
    return FillDataRange<Frame, std::span<const std::byte>>{allData, &Frame::data};
  }

  /**
   * Returns a range adaptor using the given seekable input stream buffer to amend the Frame's data.
   */
  constexpr FillDataRange<Frame, std::reference_wrapper<std::istream>> fillFrameData(std::istream &input) noexcept {
    return FillDataRange<Frame, std::reference_wrapper<std::istream>>{std::ref(input), &Frame::data};
  }

} // namespace bml::ebml::mkv
