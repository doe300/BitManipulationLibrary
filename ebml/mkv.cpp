
#include "mkv.hpp"

#include "internal.hpp"
#include "mkv_frames.hpp"

#include <algorithm>
#include <format>

namespace bml::ebml::mkv {
  void BlockHeader::read(bml::BitReader &reader, const ReadOptions & /* options */) {
    bml::detail::readMembers(reader, *this);
  }
  void BlockHeader::write(BitWriter &writer) const { bml::detail::writeMembers(writer, *this); }

  std::ostream &BlockHeader::printYAML(std::ostream &os, const bml::yaml::Options &options) const {
    using namespace bml::yaml;
    bml::yaml::detail::printMember(os, options, true /* first member */, "trackNumber, timestampOffset",
                                   trackNumber.get(), timestampOffset.get());
    if (keyframe || !options.hasFlags(PrintFlags::HIDE_DEFAULT)) {
      bml::yaml::detail::printMember(os, options, false, "keyframe", keyframe.get());
    }
    if (invisible || !options.hasFlags(PrintFlags::HIDE_DEFAULT)) {
      bml::yaml::detail::printMember(os, options, false, "invisible", invisible.get());
    }
    if (lacing != Lacing::NONE || !options.hasFlags(PrintFlags::HIDE_DEFAULT)) {
      bml::yaml::detail::printMember(os, options, false, "lacing", lacing.get());
    }
    if (discardable || !options.hasFlags(PrintFlags::HIDE_DEFAULT)) {
      bml::yaml::detail::printMember(os, options, false, "discardable", discardable.get());
    }
    return os;
  }

  namespace detail {
    static std::string formatUUID(const std::array<std::byte, 16> &uuid) {
      std::span<const uint8_t, 16> view{reinterpret_cast<const uint8_t *>(uuid.data()), uuid.size()};

      return std::format(
          "{:02X}{:02X}{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}",
          view[0], view[1], view[2], view[3], view[4], view[5], view[6], view[7], view[8], view[9], view[10], view[11],
          view[12], view[13], view[14], view[15]);
    }

    std::ostream &operator<<(std::ostream &os, const BaseUUIDElement &element) {
      return os << formatUUID(element.value);
    }

    void BaseUUIDElement::readValue(BitReader &reader, ElementId id, const ReadOptions &) {
      auto numBytes = ebml::detail::readElementHeader(reader, id);
      if (numBytes != NUM_BYTES) {
        throw std::invalid_argument("UUID element with ID '" + toHexString(static_cast<uintmax_t>(id)) +
                                    "' does not have a content-size of 16 Bytes: " + numBytes.toString());
      }
      reader.readBytesInto(std::span{value});
    }

    void BaseUUIDElement::writeValue(BitWriter &writer, ElementId id) const {
      ebml::detail::writeElementHeader(writer, id, ByteCount{value.size()});
      writer.writeBytes(std::span{value});
    }

    std::ostream &BaseUUIDElement::printYAML(std::ostream &os, const bml::yaml::Options &options) const {
      return bml::yaml::print(os, options, formatUUID(value));
    }

    void BaseBlockElement::readValue(BitReader &reader, ElementId id, const ReadOptions &options) {
      auto blockSize = ebml::detail::readElementHeader(reader, id);
      auto endPos = reader.position() + blockSize;
      header.read(reader);
      auto dataSize = (endPos - reader.position()).divide<8>();

      auto blockDataOffset = bml::ebml::detail::getUnderlyingReader(reader).position().divide<8>();
      if (options.readMediaData) {
        frameData.resize(dataSize.num);
        reader.readBytesInto(std::span{frameData});

        BitReader tmpReader{std::span{frameData}};
        frameDataRanges = readFrameRanges(tmpReader, header, {blockDataOffset, dataSize});
      } else {
        frameDataRanges = readFrameRanges(reader, header, {blockDataOffset, dataSize});
        reader.skip(endPos - reader.position());
      }
    }

