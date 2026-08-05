
if(MSVC)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8) # If 64bit
	    file(GLOB LIBS 64bit/*.lib)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # If 32bit
    	file(GLOB LIBS lib/*.lib)
	endif()
elseif(UNIX)
	if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
		file(GLOB LIBS linux-lib/*.so*)
	endif()
endif()

add_library(ffmpeg INTERFACE)
target_link_libraries(ffmpeg INTERFACE ${LIBS})
target_include_directories(ffmpeg INTERFACE include)
