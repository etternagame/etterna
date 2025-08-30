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

} // namespace Display

#endif
