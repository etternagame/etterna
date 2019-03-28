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

add_library(libmad ${SOURCES})
target_include_directories(libmad PUBLIC .)
set_target_properties(libmad PROPERTIES COMPILE_DEFINITIONS FPM_DEFAULT)
# set_property(TARGET "mad" PROPERTY FOLDER "External Libraries")
