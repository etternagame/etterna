#ifndef DISPLAY_TEXTURE_COMMAND_H
#define DISPLAY_TEXTURE_COMMAND_H

#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Graphics/RageSurface.h"
#include <cstdint>
#include <variant>

namespace Display
{

struct TextureCreationCommand
{
    RagePixelFormat pixfmt;
    RageSurface *img;
    bool bGenerateMipMaps;
};

struct TextureUpdateCommand
{
    intptr_t uTexHandle;
    RageSurface *img;
    int xoffset;
    int yoffset;
    int width;
    int height;
};

struct TextureDeletionCommand
{
    intptr_t textureHandle;
};

struct TextureClearAllCommand
{
};

enum TextureCommandType
{
    Creation,
    Update,
    Deletion,
    ClearAll,
};

using TextureCommand =
    std::variant<TextureCreationCommand, TextureUpdateCommand, TextureDeletionCommand, TextureClearAllCommand>;
} // namespace Display

#endif
