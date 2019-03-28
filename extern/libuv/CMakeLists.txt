add_library(libuv INTERFACE)
target_include_directories(libuv INTERFACE .)
target_link_libraries(libuv INTERFACE ${CMAKE_CURRENT_LIST_DIR}/libuv.a)