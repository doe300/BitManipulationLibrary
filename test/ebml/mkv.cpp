#include "mkv.hpp"
#include "mkv_frames.hpp"

#include "ebml_common.hpp"
#include "errors.hpp"

#include "cpptest-main.h"

#include <algorithm>
#include <deque>
#include <ranges>

using namespace bml;
using namespace bml::ebml;
using namespace bml::ebml::mkv;

/*
 * The test data is adapted from the official Matroska test files, which can be acquired here:
 * https://www.matroska.org/downloads/test_suite.html
 */
class TestMkvElements : public TestElementsBase {
public:
  TestMkvElements() : TestElementsBase("Mkv") {
    TEST_ADD(TestMkvElements::testSeek);
    TEST_ADD(TestMkvElements::testSeekHead);
    TEST_ADD(TestMkvElements::testInfo);
    TEST_ADD(TestMkvElements::testContentCompression);
    TEST_ADD(TestMkvElements::testContentEncoding);
    TEST_ADD(TestMkvElements::testContentEncodings);
    TEST_ADD(TestMkvElements::testAudio);
    TEST_ADD(TestMkvElements::testTrackEntry);
    TEST_ADD(TestMkvElements::testTracks);
    TEST_ADD(TestMkvElements::testSimpleTag);
    TEST_ADD(TestMkvElements::testTag);
    TEST_ADD(TestMkvElements::testTags);
    TEST_ADD(TestMkvElements::testCueTrackPositions);
    TEST_ADD(TestMkvElements::testCuePoint);
    TEST_ADD(TestMkvElements::testCues);
    TEST_ADD(TestMkvElements::testBlockGroup);
    TEST_ADD(TestMkvElements::testCluster);
    TEST_ADD(TestMkvElements::testSegment);
    TEST_ADD(TestMkvElements::testUnknownSizeSegmentWithCluster);
    TEST_ADD(TestMkvElements::testMatroskaChunked);
  }

  void testSeek() {
    Seek elem{};
    elem.crc32 = 0xCFEE2EE0;
    elem.seekID = toBytes({0x12, 0x54, 0xC3, 0x67});
    elem.seekPosition = 0x02A4;

    testElement(elem, {0x4D, 0xBB, 0x92, 0xBF, 0x84, 0xE0, 0x2E, 0xEE, 0xCF, 0x53, 0xAB,
                       0x84, 0x12, 0x54, 0xC3, 0x67, 0x53, 0xAC, 0x82, 0x02, 0xA4},
                "crc32: 0xcfee2ee0\nseekID: [0x12, 0x54, 0xc3, 0x67, ]\nseekPosition: 676");
  }

  void testSeekHead() {
    SeekHead elem{};

    {
      Seek seek{};
      seek.seekID = toBytes({0x12, 0x54, 0xC3, 0x67});
      seek.seekPosition = 0x02A4;
      elem.seeks.push_back(std::move(seek));
    }

    testElement(elem, {0x11, 0x4D, 0x9B, 0x74, 0x8F, 0x4D, 0xBB, 0x8C, 0x53, 0xAB,
                       0x84, 0x12, 0x54, 0xC3, 0x67, 0x53, 0xAC, 0x82, 0x02, 0xA4},
                R"(seeks:
  - seekID: [0x12, 0x54, 0xc3, 0x67, ]
    seekPosition: 676)");
  }

  void testInfo() {
    Info elem{};
    elem.crc32 = 0xF57861CC;
    elem.duration = SegmentTimestamp<float>{475090.0F};
    elem.timestampScale = SegmentTimescale{100000U};
    elem.muxingApp = u8"libebml2 v0.21.0 + libmatroska2 v0.22.1";
    elem.writingApp = u8"mkclean 0.8.3 ru from libebml2 v0.10.0 + libmatroska2 v0.10.1 + mkclean 0.5.5 ru from libebml "
                      u8"v1.0.0 + libmatroska v1.0.0 + mkvmerge v4.1.1 ('Bouncin' Back') built on Jul  3 2010 22:54:08";
    elem.dateUTC = bml::ebml::detail::DATE_EPOCH + std::chrono::nanoseconds{328711520000000000LL};
    elem.segmentUUID = std::array<std::byte, 16>{std::byte{0x92}, std::byte{0xB2}, std::byte{0xCE}, std::byte{0x31},
                                                 std::byte{0x8A}, std::byte{0x96}, std::byte{0x50}, std::byte{0x03},
                                                 std::byte{0x9C}, std::byte{0x48}, std::byte{0x2D}, std::byte{0x67},
                                                 std::byte{0xAA}, std::byte{0x55}, std::byte{0xCB}, std::byte{0x7B}};

    testElement(elem,
                {0x15, 0x49, 0xA9, 0x66, 0x41, 0x1B, 0xBF, 0x84, 0xCC, 0x61, 0x78, 0xF5, 0x73, 0xA4, 0x90, 0x92, 0xB2,
                 0xCE, 0x31, 0x8A, 0x96, 0x50, 0x03, 0x9C, 0x48, 0x2D, 0x67, 0xAA, 0x55, 0xCB, 0x7B, 0x2A, 0xD7, 0xB1,
                 0x83, 0x01, 0x86, 0xA0, 0x44, 0x89, 0x84, 0x48, 0xE7, 0xFA, 0x40, 0x44, 0x61, 0x88, 0x04, 0x8F, 0xD1,
                 0x62, 0xC7, 0x2D, 0xC0, 0x00, 0x4D, 0x80, 0xA7, 0x6C, 0x69, 0x62, 0x65, 0x62, 0x6D, 0x6C, 0x32, 0x20,
                 0x76, 0x30, 0x2E, 0x32, 0x31, 0x2E, 0x30, 0x20, 0x2B, 0x20, 0x6C, 0x69, 0x62, 0x6D, 0x61, 0x74, 0x72,
                 0x6F, 0x73, 0x6B, 0x61, 0x32, 0x20, 0x76, 0x30, 0x2E, 0x32, 0x32, 0x2E, 0x31, 0x57, 0x41, 0x40, 0xBB,
                 0x6D, 0x6B, 0x63, 0x6C, 0x65, 0x61, 0x6E, 0x20, 0x30, 0x2E, 0x38, 0x2E, 0x33, 0x20, 0x72, 0x75, 0x20,
                 0x66, 0x72, 0x6F, 0x6D, 0x20, 0x6C, 0x69, 0x62, 0x65, 0x62, 0x6D, 0x6C, 0x32, 0x20, 0x76, 0x30, 0x2E,
                 0x31, 0x30, 0x2E, 0x30, 0x20, 0x2B, 0x20, 0x6C, 0x69, 0x62, 0x6D, 0x61, 0x74, 0x72, 0x6F, 0x73, 0x6B,
                 0x61, 0x32, 0x20, 0x76, 0x30, 0x2E, 0x31, 0x30, 0x2E, 0x31, 0x20, 0x2B, 0x20, 0x6D, 0x6B, 0x63, 0x6C,
                 0x65, 0x61, 0x6E, 0x20, 0x30, 0x2E, 0x35, 0x2E, 0x35, 0x20, 0x72, 0x75, 0x20, 0x66, 0x72, 0x6F, 0x6D,
                 0x20, 0x6C, 0x69, 0x62, 0x65, 0x62, 0x6D, 0x6C, 0x20, 0x76, 0x31, 0x2E, 0x30, 0x2E, 0x30, 0x20, 0x2B,
                 0x20, 0x6C, 0x69, 0x62, 0x6D, 0x61, 0x74, 0x72, 0x6F, 0x73, 0x6B, 0x61, 0x20, 0x76, 0x31, 0x2E, 0x30,
                 0x2E, 0x30, 0x20, 0x2B, 0x20, 0x6D, 0x6B, 0x76, 0x6D, 0x65, 0x72, 0x67, 0x65, 0x20, 0x76, 0x34, 0x2E,
                 0x31, 0x2E, 0x31, 0x20, 0x28, 0x27, 0x42, 0x6F, 0x75, 0x6E, 0x63, 0x69, 0x6E, 0x27, 0x20, 0x42, 0x61,
                 0x63, 0x6B, 0x27, 0x29, 0x20, 0x62, 0x75, 0x69, 0x6C, 0x74, 0x20, 0x6F, 0x6E, 0x20, 0x4A, 0x75, 0x6C,
                 0x20, 0x20, 0x33, 0x20, 0x32, 0x30, 0x31, 0x30, 0x20, 0x32, 0x32, 0x3A, 0x35, 0x34, 0x3A, 0x30, 0x38},
                R"(crc32: 0xf57861cc
segmentUUID: '92B2CE31-8A96-5003-9C48-2D67AA55CB7B'
timestampScale: 100000
duration: 475090
dateUTC: 2011-06-02 12:45:18.000000000
muxingApp: 'libebml2 v0.21.0 + libmatroska2 v0.22.1'
writingApp: 'mkclean 0.8.3 ru from libebml2 v0.10.0 + libmatroska2 v0.10.1 + mkclean 0.5.5 ru from libebml v1.0.0 + libmatroska v1.0.0 + mkvmerge v4.1.1 ('Bouncin' Back') built on Jul  3 2010 22:54:08')");
  }

