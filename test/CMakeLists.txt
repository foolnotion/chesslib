cmake_minimum_required(VERSION 3.14)

project(chesslibTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(chesslib REQUIRED)
  enable_testing()
endif()

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----s

add_executable(chesslib_test source/chesslib_test.cpp)
target_link_libraries(
    chesslib_test PRIVATE
    chesslib::chesslib
    Catch2::Catch2WithMain
)
# target
target_compile_features(chesslib_test PRIVATE cxx_std_20)

catch_discover_tests(chesslib_test)

# ---- End-of-file commands ----

add_folders(Test)
