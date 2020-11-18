#include "Etterna/Globals/global.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "ALSA9Helpers.h"
#include "ALSA9Dynamic.h"
#include "Etterna/Singletons/PrefsManager.h"

#include <algorithm>

/* int err; must be defined before using this macro */
#define ALSA_CHECK(x)                                                          \
	if (err < 0) {                                                             \
		Locator::getLogger()->info("ALSA: {}: {}", x, dsnd_strerror(err));                      \
		return false;                                                          \
	}
#define ALSA_ASSERT(x)                                                         \
	if (err < 0) {                                                             \
		Locator::getLogger()->warn("ALSA: {}: {}", x, dsnd_strerror(err));                      \
	}

bool
Alsa9Buf::SetHWParams()
{
	int err;

	if (dsnd_pcm_state(pcm) == SND_PCM_STATE_PREPARED)
		dsnd_pcm_drop(pcm);

	if (dsnd_pcm_state(pcm) != SND_PCM_STATE_OPEN) {
		/* Reset the stream to SND_PCM_STATE_OPEN. */
		err = dsnd_pcm_hw_free(pcm);
		ALSA_ASSERT("dsnd_pcm_hw_free");
	}
	//	ASSERT_M( dsnd_pcm_state(pcm) == SND_PCM_STATE_OPEN, ssprintf("(%s)",
	// dsnd_pcm_state_name(dsnd_pcm_state(pcm))) );

	/* allocate the hardware parameters structure */
	snd_pcm_hw_params_t* hwparams;
	dsnd_pcm_hw_params_alloca(&hwparams);

	err = dsnd_pcm_hw_params_any(pcm, hwparams);
	ALSA_CHECK("dsnd_pcm_hw_params_any");

	/* Set to interleaved mmap mode. */
	err = dsnd_pcm_hw_params_set_access(
	  pcm, hwparams, SND_PCM_ACCESS_MMAP_INTERLEAVED);
	ALSA_CHECK("dsnd_pcm_hw_params_set_access");

	/* Set the PCM format: signed 16bit, native endian. */
	err = dsnd_pcm_hw_params_set_format(pcm, hwparams, SND_PCM_FORMAT_S16);
	ALSA_CHECK("dsnd_pcm_hw_params_set_format");

	/* Set the number of channels. */
	err = dsnd_pcm_hw_params_set_channels(pcm, hwparams, 2);
	ALSA_CHECK("dsnd_pcm_hw_params_set_channels");

	/* Set the sample rate. */
	err = dsnd_pcm_hw_params_set_rate_near(pcm, hwparams, &samplerate, 0);
	ALSA_CHECK("dsnd_pcm_hw_params_set_rate_near");

	/* Set the buffersize to the writeahead, and then copy back the actual value
	 * we got. */
	writeahead = preferred_writeahead;
	err = dsnd_pcm_hw_params_set_buffer_size_near(pcm, hwparams, &writeahead);
	ALSA_CHECK("dsnd_pcm_hw_params_set_buffer_size_near");

	/* The period size is roughly equivalent to what we call the chunksize. */
	int dir = 0;
	chunksize = preferred_chunksize;
	err =
	  dsnd_pcm_hw_params_set_period_size_near(pcm, hwparams, &chunksize, &dir);
	ALSA_CHECK("dsnd_pcm_hw_params_set_period_size_near");

	//	LOG->Info("asked for %i period, got %i", chunksize, period_size);

	/* write the hardware parameters to the device */
	err = dsnd_pcm_hw_params(pcm, hwparams);
	ALSA_CHECK("dsnd_pcm_hw_params");

	return true;
}

