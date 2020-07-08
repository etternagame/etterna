#ifndef FOREGROUND_H
#define FOREGROUND_H

#include "Etterna/Actor/Base/ActorFrame.h"

class Song;

/** @brief Foreground in front of notes while playing. */
class Foreground : public ActorFrame
{
  public:
	~Foreground() override;
	void Unload();
	void LoadFromSong(const Song* pSong);

	void Update(float fDeltaTime) override;
	void HandleMessage(const Message& msg) override;

  protected:
	struct LoadedBGA
	{
		Actor* m_bga;
		float m_fStartBeat;
		float m_fStopBeat;
		bool m_bFinished;
	};

	vector<LoadedBGA> m_BGAnimations;
	float m_fLastMusicSeconds = 0.F;
	const Song* m_pSong;
};

#endif
