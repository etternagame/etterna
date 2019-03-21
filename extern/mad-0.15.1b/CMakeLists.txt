file(GLOB SOURCES *.c)
add_library(libmad ${SOURCES})
target_include_directories(libmad PUBLIC .)
set_target_properties(libmad PROPERTIES COMPILE_DEFINITIONS FPM_DEFAULT)