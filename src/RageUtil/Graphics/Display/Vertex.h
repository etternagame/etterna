#include <RageUtil/Misc/RageTypes.h>
#ifndef DISPLAY_VERTEX_H
#define DISPLAY_VERTEX_H

namespace DisplayAdapter {

constexpr size_t MaxVertexCount = 400'000;

struct Vertex
{
	RageSpriteVertex VertexData;
	uint32_t MatrixIndex;
	uint32_t TextureIndex;
	uint32_t SamplerIndex;
};
}

#endif
