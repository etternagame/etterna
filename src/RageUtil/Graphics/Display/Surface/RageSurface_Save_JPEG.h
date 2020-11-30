/* RageSurface_Save_JPEG - Save a RageSurface to a JPEG. */

#pragma once

struct RageSurface;
class RageFile;

/** @brief Utility functions for the RageSurfaces. */
namespace RageSurfaceUtils {
bool
SaveJPEG(RageSurface* surface, RageFile& f, bool bHighQual = true);
};
