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
    ui/connection_dialog.cpp
    ui/query_editor.cpp
    ui/object_browser/tree_model.cpp
    ui/object_browser/object_browser.cpp
    ui/properties/property_viewer.cpp
    ui/result_viewer.cpp
    utils/logger.cpp
    utils/string_utils.cpp
    utils/file_utils.cpp
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
    ui/connection_dialog.h
    ui/query_editor.h
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
)

# Set C++ standard
target_compile_features(scratchrobin PRIVATE cxx_std_17)

# Installation
install(TARGETS scratchrobin scratchrobin_lib
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)