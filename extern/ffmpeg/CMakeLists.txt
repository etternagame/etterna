file(GLOB LIBS 64bit/*.lib lib/*.lib lib/*.a)
add_library(ffmpeg null.cpp)
target_link_libraries(ffmpeg ${LIBS})
target_include_directories(ffmpeg PUBLIC include)
