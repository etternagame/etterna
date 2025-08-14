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
    DrawMode drawMode;
    MatrixState matrixState;
	uint32_t vertexOffset;
	uint32_t vertexCount;
	uint32_t renderStateIndex;
};

} // namespace Display

#endif
