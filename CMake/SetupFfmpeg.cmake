set(SM_FFMPEG_VERSION "2.1.3")
set(SM_FFMPEG_SRC_LIST "${SM_EXTERN_DIR}" "/ffmpeg-linux-" "${SM_FFMPEG_VERSION}")
sm_join("${SM_FFMPEG_SRC_LIST}" "" SM_FFMPEG_SRC_DIR)
set(SM_FFMPEG_CONFIGURE_EXE "${SM_FFMPEG_SRC_DIR}/configure")
list(APPEND FFMPEG_CONFIGURE
  "${SM_FFMPEG_CONFIGURE_EXE}"
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

if (LINUX)
  # TODO: Figure out how to build with hardware video acceleration.
  # These options cause link issues when not disabled.
  list(APPEND FFMPEG_CONFIGURE "--disable-vaapi" "--disable-vdpau")
endif()

if(MACOSX)
  list(APPEND FFMPEG_CONFIGURE
    "--arch=x86_64"
    "--cc=clang -m64"
    "--enable-sse"
  )
endif()

list(APPEND FFMPEG_CONFIGURE "--enable-gpl")

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
    DOWNLOAD_COMMAND git clone "--branch" "n${SM_FFMPEG_VERSION}" "--depth" "1" "git://github.com/stepmania/ffmpeg.git" "${SM_FFMPEG_SRC_DIR}"
    CONFIGURE_COMMAND "${FFMPEG_CONFIGURE}"
    BUILD_COMMAND "${SM_FFMPEG_MAKE}"
    UPDATE_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
  )
endif()

externalproject_get_property("ffmpeg" BINARY_DIR)
set(SM_FFMPEG_ROOT ${BINARY_DIR})
