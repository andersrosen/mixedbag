find_package(Doxygen)
if (NOT DOXYGEN_FOUND)
    message(WARNING "Can't generate documentation, since doxygen was not found!")
    return()
endif()

include(FetchContent)
FetchContent_Declare(doxygen-awesome-css
    URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/heads/main.zip
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(doxygen-awesome-css)

# Save the location the files were cloned into
# This allows us to get the path to doxygen-awesome css files
FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)

set(DOXYGEN_USE_MDFILE_AS_MAINPAGE mainpage.md)
set(DOXYGEN_DISTRIBUTE_GROUP_DOC YES)
set(DOXYGEN_GENERATE_TREEVIEW YES)
set(DOXYGEN_DISABLE_INDEX NO)
set(DOXYGEN_FULL_SIDEBAR NO)
set(DOXYGEN_HTML_COLORSTYLE LIGHT)
set(DOXYGEN_HTML_EXTRA_STYLESHEET ${AWESOME_CSS_DIR}/doxygen-awesome.css ${AWESOME_CSS_DIR}/doxygen-awesome-sidebar-only.css)
doxygen_add_docs(docs mainpage.md ${CMAKE_SOURCE_DIR}/include ALL)

if (MIXEDBAG_ENABLE_INSTALL)
    install(
        DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )
endif()