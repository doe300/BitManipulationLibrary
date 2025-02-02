#include "mkv.hpp"

#include "io.hpp"
#include "reader.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

static void printHelp() {
  std::cout << "Usage: <application> [options] <MKV file>" << std::endl;
  std::cout << "The following options are available:" << std::endl;
  std::cout << "-h, --help      Print this help and exit" << std::endl;
  std::cout << "-v, --validate  Validate CRC-32s on read" << std::endl;
  std::cout << "-y, --yaml      Print the media structure as YAML document" << std::endl;
}

int main(int argc, char **argv) {

  if (argc < 2 || argc > 4) {
    printHelp();
    return EXIT_FAILURE;
  }

  bool printYaml = false;
  bml::ebml::ReadOptions options{};
  for (int i = 1; i < argc - 1; ++i) {
    if (std::string{"-h"} == argv[i] || std::string{"--help"} == argv[i]) {
      printHelp();
      return EXIT_SUCCESS;
    } else if (std::string{"-y"} == argv[i] || std::string{"--yaml"} == argv[i]) {
      printYaml = true;
    } else if (std::string{"-v"} == argv[i] || std::string{"--validate"} == argv[i]) {
      options.validateCRC32 = true;
    } else {
      std::cout << "Unknown option: " << argv[i] << std::endl;
      printHelp();
      return EXIT_FAILURE;
    }
  }

  std::ifstream fis{argv[argc - 1]};
  bml::BitReader reader{fis};
  bml::ebml::mkv::Matroska mkv{};
  mkv.read(reader, options);
  if (printYaml) {
    bml::yaml::Options opts{};
    opts.flags = bml::yaml::PrintFlags::HIDE_EMPTY | bml::yaml::PrintFlags::HIDE_DEFAULT;
    mkv.printYAML(std::cout, opts) << std::endl;
  } else {
    std::cout << mkv << std::endl;
  }

  return EXIT_SUCCESS;
}
