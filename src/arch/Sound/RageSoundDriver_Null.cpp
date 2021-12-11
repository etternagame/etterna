#include "Etterna/Globals/global.h"
#include "RageSoundDriver_Null.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/PrefsManager.h"

REGISTER_SOUND_DRIVER_CLASS(Null);

const int channels = 2;

void
RageSoundDriver_Null::Update()
{
	/* "Play" frames. */
	while (m_iLastCursorPos < GetPosition() + 1024 * 4) {
		int16_t buf[256 * channels];
		this->Mix(buf, 256, m_iLastCursorPos, GetPosition());
		m_iLastCursorPos += 256;
	}

	RageSoundDriver::Update();
}

int64_t
RageSoundDriver_Null::GetPosition() const
{
	return int64_t(RageTimer::GetTimeSinceStart() * m_iSampleRate);
}

RageSoundDriver_Null::RageSoundDriver_Null()
{
	m_iSampleRate = PREFSMAN->m_iSoundPreferredSampleRate;
	if (m_iSampleRate == 0)
		m_iSampleRate = 44100;
	m_iLastCursorPos = GetPosition();
	StartDecodeThread();
}

int
RageSoundDriver_Null::GetSampleRate() const
{
	return m_iSampleRate;
}
