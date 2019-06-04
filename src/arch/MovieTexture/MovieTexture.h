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

/*
 * (c) 2003-2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
