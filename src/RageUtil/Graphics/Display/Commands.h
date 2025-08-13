#ifndef DISPLAY_COMMANDS_H
#define DISPLAY_COMMANDS_H

#include "CommandType.h"
#include "DrawMode.h"
#include "MatrixState.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Misc/RageTypes.h"
#include <variant>

namespace Display
{
struct CullCommand
{
    CommandType type;
    CullMode cullMode;
};

struct ZTestModeCommand
{
    CommandType type;
    ZTestMode zTestMode;
};

struct BlendModeCommand
{
    CommandType type;
    BlendMode blendMode;
};

struct ZBiasCommand
{
    CommandType type;
    float zBias;
};

struct ZWriteCommand
{
    CommandType type;
    bool zWrite;
};

struct AlphaTestCommand
{
    CommandType type;
    bool alphaTest;
};

struct TextureUnitCommand
{
    CommandType type;
    TextureUnit handle;
    intptr_t value;
};

struct DrawCommand
{
    CommandType type;
    DrawMode drawMode;
    MatrixState matrices;
    const RageSpriteVertex *vertex;
    size_t vertexCount;
};

struct ClearZBufferCommand
{
	CommandType type;
};

/*
Get low (Get low), get low (Get low, yay, yay)
To the windoooooooooooooooooooooooow (To the window)
To the waaaaaaaaaaaaaaaaaaaaaaaaaall (To the wall)
TODO: hope this is not a performance concern :^)
*/
using Command = std::variant<CullCommand, ZTestModeCommand, BlendModeCommand, ZBiasCommand, ZWriteCommand,
                             AlphaTestCommand, TextureUnitCommand, ClearZBufferCommand, DrawCommand>;

// should match the ordering in Command's std::variant
enum class CommandIndex
{
    Cull,
    ZTestMode,
    BlendMode,
    ZBias,
    ZWrite,
    AlphaTest,
    TextureUnit,
    ClearZBuffer,
    Draw,
};

} // namespace Display

#endif
