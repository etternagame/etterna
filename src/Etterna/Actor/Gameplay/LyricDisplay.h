#ifndef LYRIC_DISPLAY_H
#define LYRIC_DISPLAY_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/BitmapText.h"
/** @brief Displays lyrics along with the song on Gameplay. */
class LyricDisplay : public ActorFrame
{
  public:
	LyricDisplay();
	void Update(float fDeltaTime) override;

	// Call when song changes:
	void Init();

	// Call on song failed:
	void Stop();

  private:
	BitmapText m_textLyrics[2];
	unsigned m_iCurLyricNumber;
	float m_fLastSecond;
	bool m_bStopped;
};

#endif
