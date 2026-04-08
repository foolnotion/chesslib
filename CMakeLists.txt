cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    chesslib
    VERSION 0.1.0
    DESCRIPTION "Chess library for teaching"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    chesslib_chesslib
    source/board.cpp
    source/zobrist.cpp
    source/util/fen.cpp
    source/util/perft.cpp
    source/util/san.cpp
    source/util/uci.cpp
)
add_library(chesslib::chesslib ALIAS chesslib_chesslib)
add_executable(perft source/perft/perft.cpp)

include(GenerateExportHeader)
generate_export_header(
    chesslib_chesslib
    BASE_NAME chesslib
    EXPORT_FILE_NAME export/chesslib/chesslib_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(chesslib_chesslib PUBLIC CHESSLIB_STATIC_DEFINE)
endif()

set_target_properties(
    chesslib_chesslib PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME chesslib
    OUTPUT_NAME chesslib
)

target_include_directories(
    chesslib_chesslib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    chesslib_chesslib SYSTEM
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(chesslib_chesslib PUBLIC cxx_std_20)

find_package(cxxopts CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(libassert CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(mdspan CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(small_vector CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(unordered_dense CONFIG REQUIRED)

target_link_libraries(chesslib_chesslib
    PUBLIC
    gch::small_vector
    magic_enum::magic_enum
    tl::expected
    unordered_dense::unordered_dense
    PRIVATE
    fmt::fmt
    libassert::assert
    std::mdspan
)

target_link_libraries(perft PUBLIC chesslib::chesslib cxxopts::cxxopts)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
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
