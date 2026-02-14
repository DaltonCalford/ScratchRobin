cmake_minimum_required(VERSION 3.20)
project(ScratchRobin VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SCRATCHROBIN_BUILD_TESTS "Build beta1b conformance-first tests" ON)

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

add_library(scratchrobin_core_contracts STATIC
    src/core/reject.cpp
    src/core/sha256.cpp
    src/core/simple_json.cpp
    src/core/beta1b_contracts.cpp
)

target_include_directories(scratchrobin_core_contracts PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(scratchrobin_core_contracts PUBLIC cxx_std_20)

add_library(scratchrobin_runtime STATIC
    src/runtime/runtime_config.cpp
    src/runtime/runtime_services.cpp
)

target_include_directories(scratchrobin_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(scratchrobin_runtime PUBLIC scratchrobin_core_contracts)
target_compile_features(scratchrobin_runtime PUBLIC cxx_std_20)

add_executable(scratchrobin src/main.cpp)

target_link_libraries(scratchrobin PRIVATE scratchrobin_phase_scaffold scratchrobin_core_contracts scratchrobin_runtime)

if(SCRATCHROBIN_BUILD_TESTS)
    enable_testing()

    add_executable(scratchrobin_tests_unit
        tests/unit/beta1b_contracts_unit.cpp
    )
    target_include_directories(scratchrobin_tests_unit PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_unit PRIVATE scratchrobin_core_contracts)
    add_test(NAME scratchrobin_tests_unit COMMAND scratchrobin_tests_unit)

    add_executable(scratchrobin_tests_smoke
        tests/smoke/beta1b_smoke.cpp
    )
    target_include_directories(scratchrobin_tests_smoke PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_smoke PRIVATE scratchrobin_phase_scaffold scratchrobin_core_contracts)
    add_test(NAME scratchrobin_tests_smoke COMMAND scratchrobin_tests_smoke)

    add_executable(scratchrobin_tests_integration
        tests/integration/beta1b_integration.cpp
    )
    target_include_directories(scratchrobin_tests_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_integration PRIVATE scratchrobin_core_contracts scratchrobin_runtime)
    add_test(NAME scratchrobin_tests_integration COMMAND scratchrobin_tests_integration)

    add_executable(scratchrobin_tests_runtime_integration
        tests/integration/runtime_lifecycle_integration.cpp
    )
    target_include_directories(scratchrobin_tests_runtime_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_runtime_integration PRIVATE scratchrobin_core_contracts scratchrobin_runtime)
    add_test(NAME scratchrobin_tests_runtime_integration COMMAND scratchrobin_tests_runtime_integration)

    add_executable(scratchrobin_tests_conformance
        tests/conformance/beta1b_conformance_vector.cpp
    )
    target_include_directories(scratchrobin_tests_conformance PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_conformance PRIVATE scratchrobin_core_contracts)
    add_test(NAME scratchrobin_tests_conformance COMMAND scratchrobin_tests_conformance)
endif()