bool
Alsa9Buf::SetSWParams()
{
	snd_pcm_sw_params_t* swparams;
	dsnd_pcm_sw_params_alloca(&swparams);
	dsnd_pcm_sw_params_current(pcm, swparams);

	int err = dsnd_pcm_sw_params_set_xfer_align(pcm, swparams, 1);
	ALSA_ASSERT("dsnd_pcm_sw_params_set_xfer_align");

	/* chunksize has been set to the period size.  Set avail_min to the period
	 * size, too, so poll() wakes up once per chunk. */
	err = dsnd_pcm_sw_params_set_avail_min(pcm, swparams, chunksize);
	ALSA_ASSERT("dsnd_pcm_sw_params_set_avail_min");

	/* If this fails, we might have bound dsnd_pcm_sw_params_set_avail_min to
	 * the old SW API. */
	//	ASSERT( err <= 0 );

	/* Disable SND_PCM_STATE_XRUN. */
	snd_pcm_uframes_t boundary = 0;
	err = dsnd_pcm_sw_params_get_boundary(swparams, &boundary);
	ALSA_ASSERT("dsnd_pcm_sw_params_get_boundary");

	err = dsnd_pcm_sw_params_set_stop_threshold(pcm, swparams, boundary);
	ALSA_ASSERT("dsnd_pcm_sw_params_set_stop_threshold");

	err = dsnd_pcm_sw_params(pcm, swparams);
	ALSA_ASSERT("dsnd_pcm_sw_params");

	err = dsnd_pcm_prepare(pcm);
	ALSA_ASSERT("dsnd_pcm_prepare");

	return true;
}

void
Alsa9Buf::ErrorHandler(const char* file,
					   int line,
					   const char* function,
					   int err,
					   const char* fmt,
					   ...)
{
	va_list va;
	va_start(va, fmt);
	std::string str = vssprintf(fmt, va);
	va_end(va);

	if (err)
		str += ssprintf(" (%s)", dsnd_strerror(err));

	/* Annoying: these happen both normally (eg. "out of memory" when allocating
	 * too many PCM slots) and abnormally, and there's no way to tell which is
	 * which.  I don't want to pollute the warning output. */
	Locator::getLogger()->trace("ALSA error: {}:{} {}: {}", file, line, function, str.c_str());
}

void
Alsa9Buf::InitializeErrorHandler()
{
	dsnd_lib_error_set_handler(ErrorHandler);
}

static std::string
DeviceName()
{
	if (!PREFSMAN->m_iSoundDevice.Get().empty())
		return PREFSMAN->m_iSoundDevice;
	return "default";
}

void
Alsa9Buf::GetSoundCardDebugInfo()
{
	static bool done = false;
	if (done)
		return;
	done = true;

	if (DoesFileExist("/rootfs/proc/asound/version")) {
		std::string sVersion;
		GetFileContents("/rootfs/proc/asound/version", sVersion, true);
		Locator::getLogger()->info("ALSA: {}", sVersion.c_str());
	}

	InitializeErrorHandler();

	int card = -1;
	while (dsnd_card_next(&card) >= 0 && card >= 0) {
		const std::string id = ssprintf("hw:%d", card);
		snd_ctl_t* handle;
		int err;
		err = dsnd_ctl_open(&handle, id.c_str(), 0);
		if (err < 0) {
			Locator::getLogger()->info("Couldn't open card #{} (\"{}\") to probe: {}",
					  card, id.c_str(), dsnd_strerror(err));
			continue;
		}

		snd_ctl_card_info_t* info;
		dsnd_ctl_card_info_alloca(&info);
		err = dsnd_ctl_card_info(handle, info);
		if (err < 0) {
			Locator::getLogger()->info("Couldn't get card info for card #{} (\"{}\"): {}",
					  card, id.c_str(), dsnd_strerror(err));
			dsnd_ctl_close(handle);
			continue;
		}

		int dev = -1;
		while (dsnd_ctl_pcm_next_device(handle, &dev) >= 0 && dev >= 0) {
			snd_pcm_info_t* pcminfo;
			dsnd_pcm_info_alloca(&pcminfo);
			dsnd_pcm_info_set_device(pcminfo, dev);
			dsnd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);

			err = dsnd_ctl_pcm_info(handle, pcminfo);
			if (err < 0) {
				if (err != -ENOENT)
					Locator::getLogger()->info("dsnd_ctl_pcm_info({}) ({}) failed: {}",
							  card, id.c_str(), dsnd_strerror(err));
				continue;
			}

			Locator::getLogger()->info("ALSA Driver: {}: {} [{}], device {}: {} [{}], {}/{} "
					  "subdevices avail",
					  card,
					  dsnd_ctl_card_info_get_name(info),
					  dsnd_ctl_card_info_get_id(info),
					  dev,
					  dsnd_pcm_info_get_id(pcminfo),
					  dsnd_pcm_info_get_name(pcminfo),
					  dsnd_pcm_info_get_subdevices_avail(pcminfo),
					  dsnd_pcm_info_get_subdevices_count(pcminfo));
		}
		dsnd_ctl_close(handle);
	}

	if (card == 0)
		Locator::getLogger()->info("No ALSA sound cards were found.");

	if (!PREFSMAN->m_iSoundDevice.Get().empty())
		Locator::getLogger()->info("ALSA device overridden to \"{}\"",
				  PREFSMAN->m_iSoundDevice.Get().c_str());
}

