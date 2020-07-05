#ifndef RAGE_SURFACE_LOAD_H
#define RAGE_SURFACE_LOAD_H

struct RageSurface;

/** @brief Utility functions for the RageSurfaces. */
namespace RageSurfaceUtils {
enum OpenResult
{
	OPEN_OK,
	OPEN_UNKNOWN_FILE_FORMAT = 1,
	OPEN_FATAL_ERROR = 2,
};

/* If bHeaderOnly is true, the loader is only required to return a surface
 * with the width and height set (but may return a complete surface). */
RageSurface*
LoadFile(const std::string& sPath,
		 std::string& error,
		 bool bHeaderOnly = false);
}

#endif
