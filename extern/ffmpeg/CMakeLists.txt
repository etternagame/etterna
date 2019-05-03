file(GLOB LIBS 64bit/*.lib lib/*.lib lib/*.a)
add_library(ffmpeg INTERFACE)
target_link_libraries(ffmpeg INTERFACE ${LIBS})
target_include_directories(ffmpeg INTERFACE include)
