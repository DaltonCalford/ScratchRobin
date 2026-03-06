cmake_minimum_required(VERSION 3.20)
project(scratchrobin VERSION 0.1.0 LANGUAGES CXX)

option(SCRATCHROBIN_BUILD_TESTS "Build ScratchRobin tests" ON)
option(SCRATCHROBIN_ENABLE_SBWP "Enable ScratchBird SBWP integration" ON)
set(SCRATCHBIRD_SOURCE_DIR "/home/dcalford/CliWork/ScratchBird" CACHE PATH
    "Path to ScratchBird source tree")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(src)

if(SCRATCHROBIN_BUILD_TESTS)
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()
endif()
