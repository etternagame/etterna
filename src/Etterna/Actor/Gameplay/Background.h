#ifndef BACKGROUND_H
#define BACKGROUND_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Misc/BackgroundUtil.h"

class Song;
class BackgroundImpl;
/** @brief the Background that is behind the notes while playing. */
class Background : public ActorFrame
{
  public:
	Background();
	~Background() override;
	void Init();

	virtual void LoadFromSong(const Song* pSong);
	virtual void Unload();

	void FadeToActualBrightness();
	void SetBrightness(float fBrightness); // overrides pref and Cover

	// One more piece of the puzzle that puts the notefield board above the bg
	// and under everything else.  m_disable_draw exists so that
	// ScreenGameplay can draw the background manually, and still have it as a
	// child. -Kyz
	bool m_disable_draw;
	bool EarlyAbortDraw() const override { return m_disable_draw; }

	void GetLoadedBackgroundChanges(
	  vector<BackgroundChange>** pBackgroundChangesOut);

  protected:
	BackgroundImpl* m_pImpl;
};

#endif
