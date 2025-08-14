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
    MatrixState matrices;
    const RageSpriteVertex *vertex;
    size_t vertexCount;
	size_t renderStateIndex;
};

} // namespace Display

#endif
