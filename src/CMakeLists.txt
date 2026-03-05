# ==============================================================================
# ScratchRobin Source CMake Configuration
# ==============================================================================

# Find OpenSSL (required for encryption)
find_package(OpenSSL REQUIRED)

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
    core/app_config.cpp
    core/settings.cpp
    core/scratchbird_auth.cpp
)

add_library(scratchrobin_backend STATIC
    ${SCRATCHROBIN_BACKEND_SOURCES}
)

target_include_directories(scratchrobin_backend
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(scratchrobin_backend
    PUBLIC
        OpenSSL::Crypto
)

target_compile_definitions(scratchrobin_backend
    PUBLIC
        SCRATCHROBIN_DEFAULT_SCRATCHBIRD_SOURCE_DIR="${SCRATCHBIRD_SOURCE_DIR}"
        SCRATCHROBIN_DEFAULT_PREVIEW_METADATA_PATH="${PROJECT_SOURCE_DIR}/resources/preview_metadata.json"
        SCRATCHROBIN_DEFAULT_PREVIEW_OBJECT_METADATA_PATH="${PROJECT_SOURCE_DIR}/resources/preview_object_metadata.json"
)

# ScratchBird SBWP integration
if(SCRATCHROBIN_ENABLE_SBWP)
    target_compile_definitions(scratchrobin_backend
        PUBLIC
            SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP=1
    )
    target_include_directories(scratchrobin_backend
        PUBLIC
            ${SCRATCHBIRD_SOURCE_DIR}/include
    )
endif()

set_target_properties(scratchrobin_backend PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
)

# -----------------------------------------------------------------------------
# UI Library
# -----------------------------------------------------------------------------
set(SCRATCHROBIN_UI_SOURCES
    ui/main_window.cpp
)

add_library(scratchrobin_ui STATIC
    ${SCRATCHROBIN_UI_SOURCES}
)

target_include_directories(scratchrobin_ui
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(scratchrobin_ui
    PUBLIC
        scratchrobin_backend
)

set_target_properties(scratchrobin_ui PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
)

# -----------------------------------------------------------------------------
# CLI Executable
# -----------------------------------------------------------------------------
add_executable(scratchrobin
    app/main.cpp
)

target_link_libraries(scratchrobin
    PRIVATE
        scratchrobin_backend
        scratchrobin_ui
)

set_target_properties(scratchrobin PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# -----------------------------------------------------------------------------
# GUI Executable (wxWidgets)
# -----------------------------------------------------------------------------
find_package(wxWidgets COMPONENTS core base aui stc propgrid html QUIET)

if(wxWidgets_FOUND)
    include(${wxWidgets_USE_FILE})

    # Dialog sources (18 dialogs)
    set(SCRATCHROBIN_GUI_DIALOG_SOURCES
        ui/dialogs/admin_dialog.cpp
        ui/dialogs/backup_restore_dialog.cpp
        ui/dialogs/connection_dialog.cpp
        ui/dialogs/connection_manager_dialog.cpp
        ui/dialogs/data_import_export_dialog.cpp
        ui/dialogs/database_migration_wizard.cpp
        ui/dialogs/migration_dialog.cpp
        ui/dialogs/object_metadata_dialog_new.cpp
        ui/dialogs/performance_monitor_dialog.cpp
        ui/dialogs/preferences_dialog.cpp
        ui/dialogs/query_builder_dialog.cpp
        ui/dialogs/query_history_dialog.cpp
        ui/dialogs/query_plan_visualizer.cpp
        ui/dialogs/query_profiler_dialog.cpp
        ui/dialogs/query_progress_dialog.cpp
        ui/dialogs/schema_comparison_dialog.cpp
        ui/dialogs/schema_documentation_dialog.cpp
        ui/dialogs/snippet_browser.cpp
    )

    # Component sources (7 components)
    set(SCRATCHROBIN_GUI_COMPONENT_SOURCES
        ui/components/chart_component.cpp
        ui/components/data_cell_editor.cpp
        ui/components/data_grid.cpp
        ui/components/data_grid_paginated.cpp
        ui/components/sql_code_completion.cpp
        ui/components/sql_editor.cpp
        ui/components/visual_query_builder.cpp
    )

    # Resource files
    set(SCRATCHROBIN_GUI_RES_SOURCES
        res/art_provider.cpp
        res/exception16_png.cpp
        res/fk16_png.cpp
        res/icon_manager.cpp
        res/pk16_png.cpp
        res/pkfk16_png.cpp
        res/role16_png.cpp
        res/tree_image_list.cpp
    )

    # Window framework files
    set(SCRATCHROBIN_GUI_FRAMEWORK_SOURCES
        ui/window_framework/action_catalog.cpp
        ui/window_framework/action_registry.cpp
        ui/window_framework/dockable_window.cpp
        ui/window_framework/docking_overlay.cpp
        ui/window_framework/dynamic_menu_bar.cpp
        ui/window_framework/icon_bar.cpp
        ui/window_framework/menu_editor.cpp
        ui/window_framework/menu_registry.cpp
        ui/window_framework/project_navigator_new.cpp
        ui/window_framework/toolbar_editor.cpp
        ui/window_framework/window_manager.cpp
        ui/window_framework/window_type_catalog.cpp
        ui/window_framework/window_types.cpp
    )

    # Core manager files
    set(SCRATCHROBIN_GUI_CORE_SOURCES
        core/async_query_executor.cpp
        core/audit_log_manager.cpp
        core/backup_manager.cpp
        core/bulk_edit_manager.cpp
        core/code_snippet_manager.cpp
        core/connection_pool_manager.cpp
        core/data_sync_manager.cpp
        core/database_migration_manager.cpp
        core/export_manager.cpp
        core/index_advisor.cpp
        core/migration_manager.cpp
        core/performance_monitor.cpp
        core/query_history.cpp
        core/query_profiler.cpp
        core/report_manager.cpp
        core/schema_documentation.cpp
        core/scratchbird_catalog_client.cpp
        core/scratchbird_context_parser.cpp
        core/scratchbird_dialect.cpp
        core/scratchbird_types.cpp
        core/security_manager.cpp
        core/server_monitor.cpp
        core/sql_formatter.cpp
        core/streaming_data_importer.cpp
        core/user_manager.cpp
    )

    # Additional UI files
    set(SCRATCHROBIN_GUI_UI_SOURCES
        ui/main_frame_new.cpp
        ui/object_metadata_dialog.cpp
        ui/settings_dialog.cpp
        ui/splash_screen.cpp
        ui/sql_editor_frame_new.cpp
        ui/table_metadata_dialog.cpp
        ui/unlock_dialog.cpp
        ui/wx_app.cpp
    )

    # GUI executable
    add_executable(scratchrobin-gui
        app/gui_main.cpp
        ${SCRATCHROBIN_GUI_DIALOG_SOURCES}
        ${SCRATCHROBIN_GUI_COMPONENT_SOURCES}
        ${SCRATCHROBIN_GUI_RES_SOURCES}
        ${SCRATCHROBIN_GUI_FRAMEWORK_SOURCES}
        ${SCRATCHROBIN_GUI_CORE_SOURCES}
        ${SCRATCHROBIN_GUI_UI_SOURCES}
    )

    target_include_directories(scratchrobin-gui
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src
            ${wxWidgets_INCLUDE_DIRS}
    )

    target_link_libraries(scratchrobin-gui
        PRIVATE
            scratchrobin_backend
            scratchrobin_ui
            ${wxWidgets_LIBRARIES}
            OpenSSL::Crypto
    )

    target_compile_definitions(scratchrobin-gui
        PRIVATE
            ${wxWidgets_DEFINITIONS}
    )

    set_target_properties(scratchrobin-gui PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    message(STATUS "wxWidgets found: scratchrobin-gui executable will be built")
else()
    message(STATUS "wxWidgets not found: scratchrobin-gui executable will not be built")
endif()
