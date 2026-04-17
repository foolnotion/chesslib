if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/chesslib-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package chesslib)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT chesslib_Development
)

install(
    TARGETS chesslib_chesslib
    EXPORT chesslibTargets
    RUNTIME #
    COMPONENT chesslib_Runtime
    LIBRARY #
    COMPONENT chesslib_Runtime
    NAMELINK_COMPONENT chesslib_Development
    ARCHIVE #
    COMPONENT chesslib_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    chesslib_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE chesslib_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(chesslib_INSTALL_CMAKEDIR)

configure_file(
    cmake/install-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${package}Config.cmake"
    @ONLY
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${package}Config.cmake"
    DESTINATION "${chesslib_INSTALL_CMAKEDIR}"
    COMPONENT chesslib_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${chesslib_INSTALL_CMAKEDIR}"
    COMPONENT chesslib_Development
)

install(
    EXPORT chesslibTargets
    NAMESPACE chesslib::
    DESTINATION "${chesslib_INSTALL_CMAKEDIR}"
    COMPONENT chesslib_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
