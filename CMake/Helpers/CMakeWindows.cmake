set_directory_properties(PROPERTIES VS_STARTUP_PROJECT Etterna)

# Windows prefers the binary to be placed in the Program Directory
# To be changed when swiched to an out-of-source build
set_target_properties(Etterna PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/Program>")
set_target_properties(Etterna PROPERTIES 
	RUNTIME_OUTPUT_NAME_DEBUG "Etterna-debug"
	RUNTIME_OUTPUT_NAME_RELEASE "Etterna" # Without prefix, as this is the binary to be copied when cpack is run.
	RUNTIME_OUTPUT_NAME_MINSIZEREL "Etterna-MinSizeRelease"
	RUNTIME_OUTPUT_NAME_RELWITHDEBINFO "Etterna-RelWithDebInfo")

# Universal Build Options
set_target_properties(Etterna PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /SAFESEH:NO /INCREMENTAL" COMPILE_FLAGS "/MP8 /INCREMENTAL")

# By default MSVC has a 2^16 limit on the number of sections in an object file, and this needs more than that.
set_source_files_properties(src/Etterna/Singletons/NetworkSyncManager.cpp PROPERTIES COMPILE_FLAGS /bigobj)

# Multiple Build Configuration Setup
## The "DYNAMIC_LINK" list are targets which by default are set to be built dynamically. 
## Etterna prefers statically linking as many libraries as possible as few dll files as possible, 
## except where statically linking a library will be against the library terms or service, or for 
## technical reasons, cannot be statically linked.
##
## The following generator expressions ($<>) change what runtime library we link to, depending on 
## if we are building release, or debug
set(DYNAMIC_LINK "Etterna;SQLiteCpp;sqlite3;lua;discord-rpc")
foreach(item ${DYNAMIC_LINK})
	target_compile_options(${item} PRIVATE "$<$<CONFIG:Release>:/MT>" "$<$<CONFIG:Debug>:/MTd>")
endforeach()


list(APPEND cdefs CURL_STATICLIB GLEW_STATIC)
set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS "${cdefs}")
target_compile_options(Etterna PRIVATE /W3)

# Linking - Windows Only
target_link_libraries(Etterna libcurl)
target_link_libraries(Etterna ffmpeg)

find_package(DirectX REQUIRED)
get_filename_component(DIRECTX_LIBRARY_DIR "${DIRECTX_LIBRARIES}" DIRECTORY)
target_link_directories(Etterna PUBLIC ${DIRECTX_LIBRARY_DIR})
target_include_directories(Etterna PRIVATE ${DIRECTX_INCLUDE_DIR})

# DLL - Copy to run directory
if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64" OR "${CMAKE_GENERATOR}" STREQUAL "Ninja") # If 64bit
	set(ARCH 64bit)
else()
	set(ARCH 32bit)
endif()

list(APPEND WIN_DLLS
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/avcodec-55.dll"
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/avformat-55.dll"
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/avutil-52.dll"
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/swscale-2.dll")

if(ARCH STREQUAL "32bit")
	list(APPEND WIN_DLLS "${OPENSSL_ROOT_DIR}/libssl-1_1.dll" "${OPENSSL_ROOT_DIR}/libcrypto-1_1.dll") 			# SSL
else() # 64bit
	list(APPEND WIN_DLLS "${OPENSSL_ROOT_DIR}/libssl-1_1-x64.dll" "${OPENSSL_ROOT_DIR}/libcrypto-1_1-x64.dll") 	# SSL
endif()

foreach(dll ${WIN_DLLS})
	file(COPY "${dll}" DESTINATION "${PROJECT_SOURCE_DIR}/Program/")
endforeach()
