
if(CMAKE_SIZEOF_VOID_P EQUAL 8) # If 64bit
    file(GLOB LIBS 64bit/*.lib)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # If 32bit
    file(GLOB LIBS lib/*.lib)
endif()

if(UNIX)
    file(GLOB LIBS lib/*.a)
endif()

add_library(ffmpeg INTERFACE)
target_link_libraries(ffmpeg INTERFACE ${LIBS})
target_include_directories(ffmpeg INTERFACE include)
