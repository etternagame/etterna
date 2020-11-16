list(APPEND CORE_ARCH_HPP Arch.hpp)
list(APPEND CORE_ARCH_CPP Arch.cpp)

if(WIN32)
    list(APPEND CORE_ARCH_CPP ArchWin.cpp)
elseif(APPLE)
    list(APPEND CORE_ARCH_CPP ArchMac.mm)
elseif(UNIX)
    list(APPEND CORE_ARCH_CPP ArchUnix.cpp)
else()
    message(FATAL_ERROR " What operating system is this? There is no ArchHooks defined for this platform.")
endif()

target_sources(Etterna PRIVATE ${CORE_ARCH_HPP} ${CORE_ARCH_CPP})