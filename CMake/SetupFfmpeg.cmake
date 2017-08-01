set(SM_FFMPEG_VERSION "3.3.3")
set(SM_FFMPEG_SRC_LIST "${SM_EXTERN_DIR}" "/ffmpeg-linux-" "${SM_FFMPEG_VERSION}")
sm_join("${SM_FFMPEG_SRC_LIST}" "" SM_FFMPEG_SRC_DIR)
set(SM_FFMPEG_CONFIGURE_EXE "${SM_FFMPEG_SRC_DIR}/configure")
if (MINGW)
  # Borrow from http://stackoverflow.com/q/11845823
  # string(SUBSTRING ${SM_FFMPEG_CONFIGURE_EXE} 0 1 FIRST_LETTER)
  # string(TOLOWER ${FIRST_LETTER} FIRST_LETTER_LOW)
  # string(REPLACE "${FIRST_LETTER}:" "/${FIRST_LETTER_LOW}" # SM_FFMPEG_CONFIGURE_EXE ${SM_FFMPEG_CONFIGURE_EXE})
  # string(REGEX REPLACE "\\\\" "/" SM_FFMPEG_CONFIGURE_EXE "${SM_FFMPEG_CONFIGURE_EXE}")
  set(SM_FFMPEG_CONFIGURE_EXE "extern/ffmpeg-linux-${SM_FFMPEG_VERSION}/configure")
endif()
list(APPEND FFMPEG_CONFIGURE
  "${SM_FFMPEG_CONFIGURE_EXE}"
  "--disable-muxers"
  "--disable-encoders"
  "--disable-swresample"
  "--enable-static"
  "--disable-lzma"
)

if(CMAKE_POSITION_INDEPENDENT_CODE)
    list(APPEND FFMPEG_CONFIGURE "--enable-pic")
endif()

if(MACOSX)
  # TODO: Remove these two items when Mac OS X StepMania builds in 64-bit.
  list(APPEND FFMPEG_CONFIGURE
    "--arch=i386"
    "--cc=clang -m32"
  )
endif()

#if(WITH_GPL_LIBS)
#  list(APPEND FFMPEG_CONFIGURE
#    "--enable-gpl"
#  )
#endif()

if (WITH_CRYSTALHD_DISABLED)
  list(APPEND FFMPEG_CONFIGURE "--disable-crystalhd")
endif()

if (NOT WITH_EXTERNAL_WARNINGS)
  list(APPEND FFMPEG_CONFIGURE
    "--extra-cflags=-w"
  )
endif()

list(APPEND SM_FFMPEG_MAKE
  $(MAKE)
)
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
  # --shlibdir=$our_installdir/stepmania-$VERSION
  externalproject_add("ffmpeg"
    DOWNLOAD_COMMAND git clone "--branch" "release/3.3" "--depth" "1" "git://github.com/FFmpeg/FFmpeg.git" "${SM_FFMPEG_SRC_DIR}"
    CONFIGURE_COMMAND "${FFMPEG_CONFIGURE}"
    BUILD_COMMAND "${SM_FFMPEG_MAKE}"
    UPDATE_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
  )
endif()

externalproject_get_property("ffmpeg" BINARY_DIR)
set(SM_FFMPEG_ROOT ${BINARY_DIR})

