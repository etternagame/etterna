# cppcheck
find_program(CPPCHECK_EXE "cppcheck")
if(NOT CPPCHECK_EXE)
	message(STATUS "cppcheck not found. cppcheck target will not be created.")
else()
	get_target_property(SOURCES Etterna SOURCES)
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
		${SOURCES})
endif()

# clang-tidy
find_program(CLANG_TIDY "clang-tidy")
if(NOT CLANG_TIDY)
	message(STATUS "clang-tidy not found. clang-tidy target will not be created.")
else()
	add_custom_target(clang-tidy
		COMMAND ${CLANG_TIDY}
		-p ${PROJECT_BINARY_DIR}
		-checks='*'
		${SOURCES})
endif()