    void BaseBlockElement::writeValue(BitWriter &writer, ElementId id) const {
      ebml::detail::writeElementHeader(writer, id, header.numBits() + ByteCount{frameData.size()});
      header.write(writer);
      writer.writeBytes(frameData);
    }

    std::ostream &BaseBlockElement::printYAML(std::ostream &os, const bml::yaml::Options &options) const {
      os << options.indentation(true /* first member */) << "header:";
      bml::yaml::print(os, options.nextLevel(false), header);
      if (options.hasFlags(yaml::PrintFlags::HIDE_DETAILS)) {
        os << options.indentation(false /* not first member */) << "frames:";
        return bml::yaml::print(os, options.nextLevel(false), frameDataRanges.size());
      }

      os << options.indentation(false /* second member */) << "frames:";
      return bml::yaml::print(os, options.nextLevel(true), frameDataRanges);
    }
  } // namespace detail

  void Segment::read(bml::BitReader &reader, const ReadOptions &options) {
    auto readMember = readChunked(reader, options);
    while (readMember) {
      readMember();
    }
  }

  ChunkedReader Segment::readChunked(bml::BitReader &reader, ReadOptions options) & {
    using namespace bml::ebml::detail;
    return readMasterElementChunked(reader, ID, *this, options,
                                    {wrapMemberReader(crc32), wrapMemberReader(seekHeads), wrapMemberReader(info),
                                     wrapMemberReader(tracks), wrapMemberReader(cues), wrapMemberReader(chapters),
                                     wrapMemberReader(clusters), wrapMemberReader(attachments), wrapMemberReader(tags),
                                     wrapMemberReader(voidElements)},
                                    {EBMLHeader::ID, Segment::ID});
  }

  void Segment::write(BitWriter &writer) const {
    using namespace bml::ebml::detail;
    writeMasterElement(writer, ID,
                       {wrapMemberWriter(crc32), wrapMemberWriter(seekHeads), wrapMemberWriter(info),
                        wrapMemberWriter(tracks), wrapMemberWriter(cues), wrapMemberWriter(chapters),
                        wrapMemberWriter(clusters), wrapMemberWriter(attachments), wrapMemberWriter(tags),
                        wrapMemberWriter(voidElements)});
  }

  void Segment::skip(BitReader &reader) { bml::ebml::detail::skipElement(reader, {EBMLHeader::ID, Segment::ID}); }

  void Segment::copy(BitReader &reader, BitWriter &writer) {
    bml::ebml::detail::copyElement(reader, writer, {EBMLHeader::ID, Segment::ID});
  }

  BML_YAML_DEFINE_PRINT(Segment, crc32, seekHeads, info, tracks, cues, chapters, clusters, attachments, tags,
                        voidElements)

  void Cluster::read(bml::BitReader &reader, const ReadOptions &options) {
    using namespace bml::ebml::detail;
    readMasterElement(reader, ID, *this, options,
                      {wrapMemberReader(crc32), wrapMemberReader(timestamp), wrapMemberReader(position),
                       wrapMemberReader(prevSize), wrapMemberReader(simpleBlocks), wrapMemberReader(blockGroups),
                       wrapMemberReader(voidElements)},
                      {EBMLHeader::ID, Segment::ID, SeekHead::ID, Info::ID, Tracks::ID, Cues::ID, Chapters::ID,
                       Cluster::ID, Attachments::ID, Tags::ID});
  }

  void Cluster::write(BitWriter &writer) const {
    using namespace bml::ebml::detail;
    writeMasterElement(writer, ID,
                       {wrapMemberWriter(crc32), wrapMemberWriter(timestamp), wrapMemberWriter(position),
                        wrapMemberWriter(prevSize), wrapMemberWriter(simpleBlocks), wrapMemberWriter(blockGroups),
                        wrapMemberWriter(voidElements)});
  }

  void Cluster::skip(BitReader &reader) {
    bml::ebml::detail::skipElement(reader, {EBMLHeader::ID, Segment::ID, SeekHead::ID, Info::ID, Tracks::ID, Cues::ID,
                                            Chapters::ID, Cluster::ID, Attachments::ID, Tags::ID});
  }

