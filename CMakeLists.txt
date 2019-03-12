# PREAMBLE
cmake_minimum_required(VERSION 3.13.0)
project(Etterna C CXX)

# PROJECT WIDE SETUP
## Compiler Setup
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

## CMake related variables
set(CMAKE_MODULE_PATH  "${CMAKE_SOURCE_DIR}/CMake/Modules") # Tell cmake where to get "FindXXX.cmake" files

# DEPENDENCIES - Ensure all depepdencies are avaiable for build process
find_package(Mad)

find_package(Ogg)
find_package(Vorbis)
find_package(VorbisFile)

#find_package(nasm)
#find_package(yasm)

find_package(BZip2 REQUIRED)
find_package(Iconv)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if (${Threads_FOUND})
    set(HAS_PTHREAD TRUE)
else()
    set(HAS_PTHREAD FALSE)
endif()

find_package(GTK2 2.0)

find_package(X11)

find_package(Pcre)
set(SYSTEM_PCRE_FOUND ${PCRE_FOUND})



find_package(ZLIB REQUIRED)

find_package(JPEG REQUIRED)

find_package(Dl)

find_package(Xrandr)

find_package(PulseAudio)



find_package(Va)

find_package(CURL REQUIRED)

find_package(OpenGL REQUIRED)

find_package(GLEW REQUIRED)

#find_package(OpenSSL REQUIRED)
#find_package(OpenGL REQUIRED)
#find_package(JPEG REQUIRED)
#find_package(X11 REQUIRED)
#find_package(GLEW REQUIRED)
#find_package(Xrandr)


# Add Dependencies to this project
# MAIN BUILD TARGETS
# Executable
add_executable(Etterna)
#set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS PACKAGE_NAME="Etterna")
#set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS PACKAGE_VERSION="WhyIsThisAThing")
#set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS HAVE_LIBPTHREAD)
#set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS HAVE_GTK)
list(APPEND cdefs
     CMAKE_POWERED
     UNIX
     LINUX
#     CRASH_HANDLER
     BACKTRACE_METHOD_X86_LINUX
     BACKTRACE_METHOD_TEXT="x86 custom backtrace"
     BACKTRACE_LOOKUP_METHOD_TEXT="backtrace_symbols"
     BACKTRACE_LOOKUP_METHOD_DLADDR
#     HAVE_GTK
     HAVE_LIBPTHREAD
     CPU_X86_64)
set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS "${cdefs}")


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
target_link_libraries(Etterna discord-rpc) # Link disord RPC to etterna
target_link_libraries(Etterna sqlite3)   # Link sqlite3 to etterna (Shouldn't SQLiteCpp do this for me?)
target_link_libraries(Etterna SQLiteCpp) # Link SQLiteCpp to etterna
target_link_libraries(Etterna uWS)
target_link_libraries(Etterna jsoncpp) # TODO: Remove legacy json library. Project still depends on it.
target_link_libraries(Etterna nlohmann_json)
target_link_libraries(Etterna tomcrypt)
target_link_libraries(Etterna libtommath)
target_link_libraries(Etterna fftw3f)
target_link_libraries(Etterna MinaCalc)


target_link_libraries(Etterna ${LIBMAD_LIBRARY})
target_link_libraries(Etterna ogg vorbis vorbisfile)
target_link_libraries(Etterna ${BZIP2_LIBRARY_RELEASE})
target_link_libraries(Etterna ${ICONV_LIBRARIES})
target_link_libraries(Etterna Threads::Threads)
target_link_libraries(Etterna ${GTK2_LIBRARIES})
target_link_libraries(Etterna ${X11_LIBRARIES})
target_link_libraries(Etterna pcre)
target_link_libraries(Etterna ${ZLIB_LIBRARIES})
target_link_libraries(Etterna ${JPEG_LIBRARIES})
target_link_libraries(Etterna ${DL_LIBRARIES})
target_link_libraries(Etterna ${XRANDR_LIBRARIES})
target_link_libraries(Etterna ${PULSEAUDIO_LIBRARY})
target_link_libraries(Etterna ${CURL_LIBRARIES})
target_link_libraries(Etterna ${OPENGL_LIBRARIES})
target_link_libraries(Etterna ${GLEW_LIBRARIES})

# TESTS
# PACKAGING