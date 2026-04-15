cmake_minimum_required(VERSION 3.16)

project(PTBQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/boardview.cpp
    src/columnwidget.cpp
    src/cardwidget.cpp
    src/model/appstate.cpp
    src/model/markdownio.cpp
)

add_executable(ptb ${SOURCES})

# Make src/ a root for includes so model/*.h can be included as "model/foo.h"
target_include_directories(ptb PRIVATE src)

target_link_libraries(ptb Qt6::Widgets)