  void Cluster::copy(BitReader &reader, BitWriter &writer) {
    bml::ebml::detail::copyElement(reader, writer,
                                   {EBMLHeader::ID, Segment::ID, SeekHead::ID, Info::ID, Tracks::ID, Cues::ID,
                                    Chapters::ID, Cluster::ID, Attachments::ID, Tags::ID});
  }

  BML_YAML_DEFINE_PRINT(Cluster, crc32, timestamp, position, prevSize, simpleBlocks, blockGroups, voidElements)

  std::ostream &operator<<(std::ostream &os, const Cluster &cluster) {
    static const auto addBlockSizes = [](ByteCount size, const SimpleBlock &block) {
      return size + std::accumulate(block.frameDataRanges.begin(), block.frameDataRanges.end(), 0_bytes,
                                    [](ByteCount sum, const ByteRange &range) { return sum + range.size; });
    };

    os << "Cluster{";
    bml::detail::printMember(os, "crc32, timestamp, position, prevSize, voidElements", cluster.crc32, cluster.timestamp,
                             cluster.position, cluster.prevSize, cluster.voidElements);
    os << ", simpleBlocks = " << cluster.simpleBlocks.size() << " ("
       << std::accumulate(cluster.simpleBlocks.begin(), cluster.simpleBlocks.end(), ByteCount{}, addBlockSizes)
              .toString()
       << "), ";
    bml::detail::printMember(os, "blockGroups", cluster.blockGroups);
    return os << '}';
  }

  BML_EBML_DEFINE_IO(BlockGroup, crc32, block, blockAdditions, blockDuration, referencePriority, referenceBlocks,
                     codecState, discardPadding, voidElements)
  BML_YAML_DEFINE_PRINT(BlockGroup, crc32, block, blockAdditions, blockDuration, referencePriority, referenceBlocks,
                        codecState, discardPadding, voidElements)

  void Matroska::read(BitReader &reader, const ReadOptions &options) {
    header.read(reader, options);
    segment.read(reader, options);
    hasData = options.readMediaData;
  }

  ChunkedReader Matroska::readChunked(bml::BitReader &reader, ReadOptions options) & {
    header.read(reader, options);
    co_yield header.ID;

    auto readSegmentMember = segment.readChunked(reader, options);
    while (readSegmentMember) {
      if (auto id = readSegmentMember(); id != ElementId{}) {
        co_yield id;
      }
    }
    // segment is read
    co_yield Segment::ID;
    co_return;
  }

  void Matroska::write(BitWriter &writer) const {
    header.write(writer);
    segment.write(writer);
  }

  BML_YAML_DEFINE_PRINT(Matroska, header, segment)

  const TrackEntry *Matroska::getTrackEntry(uint32_t trackNumber) const {
    if (!segment.tracks) {
      return nullptr;
    }
    const auto &tracks = segment.tracks->trackEntries;
    auto it = std::find_if(tracks.begin(), tracks.end(),
                           [trackNumber](const TrackEntry &track) { return track.trackNumber.get() == trackNumber; });
    if (it == tracks.end()) {
      return nullptr;
    }
    return &*it;
  }

  FrameView Matroska::viewFrames(uint32_t trackNumber, const TrackTimestamp<> &start) const {
    if (const auto *track = getTrackEntry(trackNumber)) {
      return FrameView{std::span{segment.clusters}, trackNumber, track->trackTimestampScale, start};
    }
    return FrameView{{}, 0, {}, {}};
  }

  BML_EBML_DEFINE_IO(SeekHead, crc32, seeks, voidElements)
  BML_YAML_DEFINE_PRINT(SeekHead, crc32, seeks, voidElements)

  BML_EBML_DEFINE_IO(Seek, crc32, seekID, seekPosition, voidElements)
  BML_YAML_DEFINE_PRINT(Seek, crc32, seekID, seekPosition, voidElements)

