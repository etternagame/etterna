#ifndef RAGE_SURFACE_UTILS_DITHER_H
#define RAGE_SURFACE_UTILS_DITHER_H

struct RageSurface;

/** @brief Utility functions for the RageSurfaces. */
namespace RageSurfaceUtils {
void
OrderedDither(const RageSurface* src, RageSurface* dst);
void
ErrorDiffusionDither(const RageSurface* src, RageSurface* dst);
};

#endif