  void testContentCompression() {
    ContentCompression elem{};
    elem.contentCompAlgo = 3;
    elem.contentCompSettings = toBytes({0xFF, 0xFB});

    testElement(elem, {0x50, 0x34, 0x89, 0x42, 0x54, 0x81, 0x03, 0x42, 0x55, 0x82, 0xFF, 0xFB},
                "contentCompAlgo: 3\ncontentCompSettings: [0xff, 0xfb, ]");
  }

  void testContentEncoding() {
    ContentEncoding elem{};

    {
      ContentCompression comp{};
      comp.contentCompAlgo = 3;
      comp.contentCompSettings = toBytes({0xFF, 0xFB});
      elem.contentCompression = std::move(comp);
    }

    testElement(elem, {0x62, 0x40, 0x8C, 0x50, 0x34, 0x89, 0x42, 0x54, 0x81, 0x03, 0x42, 0x55, 0x82, 0xFF, 0xFB},
                R"(contentEncodingOrder: 0
contentEncodingScope: 1
contentEncodingType: 0
contentCompression:
  contentCompAlgo: 3
  contentCompSettings: [0xff, 0xfb, ])");
  }

  void testContentEncodings() {
    ContentEncodings elem{};
    elem.contentEncodings.emplace_back();

    testElement(elem, {0x6D, 0x80, 0x83, 0x62, 0x40, 0x80},
                R"(contentEncodings:
  - contentEncodingOrder: 0
    contentEncodingScope: 1
    contentEncodingType: 0)");
  }

  void testAudio() {
    Audio elem{};
    elem.samplingFrequency = 48000.0F;
    elem.channels = 2;

    testElement(elem, {0xE1, 0x89, 0xB5, 0x84, 0x47, 0x3B, 0x80, 0x00, 0x9F, 0x81, 0x02},
                "samplingFrequency: 48000\nchannels: 2\nemphasis: 0");
  }

  void testTrackEntry() {
    TrackEntry elem{};
    elem.crc32 = 0x5505423D;
    elem.trackNumber = 1;
    elem.trackUID = 0x6F1A06B3;
    elem.trackType = 1;
    elem.flagLacing = false;
    elem.defaultDuration = MatroskaTimestamp{0x027BC869};
    elem.language = "und";
    elem.codecID = "V_MPEG4/ISO/AVC";
    elem.codecPrivate = toBytes({0x01, 0x4D, 0x40, 0x1F, 0xFF, 0xE1, 0x00, 0x14, 0x27, 0x4D, 0x40, 0x1F,
                                 0xA9, 0x18, 0x08, 0x00, 0x93, 0x60, 0x0D, 0x41, 0x80, 0x41, 0xAD, 0xB0,
                                 0xAD, 0x7B, 0xDF, 0x01, 0x01, 0x00, 0x04, 0x28, 0xCE, 0x09, 0xC8});

    {
      Video video{};
      video.pixelWidth = 0x0400;
      video.pixelHeight = 0x0240;
      video.displayWidth = 0x054A;
      elem.video = std::move(video);
    }

    testElement(elem, {0xAE, 0xEB, 0xBF, 0x84, 0x3d, 0x42, 0x05, 0x55, 0xD7, 0x81, 0x01, 0x73, 0xC5, 0x84, 0x6F, 0x1A,
                       0x06, 0xB3, 0x83, 0x81, 0x01, 0x9C, 0x81, 0x00, 0x23, 0xE3, 0x83, 0x84, 0x02, 0x7B, 0xC8, 0x69,
                       0x22, 0xB5, 0x9C, 0x83, 0x75, 0x6E, 0x64, 0x86, 0x8F, 0x56, 0x5F, 0x4D, 0x50, 0x45, 0x47, 0x34,
                       0x2F, 0x49, 0x53, 0x4F, 0x2F, 0x41, 0x56, 0x43, 0x63, 0xA2, 0xA3, 0x01, 0x4D, 0x40, 0x1F, 0xFF,
                       0xE1, 0x00, 0x14, 0x27, 0x4D, 0x40, 0x1F, 0xA9, 0x18, 0x08, 0x00, 0x93, 0x60, 0x0D, 0x41, 0x80,
                       0x41, 0xAD, 0xB0, 0xAD, 0x7B, 0xDF, 0x01, 0x01, 0x00, 0x04, 0x28, 0xCE, 0x09, 0xC8, 0xE0, 0x8D,
                       0xB0, 0x82, 0x04, 0x00, 0xBA, 0x82, 0x02, 0x40, 0x54, 0xB0, 0x82, 0x05, 0x4A},
                R"(crc32: 0x5505423d
trackNumber: 1
trackUID: 1863976627
trackType: 1
flagEnabled: true
flagDefault: true
flagForced: false
flagLacing: false
defaultDuration: 41666665
trackTimestampScale: 1
maxBlockAdditionID: 0
language: 'und'
codecID: 'V_MPEG4/ISO/AVC'
codecPrivate: (35 entries)
codecDecodeAll: true
codecDelay: 0
seekPreRoll: 0
video:
  flagInterlaced: 0
  fieldOrder: 2
  stereoMode: 0
  alphaMode: 0
  pixelWidth: 1024
  pixelHeight: 576
  pixelCropBottom: 0
  pixelCropTop: 0
  pixelCropLeft: 0
  pixelCropRight: 0
  displayWidth: 1354
  displayUnit: 0)");
  }

  void testTracks() {
    Tracks elem{};
    elem.crc32 = 0x146BA085;

    {
      TrackEntry entry{};
      entry.crc32 = 0x4C02C705;
      entry.trackNumber = 1;
      entry.trackUID = 0x6F1A06B3;
      entry.trackType = 1;
      entry.flagLacing = false;
      entry.defaultDuration = MatroskaTimestamp{0x027BC869};
      entry.language = "und";
      entry.codecID = "V_MPEG4/ISO/AVC";
      elem.trackEntries.push_back(std::move(entry));
    }

    testElement(elem,
                {0x16, 0x54, 0xAE, 0x6B, 0xBE, 0xBF, 0x84, 0x85, 0xA0, 0x6B, 0x14, 0xAE, 0xB6, 0xBF, 0x84, 0x05, 0xC7,
                 0x02, 0x4C, 0xD7, 0x81, 0x01, 0x73, 0xC5, 0x84, 0x6F, 0x1A, 0x06, 0xB3, 0x83, 0x81, 0x01, 0x9C, 0x81,
                 0x00, 0x23, 0xE3, 0x83, 0x84, 0x02, 0x7B, 0xC8, 0x69, 0x22, 0xB5, 0x9C, 0x83, 0x75, 0x6E, 0x64, 0x86,
                 0x8F, 0x56, 0x5F, 0x4D, 0x50, 0x45, 0x47, 0x34, 0x2F, 0x49, 0x53, 0x4F, 0x2F, 0x41, 0x56, 0x43},
                R"(crc32: 0x146ba085
trackEntries:
  - crc32: 0x4c02c705
    trackNumber: 1
    trackUID: 1863976627
    trackType: 1
    flagEnabled: true
    flagDefault: true
    flagForced: false
    flagLacing: false
    defaultDuration: 41666665
    trackTimestampScale: 1
    maxBlockAdditionID: 0
    language: 'und'
    codecID: 'V_MPEG4/ISO/AVC'
    codecDecodeAll: true
    codecDelay: 0
    seekPreRoll: 0)");
  }

