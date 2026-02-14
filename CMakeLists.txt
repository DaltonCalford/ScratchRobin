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

add_library(scratchrobin_project STATIC
    src/project/project_services.cpp
)

target_include_directories(scratchrobin_project PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(scratchrobin_project PUBLIC scratchrobin_core_contracts scratchrobin_runtime)
target_compile_features(scratchrobin_project PUBLIC cxx_std_20)

add_library(scratchrobin_connection STATIC
    src/connection/connection_services.cpp
)

target_include_directories(scratchrobin_connection PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(scratchrobin_connection PUBLIC scratchrobin_core_contracts scratchrobin_runtime)
target_compile_features(scratchrobin_connection PUBLIC cxx_std_20)

add_library(scratchrobin_ui STATIC
    src/ui/ui_workflow_services.cpp
)

target_include_directories(scratchrobin_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(scratchrobin_ui PUBLIC
    scratchrobin_core_contracts
    scratchrobin_runtime
    scratchrobin_project
    scratchrobin_connection
)
target_compile_features(scratchrobin_ui PUBLIC cxx_std_20)

add_library(scratchrobin_diagram STATIC
    src/diagram/diagram_services.cpp
)

target_include_directories(scratchrobin_diagram PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(scratchrobin_diagram PUBLIC scratchrobin_core_contracts)
target_compile_features(scratchrobin_diagram PUBLIC cxx_std_20)

add_library(scratchrobin_reporting STATIC
    src/reporting/reporting_services.cpp
)

target_include_directories(scratchrobin_reporting PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(scratchrobin_reporting PUBLIC
    scratchrobin_core_contracts
    scratchrobin_runtime
    scratchrobin_project
    scratchrobin_connection
)
target_compile_features(scratchrobin_reporting PUBLIC cxx_std_20)

add_library(scratchrobin_advanced STATIC
    src/advanced/advanced_services.cpp
)

target_include_directories(scratchrobin_advanced PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(scratchrobin_advanced PUBLIC scratchrobin_core_contracts)
target_compile_features(scratchrobin_advanced PUBLIC cxx_std_20)

add_library(scratchrobin_packaging STATIC
    src/packaging/packaging_services.cpp
)

target_include_directories(scratchrobin_packaging PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(scratchrobin_packaging PUBLIC scratchrobin_core_contracts)
target_compile_features(scratchrobin_packaging PUBLIC cxx_std_20)

add_library(scratchrobin_release STATIC
    src/release/release_conformance_services.cpp
)

target_include_directories(scratchrobin_release PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(scratchrobin_release PUBLIC scratchrobin_core_contracts)
target_compile_features(scratchrobin_release PUBLIC cxx_std_20)

add_executable(scratchrobin src/main.cpp)

target_link_libraries(scratchrobin PRIVATE
    scratchrobin_phase_scaffold
    scratchrobin_core_contracts
    scratchrobin_runtime
    scratchrobin_project
    scratchrobin_connection
    scratchrobin_ui
    scratchrobin_diagram
    scratchrobin_reporting
    scratchrobin_advanced
    scratchrobin_packaging
    scratchrobin_release
)

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
    target_link_libraries(scratchrobin_tests_integration PRIVATE
        scratchrobin_core_contracts
        scratchrobin_runtime
        scratchrobin_project
        scratchrobin_connection
        scratchrobin_ui
        scratchrobin_diagram
    )
    add_test(NAME scratchrobin_tests_integration COMMAND scratchrobin_tests_integration)

    add_executable(scratchrobin_tests_runtime_integration
        tests/integration/runtime_lifecycle_integration.cpp
    )
    target_include_directories(scratchrobin_tests_runtime_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_runtime_integration PRIVATE
        scratchrobin_core_contracts
        scratchrobin_runtime
        scratchrobin_project
        scratchrobin_connection
        scratchrobin_ui
        scratchrobin_diagram
    )
    add_test(NAME scratchrobin_tests_runtime_integration COMMAND scratchrobin_tests_runtime_integration)

    add_executable(scratchrobin_tests_project_integration
        tests/integration/project_specset_integration.cpp
    )
    target_include_directories(scratchrobin_tests_project_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_project_integration PRIVATE
        scratchrobin_core_contracts
        scratchrobin_runtime
        scratchrobin_project
        scratchrobin_connection
        scratchrobin_ui
        scratchrobin_diagram
    )
    add_test(NAME scratchrobin_tests_project_integration COMMAND scratchrobin_tests_project_integration)

    add_executable(scratchrobin_tests_connection_integration
        tests/integration/connection_adapter_integration.cpp
    )
    target_include_directories(scratchrobin_tests_connection_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_connection_integration PRIVATE
        scratchrobin_core_contracts
        scratchrobin_runtime
        scratchrobin_project
        scratchrobin_connection
        scratchrobin_ui
        scratchrobin_diagram
    )
    add_test(NAME scratchrobin_tests_connection_integration COMMAND scratchrobin_tests_connection_integration)

    add_executable(scratchrobin_tests_ui_integration
        tests/integration/ui_workflow_integration.cpp
    )
    target_include_directories(scratchrobin_tests_ui_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_ui_integration PRIVATE
        scratchrobin_core_contracts
        scratchrobin_runtime
        scratchrobin_project
        scratchrobin_connection
        scratchrobin_ui
        scratchrobin_diagram
    )
    add_test(NAME scratchrobin_tests_ui_integration COMMAND scratchrobin_tests_ui_integration)

    add_executable(scratchrobin_tests_diagram_integration
        tests/integration/diagram_workflow_integration.cpp
    )
    target_include_directories(scratchrobin_tests_diagram_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_diagram_integration PRIVATE
        scratchrobin_core_contracts
        scratchrobin_runtime
        scratchrobin_project
        scratchrobin_connection
        scratchrobin_ui
        scratchrobin_diagram
    )
    add_test(NAME scratchrobin_tests_diagram_integration COMMAND scratchrobin_tests_diagram_integration)

    add_executable(scratchrobin_tests_reporting_integration
        tests/integration/reporting_workflow_integration.cpp
    )
    target_include_directories(scratchrobin_tests_reporting_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_reporting_integration PRIVATE
        scratchrobin_core_contracts
        scratchrobin_runtime
        scratchrobin_project
        scratchrobin_connection
        scratchrobin_ui
        scratchrobin_diagram
        scratchrobin_reporting
    )
    add_test(NAME scratchrobin_tests_reporting_integration COMMAND scratchrobin_tests_reporting_integration)

    add_executable(scratchrobin_tests_advanced_integration
        tests/integration/advanced_workflow_integration.cpp
    )
    target_include_directories(scratchrobin_tests_advanced_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_advanced_integration PRIVATE
        scratchrobin_core_contracts
        scratchrobin_advanced
    )
    add_test(NAME scratchrobin_tests_advanced_integration COMMAND scratchrobin_tests_advanced_integration)

    add_executable(scratchrobin_tests_packaging_integration
        tests/integration/packaging_workflow_integration.cpp
    )
    target_include_directories(scratchrobin_tests_packaging_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_packaging_integration PRIVATE
        scratchrobin_core_contracts
        scratchrobin_packaging
    )
    add_test(NAME scratchrobin_tests_packaging_integration COMMAND scratchrobin_tests_packaging_integration)

    add_executable(scratchrobin_tests_release_integration
        tests/integration/release_conformance_integration.cpp
    )
    target_include_directories(scratchrobin_tests_release_integration PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(scratchrobin_tests_release_integration PRIVATE
        scratchrobin_core_contracts
        scratchrobin_release
    )
    add_test(NAME scratchrobin_tests_release_integration COMMAND scratchrobin_tests_release_integration)

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
