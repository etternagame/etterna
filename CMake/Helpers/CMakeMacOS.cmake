# TODO: Remove CPU_X86_64, CPU_X86, and CRASH_HANDLER
#       CRASH_HANDLER is unnecessary as the game should have that as an option component
#       CPU_X86_64, CPU_X86 already exists as compiler predefined macros. Use those instead.

list(APPEND cdefs _XOPEN_SOURCE GL_SILENCE_DEPRECATION)
if(NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
  list(APPEND cdefs CPU_X86_64)
 endif()
set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS "${cdefs}")

set_target_properties(Etterna PROPERTIES MACOSX_BUNDLE TRUE)

if(NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
  #TODO: Do we even need these on x86_64?
  # For the ARM build, it breaks if they're present, and seems to work just fine without them...
  # Seems to have made its way into this file eventually after 2d03a8163f5d10c6b569eb3c92543677c910e82e
  # That commit appears to be copying part of luajit's CMakeLists, originating at f31f6eea7bf52400929fcfb303637741bb645818
  #  (https://github.com/LuaJIT/LuaJIT/commit/f31f6eea7bf52400929fcfb303637741bb645818)
  # *That* appears to have originated from https://github.com/LuaJIT/LuaJIT/commit/f76e5a311ba543ae174acd3b585fb672fde8a3b5 which states
  ### /* OSX mmap() uses a naive first-fit linear search. That's perfect for us.
  ### ** But -pagezero_size must be set, otherwise the lower 4GB are blocked.
  ### */
  set(CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000")
endif()

set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

# Set AppBundle icon
set(MACOSX_BUNDLE_ICON_FILE etterna.icns)
set_property(SOURCE CMake/CPack/macOS/etterna.icns PROPERTY MACOSX_PACKAGE_LOCATION "Resources")
target_sources(Etterna PUBLIC CMake/CPack/macOS/etterna.icns)

# macOS Frameworks
find_library(MAC_FRAME_AUDIOUNIT AudioUnit)
find_library(MAC_FRAME_CARBON Carbon)
find_library(MAC_FRAME_COREAUDIO CoreAudio)
find_library(MAC_FRAME_IOKIT IOKit)
find_library(MAC_FRAME_METAL Metal)
target_link_libraries(Etterna PRIVATE ${MAC_FRAME_AUDIOUNIT})
target_link_libraries(Etterna PRIVATE ${MAC_FRAME_CARBON})
target_link_libraries(Etterna PRIVATE ${MAC_FRAME_COREAUDIO})
target_link_libraries(Etterna PRIVATE ${MAC_FRAME_IOKIT})
target_link_libraries(Etterna PRIVATE ${MAC_FRAME_METAL})

# Extern Libraries
target_link_libraries(Etterna PRIVATE ffmpeg)

# System Libraries
find_package(OpenGL REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Iconv REQUIRED)
target_link_libraries(Etterna PRIVATE ${OPENGL_LIBRARIES})
target_link_libraries(Etterna PRIVATE ${BZIP2_LIBRARIES})
target_link_libraries(Etterna PUBLIC ${ICONV_LIBRARIES})