  void testSimpleTag() {
    SimpleTag elem{};
    elem.tagName = u8"TITLE";
    elem.tagString = u8"Elephant Dream - test 2";

    testElement(elem, {0x67, 0xC8, 0xA2, 0x45, 0xA3, 0x85, 0x54, 0x49, 0x54, 0x4C, 0x45, 0x44, 0x87,
                       0x97, 0x45, 0x6C, 0x65, 0x70, 0x68, 0x61, 0x6E, 0x74, 0x20, 0x44, 0x72, 0x65,
                       0x61, 0x6D, 0x20, 0x2D, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x32},
                "tagName: 'TITLE'\ntagLanguage: 'und'\ntagDefault: true\ntagString: 'Elephant Dream - test 2'");
  }

  void testTag() {
    Tag elem{};
    elem.targets = {};

    {
      SimpleTag tag{};
      tag.tagName = u8"DATE_RELEASED";
      tag.tagString = u8"2010";
      elem.simpleTags.push_back(std::move(tag));
    }

    testElement(elem, {0x73, 0x73, 0x9D, 0x63, 0xC0, 0x80, 0x67, 0xC8, 0x97, 0x45, 0xA3, 0x8D, 0x44, 0x41, 0x54, 0x45,
                       0x5F, 0x52, 0x45, 0x4C, 0x45, 0x41, 0x53, 0x45, 0x44, 0x44, 0x87, 0x84, 0x32, 0x30, 0x31, 0x30},
                R"(targets:
  targetTypeValue: 50
simpleTags:
  - tagName: 'DATE_RELEASED'
    tagLanguage: 'und'
    tagDefault: true
    tagString: '2010')");
  }

  void testTags() {
    Tags elem{};
    elem.crc32 = 0x57778D22;

    {
      Tag tag{};
      SimpleTag simpleTag{};
      simpleTag.tagName = u8"DATE_RELEASED";
      simpleTag.tagString = u8"2010";
      tag.simpleTags.push_back(std::move(simpleTag));
      elem.tags.push_back(std::move(tag));
    }

    testElement(elem, {0x12, 0x54, 0xC3, 0x67, 0xA6, 0xBF, 0x84, 0x22, 0x8D, 0x77, 0x57, 0x73, 0x73, 0x9D, 0x63,
                       0xC0, 0x80, 0x67, 0xC8, 0x97, 0x45, 0xA3, 0x8D, 0x44, 0x41, 0x54, 0x45, 0x5F, 0x52, 0x45,
                       0x4C, 0x45, 0x41, 0x53, 0x45, 0x44, 0x44, 0x87, 0x84, 0x32, 0x30, 0x31, 0x30},
                R"(crc32: 0x57778d22
tags:
  - targets:
      targetTypeValue: 50
    simpleTags:
      - tagName: 'DATE_RELEASED'
        tagLanguage: 'und'
        tagDefault: true
        tagString: '2010')");
  }

  void testCueTrackPositions() {
    CueTrackPositions elem{};
    elem.cueTrack = 1;
    elem.cueClusterPosition = 0x0696;

    testElement(elem, {0xB7, 0x87, 0xF7, 0x81, 0x01, 0xF1, 0x82, 0x06, 0x96},
                "cueTrack: 1\ncueClusterPosition: 1686\ncueCodecState: 0");
  }

  void testCuePoint() {
    CuePoint elem{};

    {
      CueTrackPositions pos{};
      pos.cueTrack = 1;
      pos.cueClusterPosition = 1686;
      elem.cueTrackPositions.push_back(std::move(pos));
    }

    testElement(elem, {0xBB, 0x89, 0xB7, 0x87, 0xF7, 0x81, 0x01, 0xF1, 0x82, 0x06, 0x96},
                R"(cueTime: 0
cueTrackPositions:
  - cueTrack: 1
    cueClusterPosition: 1686
    cueCodecState: 0)");
  }

  void testCues() {
    Cues elem{};
    elem.crc32 = 0xA2CE8E36;

    {
      CuePoint cue{};
      CueTrackPositions pos{};
      pos.cueTrack = 1;
      pos.cueClusterPosition = 1686;
      cue.cueTrackPositions.push_back(std::move(pos));
      elem.cuePoints.push_back(std::move(cue));
    }

    testElement(elem, {0x1C, 0x53, 0xBB, 0x6B, 0x91, 0xBF, 0x84, 0x36, 0x8E, 0xCE, 0xA2,
                       0xBB, 0x89, 0xB7, 0x87, 0xF7, 0x81, 0x01, 0xF1, 0x82, 0x06, 0x96},
                R"(crc32: 0xa2ce8e36
cuePoints:
  - cueTime: 0
    cueTrackPositions:
      - cueTrack: 1
        cueClusterPosition: 1686
        cueCodecState: 0)");
  }

  void testBlockGroup() {
    BlockGroup elem{};

    elem.block.frameDataRanges.push_back({9_bytes, 4_bytes});
    elem.block.frameData = toBytes({0x00, 0xDE, 0xAD, 0xBE, 0xEF});
    elem.block.header.trackNumber = 1;
    elem.block.header.timestampOffset = 40;
    elem.block.header.keyframe = true;
    elem.block.header.lacing = BlockHeader::Lacing::XIPH;
    elem.blockDuration = TrackTimestamp<>{0x0341};
    elem.referenceBlocks.emplace_back(TrackTimestamp<intmax_t>{0x29});

    testElement<BlockGroup, true /* content */>(elem, {0xA0, 0x92, 0xA1, 0x89, 0x81, 0x00, 0x28, 0x82, 0x00, 0xDE,
                                                       0xAD, 0xBE, 0xEF, 0x9B, 0x82, 0x03, 0x41, 0xFB, 0x81, 0x29},
                                                R"(block:
  header:
    trackNumber: 1
    timestampOffset: 40
    keyframe: true
    invisible: false
    lacing: 1
    discardable: false
  frames: 1
blockDuration: 833
referencePriority: 0
referenceBlocks: [41, ])");
  }

  void testCluster() {
    Cluster elem{};
    elem.timestamp = SegmentTimestamp<>{0x034634};
    elem.position = 0x5B7E04;
    elem.prevSize = 0x03AEEB;

    {
      SimpleBlock block{};
      block.frameDataRanges.emplace_back(26_bytes, 4_bytes);
      block.frameData = toBytes({0xDE, 0xAD, 0xBE, 0xAF});
      block.header.trackNumber = 2;
      block.header.timestampOffset = -1;
      elem.simpleBlocks.push_back(std::move(block));
    }

    {
      BlockGroup group{};
      group.block.frameDataRanges.emplace_back(38_bytes, 4_bytes);
      group.block.frameData = toBytes({0xDE, 0xAD, 0xBE, 0xEF});
      group.block.header.trackNumber = 1;
      group.block.header.timestampOffset = 0;
      group.blockDuration = TrackTimestamp<>{0x0341};
      group.referenceBlocks.emplace_back(TrackTimestamp<intmax_t>{0x29});
      elem.blockGroups.push_back(std::move(group));
    }

    testElement<Cluster, true /* content */>(
        elem, {0x1F, 0x43, 0xB6, 0x75, 0xAC, 0xE7, 0x83, 0x03, 0x46, 0x34, 0xA7, 0x83, 0x5B, 0x7E, 0x04, 0xAB, 0x83,
               0x03, 0xAE, 0xEB, 0xA3, 0x88, 0x82, 0xFF, 0xFF, 0x00, 0xDE, 0xAD, 0xBE, 0xAF, 0xA0, 0x91, 0xA1, 0x88,
               0x81, 0x00, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0x9B, 0x82, 0x03, 0x41, 0xFB, 0x81, 0x29},
        R"(timestamp: 214580
position: 5996036
prevSize: 241387
simpleBlocks:
  - header:
      trackNumber: 2
      timestampOffset: -1
      keyframe: false
      invisible: false
      lacing: 0
      discardable: false
    frames: 1
blockGroups:
  - block:
      header:
        trackNumber: 1
        timestampOffset: 0
        keyframe: false
        invisible: false
        lacing: 0
        discardable: false
      frames: 1
    blockDuration: 833
    referencePriority: 0
    referenceBlocks: [41, ])");
  }

