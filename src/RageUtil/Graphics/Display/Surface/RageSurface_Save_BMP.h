/* RageSurface_Save_BMP - Save a RageSurface to a BMP. */

#ifndef RAGE_SURFACE_SAVE_BMP_H
#define RAGE_SURFACE_SAVE_BMP_H

struct RageSurface;
class RageFile;

/** @brief Utility functions for the RageSurfaces. */
namespace RageSurfaceUtils {
bool
SaveBMP(RageSurface* surface, RageFile& f);
};

#endif
