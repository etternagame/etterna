# Get proper CRASHPAD_HANDLER_EXE
if(NOT DEFINED ENV{CI})
    set(CRASHPAD_HANDLER_EXE ${PROJECT_BINARY_DIR}/gn_crashpad/crashpad_handler)
else()
    set(CRASHPAD_HANDLER_EXE crashpad_handler)
endif()

if(WIN32)
    string(APPEND CRASHPAD_HANDLER_EXE .exe)
endif()

execute_process(
        COMMAND git describe --tags --dirty
        OUTPUT_VARIABLE PROJECT_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
