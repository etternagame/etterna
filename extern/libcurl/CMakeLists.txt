add_library(curl INTERFACE)
target_include_directories(curl INTERFACE include)

# TODO: Convert 64bit libcurl to be statically linked.
# Note: Only 32bit Etterna statically links libcurl. 64bit is dynamically linked.
# 64bit is a dll because of how the 64bit/libcurl.lib was generated. When 32bit/libcurl_x86.lib was generated
# it was not statically linked to libcurl. It only contains function references that it expects to find in a dll.
# 64bit/libcurl.lib was not generated like this, so it requires the dll file.
if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64" OR "${CMAKE_GENERATOR}" STREQUAL "Ninja")
    target_link_libraries(curl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libs/64bit/libcurl.lib")
else()
    target_link_libraries(curl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libs/32bit/libcurl_x86.lib")
endif()
