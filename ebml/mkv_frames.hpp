#pragma once

#include "ebml.hpp"
#include "mkv_common.hpp"

#include <functional>
#include <istream>
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
     * Range of the Frame's data relative to the start of the read byte source.
     */
    ByteRange dataRange;

    /**
     * The actual data bytes of this Frame.
     *
     * This member is only filled if the underlying Block Element was read with data.
     *
     * NOTE: Since this directly references the underlying Block Element's data member, care needs to be taken to not
     * access this member after freeing the underlying Block Element.
     */
    std::span<const std::byte> data;
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

  namespace detail {
    /**
     * Pair of the Frame with its data span filled and a (possibly empty) vector owning the storage of the Frame's data.
     */
    using FilledFrame = std::pair<Frame, std::vector<std::byte>>;

    FilledFrame fillFrame(Frame frame, std::span<const std::byte> data);
    FilledFrame fillFrame(Frame frame, std::istream &input);
  } // namespace detail

  /**
   * View for amending a Frame object read from the underlying input range with the Frame's data extracted from the
   * associated data source.
   *
   * NOTE: The produced Frame's data might directly reference the underlying data source (i.e. for in-memory data
   * buffers) and thus MUST NOT outlive the lifetime of the underlying data source.
   */
  template <std::ranges::input_range V, typename Source>
    requires(std::is_same_v<Frame, typename std::ranges::range_value_t<V>>)
  class FillFrameDataView : public std::ranges::view_interface<FillFrameDataView<V, Source>> {
    using InnerIterator = std::ranges::iterator_t<V>;

    struct Sentinel {
      std::ranges::sentinel_t<V> inner;
    };

    class Iterator {
    public:
      using value_type = detail::FilledFrame;
      using difference_type = std::ptrdiff_t;

      explicit Iterator() noexcept = default;
      Iterator(InnerIterator begin, Source source) noexcept : iterator(begin), dataSource(source) {}

      Iterator &operator++() {
        ++iterator;
        return *this;
      }

      Iterator operator++(int) {
        auto copy = *this;
        ++iterator;
        return copy;
      }

      value_type operator*() const { return detail::fillFrame(*iterator, dataSource); }

      bool operator==(const Iterator &other) const noexcept { return iterator == other.iterator; }

      friend bool operator==(const Sentinel &sentinel, const Iterator &it) noexcept {
        return it.iterator == sentinel.inner;
      }

    private:
      InnerIterator iterator;
      Source dataSource;
    };

  public:
    FillFrameDataView(V &&input, Source source) noexcept : inputRange(std::move(input)), dataSource(source) {}

    Iterator begin() const { return Iterator{inputRange.begin(), dataSource}; }
    Sentinel end() const { return Sentinel{inputRange.end()}; }

  private:
    V inputRange;
    Source dataSource;
  };

  /**
   * Range adaptor to produce a FillFrameDataView amending an input range using the additional Frame data source.
   */
  template <typename Source>
  struct FillFrameData {
    template <std::ranges::input_range V>
      requires(std::is_same_v<Frame, typename std::ranges::range_value_t<V>>)
    friend FillFrameDataView<V, Source> operator|(V &&range, const FillFrameData &view) noexcept {
      return FillFrameDataView<V, Source>{std::move(range), view.dataSource};
    }

    /**
     * Directly amend the given input frame with the Frame's data as provided by the associated data source.
     *
     * NOTE: The produced Frame's data might directly reference the underlying data source (i.e. for in-memory data
     * buffers) and thus MUST NOT outlive the lifetime of the underlying data source.
     */
    detail::FilledFrame operator()(Frame frame) { return detail::fillFrame(std::move(frame), dataSource); }

    Source dataSource;
  };

  /**
   * Returns a range adaptor using the given in-memory buffer to amend the Frame's data.
   */
  constexpr FillFrameData<std::span<const std::byte>> fillFrameData(std::span<const std::byte> allData) noexcept {
    return FillFrameData<std::span<const std::byte>>{allData};
  }

  /**
   * Returns a range adaptor using the given seekable input stream buffer to amend the Frame's data.
   */
  constexpr FillFrameData<std::reference_wrapper<std::istream>> fillFrameData(std::istream &input) noexcept {
    return FillFrameData<std::reference_wrapper<std::istream>>{std::ref(input)};
  }

} // namespace bml::ebml::mkv
