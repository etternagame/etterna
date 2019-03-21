set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
set(CMAKE_OSX_DEPLOYMENT_TARGET_FULL "10.9.0")

# macOS Frameworks
find_library(MAC_FRAME_AUDIOUNIT AudioUnit)
find_library(MAC_FRAME_CARBON Carbon)
find_library(MAC_FRAME_COREAUDIO CoreAudio)
find_library(MAC_FRAME_IOKIT IOKit)
target_link_libraries(Etterna ${MAC_FRAME_AUDIOUNIT})
target_link_libraries(Etterna ${MAC_FRAME_CARBON})
target_link_libraries(Etterna ${MAC_FRAME_COREAUDIO})
target_link_libraries(Etterna ${MAC_FRAME_IOKIT})

# Extern Libraries
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
target_link_libraries(Etterna glfw)
target_link_libraries(Etterna libmad)
target_link_libraries(Etterna ogg)
target_link_libraries(Etterna vorbis)
target_link_libraries(Etterna pcre)
target_link_libraries(Etterna ffmpeg)

# System Libraries
find_package(CURL)
find_package(OpenGL)
find_package(BZip2 REQUIRED)
find_package(Iconv REQUIRED)
target_link_libraries(Etterna ${CURL_LIBRARIES})
target_link_libraries(Etterna ${OPENGL_LIBRARIES})
target_link_libraries(Etterna ${BZIP2_LIBRARIES})
target_link_libraries(Etterna ${ICONV_LIBRARIES})