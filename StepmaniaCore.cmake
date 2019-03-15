# Include the macros and functions.
cmake_minimum_required(VERSION 3.6)
include(${CMAKE_CURRENT_LIST_DIR}/CMake/CMakeMacros.cmake)

# Set up helper variables for future configuring.
set(SM_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/CMake")
set(SM_EXTERN_DIR "${CMAKE_CURRENT_LIST_DIR}/extern")
set(SM_INSTALLER_DIR "${CMAKE_CURRENT_LIST_DIR}/Installer")
set(SM_XCODE_DIR "${CMAKE_CURRENT_LIST_DIR}/Xcode")
set(SM_PROGRAM_DIR "${CMAKE_CURRENT_LIST_DIR}/Program")
set(SM_BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/Build")
set(SM_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
set(SM_DOC_DIR "${CMAKE_CURRENT_LIST_DIR}/Docs")
set(SM_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

# TODO: Reconsile the OS dependent naming scheme.
set(SM_EXE_NAME "Etterna")

# Some OS specific helpers.
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LINUX TRUE)
else()
    set(LINUX FALSE)
endif()

macro(set_win10_flag)
    if(WIN32 AND (CMAKE_SYSTEM_VERSION GREATER 10.0 OR CMAKE_SYSTEM_VERSION EQUAL 10.0))
        add_definitions(-DWIN10)
    endif()
endmacro()
set_win10_flag()

# Allow for finding our libraries in a standard location.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}" "${SM_CMAKE_DIR}/Modules/")

include("${SM_CMAKE_DIR}/DefineOptions.cmake")

include("${SM_CMAKE_DIR}/SMDefs.cmake")

# Put the predefined targets in separate groups.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Enforce the highest C++ standard used in the code base
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set up the linker flags for MSVC builds.
configure_msvc_runtime()

# Checks the standard include directories for c-style headers.
# We may use C++ in this project, but the check works better with plain C headers.
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)

# Mostly Windows functions.
check_function_exists(_mkdir HAVE__MKDIR)
check_cxx_symbol_exists(_snprintf cstdio HAVE__SNPRINTF)
check_cxx_symbol_exists(stricmp cstring HAVE_STRICMP)
check_cxx_symbol_exists(_stricmp cstring HAVE__STRICMP)

# Mostly non-Windows functions.
check_function_exists(mkdir HAVE_MKDIR)
check_cxx_symbol_exists(snprintf cstdio HAVE_SNPRINTF)
check_cxx_symbol_exists(strcasecmp cstring HAVE_STRCASECMP)

if(MINGW)
    set(NEED_WINDOWS_LOADING_WINDOW TRUE)
    check_symbol_exists(PBS_MARQUEE commctrl.h HAVE_PBS_MARQUEE)
    check_symbol_exists(PBM_SETMARQUEE commctrl.h HAVE_PBM_SETMARQUEE)
endif()


# Dependencies go here.
include(ExternalProject)

find_package(nasm)
find_package(yasm)
find_package(Iconv)

if(NOT WIN32)
	find_package(Threads)
	if(${Threads_FOUND})
		set(HAS_PTHREAD TRUE)
		list(APPEND CMAKE_REQUIRED_LIBRARIES pthread)
		check_symbol_exists(pthread_mutex_timedlock pthread.h HAVE_PTHREAD_MUTEX_TIMEDLOCK)
		check_symbol_exists(pthread_cond_timedwait pthread.h HAVE_PTHREAD_COND_TIMEDWAIT)
	else()
		set(HAS_PTHREAD FALSE)
	endif()
endif()

if(WIN32)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "Etterna")
    set(SYSTEM_PCRE_FOUND FALSE)
    find_package(DirectX REQUIRED)
    if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64|amd64)")
        link_libraries(${SM_EXTERN_DIR}/MinaCalc/MinaCalc.lib)
        find_library(LIB_CURL NAMES "libcurl" PATHS "${SM_EXTERN_DIR}/libcurl/64bit" NO_DEFAULT_PATH)
        get_filename_component(LIB_CURL ${LIB_CURL} NAME)
        find_library(LIB_UWS NAMES "uWS" PATHS "${SM_EXTERN_DIR}/uWebSocket/lib/64bit" NO_DEFAULT_PATH)
        find_library(LIB_UV NAMES "libuv" PATHS "${SM_EXTERN_DIR}/uWebSocket/lib/64bit" NO_DEFAULT_PATH)
        find_library(LIB_SSL NAMES "ssleay32" PATHS "${SM_EXTERN_DIR}/uWebSocket/lib/64bit" NO_DEFAULT_PATH)
        find_library(LIB_EAY NAMES "libeay32" PATHS "${SM_EXTERN_DIR}/uWebSocket/lib/64bit" NO_DEFAULT_PATH)
        link_libraries(${SM_EXTERN_DIR}/discord-rpc-2.0.1/lib/discord-rpc.lib)
    else()
        link_libraries(${SM_EXTERN_DIR}/MinaCalc/MinaCalc_x86.lib)
        find_library(LIB_CURL NAMES "libcurl_x86" PATHS "${SM_EXTERN_DIR}/libcurl" NO_DEFAULT_PATH)
        get_filename_component(LIB_CURL ${LIB_CURL} NAME)
        link_libraries(${SM_EXTERN_DIR}/uWebSocket/lib/x86/uWS.lib)
        link_libraries(${SM_EXTERN_DIR}/uWebSocket/lib/x86/libeay32.lib)
        link_libraries(${SM_EXTERN_DIR}/uWebSocket/lib/x86/ssleay32.lib)
        link_libraries(${SM_EXTERN_DIR}/uWebSocket/lib/x86/libuv.lib)
        link_libraries(${SM_EXTERN_DIR}/discord-rpc-2.0.1/lib/discord-rpc_x86.lib)
    endif()
    include_directories(${SM_EXTERN_DIR}/uWebSocket/include)
    include_directories(${SM_EXTERN_DIR}/uWebSocket/includelibs)
    include_directories(${SM_EXTERN_DIR}/discord-rpc-2.0.1/include)
    
    if(MINGW AND WITH_FFMPEG)
        include("${SM_CMAKE_DIR}/SetupFfmpeg.cmake")
        set(HAS_FFMPEG TRUE)
    else()
        # FFMPEG...it can be evil.
        if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64|amd64)")
            find_library(LIB_SWSCALE NAMES "swscale" PATHS "${SM_EXTERN_DIR}/ffmpeg/64bit" NO_DEFAULT_PATH)
            get_filename_component(LIB_SWSCALE ${LIB_SWSCALE} NAME)
            
            find_library(LIB_AVCODEC NAMES "avcodec" PATHS "${SM_EXTERN_DIR}/ffmpeg/64bit" NO_DEFAULT_PATH)
            get_filename_component(LIB_AVCODEC ${LIB_AVCODEC} NAME)
            
            find_library(LIB_AVFORMAT NAMES "avformat" PATHS "${SM_EXTERN_DIR}/ffmpeg/64bit" NO_DEFAULT_PATH)
            get_filename_component(LIB_AVFORMAT ${LIB_AVFORMAT} NAME)
            
            find_library(LIB_AVUTIL NAMES "avutil" PATHS "${SM_EXTERN_DIR}/ffmpeg/64bit" NO_DEFAULT_PATH)
            get_filename_component(LIB_AVUTIL ${LIB_AVUTIL} NAME)
        else()
            find_library(LIB_SWSCALE NAMES "swscale" PATHS "${SM_EXTERN_DIR}/ffmpeg/lib" NO_DEFAULT_PATH)
            get_filename_component(LIB_SWSCALE ${LIB_SWSCALE} NAME)
            
            find_library(LIB_AVCODEC NAMES "avcodec" PATHS "${SM_EXTERN_DIR}/ffmpeg/lib" NO_DEFAULT_PATH)
            get_filename_component(LIB_AVCODEC ${LIB_AVCODEC} NAME)
            
            find_library(LIB_AVFORMAT NAMES "avformat" PATHS "${SM_EXTERN_DIR}/ffmpeg/lib" NO_DEFAULT_PATH)
            get_filename_component(LIB_AVFORMAT ${LIB_AVFORMAT} NAME)
            
            find_library(LIB_AVUTIL NAMES "avutil" PATHS "${SM_EXTERN_DIR}/ffmpeg/lib" NO_DEFAULT_PATH)
            get_filename_component(LIB_AVUTIL ${LIB_AVUTIL} NAME)
        endif()
    endif()