  BML_EBML_DEFINE_IO(Info, crc32, segmentUUID, segmentFilename, prevUUID, prevFilename, nextUUID, nextFilename,
                     segmentFamilies, chapterTranslates, timestampScale, duration, dateUTC, title, muxingApp,
                     writingApp, voidElements)
  BML_YAML_DEFINE_PRINT(Info, crc32, segmentUUID, segmentFilename, prevUUID, prevFilename, nextUUID, nextFilename,
                        segmentFamilies, chapterTranslates, timestampScale, duration, dateUTC, title, muxingApp,
                        writingApp, voidElements)

  BML_EBML_DEFINE_IO(ChapterTranslate, crc32, chapterTranslateID, chapterTranslateCodec, chapterTranslateEditionUIDs,
                     voidElements)
  BML_YAML_DEFINE_PRINT(ChapterTranslate, crc32, chapterTranslateID, chapterTranslateCodec, chapterTranslateEditionUIDs,
                        voidElements)

  BML_EBML_DEFINE_IO(BlockAdditions, crc32, blockMores, voidElements)
  BML_YAML_DEFINE_PRINT(BlockAdditions, crc32, blockMores, voidElements)

  BML_EBML_DEFINE_IO(BlockMore, crc32, blockAdditional, blockAddID, voidElements)
  BML_YAML_DEFINE_PRINT(BlockMore, crc32, blockAdditional, blockAddID, voidElements)

  BML_EBML_DEFINE_IO(Tracks, crc32, trackEntries, voidElements)
  BML_YAML_DEFINE_PRINT(Tracks, crc32, trackEntries, voidElements)

  BML_EBML_DEFINE_IO(TrackEntry, crc32, trackNumber, trackUID, trackType, flagEnabled, flagDefault, flagForced,
                     flagHearingImpaired, flagVisualImpaired, flagTextDescriptions, flagOriginal, flagCommentary,
                     flagLacing, defaultDuration, defaultDecodedFieldDuration, trackTimestampScale, maxBlockAdditionID,
                     blockAdditionMappings, name, language, languageBCP47, codecID, codecPrivate, codecName,
                     attachmentLink, codecDecodeAll, trackOverlays, codecDelay, seekPreRoll, trackTranslates, video,
                     audio, trackOperation, contentEncodings, voidElements)
  BML_YAML_DEFINE_PRINT(TrackEntry, crc32, trackNumber, trackUID, trackType, flagEnabled, flagDefault, flagForced,
                        flagHearingImpaired, flagVisualImpaired, flagTextDescriptions, flagOriginal, flagCommentary,
                        flagLacing, defaultDuration, defaultDecodedFieldDuration, trackTimestampScale,
                        maxBlockAdditionID, blockAdditionMappings, name, language, languageBCP47, codecID, codecPrivate,
                        codecName, attachmentLink, codecDecodeAll, trackOverlays, codecDelay, seekPreRoll,
                        trackTranslates, video, audio, trackOperation, contentEncodings, voidElements)

  BML_EBML_DEFINE_IO(BlockAdditionMapping, crc32, blockAddIDValue, blockAddIDName, blockAddIDType, blockAddIDExtraData,
                     voidElements)
  BML_YAML_DEFINE_PRINT(BlockAdditionMapping, crc32, blockAddIDValue, blockAddIDName, blockAddIDType,
                        blockAddIDExtraData, voidElements)

  BML_EBML_DEFINE_IO(TrackTranslate, crc32, trackTranslateTrackID, trackTranslateCodec, trackTranslateEditionUIDs,
                     voidElements)
  BML_YAML_DEFINE_PRINT(TrackTranslate, crc32, trackTranslateTrackID, trackTranslateCodec, trackTranslateEditionUIDs,
                        voidElements)

