# From the CMake wiki, get the DirectX version needed.
# This assumes default directories.

# Once loaded, the following are defined:
#  DIRECTX_FOUND
#  DIRECTX_INCLUDE_DIR
#  DIRECTX_LIBRARIES

if(NOT WIN32)
  return()
endif()
if(WIN32)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set (DirectX_ARCHITECTURE x64)
	else ()
		set (DirectX_ARCHITECTURE x86)
	endif ()
	set (ProgramFiles_x86 "ProgramFiles(x86)")
	if ("$ENV{${ProgramFiles_x86}}")
		set (ProgramFiles "$ENV{${ProgramFiles_x86}}")
	else ()
		set (ProgramFiles "$ENV{ProgramFiles}")
	endif ()
	# first check specified paths
	find_path (DirectX_ROOT_DIR
		Include/d3d9.h
		PATHS
			"$ENV{DXSDK_DIR}"
			"${ProgramFiles}/Microsoft DirectX SDK (June 2010)"
			"${ProgramFiles}/Microsoft DirectX SDK (February 2010)"
			"${ProgramFiles}/Microsoft DirectX SDK (March 2009)"
			"${ProgramFiles}/Microsoft DirectX SDK (August 2008)"
			"${ProgramFiles}/Microsoft DirectX SDK (June 2008)"
			"${ProgramFiles}/Microsoft DirectX SDK (March 2008)"
			"${ProgramFiles}/Microsoft DirectX SDK (November 2007)"
			"${ProgramFiles}/Microsoft DirectX SDK (August 2007)"
			"${ProgramFiles}/Microsoft DirectX SDK"
		NO_DEFAULT_PATH
		DOC "DirectX SDK root directory"
	)
	# if specified paths do not contain dx then search PATH
	if (NOT DirectX_ROOT_DIR)
		find_path(DirectX_ROOT_DIR
			Include/d3d9.h
			DOC "DirectX SDK root directory"
		)
	endif()

	if (DirectX_ROOT_DIR)
		set (DIRECTX_INCLUDE_SEARCH_PATHS "${DirectX_ROOT_DIR}/Include")
		set (DIRECTX_LIBRARY_SEARCH_PATHS "${DirectX_ROOT_DIR}/Lib/${DirectX_ARCHITECTURE}")
		set (DirectX_BIN_SEARCH_PATH "${DirectX_ROOT_DIR}/Utilities/bin/${DirectX_ARCHITECTURE}")
	endif ()
else()
	if(NOT EXISTS "$ENV{DXSDK_DIR}")
	  message(FATAL_ERROR "Could not find Microsoft DirectX SDK installation!")
	endif()
	 set(DIRECTX_INCLUDE_SEARCH_PATHS
	  # TODO: Do not be limited to x86 in the future.
	  "$ENV{DXSDK_DIR}/Include"
	)
	 set(DIRECTX_LIBRARY_SEARCH_PATHS
	  # TODO: Do not be limited to x86 in the future.
	  "$ENV{DXSDK_DIR}/Lib/x86"
	 )
endif()

find_path(DIRECTX_INCLUDE_DIR
  NAMES "DxErr.h"
  PATHS ${DIRECTX_INCLUDE_SEARCH_PATHS}
  DOC "Where can DxErr.h be found?"
)

find_library(DIRECTX_LIBRARIES
  NAMES "DxErr.lib" "d3dx9.lib"
  PATHS ${DIRECTX_LIBRARY_SEARCH_PATHS}
  DOC "Where can the DX libraries be found?"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DirectX DEFAULT_MSG DIRECTX_INCLUDE_DIR DIRECTX_LIBRARIES)

if(DIRECTX_FOUND)
  mark_as_advanced(DIRECTX_INCLUDE_DIR DIRECTX_LIBRARIES)
endif()
