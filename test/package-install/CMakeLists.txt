cmake_minimum_required(VERSION 3.14)

project(chesslib_package_smoke LANGUAGES CXX)

find_package(chesslib REQUIRED CONFIG)

add_executable(chesslib_package_smoke main.cpp)
target_link_libraries(chesslib_package_smoke PRIVATE chesslib::chesslib)
target_compile_features(chesslib_package_smoke PRIVATE cxx_std_20)