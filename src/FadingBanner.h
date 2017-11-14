/* FadingBanner - Fades between two banners. */

#ifndef FADING_BANNER_H
#define FADING_BANNER_H

#include "Banner.h"
#include "ActorFrame.h"
#include "RageTimer.h"

class FadingBanner : public ActorFrame
{
public:
	FadingBanner();
	FadingBanner *Copy() const override;

	void ScaleToClipped( float fWidth, float fHeight );

	/* If you previously loaded a cached banner, and are now loading the full-
	 * resolution banner, set bLowResToHighRes to true. */
	void Load( const RageTextureID &ID, bool bLowResToHighRes=false );
	void LoadFromSong( const Song* pSong );		// NULL means no song
	void LoadMode();
	void LoadFromSongGroup( const RString &sSongGroup );
	void LoadIconFromCharacter( Character* pCharacter );
	void LoadRoulette();
	void LoadRandom();
	void LoadFromSortOrder( SortOrder so );
	void LoadFallback();
	void LoadCustom( const RString &sBanner );

	bool LoadFromCachedBanner( const RString &path );
	bool LoadFromCachedBackground( const RString &path );

	void SetMovingFast( bool fast ) { m_bMovingFast=fast; }
	void UpdateInternal( float fDeltaTime ) override;
	void DrawPrimitives() override;

	int GetLatestIndex(){ return m_iIndexLatest; }
	Banner GetBanner(int i){ return m_Banner[i]; }

	// Lua
	void PushSelf( lua_State *L ) override;

protected:
	void BeforeChange( bool bLowResToHighRes=false );

	static const int NUM_BANNERS = 5;
	Banner	m_Banner[NUM_BANNERS];
	int		m_iIndexLatest;

	bool	m_bMovingFast;
	bool	m_bSkipNextBannerUpdate;
};

#endif

/*
 * (c) 2001-2004 Chris Danford
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
