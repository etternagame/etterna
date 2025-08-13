#ifndef DISPLAY_COMMANDS_H
#define DISPLAY_COMMANDS_H

#include "CommandType.h"
#include "DrawMode.h"
#include "MatrixState.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Misc/RageTypes.h"
#include "RenderState.h"

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

struct Command
{
    CommandType type;
    union {
        DrawCommand draw;
		RenderState state;
    };

    Command() : type(CommandType::Invalid)
    {
    }

    Command(const Command& rhs)
	{
		type = rhs.type;

		switch (rhs.type)
		{
			case CommandType::Draw: {
				draw = rhs.draw;
				break;
			}
			case CommandType::RenderStateChanged: {
				state = rhs.state;
				break;
			}

			default:
				break;
		}
    }
};

} // namespace Display

#endif
