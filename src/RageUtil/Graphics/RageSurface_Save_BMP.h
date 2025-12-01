/* RageSurface_Save_BMP - Save a RageSurface to a BMP. */

#pragma once

struct RageSurface;
class RageFile;

/** @brief Utility functions for the RageSurfaces. */
namespace RageSurfaceUtils {
bool
SaveBMP(RageSurface* surface, RageFile& f);
};
