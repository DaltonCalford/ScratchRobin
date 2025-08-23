cmake_minimum_required(VERSION 3.20)
project(ScratchRobin VERSION 0.1.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Temporarily disable -Werror to allow build with unused parameters
    # add_compile_options(-Werror)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Dependencies
find_package(Threads REQUIRED)

# Qt5 Configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Network
    Sql
    Xml
)

if(Qt5_FOUND)
    message(STATUS "Qt5 found: ${Qt5_VERSION}")
else()
    message(FATAL_ERROR "Qt5 not found. Please install Qt5 development packages.")
endif()

# Include directories
include_directories(include)
include_directories(src)

# Subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Installation
install(TARGETS scratchrobin
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ScratchRobinConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ScratchRobinConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ScratchRobinConfig.cmake"
    INSTALL_DESTINATION lib/cmake/ScratchRobin
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ScratchRobinConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ScratchRobinConfigVersion.cmake"
    DESTINATION lib/cmake/ScratchRobin
)

# Testing
enable_testing()

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "ScratchRobin")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Database management interface for ScratchBird")
set(CPACK_PACKAGE_VENDOR "ScratchRobin Team")
set(CPACK_PACKAGE_CONTACT "team@scratchrobin.org")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "DEB")
endif()

include(CPack)
