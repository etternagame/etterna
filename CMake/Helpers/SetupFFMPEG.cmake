include(ExternalProject)

set(FFMPEG_ROOT "${PROJECT_BINARY_DIR}/ffmpeg-2.1.3-src")

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

if (WITH_CRYSTALHD_DISABLED)
  list(APPEND FFMPEG_CONFIGURE "--disable-crystalhd")
endif()

list(APPEND FFMPEG_CONFIGURE "--extra-cflags=-w")

# list(APPEND SM_FFMPEG_MAKE $(MAKE))
# if (WITH_FFMPEG_JOBS GREATER 0)
#   list(APPEND SM_FFMPEG_MAKE "-j${WITH_FFMPEG_JOBS}")
# endif()

ExternalProject_Add(ffmpeg_dl
    GIT_REPOSITORY "https://github.com/stepmania/ffmpeg.git"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
    GIT_TAG "n2.1.3"
    
    SOURCE_DIR ${FFMPEG_ROOT}
    CONFIGURE_COMMAND "${FFMPEG_CONFIGURE}"
    BUILD_COMMAND make -j10
    UPDATE_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND "")

externalproject_get_property(ffmpeg_dl BINARY_DIR)

list(APPEND SMDATA_LINK_LIB
"${BINARY_DIR}/libavformat/libavformat.a"
"${BINARY_DIR}/libavcodec/libavcodec.a"
"${BINARY_DIR}/libswscale/libswscale.a"
"${BINARY_DIR}/libavutil/libavutil.a")

add_library(ffmpeg INTERFACE)
add_dependencies(ffmpeg ffmpeg_dl)
target_link_libraries(ffmpeg INTERFACE ${SMDATA_LINK_LIB})

target_include_directories(ffmpeg INTERFACE ${FFMPEG_ROOT})
target_include_directories(ffmpeg INTERFACE ${BINARY_DIR})