  BML_EBML_DEFINE_IO(Video, crc32, flagInterlaced, fieldOrder, stereoMode, alphaMode, oldStereoMode, pixelWidth,
                     pixelHeight, pixelCropBottom, pixelCropTop, pixelCropLeft, pixelCropRight, displayWidth,
                     displayHeight, displayUnit, uncompressedFourCC, colour, projection, voidElements)
  BML_YAML_DEFINE_PRINT(Video, crc32, flagInterlaced, fieldOrder, stereoMode, alphaMode, oldStereoMode, pixelWidth,
                        pixelHeight, pixelCropBottom, pixelCropTop, pixelCropLeft, pixelCropRight, displayWidth,
                        displayHeight, displayUnit, uncompressedFourCC, colour, projection, voidElements)

  BML_EBML_DEFINE_IO(Colour, crc32, matrixCoefficients, bitsPerChannel, chromaSubsamplingHorz, chromaSubsamplingVert,
                     cbSubsamplingHorz, cbSubsamplingVert, chromaSitingHorz, chromaSitingVert, range,
                     transferCharacteristics, primaries, maxCLL, maxFALL, masteringMetadata, voidElements)
  BML_YAML_DEFINE_PRINT(Colour, crc32, matrixCoefficients, bitsPerChannel, chromaSubsamplingHorz, chromaSubsamplingVert,
                        cbSubsamplingHorz, cbSubsamplingVert, chromaSitingHorz, chromaSitingVert, range,
                        transferCharacteristics, primaries, maxCLL, maxFALL, masteringMetadata, voidElements)

  BML_EBML_DEFINE_IO(MasteringMetadata, crc32, primaryRChromaticityX, primaryRChromaticityY, primaryGChromaticityX,
                     primaryGChromaticityY, primaryBChromaticityX, primaryBChromaticityY, whitePointChromaticityX,
                     whitePointChromaticityY, luminanceMax, luminanceMin, voidElements)
  BML_YAML_DEFINE_PRINT(MasteringMetadata, crc32, primaryRChromaticityX, primaryRChromaticityY, primaryGChromaticityX,
                        primaryGChromaticityY, primaryBChromaticityX, primaryBChromaticityY, whitePointChromaticityX,
                        whitePointChromaticityY, luminanceMax, luminanceMin, voidElements)

  BML_EBML_DEFINE_IO(Projection, crc32, projectionType, projectionPrivate, projectionPoseYaw, projectionPosePitch,
                     projectionPoseRoll, voidElements)
  BML_YAML_DEFINE_PRINT(Projection, crc32, projectionType, projectionPrivate, projectionPoseYaw, projectionPosePitch,
                        projectionPoseRoll, voidElements)

  BML_EBML_DEFINE_IO(Audio, crc32, samplingFrequency, outputSamplingFrequency, channels, bitDepth, emphasis,
                     voidElements)
  BML_YAML_DEFINE_PRINT(Audio, crc32, samplingFrequency, outputSamplingFrequency, channels, bitDepth, emphasis,
                        voidElements)

  BML_EBML_DEFINE_IO(TrackOperation, crc32, trackCombinePlanes, trackJoinBlocks, voidElements)
  BML_YAML_DEFINE_PRINT(TrackOperation, crc32, trackCombinePlanes, trackJoinBlocks, voidElements)

  BML_EBML_DEFINE_IO(TrackCombinePlanes, crc32, trackPlanes, voidElements)
  BML_YAML_DEFINE_PRINT(TrackCombinePlanes, crc32, trackPlanes, voidElements)

  BML_EBML_DEFINE_IO(TrackPlane, crc32, trackPlaneUID, trackPlaneType, voidElements)
  BML_YAML_DEFINE_PRINT(TrackPlane, crc32, trackPlaneUID, trackPlaneType, voidElements)

  BML_EBML_DEFINE_IO(TrackJoinBlocks, crc32, trackJoinUIDs, voidElements)
  BML_YAML_DEFINE_PRINT(TrackJoinBlocks, crc32, trackJoinUIDs, voidElements)

  BML_EBML_DEFINE_IO(ContentEncodings, crc32, contentEncodings, voidElements)
  BML_YAML_DEFINE_PRINT(ContentEncodings, crc32, contentEncodings, voidElements)

