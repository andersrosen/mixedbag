cmake_minimum_required(VERSION 3.23)
project(mixedbag VERSION 0.0.1 LANGUAGES CXX)

option(MIXEDBAG_ENABLE_TESTS "Build tests for the mixedbag project" ${PROJECT_IS_TOP_LEVEL})
option(MIXEDBAG_ENABLE_DOCS "Generate docs for the mixedbag project" OFF)
option(MIXEDBAG_ENABLE_INSTALL "Enable installation of the mixedbag project" ${PROJECT_IS_TOP_LEVEL})

include(GNUInstallDirs)

add_library(mixedbag)
add_library(mixedbag::mixedbag ALIAS mixedbag)
target_compile_features(mixedbag PUBLIC cxx_std_20)
set_target_properties(mixedbag PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
set_target_properties(mixedbag PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES hidden
)
target_include_directories(mixedbag
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDE_DIR}>
)
target_sources(mixedbag PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/mixedbag/sparse_vector.hxx
        include/mixedbag/bookkeeping_memory_resource.hxx
)
target_sources(mixedbag PRIVATE
    source/bookkeeping_memory_resource.cxx
)

# Generated files
include(GenerateExportHeader)
generate_export_header(mixedbag EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/mixedbag/exports.h)
set_property(
    TARGET mixedbag
    PROPERTY PUBLIC_HEADER
        ${CMAKE_CURRENT_BINARY_DIR}/include/mixedbag/exports.h
)

if (MIXEDBAG_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (MIXEDBAG_ENABLE_DOCS)
    add_subdirectory(docs)
endif()

if (MIXEDBAG_ENABLE_INSTALL)
    include(cmake/install.cmake)
endif()