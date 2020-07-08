/* MovieTexture_DShow - DirectShow movie renderer. */

#ifndef RAGE_MOVIE_TEXTURE_DSHOW_H
#define RAGE_MOVIE_TEXTURE_DSHOW_H

#include "MovieTexture.h"

/* Don't know why we need this for the headers ... */
typedef char TCHAR, *PTCHAR;

/* Prevent these from using Dbg stuff, which we don't link in. */
#ifdef DEBUG
#undef DEBUG
#undef _DEBUG
#define GIVE_BACK_DEBUG
#endif

#include <atlbase.h>

#ifdef GIVE_BACK_DEBUG
#undef GIVE_BACK_DEBUG
#define _DEBUG
#define DEBUG
#endif

#include "baseclasses/streams.h"

#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Graphics/RageTexture.h"
#include "RageUtil/Misc/RageThreads.h"

class MovieTexture_DShow : public RageMovieTexture
{
  public:
	MovieTexture_DShow(RageTextureID ID);
	virtual ~MovieTexture_DShow();
	std::string Init();

	/* only called by RageTextureManager::InvalidateTextures */
	void Invalidate() { m_uTexHandle = 0; }
	void Update(float fDeltaTime);

	virtual void Reload();

	virtual void Play();
	virtual void Pause();
	virtual void SetPosition(float fSeconds);
	virtual void SetPlaybackRate(float fRate);

	void SetLooping(bool bLooping = true) { m_bLoop = bLooping; }

	void NewData(const char* pBuffer);

  private:
	const char* buffer;
	RageSemaphore buffer_lock, buffer_finished;

	std::string Create();

	void CreateTexture();
	void SkipUpdates();
	void StopSkippingUpdates();
	void CheckFrame();
	std::string GetActiveFilterList();

	unsigned GetTexHandle() const { return m_uTexHandle; }
	unsigned m_uTexHandle;

	CComPtr<IGraphBuilder> m_pGB; // GraphBuilder
	bool m_bLoop;
	bool m_bPlaying;
};

class RageMovieTextureDriver_DShow : public RageMovieTextureDriver
{
  public:
	virtual RageMovieTexture* Create(RageTextureID ID, std::string& sError);
};

#endif
