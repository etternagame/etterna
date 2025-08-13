#include "RenderState.h"
#include <algorithm>
#include <ranges>

bool Display::RenderState::operator==(RenderState &rhs)
{
    // clang-format off
	return
		this->cullMode == rhs.cullMode &&
		this->zTestMode == rhs.zTestMode &&
		this->blendMode == rhs.blendMode &&
        std::fabsf(this->zBias - rhs.zBias) < 1e-6f &&
		this->zWrite == rhs.zWrite &&
		this->alphaTest == rhs.alphaTest &&
		std::ranges::equal(this->textureWrapping, rhs.textureWrapping) &&
        std::ranges::equal(this->textureFiltering, rhs.textureFiltering) &&
        std::ranges::equal(this->textureMode, rhs.textureMode) &&
		std::ranges::equal(this->textures, rhs.textures);
    // clang-format on
}
