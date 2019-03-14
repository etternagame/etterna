# ROOT CMakeLists.txt
cmake_minimum_required(VERSION 3.13.0)
project(Etterna)

# PROJECT WIDE SETUP
## Compiler Setup
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

## System Setup
#include(${PROJECT_SOURCE_DIR}/CMake/Helpers/10DetectOperatingSystem.cmake)

## CMake related variables
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/Modules") # Tell cmake where to get "FindXXX.cmake" files
set(CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/extern")


# MAIN BUILD TARGETS
# Executable
add_executable(Etterna)

# Source
add_subdirectory(extern)
add_subdirectory(src/Etterna)
#add_subdirectory(src/arch)
#add_subdirectory(src/archutils)
#add_subdirectory(src/RageUtil)

# Includes
target_include_directories(Etterna PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated) # Add gen files include search dirs
target_include_directories(Etterna PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src") # Add src/ to include search dirs


# Linking
target_link_libraries(Etterna luajit) # Link lua to etterna
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
#target_link_libraries(Etterna ffmpeg)