  void testSegment() {
    Segment elem{};

    {
      SeekHead head{};
      head.crc32 = 0x9347D8DA;
      Seek seek{};
      seek.crc32 = 0xF736EC24;
      seek.seekID = toBytes({0x12, 0x54, 0xC3, 0x67});
      seek.seekPosition = 0x0206;
      head.seeks.push_back(std::move(seek));
      elem.seekHeads.push_back(std::move(head));
    }

    elem.info.duration = SegmentTimestamp<float>{49064.0F};
    elem.info.muxingApp = u8"libebml2 v0.11.0 + libmatroska2 v0.10.1";

    {
      Tracks tracks{};
      tracks.crc32 = 0xB9486BA7;

      TrackEntry entry{};
      entry.crc32 = 0xC5424727;
      entry.trackNumber = 1U;
      entry.trackType = 1U;
      entry.codecID = "V_MPEG4/ISO/AVC";
      Video video{};
      video.pixelWidth = 0x400;
      video.pixelHeight = 0x240;
      entry.video = std::move(video);
      tracks.trackEntries.push_back(std::move(entry));

      entry = {};
      entry.crc32 = 0x18CCF9AE;
      entry.trackNumber = 2;
      entry.trackType = 2;
      entry.codecID = "A_MPEG/L3";
      Audio audio{};
      audio.samplingFrequency = 48000.0F;
      audio.channels = 2U;
      entry.audio = std::move(audio);
      tracks.trackEntries.push_back(std::move(entry));
      elem.tracks = std::move(tracks);
    }

    {
      Cues cues{};
      cues.crc32 = 0xF826E933;

      CuePoint point{};
      point.cueTime = MatroskaTimestamp{0xBBA9U};
      CueTrackPositions pos{};
      pos.cueTrack = 1;
      pos.cueClusterPosition = 0x013C56B8;
      point.cueTrackPositions.push_back(std::move(pos));

      cues.cuePoints.push_back(std::move(point));
      elem.cues = std::move(cues);
    }

    {
      Cluster cluster{};
      cluster.timestamp = SegmentTimestamp<>{0x3E80};
      cluster.position = 0x501646;

      SimpleBlock block{};
      block.frameDataRanges.emplace_back(228_bytes, 7_bytes);
      block.frameData = toBytes({0xA2, 0x82, 0xFF, 0xF8, 0x80, 0xC4, 0x44});
      block.header.trackNumber = 1;
      block.header.timestampOffset = 2;
      cluster.simpleBlocks.push_back(std::move(block));

      block.frameDataRanges.emplace_back(241_bytes, 4_bytes);
      block.frameData = toBytes({0xDE, 0xAD, 0xBE, 0xAF});
      block.header.trackNumber = 2;
      block.header.timestampOffset = 0;
      cluster.simpleBlocks.push_back(std::move(block));
      elem.clusters.push_back(std::move(cluster));
    }

    {
      Tags tags{};
      tags.crc32 = 0x2BC50F9C;

      Tag tag{};
      SimpleTag simpleTag{};
      simpleTag.tagName = u8"TITLE";
      simpleTag.tagString = u8"Elephant Dream - test 3";
      tag.simpleTags.push_back(std::move(simpleTag));
      tags.tags.push_back(std::move(tag));
      elem.tags.push_back(std::move(tags));
    }

    testElement<Segment, true /* content */>(
        elem,
        {0x18, 0x53, 0x80, 0x67, 0x41, 0x25,
         /* Seekhead */
         0x11, 0x4D, 0x9B, 0x74, 0x9B, /* CRC32 */ 0xBF, 0x84, 0xDA, 0xD8, 0x47, 0x93,
         /* Seek */
         0x4D, 0xBB, 0x92, /* CRC32 */ 0xBF, 0x84, 0x24, 0xEC, 0x36, 0xF7,
         /* seekID */ 0x53, 0xAB, 0x84, 0x12, 0x54, 0xC3, 0x67,
         /* seekPosition */ 0x53, 0xAC, 0x82, 0x02, 0x06,
         /* Info */
         0x15, 0x49, 0xA9, 0x66, 0xB1, /* duration */ 0x44, 0x89, 0x84, 0x47, 0x3F, 0xA8, 0x00, /* muxingApp */ 0x4D,
         0x80, 0xA7, 0x6C, 0x69, 0x62, 0x65, 0x62, 0x6D, 0x6C, 0x32, 0x20, 0x76, 0x30, 0x2E, 0x31, 0x31, 0x2E, 0x30,
         0x20, 0x2B, 0x20, 0x6C, 0x69, 0x62, 0x6D, 0x61, 0x74, 0x72, 0x6F, 0x73, 0x6B, 0x61, 0x32, 0x20, 0x76, 0x30,
         0x2E, 0x31, 0x30, 0x2E, 0x31,
         /* Tracks */
         0x16, 0x54, 0xAE, 0x6B, 0xD3, /* CRC32 */ 0xBF, 0x84, 0xA7, 0x6B, 0x48, 0xB9,
         /* TrackEntry */
         0xAE, 0xA7, /* CRC32 */ 0xBF, 0x84, 0x27, 0x47, 0x42, 0xC5,
         /* trackNumber */ 0xD7, 0x81, 0x01, /* trackType */ 0x83, 0x81, 0x01,
         /* codecID */ 0x86, 0x8F, 0x56, 0x5F, 0x4D, 0x50, 0x45, 0x47, 0x34, 0x2F, 0x49, 0x53, 0x4F, 0x2F, 0x41, 0x56,
         0x43,
         /* Video */ 0xE0, 0x88, 0xB0, 0x82, 0x04, 0x00, 0xBA, 0x82, 0x02, 0x40,
         /* TrackEntry */
         0xAE, 0xA2, /* CRC32 */ 0xBF, 0x84, 0xAE, 0xF9, 0xCC, 0x18,
         /* trackNumber */ 0xD7, 0x81, 0x02, /* trackType */ 0x83, 0x81, 0x02,
         /* codecID */ 0x86, 0x89, 0x41, 0x5F, 0x4D, 0x50, 0x45, 0x47, 0x2F, 0x4C, 0x33,
         /* Audio */ 0xE1, 0x89, 0xB5, 0x84, 0x47, 0x3B, 0x80, 0x00, 0x9F, 0x81, 0x02,
         /* Cues */
         0x1C, 0x53, 0xBB, 0x6B, 0x97, /* CRC32 */ 0xBF, 0x84, 0x33, 0xE9, 0x26, 0xF8,
         /* CuePoint */
         0xBB, 0x8F, /* cueTime */ 0xB3, 0x82, 0xBB, 0xA9,
         /* CueTrackPositions */
         0xB7, 0x89, /* cueTrack */ 0xF7, 0x81, 0x01, /* cueClusterPosition */ 0xF1, 0x84, 0x01, 0x3C, 0x56, 0xB8,
         /* Cluster */
         0x1F, 0x43, 0xB6, 0x75, 0xA0, /* timestamp */ 0xE7, 0x82, 0x3E, 0x80,
         /* position */ 0xA7, 0x83, 0x50, 0x16, 0x46,
         /* SimpleBlock */
         0xA3, 0x8B, 0x81, 0x00, 0x02, 0x00, 0xA2, 0x82, 0xFF, 0xF8, 0x80, 0xC4, 0x44,
         /* SimpleBlock */
         0xA3, 0x88, 0x82, 0x00, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xAF,
         /* Tags */
         0x12, 0x54, 0xC3, 0x67, 0xB1,
         /* CRC32 */ 0xBF, 0x84, 0x9C, 0x0F, 0xC5, 0x2B,
         /* Tag */
         0x73, 0x73, 0xA8,
         /* Targets */ 0x63, 0xC0, 0x80,
         /* SimpleTag */
         0x67, 0xC8, 0xA2, /* tagName */ 0x45, 0xA3, 0x85, 0x54, 0x49, 0x54, 0x4C, 0x45, /* tagString */ 0x44, 0x87,
         0x97, 0x45, 0x6C, 0x65, 0x70, 0x68, 0x61, 0x6E, 0x74, 0x20, 0x44, 0x72, 0x65, 0x61, 0x6D, 0x20, 0x2D, 0x20,
         0x74, 0x65, 0x73, 0x74, 0x20, 0x33},
        R"(seekHeads:
  - crc32: 0x9347d8da
    seeks:
      - crc32: 0xf736ec24
        seekID: [0x12, 0x54, 0xc3, 0x67, ]
        seekPosition: 518
info:
  timestampScale: 1000000
  duration: 49064
  muxingApp: 'libebml2 v0.11.0 + libmatroska2 v0.10.1'
  writingApp: ''
tracks:
  crc32: 0xb9486ba7
  trackEntries:
    - crc32: 0xc5424727
      trackNumber: 1
      trackUID: 0
      trackType: 1
      flagEnabled: true
      flagDefault: true
      flagForced: false
      flagLacing: true
      trackTimestampScale: 1
      maxBlockAdditionID: 0
      language: 'eng'
      codecID: 'V_MPEG4/ISO/AVC'
      codecDecodeAll: true
      codecDelay: 0
      seekPreRoll: 0
      video:
        flagInterlaced: 0
        fieldOrder: 2
        stereoMode: 0
        alphaMode: 0
        pixelWidth: 1024
        pixelHeight: 576
        pixelCropBottom: 0
        pixelCropTop: 0
        pixelCropLeft: 0
        pixelCropRight: 0
        displayUnit: 0
    - crc32: 0x18ccf9ae
      trackNumber: 2
      trackUID: 0
      trackType: 2
      flagEnabled: true
      flagDefault: true
      flagForced: false
      flagLacing: true
      trackTimestampScale: 1
      maxBlockAdditionID: 0
      language: 'eng'
      codecID: 'A_MPEG/L3'
      codecDecodeAll: true
      codecDelay: 0
      seekPreRoll: 0
      audio:
        samplingFrequency: 48000
        channels: 2
        emphasis: 0
cues:
  crc32: 0xf826e933
  cuePoints:
    - cueTime: 48041
      cueTrackPositions:
        - cueTrack: 1
          cueClusterPosition: 20731576
          cueCodecState: 0
clusters:
  - timestamp: 16000
    position: 5248582
    simpleBlocks:
      - header:
          trackNumber: 1
          timestampOffset: 2
          keyframe: false
          invisible: false
          lacing: 0
          discardable: false
        frames: 1
      - header:
          trackNumber: 2
          timestampOffset: 0
          keyframe: false
          invisible: false
          lacing: 0
          discardable: false
        frames: 1
tags:
  - crc32: 0x2bc50f9c
    tags:
      - targets:
          targetTypeValue: 50
        simpleTags:
          - tagName: 'TITLE'
            tagLanguage: 'und'
            tagDefault: true
            tagString: 'Elephant Dream - test 3')");
  }

