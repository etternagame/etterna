#include "Etterna/Globals/global.h"
#include "RageTextureID.h"
#include "RageTextureManager.h"
#include "RageUtil/Utils/RageUtil.h"

void
RageTextureID::Init()
{
	iMaxSize = 2048;
	bMipMaps = false; // Most sprites (especially text) look worse with mip maps
	iAlphaBits = 4;
	iGrayscaleBits = -1;
	bDither = false;
	bStretch = false;
	iColorDepth = -1; // default
	bHotPinkColorKey = false;
	AdditionalTextureHints = "";
	Policy = TEXTUREMAN->GetDefaultTexturePolicy();
}

void
RageTextureID::SetFilename(const std::string& fn)
{
	filename = fn;
	CollapsePath(filename);
}
