# ROOT CMakeLists.txt
cmake_minimum_required(VERSION 3.15.0)

# Policies
cmake_policy(SET CMP0076 NEW)  # Tell cmake to convert target_sources input from relative path to absolute path
cmake_policy(SET CMP0077 NEW)  # Tell cmake to allow variables set through "set" and "option" to be compatible with each other
cmake_policy(SET CMP0048 NEW)  # Tell cmake to use VERSION parameter in project() for all PROJECT_VERSION_* variables
cmake_policy(SET CMP0092 NEW)  # Tell cmake not to include warning parameters by default in CMAKE_<LANG>_FLAGS variables

## Default Policies
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW) # Tell CMake to use the new policy for CMP0048. (Due to zlib not defining it)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW) # Tell CMake to use the new policy for CMP0077. (Due to fftw not defining it)

# Options
## We statically link the CRT by default. If you would like to dynamically
## link the CRT, you can append "DLL" to the end of the variable below.
## More Info: https://cmake.org/cmake/help/latest/variable/CMAKE_MSVC_RUNTIME_LIBRARY.html
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# PROJECT WIDE SETUP
project(Etterna
		VERSION 0.72.3
        HOMEPAGE_URL https://github.com/etternagame/etterna/
        LANGUAGES C CXX ASM)

## CMake and Compiler Setup
set(CMAKE_CXX_STANDARD 20)                                      # Minimum C++ Version
set(CMAKE_CXX_EXTENSIONS OFF)                                   # True if compiler extensions are necessary. (Changes -std flag)
set(CMAKE_CXX_STANDARD_REQUIRED ON)                             # True to require minimum C++ version to compile
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10)                          # Tell xcodebuild to target an older osx platform
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)                           # Export compile commands for clang-tidy
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/CMake/Modules)  # Tell CMake where to access FindXXX.cmake files
set_property(GLOBAL PROPERTY USE_FOLDERS ON)                    # Enable folders/filters within IDEs

# Project Build Targets
add_executable(Etterna)

# Project Compile Options
set(WITH_CRASHPAD TRUE CACHE BOOL "Compile with Crash Handler (Requires depot_tools installed)")

## Setting Target Properties
### Set a different name for each output binary depending on what build configuration is.
### Usually it is separated by directory, but since we have the same directory for every
### binary, we need to rename the binary. We don't rename if we are compiling on CI,
### as we want the executable to be "Etterna" regardless of which version is deployed.
if(NOT DEFINED ENV{CI})
    set_target_properties(Etterna PROPERTIES
            RUNTIME_OUTPUT_NAME_DEBUG "Etterna-debug"
            RUNTIME_OUTPUT_NAME_RELEASE "Etterna"
            RUNTIME_OUTPUT_NAME_MINSIZEREL "Etterna-MinSizeRelease"
            RUNTIME_OUTPUT_NAME_RELWITHDEBINFO "Etterna-RelWithDebInfo")
endif()

### macOS and Linux place binary in root directory
if(NOT WIN32)
    set_target_properties(Etterna PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}>")
endif()

## Includes
target_include_directories(Etterna PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)   # Add gen files include search dirs
target_include_directories(Etterna PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)          # Add src/ to include search dirs

## Package Includes
### OpenSSL is not used directly by our program, but we have to use OpenSSL first
### in order to statically link. Once find_package is run once, it's results are cached
if (WIN32 OR APPLE)
    set(OPENSSL_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
endif()
set(OPENSSL_MSVC_STATIC_RT ON CACHE BOOL "" FORCE)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(Etterna PRIVATE Threads::Threads)
target_link_libraries(Etterna PRIVATE OpenSSL::SSL)
if(WIN32)
    # These libraries are all included as the openssl instructions state they are needed for static linking.
    # The source code does include some of these through "#pragma comment(lib, LIB)" though to keep it linking
    # throughout the changes it may receive, we will include it here as-well.
    target_link_libraries(Etterna PRIVATE CRYPT32.LIB)
    target_link_libraries(Etterna PRIVATE WS2_32.LIB)
    target_link_libraries(Etterna PRIVATE GDI32.LIB)
    target_link_libraries(Etterna PRIVATE ADVAPI32.LIB)
    target_link_libraries(Etterna PRIVATE USER32.LIB)
endif()

# Load external libraries
add_subdirectory(extern EXCLUDE_FROM_ALL) # EXCLUDE_FROM_ALL to exclude from cpack binary

## Linking
target_link_libraries(Etterna PRIVATE SQLiteCpp sqlite3)
target_link_libraries(Etterna PRIVATE zlib)
target_link_libraries(Etterna PRIVATE rapidjson)
target_link_libraries(Etterna PRIVATE websocketpp)
target_link_libraries(Etterna PRIVATE libluajit)
target_link_libraries(Etterna PRIVATE discord-rpc)
target_link_libraries(Etterna PRIVATE muFFT)
target_link_libraries(Etterna PRIVATE glfw)
target_link_libraries(Etterna PRIVATE ogg)
target_link_libraries(Etterna PRIVATE vorbis)
target_link_libraries(Etterna PRIVATE pcre)
target_link_libraries(Etterna PRIVATE libmad)
target_link_libraries(Etterna PRIVATE stb)
target_link_libraries(Etterna PRIVATE libcurl)
target_link_libraries(Etterna PRIVATE fmt::fmt)
target_link_libraries(Etterna PRIVATE plog::plog)
target_link_libraries(Etterna PRIVATE nowide::nowide)
target_link_libraries(Etterna PRIVATE ghc_filesystem)

# If the user wants crashpad, and the target exists (in-case
# the user wants it, but crashpad couldn't find python)
if(WITH_CRASHPAD AND TARGET crashpad)
    target_link_libraries(Etterna PRIVATE crashpad)
endif()

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
add_subdirectory(src/Core)

## The source_group line creates the full visual studio filter layout
get_target_property(sources Etterna SOURCES)
source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX "Etterna" FILES ${sources})

# Compile Definitions
if(DEFINED ENV{CI})
    target_compile_definitions(Etterna PRIVATE ALLOW_CRASH_UPLOAD)
endif()

# Static Analysis
include(CMake/Helpers/StaticAnalysis.cmake)

# Documentation
include(CMake/Helpers/DocumentationTools.cmake)

# CPack Initialization
include(CMake/Helpers/CPackSetup.cmake)
include(CPack)
