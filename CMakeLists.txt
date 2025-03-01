cmake_minimum_required(VERSION 3.12)

project(BitManipulationLibrary VERSION 0.0.1)

option(BML_BUILD_TESTING "Build the test executables" OFF)
option(BML_BUILD_EXAMPLES "Build the example executables" OFF)
option(BML_BUILD_EBML "Build the EBML/MKV component" ON)
option(BML_BUILD_COVERAGE "Build with code coverage enabled" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
include(cmake/sanitizers.cmake)

if (MSVC)
  add_compile_options(/Zc:__cplusplus /bigobj)
endif()

add_subdirectory(src)
if (BML_BUILD_EBML)
  add_subdirectory(ebml)
endif()

if(BML_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) OR BML_BUILD_TESTING)
  include(CTest)
  if (BUILD_TESTING)
    add_subdirectory(test)
  endif()
endif()
