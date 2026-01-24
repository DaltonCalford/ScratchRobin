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

if(SCRATCHROBIN_BUILD_UI)
    set(SCRATCHROBIN_WX_COMPONENTS core base
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
    src/core/mock_backend.cpp
    src/core/network_backend.cpp
    src/core/job_queue.cpp
    src/core/config.cpp
    src/core/credentials.cpp
    src/core/metadata_model.cpp
    src/core/statement_splitter.cpp
    src/core/value_formatter.cpp
    src/core/result_exporter.cpp
    src/core/simple_json.cpp
    src/core/postgres_backend.cpp
    src/core/mysql_backend.cpp
    src/core/firebird_backend.cpp
)

add_library(scratchrobin_core ${SCRATCHROBIN_CORE_SOURCES})
target_include_directories(scratchrobin_core PUBLIC src)

if(SCRATCHROBIN_BUILD_UI)
    add_executable(scratchrobin
        src/main.cpp
        src/app/app.cpp
        src/ui/main_frame.cpp
        src/ui/diagram_frame.cpp
        src/ui/diagram_model.cpp
        src/ui/diagram_canvas.cpp
        src/ui/diagram_page.cpp
        src/ui/diagram_template_dialog.cpp
        src/ui/monitoring_frame.cpp
        src/ui/sql_editor_frame.cpp
        src/ui/users_roles_frame.cpp
        src/ui/startup_frame.cpp
        src/ui/menu_builder.cpp
        src/ui/icon_bar.cpp
        src/ui/window_manager.cpp
        src/ui/result_grid_table.cpp
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
        target_link_libraries(scratchrobin_core PRIVATE scratchbird_client)
        target_compile_definitions(scratchrobin_core PRIVATE SCRATCHROBIN_USE_SCRATCHBIRD=1)
    else()
        message(WARNING "ScratchBird root not found at ${SCRATCHBIRD_ROOT}; building without ScratchBird support.")
    endif()
endif()

enable_testing()

add_executable(scratchrobin_tests
    tests/test_main.cpp
)

target_link_libraries(scratchrobin_tests PRIVATE scratchrobin_core)
target_include_directories(scratchrobin_tests PRIVATE src)
target_compile_definitions(scratchrobin_tests PRIVATE
    SCRATCHROBIN_TEST_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\")

add_test(NAME scratchrobin_tests COMMAND scratchrobin_tests)
