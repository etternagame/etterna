#include "Etterna/Globals/global.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Core/Services/Locator.hpp"
#include "MovieTexture_Null.h"
#include "RageUtil/Graphics/RageSurface.h"

class MovieTexture_Null : public RageMovieTexture
{
  public:
	MovieTexture_Null(RageTextureID ID);
	virtual ~MovieTexture_Null();
	void Invalidate() { texHandle = 0; }
	intptr_t GetTexHandle() const { return texHandle; }
	void Update(float /* delta */) {}
	void Reload() {}
	void SetPosition(float /* seconds */) {}
	void SetPlaybackRate(float) {}
	void SetLooping(bool looping = true) { loop = looping; }

  private:
	bool loop = false;
	intptr_t texHandle;
};

MovieTexture_Null::MovieTexture_Null(RageTextureID ID)
  : RageMovieTexture(ID)
{
	Locator::getLogger()->trace("MovieTexture_Null::MovieTexture_Null(ID)");
	texHandle = 0;

	RageTextureID actualID = GetID();

	actualID.iAlphaBits = 0;
	int size = 64;
	m_iSourceWidth = size;
	m_iSourceHeight = size;
	m_iImageWidth = size;
	m_iImageHeight = size;
	m_iTextureWidth = power_of_two(size);
	m_iTextureHeight = m_iTextureWidth;
	m_iFramesWide = 1;
	m_iFramesHigh = 1;

	CreateFrameRects();

	RagePixelFormat pixfmt = RagePixelFormat_RGBA4;
	if (!DISPLAY->SupportsTextureFormat(pixfmt))
		pixfmt = RagePixelFormat_RGBA8;
	ASSERT(DISPLAY->SupportsTextureFormat(pixfmt));

	const RageDisplay::RagePixelFormatDesc* pfd =
	  DISPLAY->GetPixelFormatDesc(pixfmt);
	RageSurface* img = CreateSurface(size,
									 size,
									 pfd->bpp,
									 pfd->masks[0],
									 pfd->masks[1],
									 pfd->masks[2],
									 pfd->masks[3]);
	memset(img->pixels, 0, img->pitch * img->h);

	texHandle = DISPLAY->CreateTexture(pixfmt, img, false);

	delete img;
}

MovieTexture_Null::~MovieTexture_Null()
{
	DISPLAY->DeleteTexture(texHandle);
}

REGISTER_MOVIE_TEXTURE_CLASS(Null);

std::shared_ptr<RageMovieTexture>
RageMovieTextureDriver_Null::Create(const RageTextureID& ID,
									std::string& sError)
{
	return std::make_shared<MovieTexture_Null>(ID);
}
