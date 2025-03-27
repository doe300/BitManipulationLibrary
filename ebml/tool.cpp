#include "mkv.hpp"

#include "io.hpp"
#include "mkv_frames.hpp"
#include "reader.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

using namespace bml::ebml;

static void printHelp() {
  std::cout << R"(Usage: <application> [options] <MKV file>
The following options are available:
  -h, --help                  Print this help and exit
  -v, --validate              Validate CRC-32s on read
  -p, --print                 Print the internal representation of the media structure
  -y, --yaml                  Print the media structure as YAML document
  -i, --info                  Print the general information about the input media
  -l, --list-tracks           List all contained tracks and their generic properties
  -t, --track <num>           Select the given track for track-specific actions
  -tp, --track-private <file> Dump the selected track's codec private data (initialization) to the given file
  -td, --track-data <file>    Dump the selected track's frame data to the given file)"
            << std::endl;
}

static std::ostream *openOutput(std::string_view path, std::unique_ptr<std::ostream> &cache, bool append) {
  if (path == "-" || path == "/dev/stdout") {
    return &std::cout;
  } else if (path == "/dev/stderr") {
    return &std::cerr;
  }
  cache = std::make_unique<std::ofstream>(path.data(), std::ios::binary | (append ? std::ios::app : std::ios::trunc));
  return cache.get();
}

static void writeInfo(const mkv::Info &info) {
  std::cout << "Segment Info:" << std::endl;
  if (info.title) {
    std::cout << "- Title: " << bml::printView(*info.title) << std::endl;
  }
  if (info.duration) {
    std::cout << "- Duration: " << info.duration->get() * info.timestampScale << std::endl;
  }
  if (info.dateUTC) {
    std::cout << "- Date: " << *info.dateUTC << std::endl;
  }
  std::cout << "- Muxing App: " << bml::printView(info.muxingApp) << std::endl;
  std::cout << "- Writing App: " << bml::printView(info.writingApp) << std::endl;
  if (info.segmentFilename) {
    std::cout << "- Segment filename: " << bml::printView(*info.segmentFilename) << std::endl;
  }
  if (info.prevFilename) {
    std::cout << "- Previous filename: " << bml::printView(*info.prevFilename) << std::endl;
  }
  if (info.nextFilename) {
    std::cout << "- Next filename: " << bml::printView(*info.nextFilename) << std::endl;
  }
}

// clang-format off
static constexpr std::string_view toTypeString(mkv::TrackEntry::Type type) noexcept {
  using Type=mkv::TrackEntry::Type;
  switch (type) {
  case Type::VIDEO: return "video";
  case Type::AUDIO: return "audio";
  case Type::COMPLEX: return "complex";
  case Type::LOGO: return "logo";
  case Type::SUBTITLE: return "subtitle";
  case Type::BUTTONS: return "buttons";
  case Type::CONTROL: return "control";
  case Type::METADATA: return "metadata";
  }
  return "(unknown)";
}

static constexpr std::string_view toDisplayUnit(mkv::Video::DisplayUnit unit) noexcept {
  using Unit = mkv::Video::DisplayUnit;
  switch (unit) {
  case Unit::PIXELS: return "px";
  case Unit::CENTIMETERS: return "cm";
  case Unit::INCHES: return "in";
  case Unit::DISPLAY_ASPECT_RATIO: return "display aspect ratio";
  case Unit::UNKNOWN: return "unknown";
  }
  return "(unknown)";
}
// clang-format on

static void writeTracks(const mkv::Tracks &tracks) {
  for (const auto &track : tracks.trackEntries) {
    std::cout << "Track " << track.trackNumber << ':' << std::endl;
    std::cout << "- Type: " << toTypeString(track.trackType) << std::endl;
    std::cout << "- Flags: " << (track.flagEnabled ? "enabled" : "disabled") << (track.flagDefault ? ", default" : "")
              << (track.flagForced ? ", forced" : "")
              << (track.flagHearingImpaired && *track.flagHearingImpaired ? ", hearing impaired" : "")
              << (track.flagVisualImpaired && *track.flagVisualImpaired ? ", visual impaired" : "")
              << (track.flagTextDescriptions && *track.flagTextDescriptions ? ", text descriptions" : "")
              << (track.flagOriginal && *track.flagOriginal ? ", OV" : "")
              << (track.flagCommentary && *track.flagCommentary ? ", commentary" : "")
              << (track.flagLacing ? ", lacing" : "") << std::endl;
    if (track.defaultDuration) {
      std::cout << "- Frame duration: " << *track.defaultDuration << std::endl;
    }
    if (track.name) {
      std::cout << "-  Name: " << bml::printView(*track.name) << std::endl;
    }

    std::cout << "- Language: " << track.language;
    if (track.languageBCP47) {
      std::cout << " (" << *track.languageBCP47 << ')';
    }
    std::cout << std::endl;

    std::cout << "- Codec: " << track.codecID;
    if (track.codecName) {
      std::cout << " (" << bml::printView(*track.codecName) << ')';
    }
    std::cout << std::endl;

    if (track.audio) {
      const auto &audio = *track.audio;
      std::cout << "- Audio: " << audio.samplingFrequency << " Hz";
      if (audio.outputSamplingFrequency) {
        std::cout << " (output at " << *audio.outputSamplingFrequency << " Hz)";
      }
      std::cout << ", " << audio.channels << " channels";
      if (audio.bitDepth) {
        std::cout << ", " << *audio.bitDepth << " bits";
      }
      std::cout << std::endl;
    }
    if (track.video) {
      const auto &video = *track.video;
      std::cout << "- Video: " << video.pixelWidth << 'x' << video.pixelHeight << ' '
                << toDisplayUnit(video.displayUnit);
      if (video.pixelCropLeft || video.pixelCropTop || video.pixelCropRight || video.pixelCropBottom) {
        std::cout << " - [" << video.pixelCropLeft << ", " << video.pixelCropTop << ", " << video.pixelCropRight << ", "
                  << video.pixelCropBottom << ']';
      }
      if (video.displayWidth && video.displayHeight) {
        std::cout << " (output at " << *video.displayWidth << 'x' << *video.displayHeight << ' '
                  << toDisplayUnit(video.displayUnit) << ')';
      }

      std::cout << (video.flagInterlaced.get() == mkv::Video::Interlacing::PROGRESSIVE  ? ", progressive"
                    : video.flagInterlaced.get() == mkv::Video::Interlacing::INTERLACED ? ", interlaced"
                                                                                        : "");
      if (video.uncompressedFourCC) {
        const auto fourcc = video.uncompressedFourCC->get();
        std::cout << ", " << std::string_view{reinterpret_cast<const char *>(fourcc.data()), fourcc.size()};
      }
      std::cout << std::endl;
    }
  }
}

