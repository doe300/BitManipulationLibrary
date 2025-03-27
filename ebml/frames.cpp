#include "mkv.hpp"

#include "errors.hpp"
#include "internal.hpp"
#include "mkv_frames.hpp"

#include <algorithm>
#include <limits>

namespace bml::ebml::mkv {

  static_assert(std::ranges::input_range<FrameView>);
  static_assert(!std::ranges::borrowed_range<FrameView>);
  static_assert(std::ranges::view<FrameView>);

  static_assert(std::ranges::input_range<FillDataRangeView<FrameView, std::reference_wrapper<std::istream>>>);
  static_assert(!std::ranges::borrowed_range<FillDataRangeView<FrameView, std::reference_wrapper<std::istream>>>);
  static_assert(std::ranges::view<FillDataRangeView<FrameView, std::reference_wrapper<std::istream>>>);

  static void advanceToNextBlockForTrack(std::span<const Cluster> &clusters,
                                         detail::BaseBlockElement const *&currentBlock, uint32_t trackNumber) noexcept;
  static void advanceToFirstBlockForTrack(std::span<const Cluster> &clusters,
                                          detail::BaseBlockElement const *&currentBlock, uint32_t trackNumber,
                                          const TrackTimestamp<> &start, const TrackTimescale &scale) noexcept;
  [[nodiscard]] static bool advanceToNextLaceIndex(const detail::BaseBlockElement &block, uint8_t &laceIndex);

  FrameView::Iterator::Iterator(std::span<const Cluster> clusters, uint32_t trackNum, const TrackTimescale &scale,
                                const TrackTimestamp<> &start) noexcept
      : pendingClusters(clusters), currentBlock(nullptr), timescale(scale), trackNumber(trackNum), currentLaceIndex(0) {
    static_assert(sizeof(FrameView::Iterator) <= 5 * sizeof(std::size_t));
    static_assert(std::input_iterator<FrameView::Iterator>);
    static_assert(std::sentinel_for<Sentinel, Iterator>);

    advanceToFirstBlockForTrack(pendingClusters, currentBlock, trackNumber, start, scale);
  }

  static TrackTimestamp<> getBlockTimestamp(const Cluster &cluster, const detail::BaseBlockElement &block,
                                            const TrackTimescale &scale) noexcept {
    auto timestampValue = static_cast<intmax_t>((cluster.timestamp.get() / scale).value) + block.header.timestampOffset;
    return TrackTimestamp<>{static_cast<uintmax_t>(timestampValue)};
  }

  Frame FrameView::Iterator::operator*() const noexcept {
    if (pendingClusters.empty() || !currentBlock) {
      return {};
    }

    std::optional<TrackTimestamp<>> timestamp{};
    if (currentLaceIndex == 0) {
      timestamp = getBlockTimestamp(pendingClusters.front(), *currentBlock, timescale);
    }

    auto range = currentBlock->frameDataRanges.at(currentLaceIndex).borrow();
    return {timestamp, range};
  }

  bool FrameView::Iterator::operator==(const Iterator &other) const noexcept {
    return trackNumber == other.trackNumber && pendingClusters.size() == other.pendingClusters.size() &&
           pendingClusters.data() == other.pendingClusters.data() && currentBlock == other.currentBlock;
  }

  void FrameView::Iterator::advance() noexcept {
    if (!pendingClusters.empty() && !(currentBlock && advanceToNextLaceIndex(*currentBlock, currentLaceIndex))) {
      currentLaceIndex = 0;
      advanceToNextBlockForTrack(pendingClusters, currentBlock, trackNumber);
    }
  }

  static DataRange readDataRange(BitReader &reader, ByteRange range, bool copyFrameData) {
    if (copyFrameData) {
      std::vector<std::byte> tmp(range.size.num);
      reader.readBytesInto(tmp);
      return tmp;
    }
    reader.skip(range.size);
    return range;
  }

  static std::vector<DataRange> readFixedSizeFrameRanges(BitReader &reader, std::size_t numFrames,
                                                         const ByteRange &dataRange, bool copyFrameData) {
    auto frameSize = dataRange.size / numFrames;
    std::vector<DataRange> frames{};
    frames.reserve(numFrames);
    for (std::size_t i = 0; i < numFrames; ++i) {
      frames.push_back(readDataRange(reader, dataRange.subRange(i * frameSize, frameSize), copyFrameData));
    }
    return frames;
  }

  static std::vector<DataRange> extractFrameRanges(BitReader &reader, const std::vector<std::size_t> &frameSizesMinus1,
                                                   ByteRange dataRange, bool copyFrameData) {
    std::vector<DataRange> frames{};
    frames.reserve(frameSizesMinus1.size() + 1U);
    for (auto size : frameSizesMinus1) {
      frames.push_back(readDataRange(reader, dataRange.subRange(0_bytes, ByteCount{size}), copyFrameData));
      dataRange = dataRange.subRange(ByteCount{size});
    }
    frames.push_back(readDataRange(reader, dataRange, copyFrameData));
    return frames;
  }

