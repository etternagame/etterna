project(ffmpeg)

file(GLOB_RECURSE ALL_LIB_FILES 64bit/*.lib lib/*.a lib/*.lib)

add_library(ffmpeg null.cpp)
target_link_libraries(ffmpeg PUBLIC ${ALL_LIB_FILES})
target_include_directories(ffmpeg PUBLIC ${PROJECT_SOURCE_DIR}/include)
