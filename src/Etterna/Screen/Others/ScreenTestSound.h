#ifndef SCREEN_TEST_SOUND_H
#define SCREEN_TEST_SOUND_H

#include "Etterna/Actor/Base/BitmapText.h"
#include "RageUtil/Sound/RageSound.h"
#include "Screen.h"

/** @brief The number of sounds allowed for testing. */
const int nsounds = 5;

class ScreenTestSound : public Screen
{
  public:
	void Init() override;
	~ScreenTestSound() override;

	bool Input(const InputEventPlus& input) override;

	void Update(float f) override;
	void UpdateText(int n);

	struct Sound
	{
		RageSound s;
		BitmapText txt;
	};
	Sound s[nsounds];
	vector<RageSound*> m_sSoundCopies[nsounds];
	BitmapText HEEEEEEEEELP;

	int selected;
};

#endif
