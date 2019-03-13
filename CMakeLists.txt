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

# DEPENDENCIES - Ensure all dependencies are available for the build process
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(LINUX TRUE)
else()
	set(LINUX FALSE)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(MACOSX TRUE)
else()
	set(MACOSX FALSE)
endif()

# ALL OPERATING SYSTEMS
#find_package(nasm)
#find_package(yasm)
find_package(BZip2 REQUIRED)
find_package(Iconv)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
# TODO: This difference between being able to compile, and not being able to compile is THIS IF BLOCK. Find out why.
# REASON WHY:
if (${Threads_FOUND})
	set(HAS_PTHREAD TRUE)
else()
	set(HAS_PTHREAD FALSE)
endif()

if(WIN32) # WINDOWS ONLY
	# ASSUME HAS MP3
	# ASSUME HAS OGG
	find_package(DirectX REQUIRED)


elseif(APPLE) # OSX ONLY
	# ASSUME HAS MP3
	# ASSUME HAS OGG

	if(WITH_FFMPEG)
		include("${SM_CMAKE_DIR}/SetupFfmpeg.cmake")
		set(HAS_FFMPEG TRUE)
	endif()

	find_package(CURL REQUIRED) # ALSO WITH LINUX

elseif(LINUX) # LINUX ONLY
	# SEARCH MP3
	find_package(Mad)
	set(HAS_MP3 TRUE)

	# Search OGG
	find_package(Ogg)
	find_package(Vorbis)
	find_package(VorbisFile)
	set(HAS_OGG TRUE)

	find_package(GTK2 2.0)
	set(HAVE_GTK2 TRUE)

	find_package(X11)

	find_package(PCRE)

	find_package(ZLIB REQUIRED)
	find_package(JPEG REQUIRED)

	find_package(DLFCN REQUIRED)

	find_package(Xrandr)

	find_package(PulseAudio)
	set(HAS_PULSE TRUE)

	find_package(ALSA)
	set(HAS_ALSA TRUE)

	find_package(JACK)
	set(HAS_JACK TRUE)

	find_package(OSS)


	find_package(VideoAcceleration)
#	include(CMake/SetupFfmpeg.cmake)
#	find_package(FFMPEG)

	set(HAS_FFMPEG TRUE)
	find_package(CURL REQUIRED)  # ALSO WITH MAC
	find_package(OpenGL REQUIRED)
	find_package(GLEW REQUIRED)
endif()



list(APPEND cdefs
     "CMAKE_POWERED"
     "UNIX"
     "LINUX"
#     "CRASH_HANDLER"
     "BACKTRACE_METHOD_X86_LINUX"
     "BACKTRACE_METHOD_TEXT=\"x86 custom backtrace\""
     "BACKTRACE_LOOKUP_METHOD_TEXT=\"backtrace_symbols\""
     "BACKTRACE_LOOKUP_METHOD_DLADDR"
     #"HAVE_GTK"
     HAVE_LIBPTHREAD
     CPU_X86_64
     PACKAGE_NAME="Etterna"
     PACKAGE_VERSION="WhyIsThisAThing"
     HAVE_X11
     HAS_MP3
     HAS_WAV
     HAS_OGG
     HAS_FFMPEG)

# MAIN BUILD TARGETS
# Executable
add_executable(Etterna)
## TODO: Remove needs to compile this program out of source
set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS "${cdefs}")
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

# Linking
target_link_libraries(Etterna Threads::Threads)
target_link_libraries(Etterna luajit) # Link lua to etterna
target_link_libraries(Etterna discord-rpc) # Link disord RPC to etterna
target_link_libraries(Etterna sqlite3)   # Link sqlite3 to etterna (Shouldn't SQLiteCpp do this for me?)
target_link_libraries(Etterna SQLiteCpp) # Link SQLiteCpp to etterna
target_link_libraries(Etterna uWS)
target_link_libraries(Etterna jsoncpp) # TODO: Two JSON Libraries?
target_link_libraries(Etterna nlohmann_json)
target_link_libraries(Etterna tomcrypt)
target_link_libraries(Etterna libtommath)
target_link_libraries(Etterna fftw3f)
target_link_libraries(Etterna MinaCalc)
target_link_libraries(Etterna ffmpeg)

target_link_libraries(Etterna ${BZIP2_LIBRARIES})
target_link_libraries(Etterna ${ICONV_LIBRARIES})
target_link_libraries(Etterna ${LIBMAD_LIBRARY})
target_link_libraries(Etterna ${OGG_LIBRARY} ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY})
target_link_libraries(Etterna ${GTK2_LIBRARIES})
target_link_libraries(Etterna ${X11_LIBRARIES})
target_link_libraries(Etterna ${PCRE_LIBRARIES})
target_link_libraries(Etterna ${ZLIB_LIBRARIES})
target_link_libraries(Etterna ${JPEG_LIBRARIES})
target_link_libraries(Etterna ${DLFCN_LIBRARIES})
target_link_libraries(Etterna ${XRANDR_LIBRARIES})

target_link_libraries(Etterna ${PULSEAUDIO_LIBRARIES})
target_link_libraries(Etterna ${ALSA_LIBRARIES})
target_link_libraries(Etterna ${JACK_LIBRARIES})
target_link_libraries(Etterna ${VA_LIBRARIES})

target_link_libraries(Etterna ${CURL_LIBRARIES})
target_link_libraries(Etterna ${OPENGL_LIBRARY})
target_link_libraries(Etterna ${GLEW_LIBRARIES})
