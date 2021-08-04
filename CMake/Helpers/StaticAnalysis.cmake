# Get sources
get_target_property(SOURCES Etterna SOURCES)
list(FILTER SOURCES EXCLUDE REGEX ".icns$")

# cppcheck
find_program(CPPCHECK_EXE "cppcheck")
if(NOT CPPCHECK_EXE)
	message(STATUS "cppcheck not found. cppcheck target will not be created.")
else()
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

# clang-format
find_program(CLANG_FORMAT "clang-format")
if(NOT CLANG_FORMAT)
	message(STATUS "clang-format not found. clang-format target will not be created.")
else()
	add_custom_target(format
		COMMAND ${CLANG_FORMAT}
		-i
		--style=file
		-verbose
		${SOURCES})
endif()