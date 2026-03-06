# ==============================================================================
# ScratchRobin Source CMake Configuration (Qt6)
# ==============================================================================

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
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

add_library(scratchrobin_backend STATIC ${SCRATCHROBIN_BACKEND_SOURCES})

target_include_directories(scratchrobin_backend PUBLIC ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(scratchrobin_backend PUBLIC OpenSSL::Crypto Qt6::Core)

target_compile_definitions(scratchrobin_backend PUBLIC
    SCRATCHROBIN_DEFAULT_SCRATCHBIRD_SOURCE_DIR="${SCRATCHBIRD_SOURCE_DIR}"
    SCRATCHROBIN_DEFAULT_PREVIEW_METADATA_PATH="${PROJECT_SOURCE_DIR}/resources/preview_metadata.json"
    SCRATCHROBIN_DEFAULT_PREVIEW_OBJECT_METADATA_PATH="${PROJECT_SOURCE_DIR}/resources/preview_object_metadata.json"
)

if(SCRATCHROBIN_ENABLE_SBWP)
    target_compile_definitions(scratchrobin_backend PUBLIC SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP=1)
    target_include_directories(scratchrobin_backend PUBLIC ${SCRATCHBIRD_SOURCE_DIR}/include)
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
    ui/qt_app.cpp
)

add_library(scratchrobin_ui STATIC ${SCRATCHROBIN_UI_SOURCES})

target_include_directories(scratchrobin_ui PUBLIC ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(scratchrobin_ui PUBLIC scratchrobin_backend Qt6::Core Qt6::Gui Qt6::Widgets)

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
