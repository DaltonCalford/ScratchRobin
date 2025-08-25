# Source files
set(SOURCES
    main.cpp
    core/application.cpp
    core/connection_manager.cpp
    core/metadata_manager.cpp
    core/config_manager.cpp
    metadata/schema_collector.cpp
    metadata/object_hierarchy.cpp
    metadata/cache_manager.cpp
    metadata/metadata_manager.cpp
    search/search_engine.cpp
    editor/text_editor.cpp
    execution/sql_executor.cpp
    query/query_history.cpp
    table/table_designer.cpp
    index/index_manager.cpp
    constraint/constraint_manager.cpp
    ui/main_window.cpp
    ui/splash_screen.cpp
    ui/connection_dialog.cpp
    ui/about_dialog.cpp
    ui/preferences_dialog.cpp
    ui/update_dialog.cpp
    ui/import_export_dialog.cpp
    ui/query_history_dialog.cpp
    ui/backup_restore_dialog.cpp
    ui/find_replace_dialog.cpp
    ui/column_editor_dialog.cpp
    ui/data_editor_dialog.cpp
    ui/progress_dialog.cpp
    ui/error_details_dialog.cpp
    ui/confirmation_dialog.cpp
    ui/keyboard_shortcuts_dialog.cpp
    ui/favorites_manager_dialog.cpp
    ui/dynamic_connection_dialog.cpp
    ui/table_designer_dialog.cpp
    ui/stored_procedure_dialog.cpp
    ui/view_editor_dialog.cpp
    ui/index_manager_dialog.cpp
    ui/constraint_manager_dialog.cpp
    ui/trigger_manager_dialog.cpp
    ui/function_editor_dialog.cpp
    ui/schema_manager_dialog.cpp
    ui/user_defined_types_dialog.cpp
    database/database_driver_manager.cpp
    # database/mssql_features.cpp
    # database/mssql_catalog.cpp
    # database/mssql_connection.cpp
    # database/mssql_syntax.cpp
    # database/mariadb_features.cpp
    # database/mariadb_catalog.cpp
    # database/mariadb_connection.cpp
    # database/mariadb_syntax.cpp
    # database/mysql_features.cpp
    # database/mysql_catalog.cpp
    # database/mysql_connection.cpp
    database/postgresql_features.cpp
    database/postgresql_catalog.cpp
    database/postgresql_connection.cpp
    database/postgresql_syntax.cpp
    ui/app_icons.cpp
    ui/object_browser/tree_model.cpp
    ui/object_browser/object_browser.cpp
    ui/properties/property_viewer.cpp
    ui/result_viewer.cpp
    utils/logger.cpp
    utils/string_utils.cpp
    utils/file_utils.cpp
    ../resource/resources.qrc
    ../resource/icons.qrc
    components/sql_executor.cpp
    components/query_parser.cpp
    components/schema_loader.cpp
)

# Header files
set(HEADERS
    core/application.h
    core/connection_manager.h
    core/metadata_manager.h
    core/config_manager.h
    metadata/schema_collector.h
    metadata/object_hierarchy.h
    metadata/cache_manager.h
    metadata/metadata_manager.h
    search/search_engine.h
    editor/text_editor.h
    execution/sql_executor.h
    query/query_history.h
    table/table_designer.h
    index/index_manager.h
    constraint/constraint_manager.h
    ui/main_window.h
    ui/splash_screen.h
    ui/connection_dialog.h
    ui/about_dialog.h
    ui/preferences_dialog.h
    ui/update_dialog.h
    ui/import_export_dialog.h
    ui/query_history_dialog.h
    ui/backup_restore_dialog.h
    ui/find_replace_dialog.h
    ui/column_editor_dialog.h
    ui/data_editor_dialog.h
    ui/progress_dialog.h
    ui/error_details_dialog.h
    ui/confirmation_dialog.h
    ui/keyboard_shortcuts_dialog.h
    ui/favorites_manager_dialog.h
    ui/dynamic_connection_dialog.h
    ui/table_designer_dialog.h
    ui/stored_procedure_dialog.h
    ui/view_editor_dialog.h
    ui/index_manager_dialog.h
    ui/constraint_manager_dialog.h
    ui/trigger_manager_dialog.h
    ui/function_editor_dialog.h
    ui/schema_manager_dialog.h
    ui/user_defined_types_dialog.h
    database/database_driver_manager.h
    # database/mssql_features.h
    # database/mssql_catalog.h
    # database/mssql_connection.h
    # database/mssql_syntax.h
    # database/mariadb_features.h
    # database/mariadb_catalog.h
    # database/mariadb_connection.h
    # database/mariadb_syntax.h
    # database/mysql_features.h
    # database/mysql_catalog.h
    # database/mysql_connection.h
    database/postgresql_features.h
    database/postgresql_catalog.h
    database/postgresql_connection.h
    database/postgresql_syntax.h
    ui/app_icons.h
    ui/object_browser/tree_model.h
    ui/object_browser/object_browser.h
    ui/properties/property_viewer.h
    ui/result_viewer.h
    utils/logger.h
    utils/string_utils.h
    utils/file_utils.h
    components/sql_executor.h
    components/query_parser.h
    components/schema_loader.h
    types/database_types.h
    types/connection_types.h
    types/query_types.h
    types/result.h
)

