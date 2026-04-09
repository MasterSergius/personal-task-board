cmake_minimum_required(VERSION 3.16)

project(PTBQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/boardview.cpp
    src/columnwidget.cpp
    src/cardwidget.cpp
)

add_executable(ptb ${SOURCES})

target_link_libraries(ptb Qt6::Widgets)
