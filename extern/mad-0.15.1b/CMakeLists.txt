project(libmad)

list(APPEND SOURCES
  "bit.c"
  "decoder.c"
  "fixed.c"
  "frame.c"
  "huffman.c"
  "layer12.c"
  "layer3.c"
  "stream.c"
  "synth.c"
  "timer.c"
  "version.c")

add_library(libmad STATIC ${SOURCES})
target_include_directories(libmad PUBLIC . ${PROJECT_BINARY_DIR}/generated)

list(APPEND cdefs HAVE_CONFIG_H FPM_DEFAULT)
set_target_properties(libmad PROPERTIES COMPILE_DEFINITIONS "${cdefs}")

configure_file(config.mad.in.h ${PROJECT_BINARY_DIR}/generated/config.h)