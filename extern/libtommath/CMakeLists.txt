file(GLOB SRC *.c)
add_library(libtommath ${SRC})
target_include_directories(libtommath PUBLIC .)