  BML_EBML_DEFINE_IO(ContentEncoding, crc32, contentEncodingOrder, contentEncodingScope, contentEncodingType,
                     contentCompression, contentEncryption, voidElements)
  BML_YAML_DEFINE_PRINT(ContentEncoding, crc32, contentEncodingOrder, contentEncodingScope, contentEncodingType,
                        contentCompression, contentEncryption, voidElements)

  BML_EBML_DEFINE_IO(ContentCompression, crc32, contentCompAlgo, contentCompSettings, voidElements)
  BML_YAML_DEFINE_PRINT(ContentCompression, crc32, contentCompAlgo, contentCompSettings, voidElements)

  BML_EBML_DEFINE_IO(ContentEncryption, crc32, contentEncAlgo, contentEncKeyID, contentEncAESSettings, contentSignature,
                     contentSigKeyID, contentSigAlgo, contentSigHashAlgo, voidElements)
  BML_YAML_DEFINE_PRINT(ContentEncryption, crc32, contentEncAlgo, contentEncKeyID, contentEncAESSettings,
                        contentSignature, contentSigKeyID, contentSigAlgo, contentSigHashAlgo, voidElements)

  BML_EBML_DEFINE_IO(ContentEncAESSettings, crc32, aESSettingsCipherMode, voidElements)
  BML_YAML_DEFINE_PRINT(ContentEncAESSettings, crc32, aESSettingsCipherMode, voidElements)

  BML_EBML_DEFINE_IO(Cues, crc32, cuePoints, voidElements)
  BML_YAML_DEFINE_PRINT(Cues, crc32, cuePoints, voidElements)

  BML_EBML_DEFINE_IO(CuePoint, crc32, cueTime, cueTrackPositions, voidElements)
  BML_YAML_DEFINE_PRINT(CuePoint, crc32, cueTime, cueTrackPositions, voidElements)

  BML_EBML_DEFINE_IO(CueTrackPositions, crc32, cueTrack, cueClusterPosition, cueRelativePosition, cueDuration,
                     cueBlockNumber, cueCodecState, cueReferences, voidElements)
  BML_YAML_DEFINE_PRINT(CueTrackPositions, crc32, cueTrack, cueClusterPosition, cueRelativePosition, cueDuration,
                        cueBlockNumber, cueCodecState, cueReferences, voidElements)

  BML_EBML_DEFINE_IO(CueReference, crc32, cueRefTime, voidElements)
  BML_YAML_DEFINE_PRINT(CueReference, crc32, cueRefTime, voidElements)

  BML_EBML_DEFINE_IO(Attachments, crc32, attachedFiles, voidElements)
  BML_YAML_DEFINE_PRINT(Attachments, crc32, attachedFiles, voidElements)

  BML_EBML_DEFINE_IO(AttachedFile, crc32, fileDescription, fileName, fileMediaType, fileData, fileUID, voidElements)
  BML_YAML_DEFINE_PRINT(AttachedFile, crc32, fileDescription, fileName, fileMediaType, fileData, fileUID, voidElements)

  BML_EBML_DEFINE_IO(Chapters, crc32, editionEntries, voidElements)
  BML_YAML_DEFINE_PRINT(Chapters, crc32, editionEntries, voidElements)

  BML_EBML_DEFINE_IO(EditionEntry, crc32, editionUID, editionFlagHidden, editionFlagDefault, editionFlagOrdered,
                     editionDisplays, chapterAtoms, voidElements)
  BML_YAML_DEFINE_PRINT(EditionEntry, crc32, editionUID, editionFlagHidden, editionFlagDefault, editionFlagOrdered,
                        editionDisplays, chapterAtoms, voidElements)

  BML_EBML_DEFINE_IO(EditionDisplay, crc32, editionString, editionLanguageIETFs, voidElements)
  BML_YAML_DEFINE_PRINT(EditionDisplay, crc32, editionString, editionLanguageIETFs, voidElements)