int main(int argc, char **argv) {
  using namespace std::string_view_literals;

  if (argc < 2) {
    printHelp();
    return EXIT_FAILURE;
  }

  bool printRaw = false;
  bool printYaml = false;
  bool printInfo = false;
  bool printTracks = false;
  uint32_t trackNum = 0;

  std::string_view codecFile;
  std::string_view dataFile;

  ReadOptions options{};
  int i = 1;
  for (; i < argc - 1; ++i) {
    if ("-h"sv == argv[i] || "--help"sv == argv[i]) {
      printHelp();
      return EXIT_SUCCESS;
    } else if ("-p"sv == argv[i] || "--print"sv == argv[i]) {
      printRaw = true;
    } else if ("-y"sv == argv[i] || "--yaml"sv == argv[i]) {
      printYaml = true;
    } else if ("-i"sv == argv[i] || "--info"sv == argv[i]) {
      printInfo = true;
    } else if ("-l"sv == argv[i] || "--list-tracks"sv == argv[i]) {
      printTracks = true;
    } else if ("-v"sv == argv[i] || "--validate"sv == argv[i]) {
      options.validateCRC32 = true;
    } else if ("-t"sv == argv[i] || "--track"sv == argv[i]) {
      trackNum = static_cast<uint32_t>(std::stoul(argv[i + 1]));
      ++i;
    } else if ("-tp"sv == argv[i] || "--track-private"sv == argv[i]) {
      codecFile = argv[i + 1];
      ++i;
    } else if ("-td"sv == argv[i] || "--track-data"sv == argv[i]) {
      dataFile = argv[i + 1];
      ++i;
    } else {
      std::cout << "Unknown option: " << argv[i] << std::endl;
      printHelp();
      return EXIT_FAILURE;
    }
  }

  if (i >= argc) {
    std::cout << "Missing input file path!" << std::endl;
    printHelp();
    return EXIT_FAILURE;
  } else if (trackNum == 0 && (!codecFile.empty() || !dataFile.empty())) {
    std::cout << "Track-specific options require a non-zero track number to be selected!" << std::endl;
    printHelp();
    return EXIT_FAILURE;
  }

  std::ifstream fis{argv[argc - 1]};
  auto &input = argv[argc - 1] == "-"sv || argv[argc - 1] == "/dev/stdin"sv ? std::cin : fis;
  bml::BitReader reader{input};
  mkv::Matroska mkv{};
  mkv.read(reader, options);

  if (trackNum && !codecFile.empty()) {
    if (const auto *track = mkv.getTrackEntry(trackNum)) {
      std::unique_ptr<std::ostream> tmp{};
      auto *out = openOutput(codecFile, tmp, false /* truncate */);
      auto data = track->codecPrivate ? track->codecPrivate->get() : std::vector<std::byte>{};
      if (!data.empty()) {
        out->write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
      }
      out->flush();
    } else {
      std::cout << "The Track with the number " << trackNum << " does not exist in the input file!" << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (trackNum && !dataFile.empty()) {
    if (!mkv.getTrackEntry(trackNum)) {
      std::cout << "The Track with the number " << trackNum << " does not exist in the input file!" << std::endl;
      return EXIT_FAILURE;
    }
    std::unique_ptr<std::ostream> tmp{};
    auto *out = openOutput(dataFile, tmp, dataFile == codecFile);
    auto frames = mkv.viewFrames(trackNum) | mkv::fillFrameData(input);
    for (const auto &frame : frames) {
      auto data = frame.data.data();
      out->write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    }

    out->flush();
  }

  if (printYaml) {
    bml::yaml::Options opts{};
    opts.flags = bml::yaml::PrintFlags::HIDE_EMPTY | bml::yaml::PrintFlags::HIDE_DEFAULT;
    mkv.printYAML(std::cout, opts) << std::endl;
  } else if (printRaw) {
    std::cout << mkv << std::endl;
  }

  if (printInfo) {
    writeInfo(mkv.segment.info);
  }
  if (printTracks && mkv.segment.tracks) {
    writeTracks(*mkv.segment.tracks);
  }

  return EXIT_SUCCESS;
}
