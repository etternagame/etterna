set_directory_properties(PROPERTIES VS_STARTUP_PROJECT Etterna)

# Windows prefers the binary to be placed in the Program Directory
# To be changed when swiched to an out-of-source build
set_target_properties(Etterna PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/Program"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/Program"
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/Program"
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_SOURCE_DIR}/Program")

# Universal Build Options
set(ETTERNA_COMPILE_FLAGS "/MP8 /GL /arch:SSE2")
set(ETTERNA_LINK_FLAGS "/SUBSYSTEM:WINDOWS /SAFESEH:NO /LTCG")

# Build type dependant compile flags
if (CMAKE_BUILD_TYPE STREQUAL "Release")
	set_target_properties(SQLiteCpp sqlite3 jsoncpp lua uWS PROPERTIES COMPILE_FLAGS "/MT") # The following libraries are set to be dynamically linked. These compile flags switch them to be statically linked.
	set(ETTERNA_COMPILE_FLAGS "${ETTERNA_COMPILE_FLAGS} /MT")
	set(ETTERNA_LINK_FLAGS "${ETTERNA_LINK_FLAGS} /NODEFAULTLIB:\"LIBCMT\"")
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
	# TODO: Fix Debug build configuration
	# At the moment, to build with debug information, use RelWithDebInfo
endif()

set_target_properties(Etterna PROPERTIES LINK_FLAGS ${ETTERNA_LINK_FLAGS})
set_target_properties(Etterna PROPERTIES COMPILE_FLAGS ${ETTERNA_COMPILE_FLAGS})

# The following libraries are set to be dynamically linked.
# These compile flags switch them to be statically linked.
list(APPEND cdefs CURL_STATICLIB GLEW_STATIC)
set_target_properties(Etterna PROPERTIES COMPILE_DEFINITIONS "${cdefs}")

target_link_libraries(Etterna luajit) # Link lua to etterna
target_link_libraries(Etterna discord-rpc)
target_link_libraries(Etterna sqlite3)   # Link sqlite3 to etterna
target_link_libraries(Etterna SQLiteCpp) # Link SQLiteCpp to etterna
target_link_libraries(Etterna uWS)
target_link_libraries(Etterna jsoncpp) # TODO: Two JSON Libraries?
target_link_libraries(Etterna nlohmann_json)
target_link_libraries(Etterna tomcrypt)
target_link_libraries(Etterna libtommath)
target_link_libraries(Etterna fftw3f)
target_link_libraries(Etterna MinaCalc)
target_link_libraries(Etterna curl)
target_link_libraries(Etterna stb)
target_link_libraries(Etterna glfw)
target_link_libraries(Etterna pcre)
target_link_libraries(Etterna libmad)
target_link_libraries(Etterna ogg)
target_link_libraries(Etterna vorbis)
target_link_libraries(Etterna ffmpeg)

find_package(DirectX REQUIRED)
get_filename_component(DIRECTX_LIBRARY_DIR "${DIRECTX_LIBRARIES}" DIRECTORY)
target_link_directories(Etterna PUBLIC ${DIRECTX_LIBRARY_DIR})
target_include_directories(Etterna PRIVATE ${DIRECTX_INCLUDE_DIR})

# Copying DLL files to run directory
if(CMAKE_SIZEOF_VOID_P EQUAL 8) # If 64bit
	set(ARCH 64bit)
else()
	set(ARCH 32bit)
endif()

list(APPEND WIN_DLLS
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/avcodec-55.dll"
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/avformat-55.dll"
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/avutil-52.dll"
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/swscale-2.dll"
	"${PROJECT_SOURCE_DIR}/extern/libcurl/windows/${ARCH}/libcurl.dll")

if(ARCH STREQUAL "32bit")
	list(APPEND WIN_DLLS "${OPENSSL_ROOT_DIR}/libssl-1_1.dll" "${OPENSSL_ROOT_DIR}/libcrypto-1_1.dll") 			# SSL
else() # 64bit
	list(APPEND WIN_DLLS "${OPENSSL_ROOT_DIR}/libssl-1_1-x64.dll" "${OPENSSL_ROOT_DIR}/libcrypto-1_1-x64.dll") 	# SSL
	list(APPEND WIN_DLLS "${PROJECT_SOURCE_DIR}/extern/libcurl/windows/${ARCH}/libcurl.dll") 					# CURL
endif()

foreach(dll ${WIN_DLLS})
	file(COPY "${dll}" DESTINATION "${PROJECT_SOURCE_DIR}/Program/")
endforeach()
