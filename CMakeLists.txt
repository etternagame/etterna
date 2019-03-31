# ROOT CMakeLists.txt
cmake_minimum_required(VERSION 3.14.0)
project(Etterna)

# PROJECT WIDE SETUP
## CMake and Compiler Setup
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

## Extern Library Variables
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE) # Disable SQLiteCpp static analysis
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)  # Disable SQLiteCpp linting
set(USE_STATIC_CRT ON)  # Have discord-rpc link statically
set(BUILD_TESTING OFF)  # Disable libuv testing

## Policies
cmake_policy(SET CMP0076 NEW)  # Tell cmake to convert target_sources input from relative path to absolute path

## CMake related variables
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/CMake/Modules)
# set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/Modules") # Tell cmake where to get "FindXXX.cmake" files
set(CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/extern")

# Project Build Targets
## Etterna
add_executable(Etterna)
target_sources(Etterna PUBLIC ${PROJECT_BINARY_DIR}/generated/verstub.cpp)

# TODO: Temp output directories. Switch to out-of-source
set_target_properties(Etterna PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}")

### Source
add_subdirectory(extern)
add_subdirectory(src/Etterna)
add_subdirectory(src/arch)
add_subdirectory(src/archutils)
add_subdirectory(src/RageUtil)

### Includes
target_include_directories(Etterna PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated) # Add gen files include search dirs
target_include_directories(Etterna PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src) # Add src/ to include search dirs

## Library including
### TODO: Once all OS's are building, go through individual scripts and factor out the ones which are needed universally
if(WIN32)
    include(CMake/Helpers/CMakeWindows.cmake)
elseif(APPLE)
    include(CMake/Helpers/SetupFFMPEG.cmake)
    include(CMake/Helpers/CMakeMacOS.cmake)
elseif(UNIX)
    include(${PROJECT_SOURCE_DIR}/CMake/Helpers/CMakeLinux.cmake)
endif()