# Create library first
add_library(scratchrobin_lib STATIC ${SOURCES} ${HEADERS})

# Link libraries to the library
target_link_libraries(scratchrobin_lib
    PRIVATE
        Threads::Threads
        ${CMAKE_DL_LIBS}
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        Qt5::Network
        Qt5::Sql
        Qt5::Xml
)

# Find and link database drivers
find_package(PostgreSQL)
if(PostgreSQL_FOUND)
    target_link_libraries(scratchrobin_lib PRIVATE ${PostgreSQL_LIBRARIES})
    target_include_directories(scratchrobin_lib PRIVATE ${PostgreSQL_INCLUDE_DIRS})
    add_definitions(-DHAS_POSTGRESQL)
endif()

find_package(MySQL)
if(MySQL_FOUND)
    target_link_libraries(scratchrobin_lib PRIVATE ${MySQL_LIBRARIES})
    target_include_directories(scratchrobin_lib PRIVATE ${MySQL_INCLUDE_DIRS})
    add_definitions(-DHAS_MYSQL)
endif()

# Check for MariaDB (often uses MySQL libraries)
find_library(MARIADB_LIBRARY NAMES mariadb mariadbclient)
if(MARIADB_LIBRARY)
    target_link_libraries(scratchrobin_lib PRIVATE ${MARIADB_LIBRARY})
    add_definitions(-DHAS_MARIADB)
endif()

# Find ODBC
find_package(ODBC)
if(ODBC_FOUND)
    target_link_libraries(scratchrobin_lib PRIVATE ${ODBC_LIBRARIES})
    target_include_directories(scratchrobin_lib PRIVATE ${ODBC_INCLUDE_DIRS})
    add_definitions(-DHAS_ODBC)
endif()

# Find Firebird
find_library(FIREBIRD_LIBRARY NAMES fbclient)
if(FIREBIRD_LIBRARY)
    target_link_libraries(scratchrobin_lib PRIVATE ${FIREBIRD_LIBRARY})
    add_definitions(-DHAS_FIREBIRD)
endif()

# Find IBM DB2
find_library(DB2_LIBRARY NAMES db2)
if(DB2_LIBRARY)
    target_link_libraries(scratchrobin_lib PRIVATE ${DB2_LIBRARY})
    add_definitions(-DHAS_DB2)
endif()

# Include directories for the library
target_include_directories(scratchrobin_lib
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set C++ standard for the library
target_compile_features(scratchrobin_lib PRIVATE cxx_std_17)

# Create executable that links to the library
add_executable(scratchrobin main.cpp)

target_link_libraries(scratchrobin
    PRIVATE
        scratchrobin_lib
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        Qt5::Network
        Qt5::Sql
        Qt5::Xml
)

# Set C++ standard
target_compile_features(scratchrobin PRIVATE cxx_std_17)

# Add PostgreSQL driver explicitly if available
find_package(PostgreSQL)
if(PostgreSQL_FOUND)
    target_link_libraries(scratchrobin PRIVATE ${PostgreSQL_LIBRARIES})
    target_include_directories(scratchrobin PRIVATE ${PostgreSQL_INCLUDE_DIRS})
endif()

# Installation
install(TARGETS scratchrobin scratchrobin_lib
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)