find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(NOT DOXYGEN_FOUND)
    message(WARNING "Doxygen not found. Documentation target will not be created.")
else()
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