Alsa9Buf::Alsa9Buf()
{
	samplerate = 44100;
	samplebits = 16;
	last_cursor_pos = 0;
	preferred_writeahead = 8192;
	preferred_chunksize = 1024;
	pcm = NULL;
	channels = 1;
	buffersize = 16;
	writeahead = false;
	chunksize = 1024;
}

std::string
Alsa9Buf::Init(int channels_, int iWriteahead, int iChunkSize, int iSampleRate)
{
	channels = channels_;
	preferred_writeahead = iWriteahead;
	preferred_chunksize = iChunkSize;
	if (iSampleRate == 0)
		samplerate = 44100;
	else
		samplerate = iSampleRate;

	GetSoundCardDebugInfo();

	InitializeErrorHandler();

	/* Open the device. */
	int err;
	err = dsnd_pcm_open(
	  &pcm, DeviceName().c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (err < 0)
		return ssprintf(
		  "dsnd_pcm_open(%s): %s", DeviceName().c_str(), dsnd_strerror(err));

	if (!SetHWParams()) {
		return "SetHWParams failed";
	}

	SetSWParams();

	Locator::getLogger()->info("ALSA: Mixing at {}hz", samplerate);

	if (preferred_writeahead != writeahead)
		Locator::getLogger()->info("ALSA: writeahead adjusted from {} to {}",
				  (unsigned)preferred_writeahead,
				  (unsigned)writeahead);
	if (preferred_chunksize != chunksize)
		Locator::getLogger()->info("ALSA: chunksize adjusted from {} to {}",
				  (unsigned)preferred_chunksize,
				  (unsigned)chunksize);

	return "";
}

Alsa9Buf::~Alsa9Buf()
{
	if (pcm != NULL)
		dsnd_pcm_close(pcm);
}

/* Don't fill the buffer any more than than "writeahead" frames.  Prefer to
 * write "chunksize" frames at a time.  (These numbers are hints; if the
 * hardware parameters require it, they can be ignored.) */
int
Alsa9Buf::GetNumFramesToFill()
{
	/* Make sure we can write ahead at least two chunks.  Otherwise, we'll only
	 * fill one chunk ahead, and underrun. */
	int ActualWriteahead = std::max(writeahead, chunksize * 2);

	snd_pcm_sframes_t avail_frames = dsnd_pcm_avail_update(pcm);

	int total_frames = writeahead;
	if (avail_frames > total_frames) {
		/* underrun */
		const int size = avail_frames - total_frames;
		Locator::getLogger()->trace("underrun ({} frames)", size);
		int large_skip_threshold = 2 * samplerate;

		/* For small underruns, ignore them.  We'll return the maximum
		 * writeahead and ALSA will just discard the data.  GetPosition will
		 * return consistent values during this time, so arrows will continue to
		 * scroll smoothly until the music catches up. */
		if (size >= large_skip_threshold) {
			/* It's a large skip.  Catch up.  If we fall too far behind, the
			 * sound thread will be decoding as fast as it can, which will steal
			 * too many cycles from the rendering thread. */
			dsnd_pcm_forward(pcm, size);
		}
	}

	if (avail_frames < 0)
		avail_frames = dsnd_pcm_avail_update(pcm);

	if (avail_frames < 0) {
		Locator::getLogger()->trace("RageSoundDriver_ALSA9::GetData: dsnd_pcm_avail_update: {}",
				   dsnd_strerror(avail_frames));
		return 0;
	}

	/* Number of frames that have data: */
	const snd_pcm_sframes_t filled_frames =
	  std::max(0l, total_frames - avail_frames);

	/* Number of frames that don't have data, that are within the writeahead: */
	snd_pcm_sframes_t unfilled_frames =
	  std::clamp(ActualWriteahead - filled_frames,
				 0l,
				 (snd_pcm_sframes_t)ActualWriteahead);

	//	LOG->Trace( "total_fr: %i; avail_fr: %i; filled_fr: %i; ActualWr %i;
	// chunksize %i; unfilled_frames %i ", 			total_frames, avail_frames,
	// filled_frames, ActualWriteahead, chunksize, unfilled_frames );

	/* If we have less than a chunk empty, don't fill at all.  Otherwise, we'll
	 * spend a lot of CPU filling in partial chunks, instead of waiting for some
	 * sound to play and then filling a whole chunk at once. */
	if (unfilled_frames < (int)chunksize)
		return 0;

	return chunksize;
}

bool
Alsa9Buf::WaitUntilFramesCanBeFilled(int timeout_ms)
{
	int err = dsnd_pcm_wait(pcm, timeout_ms);
	/* EINTR is normal; don't warn. */
	if (err == -EINTR)
		return false;
	ALSA_ASSERT("snd_pcm_wait");

	return err == 1;
}

void
Alsa9Buf::Write(const int16_t* buffer, int frames)
{
	/* We should be able to write it all.  If we don't, treat it as an error. */
	int wrote;
	do {
		wrote = dsnd_pcm_mmap_writei(pcm, (const char*)buffer, frames);
	} while (wrote == -EAGAIN);

	if (wrote < 0) {
		Locator::getLogger()->trace(
		  "RageSoundDriver_ALSA9::GetData: dsnd_pcm_mmap_writei: {} ({})",
		  dsnd_strerror(wrote),
		  wrote);
		return;
	}

	last_cursor_pos += wrote;
	if (wrote < frames)
		Locator::getLogger()->trace("Couldn't write whole buffer? ({} < {})", wrote, frames);
}

/*
 * When the play buffer underruns, subsequent writes to the buffer
 * return -EPIPE.  When this happens, call Recover() to restart playback.
 */
bool
Alsa9Buf::Recover(int r)
{
	if (r == -EPIPE) {
		Locator::getLogger()->trace("RageSound_ALSA9::Recover (prepare)");
		int err = dsnd_pcm_prepare(pcm);
		ALSA_ASSERT("dsnd_pcm_prepare (Recover)");
		return true;
	}

	if (r == -ESTRPIPE) {
		Locator::getLogger()->trace("RageSound_ALSA9::Recover (resume)");
		int err;
		while ((err = dsnd_pcm_resume(pcm)) == -EAGAIN)
			usleep(10000); // 10ms

		ALSA_ASSERT("dsnd_pcm_resume (Recover)");
		return true;
	}

	return false;
}

int64_t
Alsa9Buf::GetPosition() const
{
	if (dsnd_pcm_state(pcm) == SND_PCM_STATE_PREPARED)
		return last_cursor_pos;

	dsnd_pcm_hwsync(pcm);

	/* delay is returned in frames */
	snd_pcm_sframes_t delay;
	int err = dsnd_pcm_delay(pcm, &delay);
	ALSA_ASSERT("dsnd_pcm_delay");

	return last_cursor_pos - delay;
}

void
Alsa9Buf::Play()
{
	/* NOP.  It'll start playing when it gets some data. */
}

void
Alsa9Buf::Stop()
{
	dsnd_pcm_drop(pcm);
	dsnd_pcm_prepare(pcm);
	last_cursor_pos = 0;
}

std::string
Alsa9Buf::GetHardwareID(std::string name)
{
	InitializeErrorHandler();

	if (name.empty())
		name = DeviceName();

	snd_ctl_t* handle;
	int err;
	err = dsnd_ctl_open(&handle, name.c_str(), 0);
	if (err < 0) {
		Locator::getLogger()->info("Couldn't open card \"{}\" to get ID: {}",
				  name.c_str(),
				  dsnd_strerror(err));
		return "???";
	}

	snd_ctl_card_info_t* info;
	dsnd_ctl_card_info_alloca(&info);
	err = dsnd_ctl_card_info(handle, info);
	std::string ret = dsnd_ctl_card_info_get_id(info);
	dsnd_ctl_close(handle);

	return ret;
}
