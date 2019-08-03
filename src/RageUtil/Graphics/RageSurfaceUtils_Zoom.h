#ifndef RAGE_SURFACE_UTILS_ZOOM_H
#define RAGE_SURFACE_UTILS_ZOOM_H

struct RageSurface;

/** @brief Utility functions for the RageSurfaces. */
namespace RageSurfaceUtils {
void
Zoom(RageSurface*& src, int width, int height);
};

#endif