elseif(APPLE)
    find_package(BZip2 REQUIRED)
    if(WITH_FFMPEG)
        include("${SM_CMAKE_DIR}/SetupFfmpeg.cmake")
        set(HAS_FFMPEG TRUE)
    endif()
    
    set(CURL_LIBRARY "-lcurl")
    find_package(CURL REQUIRED)
    if(NOT CURL_FOUND)
        message(FATAL_ERROR "Could not find the CURL library")
    endif()
    
    
    link_libraries(${SM_EXTERN_DIR}/MinaCalc/libMinaCalc.a)
    link_libraries(${SM_EXTERN_DIR}/discord-rpc-2.0.1/lib/libdiscord-rpcMac.a)
    include_directories(${SM_EXTERN_DIR}/discord-rpc-2.0.1/include)
    
    set(SYSTEM_PCRE_FOUND FALSE)
    set(WITH_CRASH_HANDLER TRUE)
    
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
    set(CMAKE_OSX_DEPLOYMENT_TARGET_FULL "10.9.0")
    
    find_library(MAC_FRAME_ACCELERATE Accelerate ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_APPKIT AppKit ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_AUDIOTOOLBOX AudioToolbox ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_AUDIOUNIT AudioUnit ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_CARBON Carbon ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_COCOA Cocoa ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_COREAUDIO CoreAudio ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_COREFOUNDATION CoreFoundation ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_CORESERVICES CoreServices ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_FOUNDATION Foundation ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_IOKIT IOKit ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_OPENGL OpenGL ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    find_library(MAC_FRAME_QUICKTIME QuickTime ${CMAKE_SYSTEM_FRAMEWORK_PATH})
    
    mark_as_advanced(
        MAC_FRAME_ACCELERATE
        MAC_FRAME_APPKIT
        MAC_FRAME_AUDIOTOOLBOX
        MAC_FRAME_AUDIOUNIT
        MAC_FRAME_CARBON
        MAC_FRAME_COCOA
        MAC_FRAME_COREAUDIO
        MAC_FRAME_COREFOUNDATION
        MAC_FRAME_CORESERVICES
        MAC_FRAME_FOUNDATION
        MAC_FRAME_IOKIT
        MAC_FRAME_OPENGL
        MAC_FRAME_QUICKTIME
    )
elseif(LINUX)
    find_package(Mad REQUIRED)
    
    find_package(Ogg REQUIRED)
    find_package(Vorbis REQUIRED)
    find_package(VorbisFile REQUIRED)
    find_package(BZip2 REQUIRED)
    
    #	if(WITH_GTK2)
    #    find_package("GTK2" 2.0)
    #    if (${GTK2_FOUND})
    #      set(HAS_GTK2 TRUE)
    #    else()
    #      set(HAS_GTK2 FALSE)
    #      message("GTK2 was not found on your system. There will be no loading window.")
    #    endif()
    #  else()
    #    set(HAS_GTK2 FALSE)
    #  endif()
    
    link_libraries(${SM_EXTERN_DIR}/MinaCalc/MinaCalc.a)
    link_libraries(${SM_EXTERN_DIR}/discord-rpc-2.0.1/lib/libdiscord-rpc.a)
    include_directories(${SM_EXTERN_DIR}/discord-rpc-2.0.1/include)
    
    find_package(X11)
    if(${X11_FOUND})
        set(HAS_X11 TRUE)
    else()
        set(HAS_X11 FALSE)
    endif()
    
    find_package(Pcre)
    set(SYSTEM_PCRE_FOUND ${PCRE_FOUND})
    
    find_package("ZLIB")
    if(NOT (${ZLIB_FOUND}))
        message(FATAL_ERROR "zlib support required.")
    endif()
    
    find_package(JPEG REQUIRED)
    find_package(Dl)
    
    find_package(Xrandr)
    if(${XRANDR_FOUND})
        set(HAS_XRANDR TRUE)
    else()
        set(HAX_XRANDR FALSE)
    endif()
    
    find_package(PulseAudio)
    if(PULSEAUDIO_FOUND)
        set(HAS_PULSE TRUE)
    else()
        set(HAS_PULSE FALSE)
    endif()
    
    find_package(ALSA)
    if(ALSA_FOUND)
        set(HAS_ALSA TRUE)
    else()
        set(HAS_ALSA FALSE)
    endif()
    
    find_package(JACK)
    if(JACK_FOUND)
        set(HAS_JACK TRUE)
    else()
        set(HAS_JACK FALSE)
    endif()
    
    find_package(OSS)
    if(OSS_FOUND)
        set(HAS_OSS TRUE)
    else()
        set(HAS_OSS FALSE)
    endif()
    
    if(NOT OSS_FOUND AND NOT JACK_FOUND AND NOT ALSA_FOUND AND NOT PULSE_FOUND)
        message(FATAL_ERROR "No sound libraries found. You will require at least one.")
    else()
        message(STATUS "-- At least one sound library was found. Do not worry if any were not found at this stage.")
    endif()
    
    if(WITH_FFMPEG AND NOT YASM_FOUND AND NOT NASM_FOUND)
        message("Neither NASM nor YASM were found. Please install at least one of them if you wish for ffmpeg support.")
        set(WITH_FFMPEG OFF)
    endif()
    
    find_package("Va")
    
    if(WITH_FFMPEG)
        if(WITH_SYSTEM_FFMPEG)
            find_package("FFMPEG")
            if(NOT FFMPEG_FOUND)
                message(FATAL_ERROR "System ffmpeg not found! Either install the libraries or remove the argument, then try again.")
            else()
                
                message(STATUS "-- Warning! Your version of ffmpeg may be too high! If you want to use the system ffmpeg, clear your cmake cache and do not include the system ffmpeg argument.")
                set(HAS_FFMPEG TRUE)
            endif()
        else()
            include("${SM_CMAKE_DIR}/SetupFfmpeg.cmake")
            set(HAS_FFMPEG TRUE)
        endif()
    else()
        set(HAS_FFMPEG FALSE)
    endif()
    
    set(CURL_LIBRARY "-lcurl")
    find_package(CURL REQUIRED)
    if(NOT CURL_FOUND)
        message(FATAL_ERROR "Could not find the CURL library")
    endif()
    
    find_package(OpenGL REQUIRED)
    if(NOT ${OPENGL_FOUND})
        message(FATAL_ERROR "OpenGL required to compile Etterna.")
    endif()
    
    find_package(GLEW REQUIRED)
    if(NOT ${GLEW_FOUND})
        message(FATAL_ERROR "GLEW required to compile Etterna.")
    endif()

endif()

configure_file("${SM_SRC_DIR}/config.in.hpp" "${SM_SRC_DIR}/generated/config.hpp")
configure_file("${SM_SRC_DIR}/verstub.in.cpp" "${SM_SRC_DIR}/generated/verstub.cpp")

# Define installer based items for cpack.
include("${CMAKE_CURRENT_LIST_DIR}/CMake/CPackSetup.cmake")