  BML_EBML_DEFINE_IO(ChapterAtom, crc32, chapterUID, chapterStringUID, chapterTimeStart, chapterTimeEnd,
                     chapterFlagHidden, chapterFlagEnabled, chapterSegmentUUID, chapterSkipType,
                     chapterSegmentEditionUID, chapterPhysicalEquiv, chapterTrack, chapterDisplays, chapProcess,
                     voidElements)
  BML_YAML_DEFINE_PRINT(ChapterAtom, crc32, chapterUID, chapterStringUID, chapterTimeStart, chapterTimeEnd,
                        chapterFlagHidden, chapterFlagEnabled, chapterSegmentUUID, chapterSkipType,
                        chapterSegmentEditionUID, chapterPhysicalEquiv, chapterTrack, chapterDisplays, chapProcess,
                        voidElements)

  BML_EBML_DEFINE_IO(ChapterTrack, crc32, chapterTrackUIDs, voidElements)
  BML_YAML_DEFINE_PRINT(ChapterTrack, crc32, chapterTrackUIDs, voidElements)

  BML_EBML_DEFINE_IO(ChapterDisplay, crc32, chapString, chapLanguages, chapLanguageBCP47s, chapCountries, voidElements)
  BML_YAML_DEFINE_PRINT(ChapterDisplay, crc32, chapString, chapLanguages, chapLanguageBCP47s, chapCountries,
                        voidElements)

  BML_EBML_DEFINE_IO(ChapProcess, crc32, chapProcessCodecID, chapProcessPrivate, chapProcessCommands, voidElements)
  BML_YAML_DEFINE_PRINT(ChapProcess, crc32, chapProcessCodecID, chapProcessPrivate, chapProcessCommands, voidElements)

  BML_EBML_DEFINE_IO(ChapProcessCommand, crc32, chapProcessTime, chapProcessData, voidElements)
  BML_YAML_DEFINE_PRINT(ChapProcessCommand, crc32, chapProcessTime, chapProcessData, voidElements)

  BML_EBML_DEFINE_IO(Tags, crc32, tags, voidElements)
  BML_YAML_DEFINE_PRINT(Tags, crc32, tags, voidElements)

  BML_EBML_DEFINE_IO(Tag, crc32, targets, simpleTags, voidElements)
  BML_YAML_DEFINE_PRINT(Tag, crc32, targets, simpleTags, voidElements)

  BML_EBML_DEFINE_IO(Targets, crc32, targetTypeValue, targetType, tagTrackUIDs, tagEditionUIDs, tagChapterUIDs,
                     tagAttachmentUIDs, voidElements)
  BML_YAML_DEFINE_PRINT(Targets, crc32, targetTypeValue, targetType, tagTrackUIDs, tagEditionUIDs, tagChapterUIDs,
                        tagAttachmentUIDs, voidElements)

  BML_EBML_DEFINE_IO(SimpleTag, crc32, tagName, tagLanguage, tagLanguageBCP47, tagDefault, tagString, tagBinary,
                     voidElements)
  BML_YAML_DEFINE_PRINT(SimpleTag, crc32, tagName, tagLanguage, tagLanguageBCP47, tagDefault, tagString, tagBinary,
                        voidElements)

} // namespace bml::ebml::mkv

namespace bml::ebml::detail {
  template <>
  void BaseSimpleElement<mkv::MatroskaTimestamp>::readValue(BitReader &reader, ElementId id,
                                                            const mkv::MatroskaTimestamp &defaultValue) {
    BaseSimpleElement<uintmax_t> tmp{};
    tmp.readValue(reader, id, defaultValue.value);
    value.value = tmp.value;
  }

  template <>
  void BaseSimpleElement<mkv::MatroskaTimestamp>::writeValue(BitWriter &writer, ElementId id,
                                                             const mkv::MatroskaTimestamp &defaultValue) const {
    BaseSimpleElement<uintmax_t> tmp{value.value};
    tmp.writeValue(writer, id, defaultValue.value);
  }

  template <>
  void BaseSimpleElement<mkv::SegmentTimestamp<>>::readValue(BitReader &reader, ElementId id,
                                                             const mkv::SegmentTimestamp<> &defaultValue) {
    BaseSimpleElement<uintmax_t> tmp{};
    tmp.readValue(reader, id, defaultValue.value);
    value.value = tmp.value;
  }

