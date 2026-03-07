# ==============================================================================
# ScratchRobin Source CMake Configuration (Qt6)
# ==============================================================================

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# -----------------------------------------------------------------------------
# ScratchBird Driver Integration
# -----------------------------------------------------------------------------
set(SCRATCHBIRD_DRIVER_DIR "${PROJECT_SOURCE_DIR}/../ScratchBird-driver" CACHE PATH "ScratchBird driver directory")
set(SCRATCHBIRD_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../ScratchBird" CACHE PATH "ScratchBird source directory")

# Find ScratchBird client library
find_library(SCRATCHBIRD_CLIENT_LIB 
    NAMES scratchbird_client 
    PATHS 
        "${SCRATCHBIRD_DRIVER_DIR}/build/odbc_local/scratchbird_cpp"
        "${SCRATCHBIRD_DRIVER_DIR}/build/lib"
    NO_DEFAULT_PATH
)

if(SCRATCHBIRD_CLIENT_LIB)
    message(STATUS "ScratchBird client library found: ${SCRATCHBIRD_CLIENT_LIB}")
    set(SCRATCHBIRD_FOUND TRUE)
else()
    message(WARNING "ScratchBird client library not found - building with mock implementation")
    set(SCRATCHBIRD_FOUND FALSE)
endif()

# ScratchBird include directories
set(SCRATCHBIRD_INCLUDE_DIRS
    "${SCRATCHBIRD_DRIVER_DIR}/tracks/beta/drivers/cpp/include"
    "${SCRATCHBIRD_SOURCE_DIR}/include"
)

# -----------------------------------------------------------------------------
# Backend Library
# -----------------------------------------------------------------------------
set(SCRATCHROBIN_BACKEND_SOURCES
    backend/parser_port_registry.cpp
    backend/native_parser_compiler.cpp
    backend/server_session_gateway.cpp
    backend/native_adapter_gateway.cpp
    backend/preview_metadata_store.cpp
    backend/preview_object_metadata_store.cpp
    backend/scratchbird_catalog_preview.cpp
    backend/session_client.cpp
    backend/scratchbird_sbwp_client.cpp
    backend/scratchbird_connection.cpp
    backend/scratchbird_metadata_provider.cpp
    backend/scratchbird_object_types.cpp
    core/app_config.cpp
    core/settings.cpp
    core/scratchbird_auth.cpp
    core/window_state_manager.cpp
    core/performance_profiler.cpp
)

add_library(scratchrobin_backend STATIC ${SCRATCHROBIN_BACKEND_SOURCES})

target_include_directories(scratchrobin_backend PUBLIC 
    ${PROJECT_SOURCE_DIR}/src
    ${SCRATCHBIRD_INCLUDE_DIRS}
)

target_link_libraries(scratchrobin_backend PUBLIC 
    OpenSSL::Crypto 
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Threads::Threads
)

# Link ScratchBird client if found
if(SCRATCHBIRD_FOUND)
    target_link_libraries(scratchrobin_backend PUBLIC 
        ${SCRATCHBIRD_CLIENT_LIB}
        OpenSSL::SSL
        OpenSSL::Crypto
    )
    target_compile_definitions(scratchrobin_backend PUBLIC SCRATCHBIRD_CLIENT_AVAILABLE=1)
    message(STATUS "ScratchBird integration enabled")
else()
    target_compile_definitions(scratchrobin_backend PUBLIC SCRATCHBIRD_CLIENT_AVAILABLE=0)
endif()

target_compile_definitions(scratchrobin_backend PUBLIC
    SCRATCHROBIN_DEFAULT_SCRATCHBIRD_SOURCE_DIR="${SCRATCHBIRD_SOURCE_DIR}"
    SCRATCHROBIN_DEFAULT_PREVIEW_METADATA_PATH="${PROJECT_SOURCE_DIR}/resources/preview_metadata.json"
    SCRATCHROBIN_DEFAULT_PREVIEW_OBJECT_METADATA_PATH="${PROJECT_SOURCE_DIR}/resources/preview_object_metadata.json"
)

if(SCRATCHROBIN_ENABLE_SBWP)
    target_compile_definitions(scratchrobin_backend PUBLIC SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP=1)
endif()

set_target_properties(scratchrobin_backend PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
)

# -----------------------------------------------------------------------------
# UI Library (Qt6)
# -----------------------------------------------------------------------------
set(SCRATCHROBIN_UI_SOURCES
    ui/main_window.cpp
    ui/project_navigator.cpp
    ui/splash_screen.cpp
    ui/sql_editor.cpp
    ui/connection_dialog.cpp
    ui/data_grid.cpp
    ui/preferences_dialog.cpp
    ui/preferences_dialog_tabs.cpp
    ui/er_diagram_widget.cpp
    ui/doc_generator_dialog.cpp
    ui/toolbar_editor_dialog.cpp
    ui/menu_editor_dialog.cpp
    ui/theme_manager.cpp
    ui/theme_settings_dialog.cpp
    ui/query_menu.cpp
    ui/transaction_menu.cpp
    ui/window_menu.cpp
    ui/tools_menu.cpp
    ui/find_replace_dialog.cpp
    ui/about_dialog.cpp
    ui/shortcuts_dialog.cpp
    ui/tip_of_day.cpp
    ui/export_results_dialog.cpp
    ui/import_wizard.cpp
    ui/table_designer_dialog.cpp
    ui/sql_completer.cpp
    ui/query_history.cpp
    ui/transaction_manager.cpp
    ui/explain_plan_viewer.cpp
    ui/monitoring_panels.cpp
    ui/type_renderer.cpp
    ui/dock_workspace.cpp
    ui/view_manager.cpp
    ui/trigger_manager.cpp
    ui/git_integration.cpp
    ui/reporting_system.cpp
    ui/dashboard_builder.cpp
    ui/user_role_manager.cpp
    ui/privilege_manager.cpp
    ui/procedure_manager.cpp
    ui/sequence_manager.cpp
    ui/package_manager.cpp
    ui/synonym_manager.cpp
    ui/partition_manager.cpp
    ui/foreign_data_wrapper_manager.cpp
    ui/extension_manager.cpp
    ui/fts_manager.cpp
    ui/replication_manager.cpp
    ui/connection_pool_monitor.cpp
    ui/lock_manager.cpp
    ui/session_manager.cpp
    ui/bulk_data_loader.cpp
    ui/data_compare_sync.cpp
    ui/schema_compare.cpp
    ui/schema_migration_tool.cpp
    ui/data_modeler.cpp
    ui/query_builder.cpp
    ui/query_plan_visualizer.cpp
    ui/sql_debugger.cpp
    ui/procedure_debugger.cpp
    ui/sql_profiler.cpp
    ui/code_formatter.cpp
    ui/performance_dashboard.cpp
    ui/slow_query_log_viewer.cpp
    ui/database_health_check.cpp
    ui/alert_notification_system.cpp
    ui/user_management.cpp
    ui/role_management.cpp
    ui/permission_browser.cpp
    ui/audit_log_viewer.cpp
    ui/ssl_certificate_manager.cpp
    ui/backup_restore.cpp
    ui/scheduled_jobs.cpp
    ui/vacuum_analyze_tool.cpp
    ui/data_generator.cpp
    ui/data_masking.cpp
    ui/data_cleansing.cpp
    ui/data_lineage.cpp
    ui/team_sharing.cpp
    ui/query_comments.cpp
    ui/change_tracking.cpp
    ui/plugin_system.cpp
    ui/macro_recording.cpp
    ui/scripting_console.cpp
    ui/custom_keybindings.cpp
    ui/query_favorites.cpp
    ui/query_parameters.cpp
    ui/qt_app.cpp
)

add_library(scratchrobin_ui STATIC ${SCRATCHROBIN_UI_SOURCES})

target_include_directories(scratchrobin_ui PUBLIC ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(scratchrobin_ui PUBLIC scratchrobin_backend Qt6::Core Qt6::Gui Qt6::Widgets Qt6::PrintSupport)

set_target_properties(scratchrobin_ui PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
    AUTOMOC ON
)

# -----------------------------------------------------------------------------
# CLI Executable
# -----------------------------------------------------------------------------
add_executable(scratchrobin app/main.cpp)
target_link_libraries(scratchrobin PRIVATE scratchrobin_backend scratchrobin_ui Qt6::Core)
set_target_properties(scratchrobin PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

# -----------------------------------------------------------------------------
# GUI Executable (Qt6)
# -----------------------------------------------------------------------------
add_executable(scratchrobin-gui app/gui_main.cpp)
target_link_libraries(scratchrobin-gui PRIVATE scratchrobin_backend scratchrobin_ui Qt6::Core Qt6::Gui Qt6::Widgets)
target_include_directories(scratchrobin-gui PRIVATE ${PROJECT_SOURCE_DIR}/src)
set_target_properties(scratchrobin-gui PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    AUTOMOC ON
)

message(STATUS "Qt6 found: scratchrobin-gui executable will be built")
