/* RageSpriteDrawing - information/data necessary to draw RageSpriteVertex objects */
#ifndef RAGE_SPRITE_DRAWING_H
#define RAGE_SPRITE_DRAWING_H

#include "RageUtil/Misc/RageTypes.h"
#include <vector>

// todo: commands, textures, vertices, uniforms, ?shader references?, compiled geometries, whatever else
struct RageSpriteDrawing
{
	const std::vector<RageSpriteVertex> v;
};

#endif
