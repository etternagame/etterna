# ROOT CMakeLists.txt
cmake_minimum_required(VERSION 3.14.0)

# Policies
cmake_policy(SET CMP0076 NEW)  # Tell cmake to convert target_sources input from relative path to absolute path
cmake_policy(SET CMP0077 NEW)  # Tell cmake to allow variables set through "set" and "option" to be compatible with each other
cmake_policy(SET CMP0048 NEW)  # Tell cmake to use VERSION parameter in project() for all PROJECT_VERSION_* variables

# Options
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/CMake/Helpers/CompileOptions.cmake")
option(OPENSSL_ROOT_DIR "OpenSSL Root Directory" "")

# PROJECT WIDE SETUP
project(Etterna
		VERSION 0.69.0
        HOMEPAGE_URL https://github.com/etternagame/etterna/
        LANGUAGES C CXX ASM)

## CMake and Compiler Setup
set(CMAKE_CXX_STANDARD 17)                                  # Minimum C++ Version
set(CMAKE_CXX_EXTENSIONS OFF)                               # True if compiler extensions are necessary. (Changes -std flag)
set(CMAKE_CXX_STANDARD_REQUIRED ON)                         # True to require minimum C++ version to compile
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10)                      # Tell xcodebuild to target an older osx platform
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)                       # Export compile commands for clang-tidy
set(CMAKE_CONFIGURATION_TYPES "Release;RelWithDebInfo")     # What configurations types do we want to support? (Changes what shows up in Visual Studio)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)                # Enable folders/filters within IDE's

## Ninja specific setup (Different compilers required different color force commands)
if(CMAKE_GENERATOR STREQUAL "Ninja")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options("-fcolor-diagnostics")
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options("-fdiagnostics-color=always")
    endif()
endif()

## Extern Library Variables
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE) # Disable SQLiteCpp static analysis
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)  # Disable SQLiteCpp linting
set(CLANG_FORMAT_CMD OFF CACHE BOOL "" FORCE)       # Disable discord clang-format
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)         # Disable discord examples
set(USE_STATIC_CRT ON CACHE BOOL "" FORCE)          # Have discord-rpc link statically

## CMake related variables
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/CMake/Modules)  # Tell CMake where to access FindXXX.cmake files

# Project Build Targets
add_executable(Etterna)

# Load external libraries
add_subdirectory(extern EXCLUDE_FROM_ALL) # EXCLUDE_FROM_ALL to exclude from cpack binary

## Setting Target Properties
### TODO: Temp output directories. Switch to out-of-source
set_target_properties(Etterna PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}")

if(NOT APPLE)
    set_target_properties(Etterna PROPERTIES LINK_WHAT_YOU_USE ON)
endif()

### This single source_group line creates the full visual studio filter layout
get_target_property(sources Etterna SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Etterna" FILES ${sources})

## Includes
target_include_directories(Etterna PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)   # Add gen files include search dirs
target_include_directories(Etterna PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)          # Add src/ to include search dirs

## Package Includes
find_package(Threads REQUIRED)
target_link_libraries(Etterna Threads::Threads)

## Linking
target_link_libraries(Etterna zlib)
target_link_libraries(Etterna rapidjson)
target_link_libraries(Etterna websocketpp)
target_link_libraries(Etterna luajit)
target_link_libraries(Etterna discord-rpc)
target_link_libraries(Etterna sqlite3)
target_link_libraries(Etterna SQLiteCpp)
target_link_libraries(Etterna tomcrypt)
target_link_libraries(Etterna libtommath)
target_link_libraries(Etterna fftw3f)
target_link_libraries(Etterna glfw)
target_link_libraries(Etterna ogg)
target_link_libraries(Etterna vorbis)
target_link_libraries(Etterna pcre)
target_link_libraries(Etterna libmad)
target_link_libraries(Etterna stb)

# OS Specific Initialization
if(WIN32)
    include(CMake/Helpers/CMakeWindows.cmake)
elseif(APPLE)
    include(CMake/Helpers/SetupFFMPEG.cmake)
    include(CMake/Helpers/CMakeMacOS.cmake)
elseif(UNIX)
    include(CMake/Helpers/CMakeLinux.cmake)
endif()

## Source - Add source to the Etterna target
add_subdirectory(src/Etterna)
add_subdirectory(src/arch)
add_subdirectory(src/archutils)
add_subdirectory(src/RageUtil)

# Static Analysis
include(CMake/Helpers/StaticAnalysis.cmake)

# Documentation
include(CMake/Helpers/DocumentationTools.cmake)

# CPack Initialization
include(CMake/Helpers/CPackSetup.cmake)
include(CPack)
