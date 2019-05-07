# cppcheck
find_program(CPPCHECK_EXE "cppcheck")

if(${CPPCHECK_EXE})
	get_target_property(CPPCHECK_SOURCES Etterna SOURCES)
	add_custom_target(cppcheck
		COMMENT "Running cppcheck"
		VERBATIM
		COMMAND ${CPPCHECK_EXE}
		--output-file=${PROJECT_BINARY_DIR}/cppcheck.txt
		-I${PROJECT_SOURCE_DIR}/src
		-I${PROJECT_BINARY_DIR}/generated
		--enable=all
		--std=c++14
		--template=[{severity}][{id}]\ {message}\ {callstack}\ \(On\ {file}:{line}\)
		--verbose
		${CPPCHECK_SOURCES})
endif()