  template <>
  void BaseSimpleElement<mkv::SegmentTimestamp<>>::writeValue(BitWriter &writer, ElementId id,
                                                              const mkv::SegmentTimestamp<> &defaultValue) const {
    BaseSimpleElement<uintmax_t> tmp{value.value};
    tmp.writeValue(writer, id, defaultValue.value);
  }

  template <>
  void BaseSimpleElement<mkv::SegmentTimestamp<float>>::readValue(BitReader &reader, ElementId id,
                                                                  const mkv::SegmentTimestamp<float> &defaultValue) {
    BaseSimpleElement<float> tmp{};
    tmp.readValue(reader, id, defaultValue.value);
    value.value = tmp.value;
  }

  template <>
  void
  BaseSimpleElement<mkv::SegmentTimestamp<float>>::writeValue(BitWriter &writer, ElementId id,
                                                              const mkv::SegmentTimestamp<float> &defaultValue) const {
    BaseSimpleElement<float> tmp{value.value};
    tmp.writeValue(writer, id, defaultValue.value);
  }

  template <>
  void BaseSimpleElement<mkv::TrackTimestamp<>>::readValue(BitReader &reader, ElementId id,
                                                           const mkv::TrackTimestamp<> &defaultValue) {
    BaseSimpleElement<uintmax_t> tmp{};
    tmp.readValue(reader, id, defaultValue.value);
    value.value = tmp.value;
  }

  template <>
  void BaseSimpleElement<mkv::TrackTimestamp<>>::writeValue(BitWriter &writer, ElementId id,
                                                            const mkv::TrackTimestamp<> &defaultValue) const {
    BaseSimpleElement<uintmax_t> tmp{value.value};
    tmp.writeValue(writer, id, defaultValue.value);
  }

  template <>
  void BaseSimpleElement<mkv::TrackTimestamp<intmax_t>>::readValue(BitReader &reader, ElementId id,
                                                                   const mkv::TrackTimestamp<intmax_t> &defaultValue) {
    BaseSimpleElement<intmax_t> tmp{};
    tmp.readValue(reader, id, defaultValue.value);
    value.value = tmp.value;
  }

  template <>
  void BaseSimpleElement<mkv::TrackTimestamp<intmax_t>>::writeValue(
      BitWriter &writer, ElementId id, const mkv::TrackTimestamp<intmax_t> &defaultValue) const {
    BaseSimpleElement<intmax_t> tmp{value.value};
    tmp.writeValue(writer, id, defaultValue.value);
  }

  template <>
  void BaseSimpleElement<mkv::SegmentTimescale>::readValue(BitReader &reader, ElementId id,
                                                           const mkv::SegmentTimescale &defaultValue) {
    BaseSimpleElement<uintmax_t> tmp{};
    tmp.readValue(reader, id, defaultValue.value);
    value.value = tmp.value;
  }

  template <>
  void BaseSimpleElement<mkv::SegmentTimescale>::writeValue(BitWriter &writer, ElementId id,
                                                            const mkv::SegmentTimescale &defaultValue) const {
    BaseSimpleElement<uintmax_t> tmp{value.value};
    tmp.writeValue(writer, id, defaultValue.value);
  }

  template <>
  void BaseSimpleElement<mkv::TrackTimescale>::readValue(BitReader &reader, ElementId id,
                                                         const mkv::TrackTimescale &defaultValue) {
    BaseSimpleElement<float> tmp{};
    tmp.readValue(reader, id, defaultValue.value);
    value.value = tmp.value;
  }

  template <>
  void BaseSimpleElement<mkv::TrackTimescale>::writeValue(BitWriter &writer, ElementId id,
                                                          const mkv::TrackTimescale &defaultValue) const {
    BaseSimpleElement<float> tmp{value.value};
    tmp.writeValue(writer, id, defaultValue.value);
  }

} // namespace bml::ebml::detail
