# Configure AppInfo data
include(${PROJECT_SOURCE_DIR}/CMake/Helpers/SetupAppInfo.cmake)
configure_file(Misc/AppInfo.in.cpp ${PROJECT_BINARY_DIR}/generated/AppInfo.cpp)
target_sources(Etterna PUBLIC ${PROJECT_BINARY_DIR}/generated/AppInfo.cpp)

list(APPEND CORE_HPP
        Crash/CrashpadHandler.hpp
        Misc/PlogLogger.hpp
        Services/Locator.hpp)
list(APPEND CORE_CPP
        Services/Locator.cpp
        Misc/PlogLogger.cpp)

add_subdirectory(Platform)

if(WITH_CRASHPAD)
	list(APPEND CORE_CPP Crash/CrashpadHandler.cpp)
else()
	list(APPEND CORE_CPP Crash/CrashpadHandlerNull.cpp)
endif()

target_sources(Etterna PRIVATE ${CORE_HPP} ${CORE_CPP})