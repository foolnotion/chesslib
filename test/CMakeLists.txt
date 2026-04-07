cmake_minimum_required(VERSION 3.14)

project(chesslibTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(chesslib REQUIRED)
  enable_testing()
endif()

find_package(Catch2 CONFIG REQUIRED)
include(Catch)
find_package(nanobench CONFIG REQUIRED)

# ---- Tests ----

add_executable(chesslib_test
    source/chesslib_test.cpp
    source/fen.cpp
    source/perft.cpp
    source/bench.cpp
)

target_link_libraries(chesslib_test
    PRIVATE
    chesslib::chesslib
    Catch2::Catch2WithMain
    fmt::fmt
    libassert::assert
    nlohmann_json::nlohmann_json
    nanobench::nanobench
)
target_compile_features(chesslib_test PRIVATE cxx_std_20)

catch_discover_tests(chesslib_test WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# ---- End-of-file commands ----

add_folders(Test)
