# Tell Visual Studio what project to run by default
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT Etterna)

# Windows prefers the binary to be placed in the Program Directory. A generator expression is used
# otherwise a folder would be appended with the same name as the build configuration. The gen-expression
# is only used to prevent the appending.
set_target_properties(Etterna PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/Program>")

# Universal Build Options
set_target_properties(Etterna PROPERTIES
	COMPILE_FLAGS "/W3 /MP8 /INCREMENTAL /D_HAS_STD_BYTE=0"
	LINK_FLAGS "/SUBSYSTEM:WINDOWS /SAFESEH:NO /INCREMENTAL"
	COMPILE_DEFINITIONS "GLEW_STATIC")

# By default MSVC has a 2^16 limit on the number of sections in an object file, and this needs more than that.
set_source_files_properties(src/Etterna/Singletons/NetworkSyncManager.cpp PROPERTIES COMPILE_FLAGS /bigobj)

# Ignore the safer function variants provided by VC++. They are not portable.
target_compile_definitions(Etterna PRIVATE _CRT_SECURE_NO_WARNINGS)

# Linking - Windows Only
target_link_libraries(Etterna PUBLIC ffmpeg)

find_package(DirectX REQUIRED)
get_filename_component(DIRECTX_LIBRARY_DIR "${DIRECTX_LIBRARIES}" DIRECTORY)
target_link_directories(Etterna PUBLIC ${DIRECTX_LIBRARY_DIR})
target_include_directories(Etterna PRIVATE ${DIRECTX_INCLUDE_DIR})

# DLL - Copy to run directory
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8) # If 64bit
	set(ARCH 64bit)
else()
	set(ARCH 32bit)
endif()

list(APPEND WIN_DLLS
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/avcodec-58.dll"
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/avformat-58.dll"
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/avutil-56.dll"
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/swscale-5.dll"
	"${PROJECT_SOURCE_DIR}/extern/ffmpeg/windows/${ARCH}/swresample-3.dll")

foreach(dll ${WIN_DLLS})
	# We remove the dlls if they exist already in /Program/ in case we run a different ARCH target before
	# Since we get a cryptic runtime error message otherwise when windows tries to load the wrong dll
	get_filename_component(dll_filename_without_path ${dll} NAME)
	file(REMOVE "${PROJECT_SOURCE_DIR}/Program/${dll_filename_without_path}")
	file(COPY "${dll}" DESTINATION "${PROJECT_SOURCE_DIR}/Program/")
endforeach()

target_compile_definitions(Etterna PRIVATE $<$<CONFIG:RelWithDebInfo>:RELWITHDEBINFO>)