cmake_minimum_required(VERSION 3.20)
project(ScratchRobin VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SCRATCHROBIN_USE_SCRATCHBIRD "Build with ScratchBird client support" ON)
option(SCRATCHROBIN_USE_LIBSECRET "Enable libsecret credential store integration" ON)
option(SCRATCHROBIN_BUILD_UI "Build ScratchRobin UI application" ON)
option(SCRATCHROBIN_USE_LIBPQ "Enable PostgreSQL (libpq) backend support" ON)
option(SCRATCHROBIN_USE_MYSQL "Enable MySQL/MariaDB backend support" ON)
option(SCRATCHROBIN_USE_FIREBIRD "Enable Firebird backend support" ON)

# Sanitizer and analysis options
option(SCRATCHROBIN_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(SCRATCHROBIN_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(SCRATCHROBIN_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(SCRATCHROBIN_ENABLE_MSAN "Enable MemorySanitizer" OFF)
option(SCRATCHROBIN_ENABLE_COVERAGE "Enable code coverage (gcov)" OFF)
option(SCRATCHROBIN_ENABLE_CLANG_TIDY "Enable clang-tidy checks" OFF)
option(SCRATCHROBIN_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(SCRATCHROBIN_ENABLE_IWYU "Enable include-what-you-use" OFF)
option(SCRATCHROBIN_ENABLE_CPPCHECK "Enable cppcheck" OFF)

if(SCRATCHROBIN_BUILD_UI)
    set(SCRATCHROBIN_WX_COMPONENTS core base html
        CACHE STRING "wxWidgets components to use")
    set(SCRATCHROBIN_WX_LIBRARIES "")

    find_package(wxWidgets CONFIG QUIET COMPONENTS ${SCRATCHROBIN_WX_COMPONENTS})
    if(NOT wxWidgets_FOUND)
        find_package(wxWidgets QUIET COMPONENTS ${SCRATCHROBIN_WX_COMPONENTS})
    endif()

    if(NOT wxWidgets_FOUND)
        set(SCRATCHROBIN_WX_COMPONENTS_FALLBACK ${SCRATCHROBIN_WX_COMPONENTS})
        list(REMOVE_ITEM SCRATCHROBIN_WX_COMPONENTS_FALLBACK grid)
        if(NOT SCRATCHROBIN_WX_COMPONENTS_FALLBACK STREQUAL SCRATCHROBIN_WX_COMPONENTS)
            find_package(wxWidgets CONFIG QUIET
                COMPONENTS ${SCRATCHROBIN_WX_COMPONENTS_FALLBACK})
            if(NOT wxWidgets_FOUND)
                find_package(wxWidgets QUIET
                    COMPONENTS ${SCRATCHROBIN_WX_COMPONENTS_FALLBACK})
            endif()
        endif()
    endif()

    if(wxWidgets_FOUND)
        if(DEFINED wxWidgets_USE_FILE)
            include(${wxWidgets_USE_FILE})
        endif()
        if(TARGET wxWidgets::wxWidgets)
            set(SCRATCHROBIN_WX_LIBRARIES wxWidgets::wxWidgets)
        elseif(TARGET wx::wxcore)
            set(SCRATCHROBIN_WX_LIBRARIES wx::wxcore)
            if(TARGET wx::wxbase)
                list(APPEND SCRATCHROBIN_WX_LIBRARIES wx::wxbase)
            endif()
        elseif(TARGET wxWidgets::core)
            set(SCRATCHROBIN_WX_LIBRARIES wxWidgets::core wxWidgets::base)
        else()
            set(SCRATCHROBIN_WX_LIBRARIES ${wxWidgets_LIBRARIES})
        endif()
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
    src/core/embedded_backend.cpp
    src/core/mock_backend.cpp
    src/core/network_backend.cpp
    src/core/ipc_backend.cpp
    src/core/job_queue.cpp
    src/core/config.cpp
    src/core/credentials.cpp
    src/core/metadata_model.cpp
    src/core/statement_splitter.cpp
    src/core/value_formatter.cpp
    src/core/result_exporter.cpp
    src/core/simple_json.cpp
    src/core/error_handler.cpp
    src/core/capability_detector.cpp
    src/core/postgres_backend.cpp
    src/core/mysql_backend.cpp
    src/core/firebird_backend.cpp
    src/core/session_state.cpp
    src/core/project.cpp
    src/core/testing_framework.cpp
    src/core/docker_config.cpp
    src/core/migration_generator.cpp
    src/core/deployment_plan.cpp
    src/core/collaboration.cpp
    # Phase 5: Advanced Features
    src/core/data_lineage.cpp
    src/core/data_masking.cpp
    src/core/api_generator.cpp
    src/core/cdc_streaming.cpp
    src/core/ai_assistant.cpp
    src/core/ai_providers.cpp
    # Issue Tracker Integration
    src/core/issue_tracker.cpp
    src/core/issue_tracker_jira.cpp
    src/core/issue_tracker_github.cpp
    src/core/issue_tracker_gitlab.cpp
    src/core/sync_scheduler.cpp
)

add_library(scratchrobin_core ${SCRATCHROBIN_CORE_SOURCES})
target_include_directories(scratchrobin_core PUBLIC src)

# =============================================================================
# Compiler Warnings Configuration
# =============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(SCRATCHROBIN_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
        -Wmissing-declarations
        -Wmissing-field-initializers
        -Wunreachable-code
        -Wdocumentation
    )
    
    # Add warnings as errors if requested
    if(SCRATCHROBIN_WARNINGS_AS_ERRORS)
        list(APPEND SCRATCHROBIN_WARNING_FLAGS -Werror)
    endif()
endif()

# NOTE: Compiler warnings, sanitizers, and static analysis are configured
# at the END of this file after all targets are defined, to avoid errors
# about targets not existing yet.

if(SCRATCHROBIN_BUILD_UI)
    add_executable(scratchrobin
        src/main.cpp
        src/app/app.cpp
        src/ui/main_frame.cpp
        src/ui/project_panel.cpp
        src/ui/connection_editor_dialog.cpp
        src/ui/diagram_frame.cpp
        src/ui/diagram_model.cpp
        src/ui/diagram_canvas.cpp
        src/ui/diagram_page.cpp
        src/ui/diagram_template_dialog.cpp
        src/diagram/command.cpp
        src/diagram/layout_engine.cpp
        src/diagram/reverse_engineer.cpp
        src/diagram/forward_engineer.cpp
        src/diagram/export_manager.cpp
        src/diagram/graphviz_layout.cpp
        src/diagram/migration_generator.cpp
        src/ui/domain_editor_dialog.cpp
        src/ui/domain_manager_frame.cpp
        src/ui/error_dialog.cpp
        src/ui/sequence_manager_frame.cpp
        src/ui/sequence_editor_dialog.cpp
        src/ui/view_manager_frame.cpp
        src/ui/view_editor_dialog.cpp
        src/ui/trigger_manager_frame.cpp
        src/ui/trigger_editor_dialog.cpp
        src/ui/procedure_manager_frame.cpp
        src/ui/routine_editor_dialog.cpp
        src/ui/package_manager_frame.cpp
        src/ui/package_editor_dialog.cpp
        src/ui/preferences_dialog.cpp
        src/ui/help_browser.cpp
        src/ui/shortcuts_dialog.cpp
        src/ui/shortcuts_cheat_sheet.cpp
        src/ui/backup_dialog.cpp
        src/ui/restore_dialog.cpp
        src/ui/backup_history_dialog.cpp
        src/ui/backup_schedule_dialog.cpp
        src/ui/storage_manager_frame.cpp
        src/ui/tablespace_editor_dialog.cpp
        src/ui/sessions_panel.cpp
        src/ui/locks_panel.cpp
        src/ui/statements_panel.cpp
        src/ui/table_statistics_panel.cpp
        src/ui/io_statistics_panel.cpp
        src/ui/database_manager_frame.cpp
        src/ui/database_editor_dialog.cpp
        src/ui/index_designer_frame.cpp
        src/ui/index_editor_dialog.cpp
        src/ui/job_editor_dialog.cpp
        src/ui/job_scheduler_frame.cpp
        src/ui/schema_editor_dialog.cpp
        src/ui/schema_manager_frame.cpp
        src/ui/table_designer_frame.cpp
        src/ui/table_editor_dialog.cpp
        src/ui/monitoring_frame.cpp
        src/ui/sql_editor_frame.cpp
        src/ui/users_roles_frame.cpp
        src/ui/user_editor_dialog.cpp
        src/ui/role_editor_dialog.cpp
        src/ui/privilege_editor_dialog.cpp
        src/ui/ddl_preview_dialog.cpp
        src/ui/export_options_dialog.cpp
        src/ui/reverse_engineer_wizard.cpp
        src/ui/ddl_execution_dialog.cpp
        src/ui/layout_options_dialog.cpp
        src/ui/diagram_minimap.cpp
        src/ui/diagram_grouping.cpp
        src/ui/incremental_refresh_dialog.cpp
        src/ui/notation_test_dialog.cpp
        src/ui/print_dialog.cpp
        src/ui/migration_dialog.cpp
        src/ui/startup_frame.cpp
        src/ui/menu_builder.cpp
        src/ui/icon_bar.cpp
        src/ui/window_manager.cpp
        src/ui/result_grid_table.cpp
        # Phase 7: Beta Placeholder UIs
        src/ui/cluster_manager_frame.cpp
        src/ui/replication_manager_frame.cpp
        src/ui/etl_manager_frame.cpp
        src/ui/git_integration_frame.cpp
        # New UI components
        src/ui/docker_manager_panel.cpp
        src/ui/test_runner_panel.cpp
        src/ui/whiteboard_canvas.cpp
        # Phase 5: Advanced Features UI
        src/ui/ai_assistant_panel.cpp
        src/ui/ai_settings_dialog.cpp
        # Issue Tracker UI
        src/ui/issue_tracker_panel.cpp
    )

    target_include_directories(scratchrobin PRIVATE src)
    target_link_libraries(scratchrobin PRIVATE scratchrobin_core ${SCRATCHROBIN_WX_LIBRARIES})
endif()

if(SCRATCHROBIN_USE_LIBSECRET AND LIBSECRET_FOUND)
    target_compile_definitions(scratchrobin_core PRIVATE SCRATCHROBIN_USE_LIBSECRET=1)
    target_include_directories(scratchrobin_core PRIVATE ${LIBSECRET_INCLUDE_DIRS})
    target_link_libraries(scratchrobin_core PRIVATE ${LIBSECRET_LIBRARIES})
endif()

if(SCRATCHROBIN_USE_LIBPQ)
    find_package(PostgreSQL QUIET)
    if(PostgreSQL_FOUND)
        target_compile_definitions(scratchrobin_core PRIVATE SCRATCHROBIN_USE_LIBPQ=1)
        target_include_directories(scratchrobin_core PRIVATE ${PostgreSQL_INCLUDE_DIRS})
        target_link_libraries(scratchrobin_core PRIVATE ${PostgreSQL_LIBRARIES})
    else()
        find_package(PkgConfig)
        if(PkgConfig_FOUND)
            pkg_check_modules(LIBPQ QUIET libpq)
        endif()
        if(LIBPQ_FOUND)
            target_compile_definitions(scratchrobin_core PRIVATE SCRATCHROBIN_USE_LIBPQ=1)
            target_include_directories(scratchrobin_core PRIVATE ${LIBPQ_INCLUDE_DIRS})
            target_link_libraries(scratchrobin_core PRIVATE ${LIBPQ_LIBRARIES})
        else()
            message(WARNING "libpq not found; PostgreSQL backend disabled.")
        endif()
    endif()
endif()

if(SCRATCHROBIN_USE_MYSQL)
    find_package(MySQL QUIET)
    if(MySQL_FOUND)
        target_compile_definitions(scratchrobin_core PRIVATE SCRATCHROBIN_USE_MYSQL=1)
        target_include_directories(scratchrobin_core PRIVATE ${MySQL_INCLUDE_DIRS})
        target_link_libraries(scratchrobin_core PRIVATE ${MySQL_LIBRARIES})
    else()
        find_package(PkgConfig)
        if(PkgConfig_FOUND)
            pkg_check_modules(MYSQLCLIENT QUIET mysqlclient)
            if(NOT MYSQLCLIENT_FOUND)
                pkg_check_modules(MYSQLCLIENT QUIET mariadb)
            endif()
        endif()
        if(MYSQLCLIENT_FOUND)
            target_compile_definitions(scratchrobin_core PRIVATE SCRATCHROBIN_USE_MYSQL=1)
            target_include_directories(scratchrobin_core PRIVATE ${MYSQLCLIENT_INCLUDE_DIRS})
            target_link_libraries(scratchrobin_core PRIVATE ${MYSQLCLIENT_LIBRARIES})
        else()
            message(WARNING "MySQL client library not found; MySQL backend disabled.")
        endif()
    endif()
endif()

if(SCRATCHROBIN_USE_FIREBIRD)
    find_package(Firebird QUIET)
    if(Firebird_FOUND)
        target_compile_definitions(scratchrobin_core PRIVATE SCRATCHROBIN_USE_FIREBIRD=1)
        target_include_directories(scratchrobin_core PRIVATE ${Firebird_INCLUDE_DIRS})
        target_link_libraries(scratchrobin_core PRIVATE ${Firebird_LIBRARIES})
    else()
        find_package(PkgConfig)
        if(PkgConfig_FOUND)
            pkg_check_modules(FIREBIRD QUIET fbclient)
            if(NOT FIREBIRD_FOUND)
                pkg_check_modules(FIREBIRD QUIET firebird)
            endif()
        endif()
        if(NOT FIREBIRD_FOUND)
            set(FIREBIRD_ROOT "/opt/firebird" CACHE PATH "Firebird root path")
            if(EXISTS "${FIREBIRD_ROOT}/include/ibase.h")
                find_library(FIREBIRD_CLIENT_LIBRARY NAMES fbclient
                    PATHS "${FIREBIRD_ROOT}/lib" "${FIREBIRD_ROOT}/lib64")
                if(FIREBIRD_CLIENT_LIBRARY)
                    set(FIREBIRD_FOUND TRUE)
                    set(FIREBIRD_INCLUDE_DIRS "${FIREBIRD_ROOT}/include")
                    set(FIREBIRD_LIBRARIES "${FIREBIRD_CLIENT_LIBRARY}")
                endif()
            endif()
        endif()
        if(FIREBIRD_FOUND)
            target_compile_definitions(scratchrobin_core PRIVATE SCRATCHROBIN_USE_FIREBIRD=1)
            target_include_directories(scratchrobin_core PRIVATE ${FIREBIRD_INCLUDE_DIRS})
            target_link_libraries(scratchrobin_core PRIVATE ${FIREBIRD_LIBRARIES})
        else()
            message(WARNING "Firebird client library not found; Firebird backend disabled.")
        endif()
    endif()
endif()

if(SCRATCHROBIN_USE_SCRATCHBIRD)
    set(SCRATCHBIRD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../ScratchBird" CACHE PATH "ScratchBird root path")
    if(EXISTS "${SCRATCHBIRD_ROOT}/CMakeLists.txt")
        add_subdirectory("${SCRATCHBIRD_ROOT}" "${CMAKE_BINARY_DIR}/scratchbird")
        target_include_directories(scratchrobin_core PRIVATE "${SCRATCHBIRD_ROOT}/include")
        target_link_libraries(scratchrobin_core PRIVATE scratchbird_protocol)
        target_compile_definitions(scratchrobin_core PRIVATE SCRATCHROBIN_USE_SCRATCHBIRD=1)
    else()
        message(WARNING "ScratchBird root not found at ${SCRATCHBIRD_ROOT}; building without ScratchBird support.")
    endif()
endif()

# =============================================================================
# Testing Configuration
# =============================================================================
option(SCRATCHROBIN_BUILD_TESTS "Build test suite" ON)

if(SCRATCHROBIN_BUILD_TESTS)
enable_testing()

# Find Google Test
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    # Try to fetch it if not found
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
    )
    FetchContent_MakeAvailable(googletest)
endif()

# Test executable
add_executable(scratchrobin_tests
    tests/test_main.cpp
    # Unit tests
    tests/unit/test_metadata_model.cpp
    tests/unit/test_statement_splitter.cpp
    tests/unit/test_value_formatter.cpp
    tests/unit/test_result_exporter.cpp
    tests/unit/test_config.cpp
    tests/unit/test_credentials.cpp
    tests/unit/test_simple_json.cpp
    tests/unit/test_error_handler.cpp
    tests/unit/test_capability_detector.cpp
    # Core tests
    tests/unit/test_job_queue.cpp
    tests/unit/test_session_state.cpp
    # Diagram tests
    tests/unit/test_diagram_model.cpp
    tests/unit/test_layout_engine.cpp
    tests/unit/test_forward_engineer.cpp
    # Performance tests
    tests/unit/test_diagram_performance.cpp
    tests/unit/test_memory_usage.cpp
    # Integration tests (environment-gated)
    tests/unit/test_embedded_backend.cpp
    tests/unit/test_ipc_backend.cpp
    tests/integration/test_postgres_backend.cpp
    tests/integration/test_mysql_backend.cpp
    tests/integration/test_firebird_backend.cpp
    tests/integration/test_scratchbird_backend.cpp
)

target_link_libraries(scratchrobin_tests PRIVATE 
    scratchrobin_core
    GTest::gtest
    GTest::gmock
)

target_include_directories(scratchrobin_tests PRIVATE src)
target_compile_definitions(scratchrobin_tests PRIVATE
    SCRATCHROBIN_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(scratchrobin_tests)

# Coverage target
if(SCRATCHROBIN_ENABLE_COVERAGE)
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
        COMMAND gcovr -r ${CMAKE_SOURCE_DIR} --html --html-details 
                -o ${CMAKE_BINARY_DIR}/coverage/index.html
        DEPENDS scratchrobin_tests
        COMMENT "Generating coverage report"
    )
endif()

endif() # SCRATCHROBIN_BUILD_TESTS

# =============================================================================
# Compiler Warnings Configuration (applied after targets are defined)
# =============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(SCRATCHROBIN_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
        -Wmissing-declarations
        -Wmissing-field-initializers
        -Wunreachable-code
    )
    
    if(SCRATCHROBIN_ENABLE_WARNINGS)
        # Add warnings as errors if requested
        if(SCRATCHROBIN_WARNINGS_AS_ERRORS)
            list(APPEND SCRATCHROBIN_WARNING_FLAGS -Werror)
        endif()
        
        target_compile_options(scratchrobin_core PRIVATE ${SCRATCHROBIN_WARNING_FLAGS})
        
        if(SCRATCHROBIN_BUILD_UI)
            target_compile_options(scratchrobin PRIVATE ${SCRATCHROBIN_WARNING_FLAGS})
        endif()
    endif()
endif()

# =============================================================================
# Sanitizer Configuration (applied after targets are defined)
# =============================================================================
if(SCRATCHROBIN_ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    target_compile_options(scratchrobin_core PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(scratchrobin_core PRIVATE -fsanitize=address)
    if(SCRATCHROBIN_BUILD_UI)
        target_compile_options(scratchrobin PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(scratchrobin PRIVATE -fsanitize=address)
    endif()
endif()

if(SCRATCHROBIN_ENABLE_UBSAN)
    message(STATUS "UndefinedBehaviorSanitizer enabled")
    target_compile_options(scratchrobin_core PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
    target_link_options(scratchrobin_core PRIVATE -fsanitize=undefined)
    if(SCRATCHROBIN_BUILD_UI)
        target_compile_options(scratchrobin PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
        target_link_options(scratchrobin PRIVATE -fsanitize=undefined)
    endif()
endif()

if(SCRATCHROBIN_ENABLE_TSAN)
    message(STATUS "ThreadSanitizer enabled")
    target_compile_options(scratchrobin_core PRIVATE -fsanitize=thread)
    target_link_options(scratchrobin_core PRIVATE -fsanitize=thread)
    if(SCRATCHROBIN_BUILD_UI)
        target_compile_options(scratchrobin PRIVATE -fsanitize=thread)
        target_link_options(scratchrobin PRIVATE -fsanitize=thread)
    endif()
endif()

if(SCRATCHROBIN_ENABLE_MSAN)
    message(STATUS "MemorySanitizer enabled")
    target_compile_options(scratchrobin_core PRIVATE -fsanitize=memory -fno-omit-frame-pointer)
    target_link_options(scratchrobin_core PRIVATE -fsanitize=memory)
    if(SCRATCHROBIN_BUILD_UI)
        target_compile_options(scratchrobin PRIVATE -fsanitize=memory -fno-omit-frame-pointer)
        target_link_options(scratchrobin PRIVATE -fsanitize=memory)
    endif()
endif()

# =============================================================================
# Code Coverage Configuration (applied after targets are defined)
# =============================================================================
if(SCRATCHROBIN_ENABLE_COVERAGE)
    message(STATUS "Code coverage enabled")
    target_compile_options(scratchrobin_core PRIVATE --coverage -O0)
    target_link_options(scratchrobin_core PRIVATE --coverage)
    if(SCRATCHROBIN_BUILD_UI)
        target_compile_options(scratchrobin PRIVATE --coverage -O0)
        target_link_options(scratchrobin PRIVATE --coverage)
    endif()
endif()

# =============================================================================
# Static Analysis Tools (applied after targets are defined)
# =============================================================================
if(SCRATCHROBIN_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
        set_target_properties(scratchrobin_core PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE}"
        )
        if(SCRATCHROBIN_BUILD_UI)
            set_target_properties(scratchrobin PROPERTIES
                CXX_CLANG_TIDY "${CLANG_TIDY_EXE}"
            )
        endif()
    else()
        message(WARNING "clang-tidy requested but not found")
    endif()
endif()

if(SCRATCHROBIN_ENABLE_IWYU)
    find_program(IWYU_EXE NAMES include-what-you-use iwyu)
    if(IWYU_EXE)
        message(STATUS "include-what-you-use enabled: ${IWYU_EXE}")
        set_target_properties(scratchrobin_core PROPERTIES
            CXX_INCLUDE_WHAT_YOU_USE "${IWYU_EXE}"
        )
    else()
        message(WARNING "include-what-you-use requested but not found")
    endif()
endif()

if(SCRATCHROBIN_ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES cppcheck)
    if(CPPCHECK_EXE)
        message(STATUS "cppcheck enabled: ${CPPCHECK_EXE}")
        set_target_properties(scratchrobin_core PROPERTIES
            CXX_CPPCHECK "${CPPCHECK_EXE};--enable=warning,performance,portability,style;--std=c++17"
        )
    else()
        message(WARNING "cppcheck requested but not found")
    endif()
endif()
