/* RageSurface_Save_PNG - Save a RageSurface to a PNG. */

#ifndef RAGE_SURFACE_SAVE_PNG_H
#define RAGE_SURFACE_SAVE_PNG_H

struct RageSurface;
class RageFile;

/** @brief Utility functions for the RageSurfaces. */
namespace RageSurfaceUtils {
bool
SavePNG(RageSurface* pImg, RageFile& f, std::string& sError);
};

#endif
