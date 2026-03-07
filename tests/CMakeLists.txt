# ==============================================================================
# ScratchRobin Test Suite
# ==============================================================================

find_package(Qt6 REQUIRED COMPONENTS Test Widgets)

# -----------------------------------------------------------------------------
# Backend Contract Tests
# -----------------------------------------------------------------------------
add_executable(backend_contract_tests
  backend_contract_tests.cpp
)

target_include_directories(backend_contract_tests
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(backend_contract_tests
  PRIVATE
    scratchrobin_backend
)

add_test(NAME backend_contract_tests COMMAND backend_contract_tests)

# -----------------------------------------------------------------------------
# DDL Generation Tests
# -----------------------------------------------------------------------------
add_executable(test_ddl_generation
  test_ddl_generation.cpp
)

target_include_directories(test_ddl_generation
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(test_ddl_generation
  PRIVATE
    scratchrobin_backend
    Qt6::Test
)

set_target_properties(test_ddl_generation PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    AUTOMOC ON
)

add_test(NAME test_ddl_generation COMMAND test_ddl_generation)

# -----------------------------------------------------------------------------
# UI Component Tests
# -----------------------------------------------------------------------------
add_executable(test_ui_components
  test_ui_components.cpp
)

target_include_directories(test_ui_components
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(test_ui_components
  PRIVATE
    scratchrobin_backend
    scratchrobin_ui
    Qt6::Test
    Qt6::Widgets
)

set_target_properties(test_ui_components PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    AUTOMOC ON
)

add_test(NAME test_ui_components COMMAND test_ui_components)

# ==============================================================================
# Test Summary
# ==============================================================================
message(STATUS "Test executables: backend_contract_tests, test_ddl_generation, test_ui_components")
