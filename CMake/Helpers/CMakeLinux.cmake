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
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)
find_package(DLFCN REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Xrandr)
find_package(PulseAudio)
find_package(ALSA)
find_package(JACK)
find_package(OSS)

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
target_link_libraries(Etterna curl)
target_link_libraries(Etterna stb)
target_link_libraries(Etterna glfw)
target_link_libraries(Etterna pcre)
target_link_libraries(Etterna libmad)
target_link_libraries(Etterna ogg)
target_link_libraries(Etterna vorbis)
target_link_libraries(Etterna Threads::Threads)
target_link_libraries(Etterna stb)

target_link_libraries(Etterna ${X11_LIBRARIES})
target_link_libraries(Etterna ${DL_LIBRARIES})
target_link_libraries(Etterna ${XRANDR_LIBRARIES})
target_link_libraries(Etterna ${PULSEAUDIO_LIBRARIES})
target_link_libraries(Etterna ${ALSA_LIBRARIES})
target_link_libraries(Etterna ${JACK_LIBRARIES})
target_link_libraries(Etterna ${VA_LIBRARIES})
target_link_libraries(Etterna ${CURL_LIBRARIES})
target_link_libraries(Etterna ${OPENGL_LIBRARY})
