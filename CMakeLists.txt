cmake_minimum_required(VERSION 3.20)
project(ScratchRobin VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SCRATCHROBIN_USE_SCRATCHBIRD "Build with ScratchBird client support" ON)
option(SCRATCHROBIN_USE_LIBSECRET "Enable libsecret credential store integration" ON)
option(SCRATCHROBIN_BUILD_UI "Build ScratchRobin UI application" ON)

if(SCRATCHROBIN_BUILD_UI)
    find_package(wxWidgets COMPONENTS core base grid)
    if(NOT wxWidgets_FOUND)
        message(WARNING "wxWidgets grid component not found; retrying without grid.")
        find_package(wxWidgets COMPONENTS core base)
    endif()
    if(wxWidgets_FOUND)
        include(${wxWidgets_USE_FILE})
    else()
        message(WARNING "wxWidgets not found; UI build disabled.")
        set(SCRATCHROBIN_BUILD_UI OFF)
    endif()
endif()

if(SCRATCHROBIN_USE_LIBSECRET)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBSECRET libsecret-1)
    endif()
endif()

set(SCRATCHROBIN_CORE_SOURCES
    src/core/connection_manager.cpp
    src/core/mock_backend.cpp
    src/core/network_backend.cpp
    src/core/job_queue.cpp
    src/core/config.cpp
    src/core/credentials.cpp
    src/core/metadata_model.cpp
    src/core/statement_splitter.cpp
    src/core/value_formatter.cpp
    src/core/result_exporter.cpp
)

add_library(scratchrobin_core ${SCRATCHROBIN_CORE_SOURCES})
target_include_directories(scratchrobin_core PUBLIC src)

if(SCRATCHROBIN_BUILD_UI)
    add_executable(scratchrobin
        src/main.cpp
        src/app/app.cpp
        src/ui/main_frame.cpp
        src/ui/sql_editor_frame.cpp
        src/ui/window_manager.cpp
        src/ui/result_grid_table.cpp
    )

    target_include_directories(scratchrobin PRIVATE src)
    target_link_libraries(scratchrobin PRIVATE scratchrobin_core ${wxWidgets_LIBRARIES})
endif()

if(SCRATCHROBIN_USE_LIBSECRET AND LIBSECRET_FOUND)
    target_compile_definitions(scratchrobin_core PRIVATE SCRATCHROBIN_USE_LIBSECRET=1)
    target_include_directories(scratchrobin_core PRIVATE ${LIBSECRET_INCLUDE_DIRS})
    target_link_libraries(scratchrobin_core PRIVATE ${LIBSECRET_LIBRARIES})
endif()

if(SCRATCHROBIN_USE_SCRATCHBIRD)
    set(SCRATCHBIRD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../ScratchBird" CACHE PATH "ScratchBird root path")
    if(EXISTS "${SCRATCHBIRD_ROOT}/CMakeLists.txt")
        add_subdirectory("${SCRATCHBIRD_ROOT}" "${CMAKE_BINARY_DIR}/scratchbird")
        target_include_directories(scratchrobin_core PRIVATE "${SCRATCHBIRD_ROOT}/include")
        target_link_libraries(scratchrobin_core PRIVATE scratchbird_client)
        target_compile_definitions(scratchrobin_core PRIVATE SCRATCHROBIN_USE_SCRATCHBIRD=1)
    else()
        message(WARNING "ScratchBird root not found at ${SCRATCHBIRD_ROOT}; building without ScratchBird support.")
    endif()
endif()

enable_testing()

add_executable(scratchrobin_tests
    tests/test_main.cpp
)

target_link_libraries(scratchrobin_tests PRIVATE scratchrobin_core)
target_include_directories(scratchrobin_tests PRIVATE src)
target_compile_definitions(scratchrobin_tests PRIVATE
    SCRATCHROBIN_TEST_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\")

add_test(NAME scratchrobin_tests COMMAND scratchrobin_tests)
