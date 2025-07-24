# Generate CMake package configuration files, to support find_package()
include(CMakePackageConfigHelpers)
configure_package_config_file(
    mixedbag-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/mixedbag-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mixedbag
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/mixedbag-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
# Meta files that should always be installed
install(
    FILES
        ${CMAKE_SOURCE_DIR}/LICENSE
        ${CMAKE_SOURCE_DIR}/README.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    COMPONENT mixedbag_Runtime
)

# Library
install(TARGETS mixedbag
    EXPORT mixedbag-exports
    LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT mixedbag_Runtime
        NAMELINK_COMPONENT mixedbag_Development
    ARCHIVE
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT mixedbag_Development
    PUBLIC_HEADER
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mixedbag
        COMPONENT mixedbag_Development
    FILE_SET HEADERS
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT mixedbag_Development
)

# CMake target exports
install(EXPORT mixedbag-exports
    FILE mixedbag-targets.cmake
    NAMESPACE mixedbag::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mixedbag
    COMPONENT mixedbag_Development
)
