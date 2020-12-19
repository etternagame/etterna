# Add AppInfo metadata file
execute_process(COMMAND git describe --tags --dirty OUTPUT_VARIABLE PROJECT_GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(Misc/AppInfo.in.cpp ${PROJECT_BINARY_DIR}/generated/AppInfo.cpp)
target_sources(Etterna PUBLIC ${PROJECT_BINARY_DIR}/generated/AppInfo.cpp)

list(APPEND CORE_HPP
        Crash/CrashpadHandler.hpp
        Misc/PlogLogger.hpp
        Services/Locator.hpp)
list(APPEND CORE_CPP
        Services/ILogger.cpp
        Services/Locator.cpp
        Misc/PlogLogger.cpp)

if(WITH_CRASHPAD)
	list(APPEND CORE_CPP Crash/CrashpadHandler.cpp)
else()
	list(APPEND CORE_CPP Crash/CrashpadHandlerNull.cpp)
endif()

target_sources(Etterna PRIVATE ${CORE_HPP} ${CORE_CPP})