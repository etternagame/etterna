set(SM_FFMPEG_VERSION "3.3.3")
set(SM_FFMPEG_SRC_LIST "${SM_EXTERN_DIR}/ffmpeg-linux-${SM_FFMPEG_VERSION}")
sm_join("${SM_FFMPEG_SRC_LIST}" "" SM_FFMPEG_SRC_DIR)
set(SM_FFMPEG_CONFIGURE_EXE "${SM_FFMPEG_SRC_DIR}/configure")
list(APPEND FFMPEG_CONFIGURE
  "${SM_FFMPEG_CONFIGURE_EXE}"
  "--disable-muxers"
  "--disable-encoders"
  "--disable-programs"
  "--disable-doc"
  "--disable-avdevice"
  "--disable-avfilter"
  "--disable-lzma"
)

if(CMAKE_POSITION_INDEPENDENT_CODE)
    list(APPEND FFMPEG_CONFIGURE "--enable-pic")
endif()

if (LINUX)
  # TODO: Figure out how to build with hardware video acceleration.
  # These options cause link issues when not disabled.
  list(APPEND FFMPEG_CONFIGURE "--disable-vaapi" "--disable-vdpau")
endif()

if(MACOSX)
  # TODO: Remove these two items when Mac OS X StepMania builds in 64-bit.
  list(APPEND FFMPEG_CONFIGURE
    "--arch=i386"
    "--cc=clang -m32"
  )
endif()

if (WITH_CRYSTALHD_DISABLED)
  list(APPEND FFMPEG_CONFIGURE "--disable-crystalhd")
endif()

if (NOT WITH_EXTERNAL_WARNINGS)
  list(APPEND FFMPEG_CONFIGURE "--extra-cflags=-w")
endif()

list(APPEND SM_FFMPEG_MAKE $(MAKE))
if (WITH_FFMPEG_JOBS GREATER 0)
  list(APPEND SM_FFMPEG_MAKE "-j${WITH_FFMPEG_JOBS}")
endif()

if (IS_DIRECTORY "${SM_FFMPEG_SRC_DIR}")
  externalproject_add("ffmpeg"
    SOURCE_DIR "${SM_FFMPEG_SRC_DIR}"
    CONFIGURE_COMMAND ${FFMPEG_CONFIGURE}
    BUILD_COMMAND "${SM_FFMPEG_MAKE}"
    UPDATE_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
  )
else()
  externalproject_add("ffmpeg"
    DOWNLOAD_COMMAND git clone "--depth" "1" "git://github.com/etternagame/FFmpeg.git" "${SM_FFMPEG_SRC_DIR}"
    CONFIGURE_COMMAND "${FFMPEG_CONFIGURE}"
    BUILD_COMMAND "${SM_FFMPEG_MAKE}"
    UPDATE_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
  )
endif()

externalproject_get_property("ffmpeg" BINARY_DIR)
set(SM_FFMPEG_ROOT ${BINARY_DIR})
