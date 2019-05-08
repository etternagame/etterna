# cppcheck
find_program(CPPCHECK_EXE "cppcheck")

get_target_property(CPPCHECK_SOURCES Etterna SOURCES)
add_custom_target(cppcheck
	COMMENT "Running cppcheck"
	VERBATIM
	COMMAND ${CPPCHECK_EXE}
	--output-file=${PROJECT_BINARY_DIR}/cppcheck.txt
	--language=c++
	--enable=all
	--std=c++14
	--template=[{severity}][{id}][{file}:{line}]\ {message}\ {callstack}
	--verbose
	${CPPCHECK_SOURCES})