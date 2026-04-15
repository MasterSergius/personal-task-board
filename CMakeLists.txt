cmake_minimum_required(VERSION 3.16)

project(PTBQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

# ── Model library ─────────────────────────────────────────────────────────────
# Compiled separately so the test suite can link it without pulling in
# Qt::Widgets or the application's UI code.

add_library(ptb_model STATIC
    src/model/appstate.cpp
    src/model/markdownio.cpp
)
set_target_properties(ptb_model PROPERTIES AUTOMOC OFF)
target_include_directories(ptb_model PUBLIC src)
target_link_libraries(ptb_model Qt6::Core)

# ── Main application ──────────────────────────────────────────────────────────

add_executable(ptb
    src/main.cpp
    src/mainwindow.cpp
    src/boardview.cpp
    src/columnwidget.cpp
    src/cardwidget.cpp
)
target_link_libraries(ptb ptb_model Qt6::Widgets)

# ── Tests ─────────────────────────────────────────────────────────────────────

enable_testing()
add_subdirectory(tests)
