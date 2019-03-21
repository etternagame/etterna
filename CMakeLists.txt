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

list(APPEND cdefs _XOPEN_SOURCE CRASH_HANDLER CPU_X86_64)
set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS "${cdefs}")

## Library including
if(WIN32)

elseif(APPLE)
  find_package(CURL)
  find_package(OpenGL)

  # include(CMake/Helpers/SetupFFMPEG.cmake)
  # include(CMake/SetupFfmpeg.cmake)
  # set_target_properties("${SM_EXE_NAME}" PROPERTIES MACOSX_BUNDLE TRUE)
  find_library(MAC_FRAME_ACCELERATE Accelerate ${CMAKE_SYSTEM_FRAMEWORK_PATH})
  find_library(MAC_FRAME_APPKIT AppKit ${CMAKE_SYSTEM_FRAMEWORK_PATH})
  find_library(MAC_FRAME_AUDIOTOOLBOX AudioToolbox)
  find_library(MAC_FRAME_AUDIOUNIT AudioUnit ${CMAKE_SYSTEM_FRAMEWORK_PATH})
  find_library(MAC_FRAME_CARBON Carbon ${CMAKE_SYSTEM_FRAMEWORK_PATH})
  find_library(MAC_FRAME_COCOA Cocoa ${CMAKE_SYSTEM_FRAMEWORK_PATH})
  find_library(MAC_FRAME_COREAUDIO CoreAudio ${CMAKE_SYSTEM_FRAMEWORK_PATH})
  find_library(MAC_FRAME_COREFOUNDATION CoreFoundation ${CMAKE_SYSTEM_FRAMEWORK_PATH})
  find_library(MAC_FRAME_CORESERVICES CoreServices ${CMAKE_SYSTEM_FRAMEWORK_PATH})
  find_library(MAC_FRAME_FOUNDATION Foundation ${CMAKE_SYSTEM_FRAMEWORK_PATH})
  find_library(MAC_FRAME_IOKIT IOKit)
  # find_library(MAC_FRAME_QUICKTIME QuickTime ${CMAKE_SYSTEM_FRAMEWORK_PATH})

  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
  set(CMAKE_OSX_DEPLOYMENT_TARGET_FULL "10.9.0")

  target_link_libraries(Etterna ${MAC_FRAME_ACCELERATE})
  target_link_libraries(Etterna ${MAC_FRAME_APPKIT})
  target_link_libraries(Etterna ${MAC_FRAME_AUDIOTOOLBOX})
  target_link_libraries(Etterna ${MAC_FRAME_AUDIOUNIT})
  target_link_libraries(Etterna ${MAC_FRAME_CARBON})
  target_link_libraries(Etterna ${MAC_FRAME_COCOA})
  target_link_libraries(Etterna ${MAC_FRAME_COREAUDIO})
  target_link_libraries(Etterna ${MAC_FRAME_COREFOUNDATION})
  target_link_libraries(Etterna ${MAC_FRAME_CORESERVICES})
  target_link_libraries(Etterna ${MAC_FRAME_FOUNDATION})
  target_link_libraries(Etterna ${MAC_FRAME_IOKIT})
  # target_link_libraries(Etterna ${MAC_FRAME_QUICKTIME})

  target_link_libraries(Etterna luajit)
  target_link_libraries(Etterna discord-rpc)
  target_link_libraries(Etterna sqlite3)   # Link sqlite3 to etterna
  target_link_libraries(Etterna SQLiteCpp) # Link SQLiteCpp to etterna
  target_link_libraries(Etterna uWS)
  target_link_libraries(Etterna jsoncpp) # TODO: Two JSON Libraries?
  target_link_libraries(Etterna nlohmann_json)
  target_link_libraries(Etterna tomcrypt)
  target_link_libraries(Etterna libtommath)
  target_link_libraries(Etterna fftw3f)
  target_link_libraries(Etterna MinaCalc)
  # target_link_libraries(Etterna Threads::Threads)
  target_link_libraries(Etterna glfw)

  target_link_libraries(Etterna libmad)
  target_link_libraries(Etterna ogg)
  target_link_libraries(Etterna vorbis)
  target_link_libraries(Etterna pcre)
  target_link_libraries(Etterna ffmpeg)

  target_link_libraries(Etterna ${CURL_LIBRARIES})
  target_link_libraries(Etterna ${OPENGL_LIBRARIES})

elseif(UNIX)
  include(${PROJECT_SOURCE_DIR}/CMake/Helpers/CMakeLinux.cmake)
endif()