  void testUnknownSizeSegmentWithCluster() {
    const auto DATA = toBytes({0x18, 0x53, 0x80, 0x67, 0xFF, /* CRC32 */ 0xBF, 0x84, 0x0E, 0x7A, 0x15, 0x19,
                               /* Cluster */
                               0x1F, 0x43, 0xB6, 0x75, 0xFF, /* CRC32 */ 0xBF, 0x84, 0x45, 0x38, 0xC8, 0x5E,
                               /* timestamp */ 0xE7, 0x82, 0x3E, 0x80,
                               /* position */ 0xA7, 0x83, 0x50, 0x16, 0x46,
                               /* SimpleBlock */
                               0xA3, 0x8B, 0x83, 0x00, 0x00, 0x00, 0xA2, 0x82, 0xFF, 0xF8, 0x80, 0xC4, 0x44,
                               /* SimpleBlock */
                               0xA3, 0x88, 0x81, 0x00, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xAF,
                               /* Tags */
                               0x12, 0x54, 0xC3, 0x67, 0xB1,
                               /* CRC32 */ 0xBF, 0x84, 0x9C, 0x0F, 0xC5, 0x2B,
                               /* Tag */
                               0x73, 0x73, 0xA8,
                               /* Targets */ 0x63, 0xC0, 0x80,
                               /* SimpleTag */
                               0x67, 0xC8, 0xA2, /* tagName */ 0x45, 0xA3, 0x85, 0x54, 0x49, 0x54, 0x4C, 0x45,
                               /* tagString */ 0x44, 0x87, 0x97, 0x45, 0x6C, 0x65, 0x70, 0x68, 0x61, 0x6E, 0x74, 0x20,
                               0x44, 0x72, 0x65, 0x61, 0x6D, 0x20, 0x2D, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x33});

    Segment expectedSegment{};
    expectedSegment.crc32 = 0x19157A0E;
    {
      Cluster cluster{};
      cluster.crc32 = 0x5EC83845;
      cluster.timestamp = SegmentTimestamp<>{0x3E80};
      cluster.position = 0x501646;

      SimpleBlock block{};
      block.frameDataRanges.emplace_back(37_bytes, 7_bytes);
      block.frameData = toBytes({0xA2, 0x82, 0xFF, 0xF8, 0x80, 0xC4, 0x44});
      block.header.trackNumber = 3;
      cluster.simpleBlocks.push_back(std::move(block));

      block.frameDataRanges.emplace_back(50_bytes, 4_bytes);
      block.frameData = toBytes({0xDE, 0xAD, 0xBE, 0xAF});
      block.header.trackNumber = 1;
      cluster.simpleBlocks.push_back(std::move(block));
      expectedSegment.clusters.push_back(std::move(cluster));
    }
    {
      Tags tags{};
      tags.crc32 = 0x2BC50F9C;

      Tag tag{};
      SimpleTag simpleTag{};
      simpleTag.tagName = u8"TITLE";
      simpleTag.tagString = u8"Elephant Dream - test 3";
      tag.simpleTags.push_back(std::move(simpleTag));
      tags.tags.push_back(std::move(tag));
      expectedSegment.tags.push_back(std::move(tags));
    }

    bml::BitReader reader{std::span{DATA}};
    Segment segment{};
    segment.read(reader, ReadOptions{true, true});
    TEST_THROWS_NOTHING(reader.assertAlignment(bml::ByteCount{1}));
    TEST_ASSERT_FALSE(reader.hasMoreBytes());
    TEST_ASSERT_EQUALS(expectedSegment, segment);

    checkSkipElement<Segment>(std::span{DATA});
    checkCopyElement<Segment>(std::span{DATA});
  }

