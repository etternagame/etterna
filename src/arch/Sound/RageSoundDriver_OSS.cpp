#include "Etterna/Globals/global.h"
#include "RageSoundDriver_OSS.h"

#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Sound/RageSound.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "RageUtil/Utils/RageUtil.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/select.h>

REGISTER_SOUND_DRIVER_CLASS(OSS);

#if !defined(SNDCTL_DSP_SPEED)
#define SNDCTL_DSP_SPEED SOUND_PCM_WRITE_RATE
#endif

/* samples */
const int channels = 2;
const int bytes_per_frame = channels * 2; /* 16-bit */
const int chunk_order = 12;
const int num_chunks = 4;
const int buffersize = num_chunks * (1 << (chunk_order - 1)); /* in bytes */
const int buffersize_frames = buffersize / bytes_per_frame;   /* in frames */

int
RageSoundDriver_OSS::MixerThread_start(void* p)
{
	((RageSoundDriver_OSS*)p)->MixerThread();
	return 0;
}

void
RageSoundDriver_OSS::MixerThread()
{
	/* We want to set a higher priority, but Unix only lets root renice
	 * < 0, which is silly.  Give it a try, anyway. */
	int status = nice(-10);
	if (status != -1)
		LOG->Trace("Set MixerThread nice value to %d", status);

	while (!shutdown) {
		while (GetData())
			;

		fd_set f;
		FD_ZERO(&f);
		FD_SET(fd, &f);

		usleep(10000);

		struct timeval tv = { 0, 10000 };
		select(fd + 1, NULL, &f, NULL, &tv);
	}
}

void
RageSoundDriver_OSS::SetupDecodingThread()
{
	int status = nice(-5);
	if (status != -1)
		LOG->Trace("Set DecodingThread nice value to %d", status);
}

bool
RageSoundDriver_OSS::GetData()
{
	/* Look for a free buffer. */
	audio_buf_info ab;
	if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &ab) == -1)
		FAIL_M(ssprintf("ioctl(SNDCTL_DSP_GETOSPACE): %s", strerror(errno)));

	if (!ab.fragments)
		return false;

	const int chunksize = ab.fragsize;

	static int16_t* buf = NULL;
	if (!buf)
		buf = new int16_t[chunksize / sizeof(int16_t)];

	this->Mix(buf, chunksize / bytes_per_frame, last_cursor_pos, GetPosition());

	int wrote = write(fd, buf, chunksize);
	if (wrote != chunksize)
		FAIL_M(ssprintf(
		  "write didn't: %i (%s)", wrote, wrote == -1 ? strerror(errno) : ""));

	/* Increment last_cursor_pos. */
	last_cursor_pos += chunksize / bytes_per_frame;

	return true;
}

/* XXX: There's a race on last_cursor_pos here: new data might be written after
 * the ioctl returns, incrementing last_cursor_pos. */
int64_t
RageSoundDriver_OSS::GetPosition() const
{
	ASSERT(fd != -1);

	int delay;
	if (ioctl(fd, SNDCTL_DSP_GETODELAY, &delay) == -1)
		FAIL_M(ssprintf("RageSoundDriver_OSS: ioctl(SNDCTL_DSP_GETODELAY): %s",
						strerror(errno)));

	return last_cursor_pos - (delay / bytes_per_frame);
}

