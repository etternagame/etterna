# ROOT CMakeLists.txt
cmake_minimum_required(VERSION 3.12.0)
project(Etterna)

# PROJECT WIDE SETUP
## Compiler Setup
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

## Policies
cmake_policy(SET CMP0076 NEW)  # Tell cmake to convert target_sources input from relative path to absolute path

## System Setup
#include(${PROJECT_SOURCE_DIR}/CMake/Helpers/10DetectOperatingSystem.cmake)

## CMake related variables
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/CMake/Modules)
# set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/Modules") # Tell cmake where to get "FindXXX.cmake" files
set(CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/extern")

# MAIN BUILD TARGETS
# Executable
add_executable(Etterna)
target_sources(Etterna PUBLIC ${PROJECT_BINARY_DIR}/generated/verstub.cpp)
## TODO: Remove line below when building in source is no longer necessary
set_target_properties(Etterna PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}")

# Source
add_subdirectory(extern)
add_subdirectory(src/Etterna)
add_subdirectory(src/arch)
add_subdirectory(src/archutils)
add_subdirectory(src/RageUtil)

# Includes
target_include_directories(Etterna PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated) # Add gen files include search dirs
target_include_directories(Etterna PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src") # Add src/ to include search dirs

# TODO: Remove CPU_X86_64, CPU_X86, and CRASH_HANDLER
#       CRASH_HANDLER is unnecessary as the game should have that as an option component
#       CPU_X86_64, CPU_X86 already exists as compiler predefined macros. Use those instead.
list(APPEND cdefs _XOPEN_SOURCE CRASH_HANDLER CPU_X86_64)
set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS "${cdefs}")

## Library including
### TODO: Once all OS's are building, go through individual scripts and factor out the ones which are needed universally
if(WIN32)

elseif(APPLE)
  include(CMake/Helpers/SetupFFMPEG.cmake)
  include(CMake/Helpers/CMakeMacOS.cmake)
elseif(UNIX)
  include(${PROJECT_SOURCE_DIR}/CMake/Helpers/CMakeLinux.cmake)
endif()





