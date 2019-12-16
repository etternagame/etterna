#ifndef RAGE_SOUND_NULL
#define RAGE_SOUND_NULL

#include "RageSoundDriver.h"

class RageSoundDriver_Null : public RageSoundDriver
{
  public:
	RageSoundDriver_Null();
	int64_t GetPosition() const;
	int GetSampleRate() const;
	void Update();

  private:
	int64_t m_iLastCursorPos;
	int m_iSampleRate;
};
#define USE_RAGE_SOUND_NULL

#endif
