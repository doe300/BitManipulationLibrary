#pragma once

#include "ebml.hpp"
#include "mkv_common.hpp"

#include <optional>

/**
 * Types for reading/writing the container structure of the Matroska (MKV) media container format.
 *
 * See: https://www.matroska.org/technical/elements.html
 */
namespace bml::ebml::mkv {
  struct Seek : MasterElement {
    static constexpr ElementId ID{0x4DBB};

    BinaryElement<0x53AB_id> seekID;
    UnsignedIntElement<0x53AC_id> seekPosition;

    constexpr auto operator<=>(const Seek &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Seek, crc32, seekID, seekPosition, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct SeekHead : MasterElement {
    static constexpr ElementId ID{0x114D9B74};

    std::vector<Seek> seeks;

    constexpr auto operator<=>(const SeekHead &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(SeekHead, crc32, seeks, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct ChapterTranslate : MasterElement {
    static constexpr ElementId ID{0x6924};

    BinaryElement<0x69A5_id> chapterTranslateID;
    UnsignedIntElement<0x69BF_id> chapterTranslateCodec;
    std::vector<UnsignedIntElement<0x69FC_id>> chapterTranslateEditionUIDs;

    constexpr auto operator<=>(const ChapterTranslate &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(ChapterTranslate, crc32, chapterTranslateID, chapterTranslateCodec, chapterTranslateEditionUIDs,
                     voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Info : MasterElement {
    static constexpr ElementId ID{0x1549A966};

    std::optional<UUIDElement<0x73A4_id>> segmentUUID;
    std::optional<Utf8StringElement<0x7384_id>> segmentFilename;
    std::optional<UUIDElement<0x3CB923_id>> prevUUID;
    std::optional<Utf8StringElement<0x3C83AB_id>> prevFilename;
    std::optional<UUIDElement<0x3EB923_id>> nextUUID;
    std::optional<Utf8StringElement<0x3E83BB_id>> nextFilename;
    std::vector<BinaryElement<0x4444_id>> segmentFamilies;
    std::vector<ChapterTranslate> chapterTranslates;
    SegmentTimescaleElement<0x2AD7B1_id, 1000000> timestampScale;
    std::optional<SegmentTimestampElement<0x4489_id, float>> duration;
    std::optional<DateElement<0x4461_id>> dateUTC;
    std::optional<Utf8StringElement<0x7BA9_id>> title;
    Utf8StringElement<0x4D80_id> muxingApp;
    Utf8StringElement<0x5741_id> writingApp;

    constexpr auto operator<=>(const Info &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Info, crc32, segmentUUID, segmentFilename, prevUUID, prevFilename, nextUUID, nextFilename,
                     segmentFamilies, chapterTranslates, timestampScale, duration, dateUTC, title, muxingApp,
                     writingApp, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct BlockMore : MasterElement {
    static constexpr ElementId ID{0xA6};

    BinaryElement<0xA5_id> blockAdditional;
    UnsignedIntElement<0xEE_id, 1> blockAddID;

    constexpr auto operator<=>(const BlockMore &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(BlockMore, crc32, blockAdditional, blockAddID, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct BlockAdditions : MasterElement {
    static constexpr ElementId ID{0x75A1};

    std::vector<BlockMore> blockMores;

    constexpr auto operator<=>(const BlockAdditions &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(BlockAdditions, crc32, blockMores, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct SimpleBlock : detail::BaseBlockElement {
    static constexpr ElementId ID{0xA3};

    void read(BitReader &reader, const ReadOptions &options = {}) { readValue(reader, ID, options); }
    void write(BitWriter &writer) const { writeValue(writer, ID); }
  };

  struct Block : detail::BaseBlockElement {
    static constexpr ElementId ID{0xA1};

    void read(BitReader &reader, const ReadOptions &options = {}) { readValue(reader, ID, options); }
    void write(BitWriter &writer) const { writeValue(writer, ID); }
  };

  struct BlockGroup : MasterElement {
    static constexpr ElementId ID{0xA0};

    Block block;
    std::optional<BlockAdditions> blockAdditions;
    std::optional<TrackTimestampElement<0x9B_id>> blockDuration;
    UnsignedIntElement<0xFA_id, 0> referencePriority;
    std::vector<TrackTimestampElement<0xFB_id, intmax_t>> referenceBlocks;
    std::optional<BinaryElement<0xA4_id>> codecState;
    std::optional<MatroskaTimestampElement<0x75A2_id>> discardPadding;

    constexpr auto operator<=>(const BlockGroup &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(BlockGroup, crc32, block, blockAdditions, blockDuration, referencePriority, referenceBlocks,
                     codecState, discardPadding, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Cluster : MasterElement {
    static constexpr ElementId ID{0x1F43B675};

    SegmentTimestampElement<0xE7_id> timestamp;
    std::optional<UnsignedIntElement<0xA7_id>> position;
    std::optional<UnsignedIntElement<0xAB_id>> prevSize;
    std::vector<SimpleBlock> simpleBlocks;
    std::vector<BlockGroup> blockGroups;

    constexpr auto operator<=>(const Cluster &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    static void skip(BitReader &reader);
    static void copy(BitReader &reader, BitWriter &writer);

    friend std::ostream &operator<<(std::ostream &os, const Cluster &cluster);
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct BlockAdditionMapping : MasterElement {
    static constexpr ElementId ID{0x41E4};

    std::optional<UnsignedIntElement<0x41F0_id>> blockAddIDValue;
    std::optional<StringElement<0x41A4_id>> blockAddIDName;
    UnsignedIntElement<0x41E7_id, 0> blockAddIDType;
    std::optional<BinaryElement<0x41ED_id>> blockAddIDExtraData;

    constexpr auto operator<=>(const BlockAdditionMapping &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(BlockAdditionMapping, crc32, blockAddIDValue, blockAddIDName, blockAddIDType, blockAddIDExtraData,
                     voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct TrackTranslate : MasterElement {
    static constexpr ElementId ID{0x6624};

    BinaryElement<0x66A5_id> trackTranslateTrackID;
    UnsignedIntElement<0x66BF_id> trackTranslateCodec;
    std::vector<UnsignedIntElement<0x66FC_id>> trackTranslateEditionUIDs;

    constexpr auto operator<=>(const TrackTranslate &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(TrackTranslate, crc32, trackTranslateTrackID, trackTranslateCodec, trackTranslateEditionUIDs,
                     voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct MasteringMetadata : MasterElement {
    static constexpr ElementId ID{0x55D0};

    std::optional<FloatElement<0x55D1_id>> primaryRChromaticityX;
    std::optional<FloatElement<0x55D2_id>> primaryRChromaticityY;
    std::optional<FloatElement<0x55D3_id>> primaryGChromaticityX;
    std::optional<FloatElement<0x55D4_id>> primaryGChromaticityY;
    std::optional<FloatElement<0x55D5_id>> primaryBChromaticityX;
    std::optional<FloatElement<0x55D6_id>> primaryBChromaticityY;
    std::optional<FloatElement<0x55D7_id>> whitePointChromaticityX;
    std::optional<FloatElement<0x55D8_id>> whitePointChromaticityY;
    std::optional<FloatElement<0x55D9_id>> luminanceMax;
    std::optional<FloatElement<0x55DA_id>> luminanceMin;

    constexpr auto operator<=>(const MasteringMetadata &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(MasteringMetadata, crc32, primaryRChromaticityX, primaryRChromaticityY, primaryGChromaticityX,
                     primaryGChromaticityY, primaryBChromaticityX, primaryBChromaticityY, whitePointChromaticityX,
                     whitePointChromaticityY, luminanceMax, luminanceMin, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Colour : MasterElement {
    static constexpr ElementId ID{0x55B0};

    enum class Coefficients : uint8_t {
      IDENTITY = 0,
      ITU_R_BT_709 = 1,
      UNSPECIFIED = 2,
      RESERVES = 3,
      US_FCC_73_682 = 4,
      ITU_R_BT_470BG = 5,
      SMPTE_170M = 6,
      SMPTE_240M = 7,
      YCoCg = 8,
      ITU_R_BT_BT2020_NCL = 9,
      ITU_R_BT_BT2020_CL = 10,
      SMPTE_ST_2085 = 11,
      CHROMA_DERIVED_NCL = 12,
      CHROMA_DERIVED_CL = 13,
      ITU_R_BT_2100 = 14,
    };

    enum class Characteristics : uint8_t {
      RESERVED = 0,
      ITU_R_BT_709 = 1,
      UNSPECIFIED = 2,
      RESERVED2 = 3,
      ITU_R_BT_470M = 4,
      ITU_R_BT_470BG = 5,
      SMPTE_170M = 6,
      SMPTE_240M = 7,
      LINEAR = 8,
      LOG = 9,
      LOG_SQRT = 10,
      IEC_61966_2_4 = 11,
      ITU_R_BT_1361 = 12,
      IEC_61966_2_1 = 13,
      ITU_R_BT_2020_10BIT = 14,
      ITU_R_BT_2020_12BIT = 15,
      ITU_R_BT_2100_PQ = 16,
      SMPTE_ST_428_1 = 17,
      ARIB_STD_B67_HLG = 18
    };

    enum class Primaries : uint8_t {
      RESERVED = 0,
      ITU_R_BT_709 = 1,
      UNSPECIFIED = 2,
      RESERVED2 = 3,
      ITU_R_BT_470M = 4,
      ITU_R_BT_470BG = 5,
      SMPTE_170M = 6,
      SMPTE_240M = 7,
      FILM = 8,
      ITU_R_BT_2020 = 9,
      SMPTE_ST_428_1 = 10,
      SMPTE_RP_432_2 = 11,
      SMPTE_EG_432_2 = 12,
      EBU_TECH_3213_E = 22,
    };

    EnumElement<0x55B1_id, Coefficients, Coefficients::UNSPECIFIED> matrixCoefficients;
    UnsignedIntElement<0x55B2_id, 0> bitsPerChannel;
    std::optional<UnsignedIntElement<0x55B3_id>> chromaSubsamplingHorz;
    std::optional<UnsignedIntElement<0x55B4_id>> chromaSubsamplingVert;
    std::optional<UnsignedIntElement<0x55B5_id>> cbSubsamplingHorz;
    std::optional<UnsignedIntElement<0x55B6_id>> cbSubsamplingVert;
    UnsignedIntElement<0x55B7_id, 0> chromaSitingHorz;
    UnsignedIntElement<0x55B8_id, 0> chromaSitingVert;
    UnsignedIntElement<0x55B9_id, 0> range;
    EnumElement<0x55BA_id, Characteristics, Characteristics::UNSPECIFIED> transferCharacteristics;
    EnumElement<0x55BB_id, Primaries, Primaries::UNSPECIFIED> primaries;
    std::optional<UnsignedIntElement<0x55BC_id>> maxCLL;
    std::optional<UnsignedIntElement<0x55BD_id>> maxFALL;
    std::optional<MasteringMetadata> masteringMetadata;

    constexpr auto operator<=>(const Colour &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Colour, crc32, matrixCoefficients, bitsPerChannel, chromaSubsamplingHorz, chromaSubsamplingVert,
                     cbSubsamplingHorz, cbSubsamplingVert, chromaSitingHorz, chromaSitingVert, range,
                     transferCharacteristics, primaries, maxCLL, maxFALL, masteringMetadata, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Projection : MasterElement {
    static constexpr ElementId ID{0x7670};

    enum class Type : uint8_t {
      RECTANGULAR = 0,
      EQUIRECTANGULAR = 1,
      CUBEMAP = 2,
      MESH = 3,
    };

    EnumElement<0x7671_id, Type, Type::RECTANGULAR> projectionType;
    std::optional<BinaryElement<0x7672_id>> projectionPrivate;
    FloatElement<0x7673_id, float, 0.0F> projectionPoseYaw;
    FloatElement<0x7674_id, float, 0.0F> projectionPosePitch;
    FloatElement<0x7675_id, float, 0.0F> projectionPoseRoll;

    constexpr auto operator<=>(const Projection &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Projection, crc32, projectionType, projectionPrivate, projectionPoseYaw, projectionPosePitch,
                     projectionPoseRoll, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Video : MasterElement {
    static constexpr ElementId ID{0xE0};

    enum class Interlacing : uint8_t {
      UNDETERMINATED = 0,
      INTERLACED = 1,
      PROGRESSIVE = 2,
    };

    enum class DisplayUnit : uint8_t {
      PIXELS = 0,
      CENTIMETERS = 1,
      INCHES = 2,
      DISPLAY_ASPECT_RATIO = 3,
      UNKNOWN = 4,
    };

    EnumElement<0x9A_id, Interlacing, Interlacing::UNDETERMINATED> flagInterlaced;
    UnsignedIntElement<0x9D_id, 2> fieldOrder;
    UnsignedIntElement<0x53B8_id, 0> stereoMode;
    UnsignedIntElement<0x53C0_id, 0> alphaMode;
    std::optional<UnsignedIntElement<0x53B9_id>> oldStereoMode;
    UnsignedIntElement<0xB0_id> pixelWidth;
    UnsignedIntElement<0xBA_id> pixelHeight;
    UnsignedIntElement<0x54AA_id, 0> pixelCropBottom;
    UnsignedIntElement<0x54BB_id, 0> pixelCropTop;
    UnsignedIntElement<0x54CC_id, 0> pixelCropLeft;
    UnsignedIntElement<0x54DD_id, 0> pixelCropRight;
    std::optional<UnsignedIntElement<0x54B0_id>> displayWidth;
    std::optional<UnsignedIntElement<0x54BA_id>> displayHeight;
    EnumElement<0x54B2_id, DisplayUnit, DisplayUnit::PIXELS> displayUnit;
    std::optional<BinaryElement<0x2EB524_id>> uncompressedFourCC;
    std::optional<Colour> colour;
    std::optional<Projection> projection;

    constexpr auto operator<=>(const Video &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Video, crc32, flagInterlaced, fieldOrder, stereoMode, alphaMode, oldStereoMode, pixelWidth,
                     pixelHeight, pixelCropBottom, pixelCropTop, pixelCropLeft, pixelCropRight, displayWidth,
                     displayHeight, displayUnit, uncompressedFourCC, colour, projection, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Audio : MasterElement {
    static constexpr ElementId ID{0xE1};

    FloatElement<0xB5_id, float, 8000.0F> samplingFrequency;
    std::optional<FloatElement<0x78B5_id>> outputSamplingFrequency;
    UnsignedIntElement<0x9F_id, 1> channels;
    std::optional<UnsignedIntElement<0x6264_id>> bitDepth;
    UnsignedIntElement<0x52F1_id, 0> emphasis;

    constexpr auto operator<=>(const Audio &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Audio, crc32, samplingFrequency, outputSamplingFrequency, channels, bitDepth, emphasis,
                     voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct TrackPlane : MasterElement {
    static constexpr ElementId ID{0xE4};

    UnsignedIntElement<0xE5_id> trackPlaneUID;
    UnsignedIntElement<0xE6_id> trackPlaneType;

    constexpr auto operator<=>(const TrackPlane &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(TrackPlane, crc32, trackPlaneUID, trackPlaneType, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct TrackCombinePlanes : MasterElement {
    static constexpr ElementId ID{0xE3};

    std::vector<TrackPlane> trackPlanes;

    constexpr auto operator<=>(const TrackCombinePlanes &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(TrackCombinePlanes, crc32, trackPlanes, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct TrackJoinBlocks : MasterElement {
    static constexpr ElementId ID{0xE9};

    std::vector<UnsignedIntElement<0xED_id>> trackJoinUIDs;

    constexpr auto operator<=>(const TrackJoinBlocks &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(TrackJoinBlocks, crc32, trackJoinUIDs, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct TrackOperation : MasterElement {
    static constexpr ElementId ID{0xE2};

    std::optional<TrackCombinePlanes> trackCombinePlanes;
    std::optional<TrackJoinBlocks> trackJoinBlocks;

    constexpr auto operator<=>(const TrackOperation &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(TrackOperation, crc32, trackCombinePlanes, trackJoinBlocks, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct ContentCompression : MasterElement {
    static constexpr ElementId ID{0x5034};

    UnsignedIntElement<0x4254_id, 0> contentCompAlgo;
    std::optional<BinaryElement<0x4255_id>> contentCompSettings;

    constexpr auto operator<=>(const ContentCompression &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(ContentCompression, crc32, contentCompAlgo, contentCompSettings, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct ContentEncAESSettings : MasterElement {
    static constexpr ElementId ID{0x47E7};

    UnsignedIntElement<0x47E8_id> aESSettingsCipherMode;

    constexpr auto operator<=>(const ContentEncAESSettings &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(ContentEncAESSettings, crc32, aESSettingsCipherMode, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct ContentEncryption : MasterElement {
    static constexpr ElementId ID{0x5035};

    UnsignedIntElement<0x47E1_id, 0> contentEncAlgo;
    std::optional<BinaryElement<0x47E2_id>> contentEncKeyID;
    std::optional<ContentEncAESSettings> contentEncAESSettings;
    std::optional<BinaryElement<0x47E3_id>> contentSignature;
    std::optional<BinaryElement<0x47E4_id>> contentSigKeyID;
    std::optional<UnsignedIntElement<0x47E5_id, 0>> contentSigAlgo;
    std::optional<UnsignedIntElement<0x47E6_id, 0>> contentSigHashAlgo;

    constexpr auto operator<=>(const ContentEncryption &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(ContentEncryption, crc32, contentEncAlgo, contentEncKeyID, contentEncAESSettings, contentSignature,
                     contentSigKeyID, contentSigAlgo, contentSigHashAlgo, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct ContentEncoding : MasterElement {
    static constexpr ElementId ID{0x6240};

    UnsignedIntElement<0x5031_id, 0> contentEncodingOrder;
    UnsignedIntElement<0x5032_id, 1> contentEncodingScope;
    UnsignedIntElement<0x5033_id, 0> contentEncodingType;
    std::optional<ContentCompression> contentCompression;
    std::optional<ContentEncryption> contentEncryption;

    constexpr auto operator<=>(const ContentEncoding &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(ContentEncoding, crc32, contentEncodingOrder, contentEncodingScope, contentEncodingType,
                     contentCompression, contentEncryption, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct ContentEncodings : MasterElement {
    static constexpr ElementId ID{0x6D80};

    std::vector<ContentEncoding> contentEncodings;

    constexpr auto operator<=>(const ContentEncodings &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(ContentEncodings, crc32, contentEncodings, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct TrackEntry : MasterElement {
    static constexpr ElementId ID{0xAE};

    enum class Type : uint8_t {
      VIDEO = 1,
      AUDIO = 2,
      COMPLEX = 3,
      LOGO = 16,
      SUBTITLE = 17,
      BUTTONS = 18,
      CONTROL = 32,
      METADATA = 33,
    };

    UnsignedIntElement<0xD7_id> trackNumber;
    UnsignedIntElement<0x73C5_id> trackUID;
    EnumElement<0x83_id, Type> trackType;
    BoolElement<0xB9_id, true> flagEnabled;
    BoolElement<0x88_id, true> flagDefault;
    BoolElement<0x55AA_id, false> flagForced;
    std::optional<BoolElement<0x55AB_id>> flagHearingImpaired;
    std::optional<BoolElement<0x55AC_id>> flagVisualImpaired;
    std::optional<BoolElement<0x55AD_id>> flagTextDescriptions;
    std::optional<BoolElement<0x55AE_id>> flagOriginal;
    std::optional<BoolElement<0x55AF_id>> flagCommentary;
    BoolElement<0x9C_id, true> flagLacing;
    std::optional<MatroskaTimestampElement<0x23E383_id>> defaultDuration;
    std::optional<MatroskaTimestampElement<0x234E7A_id>> defaultDecodedFieldDuration;
    TrackTimescaleElement<0x23314F_id, 1.0F> trackTimestampScale;
    UnsignedIntElement<0x55EE_id, 0> maxBlockAdditionID;
    std::vector<BlockAdditionMapping> blockAdditionMappings;
    std::optional<Utf8StringElement<0x536E_id>> name;
    StringElement<0x22B59C_id, "eng"> language;
    std::optional<StringElement<0x22B59D_id>> languageBCP47;
    StringElement<0x86_id> codecID;
    std::optional<BinaryElement<0x63A2_id>> codecPrivate;
    std::optional<Utf8StringElement<0x258688_id>> codecName;
    std::optional<UnsignedIntElement<0x7446_id>> attachmentLink;
    BoolElement<0xAA_id, true> codecDecodeAll;
    std::vector<UnsignedIntElement<0x6FAB_id>> trackOverlays;
    MatroskaTimestampElement<0x56AA_id> codecDelay;
    MatroskaTimestampElement<0x56BB_id> seekPreRoll;
    std::vector<TrackTranslate> trackTranslates;
    std::optional<Video> video;
    std::optional<Audio> audio;
    std::optional<TrackOperation> trackOperation;
    std::optional<ContentEncodings> contentEncodings;

    constexpr auto operator<=>(const TrackEntry &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(TrackEntry, crc32, trackNumber, trackUID, trackType, flagEnabled, flagDefault, flagForced,
                     flagHearingImpaired, flagVisualImpaired, flagTextDescriptions, flagOriginal, flagCommentary,
                     flagLacing, defaultDuration, defaultDecodedFieldDuration, trackTimestampScale, maxBlockAdditionID,
                     blockAdditionMappings, name, language, languageBCP47, codecID, codecPrivate, codecName,
                     attachmentLink, codecDecodeAll, trackOverlays, codecDelay, seekPreRoll, trackTranslates, video,
                     audio, trackOperation, contentEncodings, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Tracks : MasterElement {
    static constexpr ElementId ID{0x1654AE6B};

    std::vector<TrackEntry> trackEntries;

    constexpr auto operator<=>(const Tracks &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Tracks, crc32, trackEntries, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct CueReference : MasterElement {
    static constexpr ElementId ID{0xDB};

    MatroskaTimestampElement<0x96_id> cueRefTime;

    constexpr auto operator<=>(const CueReference &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(CueReference, crc32, cueRefTime, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct CueTrackPositions : MasterElement {
    static constexpr ElementId ID{0xB7};

    UnsignedIntElement<0xF7_id> cueTrack;
    UnsignedIntElement<0xF1_id> cueClusterPosition;
    std::optional<UnsignedIntElement<0xF0_id>> cueRelativePosition;
    std::optional<SegmentTimestampElement<0xB2_id>> cueDuration;
    std::optional<UnsignedIntElement<0x5378_id>> cueBlockNumber;
    UnsignedIntElement<0xEA_id, 0> cueCodecState;
    std::vector<CueReference> cueReferences;

    constexpr auto operator<=>(const CueTrackPositions &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(CueTrackPositions, crc32, cueTrack, cueClusterPosition, cueRelativePosition, cueDuration,
                     cueBlockNumber, cueCodecState, cueReferences, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct CuePoint : MasterElement {
    static constexpr ElementId ID{0xBB};

    MatroskaTimestampElement<0xB3_id> cueTime;
    std::vector<CueTrackPositions> cueTrackPositions;

    constexpr auto operator<=>(const CuePoint &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(CuePoint, crc32, cueTime, cueTrackPositions, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Cues : MasterElement {
    static constexpr ElementId ID{0x1C53BB6B};

    std::vector<CuePoint> cuePoints;

    constexpr auto operator<=>(const Cues &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Cues, crc32, cuePoints, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct AttachedFile : MasterElement {
    static constexpr ElementId ID{0x61A7};

    std::optional<Utf8StringElement<0x467E_id>> fileDescription;
    Utf8StringElement<0x466E_id> fileName;
    StringElement<0x4660_id> fileMediaType;
    BinaryElement<0x465C_id> fileData;
    UnsignedIntElement<0x46AE_id> fileUID;

    constexpr auto operator<=>(const AttachedFile &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(AttachedFile, crc32, fileDescription, fileName, fileMediaType, fileData, fileUID, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Attachments : MasterElement {
    static constexpr ElementId ID{0x1941A469};

    std::vector<AttachedFile> attachedFiles;

    constexpr auto operator<=>(const Attachments &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Attachments, crc32, attachedFiles, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct EditionDisplay : MasterElement {
    static constexpr ElementId ID{0x4520};

    Utf8StringElement<0x4521_id> editionString;
    std::vector<StringElement<0x45E4_id>> editionLanguageIETFs;

    constexpr auto operator<=>(const EditionDisplay &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(EditionDisplay, crc32, editionString, editionLanguageIETFs, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct ChapterTrack : MasterElement {
    static constexpr ElementId ID{0x8F};

    std::vector<UnsignedIntElement<0x89_id>> chapterTrackUIDs;

    constexpr auto operator<=>(const ChapterTrack &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(ChapterTrack, crc32, chapterTrackUIDs, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct ChapterDisplay : MasterElement {
    static constexpr ElementId ID{0x80};

    Utf8StringElement<0x85_id> chapString;
    std::vector<StringElement<0x437C_id, "eng">> chapLanguages;
    std::vector<StringElement<0x437D_id>> chapLanguageBCP47s;
    std::vector<StringElement<0x437E_id>> chapCountries;

    constexpr auto operator<=>(const ChapterDisplay &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(ChapterDisplay, crc32, chapString, chapLanguages, chapLanguageBCP47s, chapCountries, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct ChapProcessCommand : MasterElement {
    static constexpr ElementId ID{0x6911};

    UnsignedIntElement<0x6922_id> chapProcessTime;
    BinaryElement<0x6933_id> chapProcessData;

    constexpr auto operator<=>(const ChapProcessCommand &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(ChapProcessCommand, crc32, chapProcessTime, chapProcessData, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct ChapProcess : MasterElement {
    static constexpr ElementId ID{0x6944};

    UnsignedIntElement<0x6955_id, 0> chapProcessCodecID;
    std::optional<BinaryElement<0x450D_id>> chapProcessPrivate;
    std::vector<ChapProcessCommand> chapProcessCommands;

    constexpr auto operator<=>(const ChapProcess &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(ChapProcess, crc32, chapProcessCodecID, chapProcessPrivate, chapProcessCommands, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct ChapterAtom : MasterElement {
    static constexpr ElementId ID{0xB6};

    enum class SkipType : uint8_t {
      NONE = 0,
      OPENING_CREDITS = 1,
      END_CREDITS = 2,
      RECAP = 3,
      NEXT_PREVIEW = 4,
      PREVIEW = 5,
      ADVERTISEMENT = 6,
    };

    UnsignedIntElement<0x73C4_id> chapterUID;
    std::optional<Utf8StringElement<0x5654_id>> chapterStringUID;
    MatroskaTimestampElement<0x91_id> chapterTimeStart;
    std::optional<MatroskaTimestampElement<0x92_id>> chapterTimeEnd;
    BoolElement<0x98_id, false> chapterFlagHidden;
    BoolElement<0x4598_id, true> chapterFlagEnabled;
    std::optional<UUIDElement<0x6E67_id>> chapterSegmentUUID;
    std::optional<EnumElement<0x4588_id, SkipType>> chapterSkipType;
    std::optional<UnsignedIntElement<0x6EBC_id>> chapterSegmentEditionUID;
    std::optional<UnsignedIntElement<0x63C3_id>> chapterPhysicalEquiv;
    std::optional<ChapterTrack> chapterTrack;
    std::vector<ChapterDisplay> chapterDisplays;
    std::vector<ChapProcess> chapProcess;

    constexpr auto operator<=>(const ChapterAtom &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(ChapterAtom, crc32, chapterUID, chapterStringUID, chapterTimeStart, chapterTimeEnd,
                     chapterFlagHidden, chapterFlagEnabled, chapterSegmentUUID, chapterSkipType,
                     chapterSegmentEditionUID, chapterPhysicalEquiv, chapterTrack, chapterDisplays, chapProcess,
                     voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct EditionEntry : MasterElement {
    static constexpr ElementId ID{0x45B9};

    std::optional<UnsignedIntElement<0x45BC_id>> editionUID;
    BoolElement<0x45BD_id, false> editionFlagHidden;
    BoolElement<0x45DB_id, false> editionFlagDefault;
    BoolElement<0x45DD_id, false> editionFlagOrdered;
    std::vector<EditionDisplay> editionDisplays;
    std::vector<ChapterAtom> chapterAtoms;

    constexpr auto operator<=>(const EditionEntry &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(EditionEntry, crc32, editionUID, editionFlagHidden, editionFlagDefault, editionFlagOrdered,
                     editionDisplays, chapterAtoms, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Chapters : MasterElement {
    static constexpr ElementId ID{0x1043A770};

    std::vector<EditionEntry> editionEntries;

    constexpr auto operator<=>(const Chapters &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Chapters, crc32, editionEntries, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Targets : MasterElement {
    static constexpr ElementId ID{0x63C0};

    enum class Type : uint8_t {
      COLLECTION = 70,
      EDITION = 60,
      ALBUM = 50,
      PART = 40,
      TRACK = 30,
      SUBTRACK = 20,
      SHOT = 10,
    };

    EnumElement<0x68CA_id, Type, Type::ALBUM> targetTypeValue;
    std::optional<StringElement<0x63CA_id>> targetType;
    std::vector<UnsignedIntElement<0x63C5_id, 0>> tagTrackUIDs;
    std::vector<UnsignedIntElement<0x63C9_id, 0>> tagEditionUIDs;
    std::vector<UnsignedIntElement<0x63C4_id, 0>> tagChapterUIDs;
    std::vector<UnsignedIntElement<0x63C6_id, 0>> tagAttachmentUIDs;

    constexpr auto operator<=>(const Targets &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Targets, crc32, targetTypeValue, targetType, tagTrackUIDs, tagEditionUIDs, tagChapterUIDs,
                     tagAttachmentUIDs, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct SimpleTag : MasterElement {
    static constexpr ElementId ID{0x67C8};

    Utf8StringElement<0x45A3_id> tagName;
    StringElement<0x447A_id, "und"> tagLanguage;
    std::optional<StringElement<0x447B_id>> tagLanguageBCP47;
    BoolElement<0x4484_id, true> tagDefault;
    std::optional<Utf8StringElement<0x4487_id>> tagString;
    std::optional<BinaryElement<0x4485_id>> tagBinary;

    constexpr auto operator<=>(const SimpleTag &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(SimpleTag, crc32, tagName, tagLanguage, tagLanguageBCP47, tagDefault, tagString, tagBinary,
                     voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Tag : MasterElement {
    static constexpr ElementId ID{0x7373};

    Targets targets;
    std::vector<SimpleTag> simpleTags;

    constexpr auto operator<=>(const Tag &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Tag, crc32, targets, simpleTags, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Tags : MasterElement {
    static constexpr ElementId ID{0x1254C367};

    std::vector<Tag> tags;

    constexpr auto operator<=>(const Tags &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    void write(bml::BitWriter &writer) const;

    BML_DEFINE_PRINT(Tags, crc32, tags, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  struct Segment : MasterElement {
    static constexpr ElementId ID{0x18538067};

    std::vector<SeekHead> seekHeads;
    Info info;
    std::optional<Tracks> tracks;
    std::optional<Cues> cues;
    std::optional<Chapters> chapters;
    std::vector<Cluster> clusters;
    std::optional<Attachments> attachments;
    std::vector<Tags> tags;

    constexpr auto operator<=>(const Segment &) const noexcept = default;

    void read(bml::BitReader &reader, const ReadOptions &options = {});
    [[nodiscard]] ChunkedReader readChunked(bml::BitReader &reader, ReadOptions options = {}) &;
    void write(bml::BitWriter &writer) const;

    static void skip(BitReader &reader);
    static void copy(BitReader &reader, BitWriter &writer);

    BML_DEFINE_PRINT(Segment, crc32, seekHeads, info, tracks, cues, chapters, clusters, attachments, tags, voidElements)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;
  };

  class FrameView;

  /**
   * Container for a whole Matroska stream/file.
   */
  struct Matroska {
    EBMLHeader header;
    Segment segment;

    constexpr auto operator<=>(const Matroska &) const noexcept = default;

    /**
     * Reads the whole Matroska container from the underlying input.
     */
    void read(BitReader &reader, const ReadOptions &options);
    [[nodiscard]] ChunkedReader readChunked(bml::BitReader &reader, ReadOptions options = {}) &;

    /**
     * Writes the whole Matroska container to the underlying output.
     *
     * NOTE: Attempting to write a Matroska object without data will result in an exception being thrown.
     */
    void write(BitWriter &writer) const;

    BML_DEFINE_PRINT(Matroska, header, segment)
    std::ostream &printYAML(std::ostream &os, const bml::yaml::Options &options) const;

    /**
     * Returns a pointer to the TrackEntry for the given track number, if present in this Matroska container.
     */
    const TrackEntry *getTrackEntry(uint32_t trackNumber) const;

    /**
     * Returns an input range producing each stored Frame of the given Track number.
     *
     * If the optional start parameter is given, the returned view will start off at the next Frame with a timestamp not
     * smaller than the given value.
     *
     * NOTE: Only the first Frames per Block are considered when searching for a specified start timestamp, i.e. the
     * effective timestamps of the other laced Frames are not interpolated!
     */
    FrameView viewFrames(uint32_t trackNumber, const TrackTimestamp<> &start = {}) const;
  };

} // namespace bml::ebml::mkv
