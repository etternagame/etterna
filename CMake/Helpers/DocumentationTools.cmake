
# doxygen
find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(NOT DOXYGEN_FOUND)
    message(WARNING "Doxygen not found. Documentation target will not be created.")
else()
    include(FetchContent)
    # m.css theme
    FetchContent_Declare(m.css GIT_REPOSITORY https://github.com/mosra/m.css)
    FetchContent_MakeAvailable(m.css)
    FetchContent_GetProperties(m.css SOURCE_DIR MCSS_SOURCE)
    set(DOXYGEN_DIR ${PROJECT_SOURCE_DIR}/Docs/Doxygen)

    # configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    # note the option ALL which allows to build the docs together with the application
    add_custom_target(doxygen 
        COMMAND ${MCSS_SOURCE}/documentation/doxygen.py ${PROJECT_BINARY_DIR}/conf.py
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
endif()

# LDoc
find_program(LDOC_EXE "ldoc")
if(NOT LDOC_EXE)
    message(WARNING "LDoc not found. Documentation target will not be created.")
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