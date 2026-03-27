cmake_minimum_required(VERSION 3.20)
project(scratchrobin VERSION 0.1.0 LANGUAGES CXX)

option(SCRATCHROBIN_BUILD_TESTS "Build ScratchRobin tests" ON)
option(SCRATCHROBIN_ENABLE_SBWP "Enable ScratchBird SBWP integration" ON)
option(SCRATCHROBIN_ENABLE_EXCEL "Enable Excel import/export via QXlsx" ON)
set(SCRATCHBIRD_SOURCE_DIR "/home/dcalford/CliWork/ScratchBird" CACHE PATH
    "Path to ScratchBird source tree")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# -----------------------------------------------------------------------------
# QXlsx Excel Library (FetchContent)
# -----------------------------------------------------------------------------
if(SCRATCHROBIN_ENABLE_EXCEL)
    include(FetchContent)
    
    # Find Qt before QXlsx to ensure Qt6::GuiPrivate is available
    find_package(Qt6 COMPONENTS Core Gui REQUIRED)
    find_package(Qt6GuiPrivate REQUIRED)  # Find the private component separately
    
    # Set QT_VERSION_MAJOR for QXlsx
    set(QT_VERSION_MAJOR 6)
    
    FetchContent_Declare(
        QXlsx
        GIT_REPOSITORY https://github.com/QtExcel/QXlsx.git
        GIT_TAG        v1.5.0  # Updated for better Qt6 support
        SOURCE_SUBDIR  QXlsx
    )
    
    # QXlsx will use Qt6 (already found)
    FetchContent_MakeAvailable(QXlsx)
    
    message(STATUS "QXlsx enabled - Excel import/export will be available")
else()
    message(STATUS "QXlsx disabled - Excel features not available")
endif()

add_subdirectory(src)

if(SCRATCHROBIN_BUILD_TESTS)
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()
endif()
