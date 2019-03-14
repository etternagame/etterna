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
target_sources(Etterna PUBLIC ${PROJECT_BINARY_DIR}/generated/verstub.cpp
               
               )
list(APPEND cdefs CPU_X86_64 HAVE_LIBPTHREAD
     "BACKTRACE_METHOD_X86_LINUX"
     "BACKTRACE_METHOD_TEXT=\"x86 custom backtrace\""
     "BACKTRACE_LOOKUP_METHOD_TEXT=\"backtrace_symbols\""
     "BACKTRACE_LOOKUP_METHOD_DLADDR"
     PACKAGE_NAME="Etterna"
     PACKAGE_VERSION="WhyIsThisAThing")
set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS "${cdefs}")
set_target_properties(Etterna PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}")


## Library including
### Universal Includes first
find_package(Threads)

if(WIN32)

elseif(APPLE)

elseif(UNIX)
  find_package(Mad)
  find_package(Ogg)
  find_package(Vorbis)
  find_package(VorbisFile)

  find_package(X11 REQUIRED)
  find_package(PCRE)
  find_package(ZLIB REQUIRED)
  find_package(JPEG REQUIRED)
  
  find_package(DLFCN REQUIRED)
  find_package(Xrandr)
  find_package(PulseAudio REQUIRED)
  find_package(ALSA)
  find_package(JACK)
  find_package(OSS)

  find_package(CURL REQUIRED)  # ALSO WITH MAC
  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
  
endif()

# Source
add_subdirectory(extern)
add_subdirectory(src/Etterna)
add_subdirectory(src/arch)
add_subdirectory(src/archutils)
add_subdirectory(src/RageUtil)

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
target_link_libraries(Etterna Threads::Threads)
#target_link_libraries(Etterna ffmpeg)

#target_link_libraries(Etterna ${DL_LIBRARIES})

target_link_libraries(Etterna ${BZIP2_LIBRARIES})
target_link_libraries(Etterna ${ICONV_LIBRARIES})
target_link_libraries(Etterna ${LIBMAD_LIBRARY})
target_link_libraries(Etterna ${OGG_LIBRARY} ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY})
#target_link_libraries(Etterna ${GTK2_LIBRARIES})
target_link_libraries(Etterna ${X11_LIBRARIES})
target_link_libraries(Etterna ${PCRE_LIBRARY})
target_link_libraries(Etterna ${ZLIB_LIBRARIES})
target_link_libraries(Etterna ${JPEG_LIBRARIES})
target_link_libraries(Etterna ${DL_LIBRARIES})
target_link_libraries(Etterna ${XRANDR_LIBRARIES})

target_link_libraries(Etterna ${PULSEAUDIO_LIBRARIES})
target_link_libraries(Etterna ${ALSA_LIBRARIES})
target_link_libraries(Etterna ${JACK_LIBRARIES})
target_link_libraries(Etterna ${VA_LIBRARIES})

target_link_libraries(Etterna ${CURL_LIBRARIES})
target_link_libraries(Etterna ${OPENGL_LIBRARY})
target_link_libraries(Etterna ${GLEW_LIBRARIES})