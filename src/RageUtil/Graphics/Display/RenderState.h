#ifndef DISPLAY_RENDER_STATE_H
#define DISPLAY_RENDER_STATE_H

namespace Display {

struct RenderState
{
	CullMode cullMode;
	ZTestMode zTestMode;
	BlendMode blendMode;
	float zBias;
	bool zWrite;
	bool alphaTest;
	bool textureWrapping[NUM_TextureUnit];
	bool textureFiltering[NUM_TextureUnit];
	uint8_t textureMode[NUM_TextureUnit];
	intptr_t textures[NUM_TextureUnit];
};

}

#endif