  static std::vector<DataRange> readEBMLFrameRanges(BitReader &reader, const ByteRange &dataRange, bool copyFrameData) {
    auto start = reader.position();
    auto numFramesMinus1 = reader.readBytes<uint8_t>(1_bytes);

    /*
     * "The first frame size is encoded as an EBML Variable-Size Integer value [...]. The remaining frame sizes are
     * encoded as signed values using the difference between the frame size and the previous frame size. These signed
     * values are encoded as VINT, with a mapping from signed to unsigned numbers. Decoding the unsigned number stored
     * in the VINT to a signed number is done by subtracting 2^((7*n)-1)^-1, where n is the octet size of the VINT."
     */
    std::vector<std::size_t> frameSizesMinus1{};
    frameSizesMinus1.reserve(numFramesMinus1);
    frameSizesMinus1.push_back(static_cast<std::size_t>(ebml::detail::readVariableSizeInteger(reader, false).first));
    for (uint32_t i = 0; i < (numFramesMinus1 - 1U); ++i) {
      auto [val, numBits] = ebml::detail::readVariableSizeInteger(reader, false);
      auto difference = static_cast<intmax_t>(val) - ((1LL << (numBits - 1_bits)) - 1);
      frameSizesMinus1.push_back(static_cast<std::size_t>(static_cast<intmax_t>(frameSizesMinus1.back()) + difference));
    }

    auto remainingRange = dataRange.subRange((reader.position() - start).divide<8>());
    return extractFrameRanges(reader, frameSizesMinus1, remainingRange, copyFrameData);
  }

  static std::size_t readXiphSize(BitReader &reader) {
    std::size_t value = 0;

    while (reader.hasMoreBytes()) {
      auto tmp = reader.readBytes(1_bytes);
      value += tmp;
      if (tmp != 0xFF) {
        break;
      }
    }
    return value;
  }

  static std::vector<DataRange> readXiphFrameRanges(BitReader &reader, const ByteRange &dataRange, bool copyFrameData) {
    auto start = reader.position();
    auto numFramesMinus1 = reader.readBytes<uint8_t>(1_bytes);

    std::vector<std::size_t> frameSizesMinus1{};
    frameSizesMinus1.reserve(numFramesMinus1);
    for (uint32_t i = 0; i < numFramesMinus1; ++i) {
      frameSizesMinus1.push_back(readXiphSize(reader));
    }

    auto remainingRange = dataRange.subRange((reader.position() - start).divide<8>());
    return extractFrameRanges(reader, frameSizesMinus1, remainingRange, copyFrameData);
  }

  std::vector<DataRange> readFrameRanges(BitReader &reader, const BlockHeader &header, const ByteRange &dataRange,
                                         bool copyFrameData) {
    switch (header.lacing) {
    case BlockHeader::Lacing::NONE:
      return readFixedSizeFrameRanges(reader, 1U, dataRange, copyFrameData);
    case BlockHeader::Lacing::FIXED_SIZE:
      return readFixedSizeFrameRanges(reader, 1U + reader.readBytes<uint8_t>(1_bytes), dataRange.subRange(1_bytes),
                                      copyFrameData);
    case BlockHeader::Lacing::EBML:
      return readEBMLFrameRanges(reader, dataRange, copyFrameData);
    case BlockHeader::Lacing::XIPH:
      return readXiphFrameRanges(reader, dataRange, copyFrameData);
    }
    return {};
  }

  static void writeXiphSize(BitWriter &writer, std::size_t value) {
    while (value > 255) {
      writer.writeByte(std::byte{0xFF});
      value -= 255U;
    }
    writer.writeBytes(value, 1_bytes);
  }

  void writeFrameRanges(BitWriter &writer, const std::vector<DataRange> &frameRanges, BlockHeader::Lacing lacing) {
    if (!std::all_of(frameRanges.begin(), frameRanges.end(), [](const DataRange &range) { return range.hasData(); })) {
      throw NoReferencedDataError{"Cannot write Frame without referenced data"};
    }

    if (lacing != BlockHeader::Lacing::NONE) {
      writer.writeBytes(frameRanges.size() - 1U, 1_bytes);
    }

    if (lacing == BlockHeader::Lacing::EBML) {
      VariableSizeInteger tmp{frameRanges.front().size()};
      tmp.write(writer);
      for (std::size_t i = 1; i < frameRanges.size() - 1U; ++i) {
        auto difference =
            static_cast<intmax_t>(frameRanges[i].size()) - static_cast<intmax_t>(frameRanges[i - 1U].size());
        BitCount numBits{std::bit_ceil(static_cast<uintmax_t>(std::abs(difference)))};
        numBits = ((numBits + 6_bits) / 7_bits) * 7_bits;
        tmp.set(static_cast<uintmax_t>(difference + ((1LL << (numBits - 1_bits)) - 1)));
        tmp.write(writer);
      }
    } else if (lacing == BlockHeader::Lacing::XIPH) {
      for (std::size_t i = 0; i < frameRanges.size() - 1U; ++i) {
        writeXiphSize(writer, frameRanges[i].size());
      }
    }

    for (const auto &frame : frameRanges) {
      writer.writeBytes(frame.data());
    }
  }

