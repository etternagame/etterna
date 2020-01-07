#ifndef MOVIE_TEXTURE_H
#define MOVIE_TEXTURE_H

#include "RageUtil/Graphics/RageTexture.h"
#include "arch/RageDriver.h"
#include <map>

void
ForceToAscii(RString& str);

class RageMovieTexture : public RageTexture
{
  public:
	static RageMovieTexture* Create(const RageTextureID& ID);

	RageMovieTexture(const RageTextureID& ID)
	  : RageTexture(ID)
	{
	}
	~RageMovieTexture() override = default;
	void Update(float /* fDeltaTime */) override {}

	void Reload() override = 0;

	void SetPosition(float fSeconds) override = 0;
	void SetPlaybackRate(float fRate) override = 0;
	void SetLooping(bool = true) override {}

	bool IsAMovie() const override { return true; }

	static bool GetFourCC(const RString& fn, RString& handler, RString& type);
};

class RageMovieTextureDriver : public RageDriver
{
  public:
	~RageMovieTextureDriver() override = default;
	virtual RageMovieTexture* Create(const RageTextureID& ID,
									 RString& sError) = 0;
	static DriverList m_pDriverList;
};

#define REGISTER_MOVIE_TEXTURE_CLASS(name)                                     \
	static RegisterRageDriver register_##name(                                 \
	  &RageMovieTextureDriver::m_pDriverList,                                  \
	  #name,                                                                   \
	  CreateClass<RageMovieTextureDriver_##name, RageDriver>)

#endif
