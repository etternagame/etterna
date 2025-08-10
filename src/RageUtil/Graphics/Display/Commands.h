#ifndef DISPLAY_COMMANDS_H
#define DISPLAY_COMMANDS_H

#include "CommandType.h"
#include "DrawMode.h"
#include "MatrixState.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Misc/RageTypes.h"

namespace Display
{
struct Command
{
    CommandType type;
    int32_t sizeInBytes;
    union {
        CullMode cullMode;
        ZTestMode zTestMode;
        BlendMode blendMode;
        float zBias;
        bool zWrite;
        bool alphaTest;
        struct
        {
            TextureUnit handle;
            intptr_t value;
        } texture;
        DrawMode drawMode;
    };
};

struct DrawCommand
{
    Command command;
    MatrixState matrices;
    RageSpriteVertex verts[1];
};
} // namespace Display

#endif
