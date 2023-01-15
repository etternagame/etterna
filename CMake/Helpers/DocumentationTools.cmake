include(FetchContent)

set(DOCS_OUTPUT_DIR ${PROJECT_BINARY_DIR}/_DOCS)

# doxygen
find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(NOT DOXYGEN_FOUND)
    message(STATUS "Doxygen not found. Doxygen target will not be created. Please ensure doxygen is accessibe within your path.")
else()
    # Download Doxygen Theme
    FetchContent_Declare(doxygen_theme
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_TAG	main
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(doxygen_theme)
    FetchContent_GetProperties(doxygen_theme SOURCE_DIR DOXY_THEME_DIR)

    # set input and output files
    set(DOXYGEN_IN ${PROJECT_SOURCE_DIR}/Docs/Doxyfile.in)
    set(DOXYGEN_OUT ${PROJECT_BINARY_DIR}/Doxyfile)

    # configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    # note the option ALL which allows to build the docs together with the application
    add_custom_target(doxygen 
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
endif()

# LDoc
find_program(LDOC_EXE NAMES "ldoc" "ldoc.bat")
if(NOT LDOC_EXE)
    message(STATUS "LDoc not found. LDoc target will not be created. Please ensure ldoc is accessible within your path.")
else()
    # set input and output files
    set(LDOC_IN ${PROJECT_SOURCE_DIR}/Docs/LDoc.in)
    set(LDOC_OUT ${PROJECT_BINARY_DIR}/config.ld)

    # configure the file
    configure_file(${LDOC_IN} ${LDOC_OUT} @ONLY)

    # note the option ALL which allows to build the docs together with the application
    add_custom_target(ldoc 
        COMMENT "Generating Lua documentation with LDoc"
        VERBATIM
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMAND ${LDOC_EXE} .)
        
endif()