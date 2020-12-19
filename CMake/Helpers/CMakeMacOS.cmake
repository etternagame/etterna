# TODO: Remove CPU_X86_64, CPU_X86, and CRASH_HANDLER
#       CRASH_HANDLER is unnecessary as the game should have that as an option component
#       CPU_X86_64, CPU_X86 already exists as compiler predefined macros. Use those instead.
list(APPEND cdefs _XOPEN_SOURCE CPU_X86_64)
set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS "${cdefs}")
set_target_properties(Etterna PROPERTIES MACOSX_BUNDLE TRUE)
set(CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000")
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