  void testMatroskaChunked() {
    std::deque<uint8_t> buffer{0x1a, 0x45, 0xdf, 0xa3, 0xa9, 0xBF, 0x84, 0x84, 0x3f, 0xc8, 0xf0, 0x42,
                               0x86, 0x81, 0x02, 0x42, 0xf7, 0x81, 0x01, 0x42, 0xf2, 0x81, 0x04, 0x42,
                               0xf3, 0x81, 0x08, 0x42, 0x82, 0x88, 0x6d, 0x61, 0x74, 0x72, 0x6f, 0x73,
                               0x6b, 0x61, 0x42, 0x87, 0x81, 0x04, 0x42, 0x85, 0x81, 0x02};

    bml::BitReader reader{[&buffer](std::byte &out) {
      if (buffer.empty()) {
        return false;
      }
      out = std::bit_cast<std::byte>(buffer.front());
      buffer.pop_front();
      return true;
    }};
    Matroska mkv{};
    auto readChunked = mkv.readChunked(reader);

    TEST_ASSERT(readChunked);
    TEST_ASSERT_EQUALS(EBMLHeader::ID, readChunked());
    TEST_ASSERT_EQUALS("matroska", mkv.header.docType.get());
    TEST_ASSERT_EQUALS(4U, mkv.header.docTypeVersion);
    TEST_ASSERT(buffer.empty());

    buffer = {/* Segment */ 0x18, 0x53, 0x80, 0x67, 0xFF, /* Cues */
              0x1C, 0x53, 0xBB, 0x6B, 0x97,
              /* CRC32 */ 0xBF, 0x84, 0x33, 0xE9, 0x26, 0xF8,
              /* CuePoint */
              0xBB, 0x8F,
              /* cueTime */ 0xB3, 0x82, 0xBB, 0xA9,
              /* CueTrackPositions */
              0xB7, 0x89,
              /* cueTrack */ 0xF7, 0x81, 0x01,
              /* cueClusterPosition */ 0xF1, 0x84, 0x01, 0x3C, 0x56, 0xB8,
              /* Cluster (begin) */
              0x1F, 0x43};
    TEST_ASSERT(readChunked);
    TEST_ASSERT_EQUALS(Cues::ID, readChunked());
    TEST_ASSERT(mkv.segment.cues);
    TEST_ASSERT_EQUALS(1U, mkv.segment.cues.value().cuePoints.size());
    TEST_ASSERT_EQUALS(0xBBA9U, mkv.segment.cues.value().cuePoints.front().cueTime.get().value);
    TEST_ASSERT_EQUALS(2U, buffer.size());

    buffer.insert(buffer.end(), {/* Cluster (cont.) */ 0xB6, 0x75, 0xFF, /* CRC32 */ 0xBF, 0x84, 0x45, 0x38, 0xC8, 0x5E,
                                 /* timestamp */ 0xE7, 0x82, 0x3E, 0x80,
                                 /* position */ 0xA7, 0x83, 0x50, 0x16, 0x46,
                                 /* SimpleBlock */
                                 0xA3, 0x8B, 0x83, 0x00, 0x00, 0x00, 0xA2, 0x82, 0xFF, 0xF8, 0x80, 0xC4, 0x44,
                                 /* SimpleBlock */
                                 0xA3, 0x88, 0x81, 0x00, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xAF});
    TEST_ASSERT(readChunked);
    TEST_ASSERT_EQUALS(Cluster::ID, readChunked());
    TEST_ASSERT_EQUALS(1U, mkv.segment.clusters.size());
    TEST_ASSERT_EQUALS(0x3E80U, mkv.segment.clusters.front().timestamp.get().value);
    TEST_ASSERT_EQUALS(0x501646, mkv.segment.clusters.front().position.value());
    TEST_ASSERT(buffer.empty());

    buffer = {/* Cluster */ 0x1F, 0x43, 0xB6, 0x75, 0xAC,
              /* timestamp */
              0xE7, 0x83, 0x03, 0x46, 0x34,
              /* position */ 0xA7, 0x83, 0x5B, 0x7E, 0x04, 0xAB, 0x83, 0x03, 0xAE, 0xEB,
              /* SimpleBlock */ 0xA3, 0x88, 0x82, 0xFF, 0xFF, 0x00, 0xDE, 0xAD, 0xBE, 0xAF,
              /* BlockGroup */ 0xA0, 0x91,
              /* Block */ 0xA1, 0x88, 0x81, 0x00, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF,
              /* blockDuration */ 0x9B, 0x82, 0x03, 0x41, 0xFB, 0x81, 0x29};
    TEST_ASSERT(readChunked);
    TEST_ASSERT_EQUALS(Cluster::ID, readChunked());
    TEST_ASSERT_EQUALS(2U, mkv.segment.clusters.size());
    TEST_ASSERT_EQUALS(214580U, mkv.segment.clusters.back().timestamp.get().value);
    TEST_ASSERT_EQUALS(5996036U, mkv.segment.clusters.back().position.value());
    TEST_ASSERT_EQUALS(241387U, mkv.segment.clusters.back().prevSize.value());

    TEST_ASSERT(readChunked);
    TEST_ASSERT_EQUALS(Segment::ID, readChunked());
    TEST_ASSERT(readChunked);
    TEST_ASSERT_EQUALS(ElementId{}, readChunked());
    TEST_ASSERT_FALSE(readChunked);
  }

private:
  template <EBMLElement Element, bool WithContent = false>
  void testElement(const Element &expectedElement, const std::vector<uint8_t> &data,
                   std::string_view expectedYAMLString) {
    Element element{};
    checkParseElement<Element, WithContent>(std::as_bytes(std::span{data}), element);
    TEST_ASSERT_EQUALS(expectedElement, element);

    checkWriteElement(std::as_bytes(std::span{data}), expectedElement);
    checkPrintYAMLElement(expectedElement, expectedYAMLString);
    checkSkipElement<Element>(std::as_bytes(std::span{data}));
    checkCopyElement<Element>(std::as_bytes(std::span{data}));
  }
};

class TestMkvFrames : public TestElementsBase {
public:
  TestMkvFrames() : TestElementsBase("MkvFrames") {
    TEST_ADD(TestMkvFrames::testBlockHeader);
    TEST_ADD(TestMkvFrames::testBlockNoLacing);
    TEST_ADD(TestMkvFrames::testBlockXiphLacing);
    TEST_ADD(TestMkvFrames::testBlockFixedLacing);
    TEST_ADD(TestMkvFrames::testBlockEBMLLacing);
    TEST_ADD(TestMkvFrames::testFrameView);
    TEST_ADD(TestMkvFrames::testFillFrameDataView);
  }

  void testBlockHeader() {
    BlockHeader header{};
    testValue(header, {0x80, 0x00, 0x00, 0x00}, "trackNumber: 0\ntimestampOffset: 0");

    header.trackNumber = 1012;
    header.timestampOffset = -112;
    header.keyframe = true;
    header.invisible = true;
    header.lacing = BlockHeader::Lacing::FIXED_SIZE;
    header.discardable = true;

    testValue(header, {0x43, 0xF4, 0xFF, 0x90, 0b10001101}, R"(trackNumber: 1012
timestampOffset: -112
keyframe: true
invisible: true
lacing: 2
discardable: true)");
  }

  void testBlockNoLacing() {
    SimpleBlock block{};
    block.frameDataRanges.push_back({6_bytes, 4_bytes});
    block.frameData = toBytes({0xDE, 0xAD, 0xBE, 0xEF});
    TEST_ASSERT_EQUALS(BlockHeader::Lacing::NONE, block.header.lacing);

    testValue(block, {0xA3, 0x88, 0x80, 0x00, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF}, R"(header:
  trackNumber: 0
  timestampOffset: 0
frames:
  - offset: 6
    size: 4)");
  }

  void testBlockXiphLacing() {
    SimpleBlock block{};
    block.header.lacing = BlockHeader::Lacing::XIPH;
    block.frameDataRanges.push_back({9_bytes, 4_bytes});
    block.frameDataRanges.push_back({13_bytes, 2_bytes});
    block.frameDataRanges.push_back({15_bytes, 6_bytes});
    block.frameData =
        toBytes({0x02, 0x04, 0x2, 0xDE, 0xAD, 0xBE, 0xEF, 0xB0, 0x0B, 0xDE, 0xAD, 0xBE, 0xEF, 0xF0, 0x0B});

    testValue(block, {0xA3, 0x93, 0x80, 0x00, 0x00, 0x02, 0x02, 0x04, 0x2,  0xDE, 0xAD,
                      0xBE, 0xEF, 0xB0, 0x0B, 0xDE, 0xAD, 0xBE, 0xEF, 0xF0, 0x0B},
              R"(header:
  trackNumber: 0
  timestampOffset: 0
  lacing: 1
frames:
  - offset: 9
    size: 4
  - offset: 13
    size: 2
  - offset: 15
    size: 6)");
  }

  void testBlockFixedLacing() {
    SimpleBlock block{};
    block.header.lacing = BlockHeader::Lacing::FIXED_SIZE;
    block.frameDataRanges.push_back({7_bytes, 4_bytes});
    block.frameDataRanges.push_back({11_bytes, 4_bytes});
    block.frameData = toBytes({0x01, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF});

    testValue(block, {0xA3, 0x8D, 0x80, 0x00, 0x00, 0x04, 0x01, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF},
              R"(header:
  trackNumber: 0
  timestampOffset: 0
  lacing: 2
frames:
  - offset: 7
    size: 4
  - offset: 11
    size: 4)");
  }

