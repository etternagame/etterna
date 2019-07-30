
if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64" OR "${CMAKE_GENERATOR}" STREQUAL "Ninja")
    file(GLOB LIBS 64bit/*.lib)
elseif("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32")
    file(GLOB LIBS lib/*.lib)
endif()

if(UNIX)
    file(GLOB LIBS lib/*.a)
endif()

add_library(ffmpeg INTERFACE)
target_link_libraries(ffmpeg INTERFACE ${LIBS})
target_include_directories(ffmpeg INTERFACE include)
