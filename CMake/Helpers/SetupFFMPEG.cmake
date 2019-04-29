include(ExternalProject)

set(FFMPEG_ROOT "${PROJECT_BINARY_DIR}/ffmpeg_dl/ffmpeg-2.1.3-src")
set(FFMPEG_BIN  "${PROJECT_BINARY_DIR}/ffmpeg_dl/ffmpeg_dl-build")

list(APPEND FFMPEG_CONFIGURE
  "${FFMPEG_ROOT}/configure"
  "--disable-programs"
  "--disable-doc"
  "--disable-debug"
  "--disable-avdevice"
  "--disable-swresample"
  "--disable-postproc"
  "--disable-avfilter"
  "--disable-shared"
  "--enable-static"
)

if(CMAKE_POSITION_INDEPENDENT_CODE)
    list(APPEND FFMPEG_CONFIGURE "--enable-pic")
endif()

if (UNIX)
  # TODO: Figure out how to build with hardware video acceleration.
  # These options cause link issues when not disabled.
  list(APPEND FFMPEG_CONFIGURE "--disable-vaapi" "--disable-vdpau")
endif()

if(APPLE)
  list(APPEND FFMPEG_CONFIGURE
    "--arch=x86_64"
    "--cc=clang -m64"
    "--enable-sse")
endif()
list(APPEND FFMPEG_CONFIGURE "--enable-gpl")
list(APPEND FFMPEG_CONFIGURE "--extra-cflags=-mmacosx-version-min=10.8 -w")

list(APPEND FFMPEG_BUILD_LIBS
"${FFMPEG_BIN}/libavformat/libavformat.a"
"${FFMPEG_BIN}/libavcodec/libavcodec.a"
"${FFMPEG_BIN}/libswscale/libswscale.a"
"${FFMPEG_BIN}/libavutil/libavutil.a")


ExternalProject_Add(ffmpeg_dl
    PREFIX ${PROJECT_BINARY_DIR}/ffmpeg_dl
    GIT_REPOSITORY "https://github.com/stepmania/ffmpeg.git"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
    GIT_TAG "n2.1.3"
    
    BUILD_BYPRODUCTS ${FFMPEG_BUILD_LIBS}
    SOURCE_DIR ${FFMPEG_ROOT}
    BINARY_DIR ${FFMPEG_BIN}
    CONFIGURE_COMMAND "${FFMPEG_CONFIGURE}"
    BUILD_COMMAND make -j10
    UPDATE_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND "")

add_library(ffmpeg INTERFACE)
add_dependencies(ffmpeg ffmpeg_dl)
target_link_libraries(ffmpeg INTERFACE ${FFMPEG_BUILD_LIBS})

target_include_directories(ffmpeg INTERFACE ${FFMPEG_ROOT})
target_include_directories(ffmpeg INTERFACE ${FFMPEG_BIN})
