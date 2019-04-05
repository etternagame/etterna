# This should be modified and eventually removed with better methods for passing this info to the compiler
list(APPEND cdefs CPU_X86_64 HAVE_LIBPTHREAD
     "BACKTRACE_METHOD_X86_LINUX"
     "BACKTRACE_METHOD_TEXT=\"x86 custom backtrace\""
     "BACKTRACE_LOOKUP_METHOD_TEXT=\"backtrace_symbols\""
     "BACKTRACE_LOOKUP_METHOD_DLADDR"
     PACKAGE_NAME="Etterna"
     PACKAGE_VERSION="EtteraVersion")
set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS "${cdefs}")

# Find Libraries
find_package(X11 REQUIRED)
find_package(DLFCN REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Xrandr REQUIRED)
find_package(PulseAudio)
find_package(ALSA)
find_package(JACK)
find_package(OSS)

# Linking
target_link_libraries(Etterna ${X11_LIBRARIES})
target_link_libraries(Etterna ${DL_LIBRARIES})
target_link_libraries(Etterna ${XRANDR_LIBRARIES})
target_link_libraries(Etterna ${PULSEAUDIO_LIBRARIES})
target_link_libraries(Etterna ${ALSA_LIBRARIES})
target_link_libraries(Etterna ${JACK_LIBRARIES})
target_link_libraries(Etterna ${VA_LIBRARIES})
target_link_libraries(Etterna ${CURL_LIBRARIES})
target_link_libraries(Etterna ${OPENGL_LIBRARY})
