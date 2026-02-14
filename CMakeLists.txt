cmake_minimum_required(VERSION 3.20)
project(ScratchRobin VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(scratchrobin_phase_scaffold STATIC
    src/phases/phase00_governance.cpp
    src/phases/phase01_scope.cpp
    src/phases/phase02_runtime.cpp
    src/phases/phase03_project_data.cpp
    src/phases/phase04_backend.cpp
    src/phases/phase05_ui.cpp
    src/phases/phase06_diagramming.cpp
    src/phases/phase07_reporting.cpp
    src/phases/phase08_advanced.cpp
    src/phases/phase09_packaging.cpp
    src/phases/phase10_conformance_release.cpp
    src/phases/phase_registry.cpp
)

target_include_directories(scratchrobin_phase_scaffold PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(scratchrobin_phase_scaffold PUBLIC cxx_std_20)

add_executable(scratchrobin src/main.cpp)

target_link_libraries(scratchrobin PRIVATE scratchrobin_phase_scaffold)
