#ifndef DISPLAY_DRAW_COMMAND_H
#define DISPLAY_DRAW_COMMAND_H

#include "DrawMode.h"
#include "MatrixState.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Misc/RageTypes.h"

namespace Display
{

struct DrawCommand
{
	uint32_t VertexCountPerInstance;
	uint32_t InstanceCount;
	uint32_t StartVertexLocation;
	uint32_t StartInstanceLocation;
};

struct DrawCommandArgument
{
	uint32_t matrixStateIndex;
	uint32_t renderStateIndex;
};

#pragma pack(push, 4)
struct IndirectCommand
{
	DrawCommandArgument args;
	DrawCommand draw;
};
#pragma pack(pop)
static_assert(
  sizeof(IndirectCommand) == 24,
  "IndirectCommand size should match the HLSL compute shader definition");

} // namespace Display

#endif