  void testBlockEBMLLacing() {
    SimpleBlock block{};
    block.header.lacing = BlockHeader::Lacing::EBML;
    block.frameDataRanges.push_back({9_bytes, 4_bytes});
    block.frameDataRanges.push_back({13_bytes, 2_bytes});
    block.frameDataRanges.push_back({15_bytes, 6_bytes});
    block.frameData =
        toBytes({0x02, 0x84, 0xBD, 0xDE, 0xAD, 0xBE, 0xEF, 0xB0, 0x0B, 0xDE, 0xAD, 0xBE, 0xEF, 0xF0, 0x0B});

    testValue(block, {0xA3, 0x93, 0x80, 0x00, 0x00, 0x06, 0x02, 0x84, 0xBD, 0xDE, 0xAD,
                      0xBE, 0xEF, 0xB0, 0x0B, 0xDE, 0xAD, 0xBE, 0xEF, 0xF0, 0x0B},
              R"(header:
  trackNumber: 0
  timestampOffset: 0
  lacing: 3
frames:
  - offset: 9
    size: 4
  - offset: 13
    size: 2
  - offset: 15
    size: 6)");
  }

  void testFrameView() {
    Matroska mkv{};

    {
      TrackEntry track{};
      track.trackNumber = 2U;
      track.trackTimestampScale = TrackTimescale{2.0F};

      Tracks tracks{};
      tracks.trackEntries.push_back(std::move(track));
      mkv.segment.tracks = std::move(tracks);
    }

    {
      Cluster cluster{};
      cluster.timestamp = SegmentTimestamp<>{1000};
      {
        SimpleBlock block{};
        block.header.trackNumber = 2;
        block.header.timestampOffset = -42;
        block.frameDataRanges.push_back({10_bytes, 4_bytes});
        block.frameData = toBytes({0xDE, 0xAD, 0xBE, 0xEF});
        cluster.simpleBlocks.push_back(std::move(block));
      }
      {
        SimpleBlock block{};
        block.header.trackNumber = 1;
        block.header.timestampOffset = -22;
        block.frameDataRanges.push_back({15_bytes, 4_bytes});
        block.frameData = toBytes({0xDE, 0xAD, 0xBE, 0xEF});
        cluster.simpleBlocks.push_back(std::move(block));
      }
      {
        BlockGroup group{};
        group.block.header.trackNumber = 2;
        group.block.header.timestampOffset = -7;
        group.block.header.lacing = BlockHeader::Lacing::XIPH;
        group.block.frameDataRanges.push_back({20_bytes, 4_bytes});
        group.block.frameDataRanges.push_back({24_bytes, 2_bytes});
        group.block.frameDataRanges.push_back({26_bytes, 6_bytes});
        group.block.frameData =
            toBytes({0x02, 0x04, 0x2, 0xDE, 0xAD, 0xBE, 0xEF, 0xB0, 0x0B, 0xDE, 0xAD, 0xBE, 0xEF, 0xF0, 0x0B});
        cluster.blockGroups.push_back(std::move(group));
      }
      mkv.segment.clusters.push_back(std::move(cluster));
    }

    {
      Cluster cluster{};
      cluster.timestamp = SegmentTimestamp<>{2000};
      {
        BlockGroup group{};
        group.block.header.trackNumber = 1;
        group.block.header.timestampOffset = -70;
        group.block.header.lacing = BlockHeader::Lacing::EBML;
        group.block.frameDataRanges.push_back({30_bytes, 4_bytes});
        group.block.frameDataRanges.push_back({34_bytes, 2_bytes});
        group.block.frameDataRanges.push_back({36_bytes, 6_bytes});
        group.block.frameData =
            toBytes({0x02, 0x84, 0xBD, 0xDE, 0xAD, 0xBE, 0xEF, 0xB0, 0x0B, 0xDE, 0xAD, 0xBE, 0xEF, 0xF0, 0x0B});
        cluster.blockGroups.push_back(std::move(group));
      }
      {
        BlockGroup group{};
        group.block.header.trackNumber = 2;
        group.block.header.timestampOffset = -50;
        group.block.header.lacing = BlockHeader::Lacing::EBML;
        group.block.frameDataRanges.push_back({40_bytes, 4_bytes});
        group.block.frameDataRanges.push_back({44_bytes, 2_bytes});
        group.block.frameDataRanges.push_back({46_bytes, 6_bytes});
        group.block.frameData =
            toBytes({0x02, 0x84, 0xBD, 0xDE, 0xAD, 0xBE, 0xEF, 0xB0, 0x0B, 0xDE, 0xAD, 0xBE, 0xEF, 0xF0, 0x0B});
        cluster.blockGroups.push_back(std::move(group));
      }
      {
        SimpleBlock block{};
        block.header.trackNumber = 2;
        block.header.timestampOffset = 117;
        block.header.lacing = BlockHeader::Lacing::FIXED_SIZE;
        block.frameDataRanges.push_back({50_bytes, 4_bytes});
        block.frameDataRanges.push_back({54_bytes, 4_bytes});
        block.frameData = toBytes({0x01, 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF});
        cluster.simpleBlocks.push_back(std::move(block));
      }
      mkv.segment.clusters.push_back(std::move(cluster));
    }

    {
      // There is no TrackEntry for track 3
      auto frames = mkv.viewFrames(3U);
      TEST_ASSERT(frames.begin() == frames.end());
    }

    {
      const std::vector<ByteRange> EXPECTED_RANGES = {{10_bytes, 4_bytes}, {20_bytes, 4_bytes}, {24_bytes, 2_bytes},
                                                      {26_bytes, 6_bytes}, {40_bytes, 4_bytes}, {44_bytes, 2_bytes},
                                                      {46_bytes, 6_bytes}, {50_bytes, 4_bytes}, {54_bytes, 4_bytes}};
      const std::vector<std::optional<uintmax_t>> EXPECTED_TIMESTAMPS{
          1000 / 2 - 42, 1000 / 2 - 7, {}, {}, 2000 / 2 - 50, {}, {}, 2000 / 2 + 117, {}};

      std::size_t i = 0;
      for (auto frame : mkv.viewFrames(2U)) {
        TEST_ASSERT_EQUALS(EXPECTED_RANGES.at(i), frame.dataRange);
        if (frame.timestamp) {
          TEST_ASSERT_EQUALS(EXPECTED_TIMESTAMPS.at(i), frame.timestamp->value);
        } else {
          TEST_ASSERT_FALSE(EXPECTED_TIMESTAMPS.at(i));
        }
        ++i;
      }
      TEST_ASSERT_EQUALS(EXPECTED_RANGES.size(), i);
      TEST_ASSERT_EQUALS(EXPECTED_TIMESTAMPS.size(), i);
    }

    {
      // Start at timestamp offset
      auto tmp = mkv.viewFrames(2U, TrackTimestamp<>{50}) | std::views::common;
      std::vector<Frame> frames(tmp.begin(), tmp.end());
      TEST_ASSERT_EQUALS(9U, frames.size());
      TEST_ASSERT_EQUALS(1000U / 2U - 42U, frames.front().timestamp.value().value);

      tmp = mkv.viewFrames(2U, TrackTimestamp<>{490}) | std::views::common;
      frames = std::vector<Frame>(tmp.begin(), tmp.end());
      TEST_ASSERT_EQUALS(8U, frames.size());
      TEST_ASSERT_EQUALS(1000U / 2U - 7U, frames.front().timestamp.value().value);

      tmp = mkv.viewFrames(2U, TrackTimestamp<>{500}) | std::views::common;
      frames = std::vector<Frame>(tmp.begin(), tmp.end());
      TEST_ASSERT_EQUALS(5U, frames.size());
      TEST_ASSERT_EQUALS(2000U / 2U - 50U, frames.front().timestamp.value().value);

      tmp = mkv.viewFrames(2U, TrackTimestamp<>{1000}) | std::views::common;
      frames = std::vector<Frame>(tmp.begin(), tmp.end());
      TEST_ASSERT_EQUALS(2U, frames.size());
      TEST_ASSERT_EQUALS(2000U / 2U + 117U, frames.front().timestamp.value().value);

      tmp = mkv.viewFrames(2U, TrackTimestamp<>{1500}) | std::views::common;
      frames = std::vector<Frame>(tmp.begin(), tmp.end());
      TEST_ASSERT(frames.empty());
    }
  }

