cmake_minimum_required(VERSION 3.20)
project(ScratchRobin VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

add_executable(scratchrobin
    src/main.cpp
    src/app/app.cpp
    src/ui/main_frame.cpp
    src/ui/sql_editor_frame.cpp
    src/core/connection_manager.cpp
    src/core/config.cpp
)

target_include_directories(scratchrobin PRIVATE src)

target_link_libraries(scratchrobin PRIVATE ${wxWidgets_LIBRARIES})