RString
RageSoundDriver_OSS::CheckOSSVersion(int fd)
{
	int version = 0;

#if defined(HAVE_OSS_GETVERSION)
	if (ioctl(fd, OSS_GETVERSION, &version) != 0) {
		LOG->Warn("OSS_GETVERSION failed: %s", strerror(errno));
		version = 0;
	}
#endif

	/*
	 * Find out if /dev/dsp is really ALSA emulating it.  ALSA's OSS
	 * emulation has been buggy.  If we got here, we probably failed to init
	 * ALSA.  The only case I've seen of this so far was not having access
	 * to /dev/snd devices.
	 */
	/* Reliable but only too recently available:
	if (ioctl(fd, OSS_ALSAEMULVER, &ver) == 0 && ver ) */

	/*
	 * Ack.  We can't just check for /proc/asound, since a few systems have
	 * ALSA loaded but actually use OSS.  ALSA returns a specific version;
	 * check that, too.  It looks like that version is potentially a valid
	 * OSS version, so check both.
	 */
#ifndef FORCE_OSS
#define ALSA_SNDRV_OSS_VERSION ((3 << 16) | (8 << 8) | (1 << 4) | (0))
	if (version == ALSA_SNDRV_OSS_VERSION &&
		IsADirectory("/rootfs/proc/asound"))
		return "RageSoundDriver_OSS: ALSA detected.  ALSA OSS emulation is "
			   "buggy; use ALSA natively.";
#endif
	if (version) {
		int major, minor, rev;
		if (version < 361) {
			major = (version / 100) % 10;
			minor = (version / 10) % 10;
			rev = (version / 1) % 10;
		} else {
			major = (version / 0x10000) % 0x100;
			minor = (version / 0x00100) % 0x100;
			rev = (version / 0x00001) % 0x100;
		}

		LOG->Info("OSS: %i.%i.%i", major, minor, rev);
	}

	return "";
}

RageSoundDriver_OSS::RageSoundDriver_OSS()
{
	fd = -1;
	shutdown = false;
	last_cursor_pos = 0;
}

RString
RageSoundDriver_OSS::Init()
{
	fd = open("/dev/dsp", O_WRONLY | O_NONBLOCK);
	if (fd == -1)
		return ssprintf("RageSoundDriver_OSS: Couldn't open /dev/dsp: %s",
						strerror(errno));

	RString sError = CheckOSSVersion(fd);
	if (sError != "")
		return sError;

	int i = AFMT_S16_LE;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &i) == -1)
		return ssprintf("RageSoundDriver_OSS: ioctl(SNDCTL_DSP_SETFMT, %i): %s",
						i,
						strerror(errno));
	if (i != AFMT_S16_LE)
		return ssprintf("RageSoundDriver_OSS: Wanted format %i, got %i instead",
						AFMT_S16_LE,
						i);

	i = channels;
	if (ioctl(fd, SNDCTL_DSP_CHANNELS, &i) == -1)
		return ssprintf(
		  "RageSoundDriver_OSS: ioctl(SNDCTL_DSP_CHANNELS, %i): %s",
		  i,
		  strerror(errno));
	if (i != channels)
		return ssprintf(
		  "RageSoundDriver_OSS: Wanted %i channels, got %i instead",
		  channels,
		  i);

	i = 44100;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &i) == -1)
		return ssprintf("RageSoundDriver_OSS: ioctl(SNDCTL_DSP_SPEED, %i): %s",
						i,
						strerror(errno));
	samplerate = i;
	LOG->Trace("RageSoundDriver_OSS: sample rate %i", samplerate);
	i = (num_chunks << 16) + chunk_order;
	if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &i) == -1)
		return ssprintf(
		  "RageSoundDriver_OSS: ioctl(SNDCTL_DSP_SETFRAGMENT, %i): %s",
		  i,
		  strerror(errno));
	StartDecodeThread();

	MixingThread.SetName("RageSoundDriver_OSS");
	MixingThread.Create(MixerThread_start, this);

	return "";
}

RageSoundDriver_OSS::~RageSoundDriver_OSS()
{
	if (MixingThread.IsCreated()) {
		/* Signal the mixing thread to quit. */
		shutdown = true;
		LOG->Trace("Shutting down mixer thread ...");
		MixingThread.Wait();
		LOG->Trace("Mixer thread shut down.");
	}

	if (fd != -1)
		close(fd);
}

float
RageSoundDriver_OSS::GetPlayLatency() const
{
	return 0; // (1.0f / samplerate) * (buffersize_frames - chunksize_frames);
}