  void testFillFrameDataView() {
    const auto DATA = toBytes({// 1st frame
                               0xDE, 0xAD, 0xBE, 0xEF,
                               // 2nd frame
                               0xDE, 0xAD, 0xBE, 0xEF,
                               // garbage
                               0xFF, 0x00, 0xBA, 0xBE, 0xFF, 0xFF, 0xFF,
                               // 3rd frame
                               0xDE, 0xAD, 0xBE, 0xEF,
                               // 4th frame
                               0xB0, 0x0B,
                               // 5th frame
                               0xDE, 0xAD, 0xBE, 0xEF, 0xF0, 0x0B});
    const std::vector<std::pair<uint64_t, std::vector<std::byte>>> FRAMES_DATA{
        {1000 - 42, toBytes({0xDE, 0xAD, 0xBE, 0xEF})},
        {1000 + 22, toBytes({0xDE, 0xAD, 0xBE, 0xEF})},
        {1000 + 70, toBytes({0xDE, 0xAD, 0xBE, 0xEF})},
        {0, toBytes({0xB0, 0x0B})},
        {0, toBytes({0xDE, 0xAD, 0xBE, 0xEF, 0xF0, 0x0B})}};
    Matroska mkv{};

    {
      TrackEntry track{};
      track.trackNumber = 1U;
      Tracks tracks{};
      tracks.trackEntries.push_back(std::move(track));
      mkv.segment.tracks = std::move(tracks);

      Cluster cluster{};
      cluster.timestamp = SegmentTimestamp<>{1000};
      {
        SimpleBlock block{};
        block.header.trackNumber = 1;
        block.header.timestampOffset = -42;
        block.frameDataRanges.push_back({0_bytes, 4_bytes});
        block.frameData = toBytes({0xDE, 0xAD, 0xBE, 0xEF});
        cluster.simpleBlocks.push_back(std::move(block));
      }
      {
        SimpleBlock block{};
        block.header.trackNumber = 1;
        block.header.timestampOffset = 22;
        block.frameDataRanges.push_back({4_bytes, 4_bytes});
        block.frameData = toBytes({0xDE, 0xAD, 0xBE, 0xEF});
        cluster.simpleBlocks.push_back(std::move(block));
      }
      {
        BlockGroup group{};
        group.block.header.trackNumber = 1;
        group.block.header.timestampOffset = 70;
        group.block.header.lacing = BlockHeader::Lacing::XIPH;
        group.block.frameDataRanges.push_back({15_bytes, 4_bytes});
        group.block.frameDataRanges.push_back({19_bytes, 2_bytes});
        group.block.frameDataRanges.push_back({21_bytes, 6_bytes});
        group.block.frameDataRanges.push_back({27_bytes, 2_bytes});
        group.block.frameData = toBytes({0x03, 0x04, 0x2, 0x06, 0xDE, 0xAD, 0xBE, 0xEF, 0xB0, 0x0B, 0xDE, 0xAD, 0xBE,
                                         0xEF, 0xF0, 0x0B, 0xDE, 0xAD});
        cluster.blockGroups.push_back(std::move(group));
      }
      mkv.segment.clusters.push_back(std::move(cluster));
    }

    auto range = mkv.viewFrames(1) | std::ranges::views::take(5) | fillFrameData(DATA);
    auto it = FRAMES_DATA.begin();
    for (auto frame : range) {
      TEST_ASSERT_FALSE(frame.first.data.empty());
      TEST_ASSERT(frame.second.empty());

      if (it->first == 0) {
        TEST_ASSERT_FALSE(frame.first.timestamp);
      } else {
        TEST_ASSERT_EQUALS(it->first, frame.first.timestamp.value().value);
      }
      TEST_ASSERT_EQUALS(it->second, frame.first.data);

      ++it;
    }
    TEST_ASSERT(it == FRAMES_DATA.end());

    std::stringstream ss{};
    ss.write(reinterpret_cast<const char *>(DATA.data()), static_cast<std::streamsize>(DATA.size()));
    auto range2 = mkv.viewFrames(1) | std::ranges::views::take(3) | fillFrameData(ss);
    it = FRAMES_DATA.begin();
    for (auto frame : range2) {
      TEST_ASSERT_FALSE(frame.first.data.empty());
      TEST_ASSERT_FALSE(frame.second.empty());
      TEST_ASSERT_EQUALS(frame.second, frame.first.data);

      if (it->first == 0) {
        TEST_ASSERT_FALSE(frame.first.timestamp);
      } else {
        TEST_ASSERT_EQUALS(it->first, frame.first.timestamp.value().value);
      }
      TEST_ASSERT_EQUALS(it->second, frame.first.data);

      ++it;
    }
    TEST_ASSERT_EQUALS(3, std::distance(FRAMES_DATA.begin(), it));

    // Check error on Frame byte range not in source
    {
      auto fullIt = (mkv.viewFrames(1) | fillFrameData(DATA)).begin();
      for (uint32_t i = 0; i < 5; ++i) {
        TEST_THROWS_NOTHING(*fullIt);
        ++fullIt;
      }
      TEST_THROWS(*fullIt, EndOfStreamError);
    }

    {
      std::stringstream stream{};
      stream.write(reinterpret_cast<const char *>(DATA.data()), static_cast<std::streamsize>(DATA.size()));
      auto fullIt = (mkv.viewFrames(1) | fillFrameData(stream)).begin();
      for (uint32_t i = 0; i < 5; ++i) {
        TEST_THROWS_NOTHING(*fullIt);
        ++fullIt;
      }
      TEST_THROWS(*fullIt, EndOfStreamError);
    }
  }

private:
  template <typename Element>
  void testValue(const Element &expectedElement, const std::vector<uint8_t> &data,
                 std::string_view expectedYAMLString) {
    Element element{};
    checkParseElement<Element, true>(std::as_bytes(std::span{data}), element);
    TEST_ASSERT_EQUALS(expectedElement, element);

    checkWriteElement(std::as_bytes(std::span{data}), expectedElement);
    checkPrintYAMLElement<Element, bml::yaml::PrintFlags::HIDE_DEFAULT>(expectedElement, expectedYAMLString);
    checkSkipElement<Element>(std::as_bytes(std::span{data}));
    checkCopyElement<Element>(std::as_bytes(std::span{data}));
  }
};

class TestMkvOther : public TestElementsBase {
public:
  TestMkvOther() : TestElementsBase("MkvOther") { TEST_ADD(TestMkvOther::testTimestamps); }

  void testTimestamps() {
    TEST_ASSERT_EQUALS(std::chrono::nanoseconds{17}, MatroskaTimestamp{17});

    TEST_ASSERT_EQUALS(MatroskaTimestamp{42}, SegmentTimestamp<>{2} * SegmentTimescale{21});
    TEST_ASSERT_EQUALS(MatroskaTimestamp{50}, SegmentTimestamp<float>{2.4F} * SegmentTimescale{21});
    // round to nearest
    TEST_ASSERT_EQUALS(MatroskaTimestamp{55}, SegmentTimestamp<float>{2.6F} * SegmentTimescale{21});
    TEST_ASSERT_EQUALS(SegmentTimestamp<>{2}, MatroskaTimestamp{42} / SegmentTimescale{21});

    TEST_ASSERT_EQUALS(SegmentTimestamp<>{8}, TrackTimestamp<>{2} * TrackTimescale{4});
    TEST_ASSERT_EQUALS(SegmentTimestamp<>{8}, TrackTimestamp<intmax_t>{2} * TrackTimescale{4});
    TEST_ASSERT_EQUALS(TrackTimestamp<>{2}, SegmentTimestamp<>{8} / TrackTimescale{4});

    TEST_ASSERT_EQUALS(SegmentTimestamp<>{9}, TrackTimestamp<>{2} * TrackTimescale{4.3F});
    TEST_ASSERT_EQUALS(SegmentTimestamp<>{9}, TrackTimestamp<intmax_t>{2} * TrackTimescale{4.3F});
    TEST_ASSERT_EQUALS(TrackTimestamp<>{2}, SegmentTimestamp<>{8} / TrackTimescale{4.3F});
  }
};

void registerMkvTestSuites() {
  Test::registerSuite(Test::newInstance<TestMkvElements>, "mkv");
  Test::registerSuite(Test::newInstance<TestMkvFrames>, "frames");
  Test::registerSuite(Test::newInstance<TestMkvOther>, "other");
}
