# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Test executables
add_executable(unit_tests
    unit/test_string_utils.cpp
    unit/test_logger.cpp
    unit/test_connection_manager.cpp
    unit/test_metadata_manager.cpp
    unit/test_application.cpp
)

add_executable(integration_tests
    integration/test_database_connection.cpp
    integration/test_schema_operations.cpp
    integration/test_query_execution.cpp
    integration/test_metadata_caching.cpp
)

add_executable(e2e_tests
    e2e/test_full_workflow.cpp
    e2e/test_ui_interactions.cpp
    e2e/test_cli_commands.cpp
)

# Link libraries
target_link_libraries(unit_tests
    PRIVATE
        scratchrobin_lib
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
)

target_link_libraries(integration_tests
    PRIVATE
        scratchrobin_lib
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
)

target_link_libraries(e2e_tests
    PRIVATE
        scratchrobin_lib
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
)

# Include directories for tests
target_include_directories(unit_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_include_directories(integration_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_include_directories(e2e_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Add tests to CTest
add_test(NAME UnitTests COMMAND unit_tests)
add_test(NAME IntegrationTests COMMAND integration_tests)
add_test(NAME E2ETests COMMAND e2e_tests)

# Set test properties
set_tests_properties(UnitTests PROPERTIES
    TIMEOUT 60
    LABELS "unit"
)

set_tests_properties(IntegrationTests PROPERTIES
    TIMEOUT 300
    LABELS "integration"
)

set_tests_properties(E2ETests PROPERTIES
    TIMEOUT 600
    LABELS "e2e"
)

# Create test data directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)

# Create test configuration file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/test_data/config.json
"{
  \"database\": {
    \"host\": \"localhost\",
    \"port\": 5432,
    \"name\": \"test_db\",
    \"user\": \"test_user\",
    \"password\": \"test_password\"
  },
  \"logging\": {
    \"level\": \"debug\",
    \"file\": \"test.log\"
  },
  \"ui\": {
    \"theme\": \"light\",
    \"font_size\": 12
  }
}"
)

# Test coverage (optional)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O0 -g --coverage)
        add_link_options(--coverage)
    endif()
endif()