  static const detail::BaseBlockElement *findNextBlock(std::span<const SimpleBlock> simpleBlocks,
                                                       std::span<const BlockGroup> blockGroups, int16_t timestampOffset,
                                                       uint32_t trackNumber) noexcept {
    auto nextSimpleBlockIt = std::lower_bound(
        simpleBlocks.begin(), simpleBlocks.end(), timestampOffset,
        [](const SimpleBlock &block, int16_t offset) { return block.header.timestampOffset <= offset; });
    auto nextBlockIt = std::lower_bound(
        blockGroups.begin(), blockGroups.end(), timestampOffset,
        [](const BlockGroup &group, int16_t offset) { return group.block.header.timestampOffset <= offset; });

    if (nextSimpleBlockIt != simpleBlocks.end() && nextSimpleBlockIt->header.trackNumber != trackNumber) {
      nextSimpleBlockIt =
          std::find_if(nextSimpleBlockIt, simpleBlocks.end(), [trackNumber, timestampOffset](const SimpleBlock &block) {
            return block.header.trackNumber == trackNumber && block.header.timestampOffset > timestampOffset;
          });
    }
    if (nextBlockIt != blockGroups.end() && nextBlockIt->block.header.trackNumber != trackNumber) {
      nextBlockIt =
          std::find_if(nextBlockIt, blockGroups.end(), [trackNumber, timestampOffset](const BlockGroup &group) {
            return group.block.header.trackNumber == trackNumber &&
                   group.block.header.timestampOffset > timestampOffset;
          });
    }

    if (nextSimpleBlockIt != simpleBlocks.end() &&
        (nextBlockIt == blockGroups.end() ||
         nextSimpleBlockIt->header.timestampOffset < nextBlockIt->block.header.timestampOffset)) {
      return &*nextSimpleBlockIt;
    } else if (nextBlockIt != blockGroups.end() &&
               (nextSimpleBlockIt == simpleBlocks.end() ||
                nextBlockIt->block.header.timestampOffset <= nextSimpleBlockIt->header.timestampOffset)) {
      return &nextBlockIt->block;
    }
    return nullptr;
  }

  static void advanceToNextBlockForTrack(std::span<const Cluster> &clusters,
                                         detail::BaseBlockElement const *&currentBlock, uint32_t trackNumber) noexcept {
    if (clusters.empty()) {
      currentBlock = nullptr;
      return;
    }
    const auto &cluster = clusters.front();
    auto timestampOffset = currentBlock ? currentBlock->header.timestampOffset : std::numeric_limits<int16_t>::min();
    if (const auto *block = findNextBlock(cluster.simpleBlocks, cluster.blockGroups, timestampOffset, trackNumber)) {
      currentBlock = block;
      return;
    }

    // Try with next cluster
    clusters = clusters.subspan<1>();
    currentBlock = nullptr;
    advanceToNextBlockForTrack(clusters, currentBlock, trackNumber);
  }

  static void advanceToFirstBlockForTrack(std::span<const Cluster> &clusters,
                                          detail::BaseBlockElement const *&currentBlock, uint32_t trackNumber,
                                          const TrackTimestamp<> &start, const TrackTimescale &scale) noexcept {
    if (clusters.empty()) {
      currentBlock = nullptr;
      return;
    }
    if (start.value == 0) {
      return advanceToNextBlockForTrack(clusters, currentBlock, trackNumber);
    }

    // binary search through clusters to find the last smaller one (first candidate)
    auto it = std::lower_bound(
        clusters.begin(), clusters.end(), start * scale,
        [](const Cluster &cluster, const SegmentTimestamp<> &time) { return cluster.timestamp < time; });
    // The last Cluster with a timestamp smaller than the target may be the Cluster containing the target timestamp
    it = it != clusters.begin() ? it - 1 : it;
    clusters = clusters.subspan(static_cast<std::size_t>(std::distance(clusters.begin(), it)));

    do {
      advanceToNextBlockForTrack(clusters, currentBlock, trackNumber);
    } while (!clusters.empty() && currentBlock && getBlockTimestamp(clusters.front(), *currentBlock, scale) < start);
  }

  [[nodiscard]] static bool advanceToNextLaceIndex(const detail::BaseBlockElement &block, uint8_t &laceIndex) {
    if ((laceIndex + 1U) < block.frameDataRanges.size()) {
      ++laceIndex;
      return true;
    }

    laceIndex = 0;
    return false;
  }
} // namespace bml::ebml::mkv
