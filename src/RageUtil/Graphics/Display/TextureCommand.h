#ifndef DISPLAY_TEXTURE_COMMAND_H
#define DISPLAY_TEXTURE_COMMAND_H

#include <cstdint>
#include <variant>
#include "RageUtil/Graphics/RageSurface.h"
#include "RageUtil/Graphics/RageDisplay.h"

namespace Display {

struct TextureCreationCommand
{
	RagePixelFormat pixfmt;
	RageSurface* img;
	bool bGenerateMipMaps;
};

struct TextureUpdateCommand
{
	intptr_t uTexHandle;
	RageSurface* img;
	int xoffset;
	int yoffset;
	int width;
	int height;
};

struct TextureDeletionCommand
{
	intptr_t textureHandle;
};

using TextureCommand = std::
  variant<TextureCreationCommand, TextureUpdateCommand, TextureDeletionCommand>;
}

#endif