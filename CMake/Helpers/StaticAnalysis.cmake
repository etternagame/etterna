# cppcheck
find_program(CPPCHECK_EXE "cppcheck")
get_target_property(CPPCHECK_SOURCES Etterna SOURCES)
add_custom_target(cppcheck ALL
        COMMAND ${CPPCHECK_EXE}
        --output-file=${PROJECT_BINARY_DIR}/cppcheck.txt
        -I${PROJECT_SOURCE_DIR}/src
        -I${PROJECT_BINARY_DIR}/generated
        --enable=all #warning,performance,portability,information,missingInclude
        --std=c++14
        --template=[{severity}][{id}]\ {message}\ {callstack}\ \(On\ {file}:{line}\)
        --verbose
        ${CPPCHECK_SOURCES}
        VERBATIM)