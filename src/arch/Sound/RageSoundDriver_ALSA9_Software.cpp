#include "Etterna/Globals/global.h"
#include "RageSoundDriver_ALSA9_Software.h"

#include "Core/Services/Locator.hpp"
#include "RageUtil/Sound/RageSound.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "ALSA9Dynamic.h"
#include "Etterna/Singletons/PrefsManager.h"

#include "archutils/Unix/GetSysInfo.h"

#include <sys/time.h>
#include <sys/resource.h>

// clang-format off
REGISTER_SOUND_DRIVER_CLASS2(ALSA-sw, ALSA9_Software);
// clang-format on

static const int channels = 2;
static const int samples_per_frame = channels;
static const int bytes_per_frame = sizeof(int16_t) * samples_per_frame;

/* Linux 2.6 has a fine-grained scheduler.  We can almost always use a smaller
 * buffer size than in 2.4.  XXX: Some cards can handle smaller buffer sizes
 * than others. */
static const unsigned g_iMaxWriteahead_linux_26 = 512;
static const unsigned safe_writeahead = 1024 * 4;
static unsigned g_iMaxWriteahead;
const int num_chunks = 8;

int
RageSoundDriver_ALSA9_Software::MixerThread_start(void* p)
{
	((RageSoundDriver_ALSA9_Software*)p)->MixerThread();
	return 0;
}

void
RageSoundDriver_ALSA9_Software::MixerThread()
{
	setpriority(PRIO_PROCESS, 0, -15);

	while (!m_bShutdown) {
		while (!m_bShutdown && GetData())
			;

		m_pPCM->WaitUntilFramesCanBeFilled(100);
	}
}

/* Returns the number of frames processed */
bool
RageSoundDriver_ALSA9_Software::GetData()
{
	const int frames_to_fill = m_pPCM->GetNumFramesToFill();
	if (frames_to_fill <= 0)
		return false;

	static int16_t* buf = NULL;
	static int bufsize = 0;
	if (buf && bufsize < frames_to_fill) {
		delete[] buf;
		buf = NULL;
	}
	if (!buf) {
		buf = new int16_t[frames_to_fill * samples_per_frame];
		bufsize = frames_to_fill;
	}

	const int64_t play_pos = m_pPCM->GetPlayPos();
	const int64_t cur_play_pos = m_pPCM->GetPosition();

	this->Mix(buf, frames_to_fill, play_pos, cur_play_pos);
	m_pPCM->Write(buf, frames_to_fill);

	return true;
}

int64_t
RageSoundDriver_ALSA9_Software::GetPosition() const
{
	return m_pPCM->GetPosition();
}

void
RageSoundDriver_ALSA9_Software::SetupDecodingThread()
{
	setpriority(PRIO_PROCESS, 0, -5);
}

RageSoundDriver_ALSA9_Software::RageSoundDriver_ALSA9_Software()
{
	m_pPCM = NULL;
	m_bShutdown = false;
	m_iSampleRate = 44100;
}

std::string
RageSoundDriver_ALSA9_Software::Init()
{
	std::string sError = LoadALSA();
	if (sError != "")
		return ssprintf("Driver unusable: %s", sError.c_str());

	g_iMaxWriteahead = safe_writeahead;
	std::string sys;
	int vers;
	GetKernel(sys, vers);
	Locator::getLogger()->trace("OS: {} ver {}", sys.c_str(), vers);
	if (sys == "Linux" && vers >= 20600)
		g_iMaxWriteahead = g_iMaxWriteahead_linux_26;

	if (PREFSMAN->m_iSoundWriteAhead)
		g_iMaxWriteahead = PREFSMAN->m_iSoundWriteAhead;

	m_pPCM = new Alsa9Buf();
	sError = m_pPCM->Init(channels,
						  g_iMaxWriteahead,
						  g_iMaxWriteahead / num_chunks,
						  PREFSMAN->m_iSoundPreferredSampleRate);
	if (sError != "")
		return sError;

	m_iSampleRate = m_pPCM->GetSampleRate();

	StartDecodeThread();

	m_MixingThread.SetName("RageSoundDriver_ALSA9_Software");
	m_MixingThread.Create(MixerThread_start, this);

	return "";
}

RageSoundDriver_ALSA9_Software::~RageSoundDriver_ALSA9_Software()
{
	if (m_MixingThread.IsCreated()) {
		/* Signal the mixing thread to quit. */
		m_bShutdown = true;
		Locator::getLogger()->trace("Shutting down mixer thread ...");
		m_MixingThread.Wait();
		Locator::getLogger()->trace("Mixer thread shut down.");
	}

	delete m_pPCM;

	UnloadALSA();
}

float
RageSoundDriver_ALSA9_Software::GetPlayLatency() const
{
	return float(g_iMaxWriteahead) / m_iSampleRate;
}
