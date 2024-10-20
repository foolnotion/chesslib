cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    chesslib
    VERSION 0.1.0
    DESCRIPTION "Chess Library for Teaching"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----
set(CMAKE_VERIFY_INTERFACE_HEADER_SETS OFF)
add_library(chesslib_chesslib INTERFACE)
add_library(chesslib::chesslib ALIAS chesslib_chesslib)

set_property(
    TARGET chesslib_chesslib PROPERTY
    EXPORT_NAME chesslib
)

target_include_directories(
    chesslib_chesslib ${warning_guard}
    INTERFACE
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(chesslib_chesslib INTERFACE cxx_std_20)

find_package(fmt CONFIG REQUIRED)
find_package(libassert CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(small_vector CONFIG REQUIRED)
find_package(unordered_dense CONFIG REQUIRED)
target_link_libraries(chesslib_chesslib
    INTERFACE
    fmt::fmt
    gch::small_vector
    libassert::assert
    magic_enum::magic_enum
    unordered_dense::unordered_dense
)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${chesslib_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT chesslib_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of chesslib"
  )
endif()

include(cmake/dev-mode.cmake)
