list(APPEND CORE_PLATFORM_HPP Platform.hpp)
list(APPEND CORE_PLATFORM_CPP Platform.cpp)

if(WIN32)
    list(APPEND CORE_PLATFORM_CPP PlatformWin.cpp)
elseif(APPLE)
    list(APPEND CORE_PLATFORM_CPP PlatformMac.mm)
elseif(UNIX)
    list(APPEND CORE_PLATFORM_CPP PlatformUnix.cpp)
else()
    message(FATAL_ERROR " What operating system is this? There is no platform source file defined.")
endif()

target_sources(Etterna PRIVATE ${CORE_PLATFORM_HPP} ${CORE_PLATFORM_CPP})