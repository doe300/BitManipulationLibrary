#include "mkv.hpp"

#include "io.hpp"
#include "reader.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

static void printHelp() {
  std::cout << "Usage: <application> [options] <MKV/WEBM file>" << std::endl;
  std::cout << "The following options are available:" << std::endl;
  std::cout << "-h, --help      Print this help and exit" << std::endl;
  std::cout << "-y, --yaml      Print the media structure as YAML document" << std::endl;
}

int main(int argc, char **argv) {

  if (argc < 2 || argc > 3) {
    printHelp();
    return EXIT_FAILURE;
  }

  bool printYaml = false;
  for (int i = 1; i < argc - 1; ++i) {
    if (std::string{"-h"} == argv[i] || std::string{"--help"} == argv[i]) {
      printHelp();
      return EXIT_SUCCESS;
    } else if (std::string{"-y"} == argv[i] || std::string{"--yaml"} == argv[i]) {
      printYaml = true;
    } else {
      std::cout << "Unknown option: " << argv[i] << std::endl;
      printHelp();
      return EXIT_FAILURE;
    }
  }

  std::ifstream fis{argv[argc - 1]};
  bml::BitReader reader{fis};
  auto mkv = bml::read<bml::ebml::mkv::Matroska>(reader);
  if (printYaml) {
    bml::yaml::Options opts{};
    opts.hideEmpty = true;
    mkv.printYAML(std::cout, opts) << std::endl;
  } else {
    std::cout << mkv << std::endl;
  }

  return EXIT_SUCCESS;
}
