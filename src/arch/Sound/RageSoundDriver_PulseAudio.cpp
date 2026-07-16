#include "Etterna/Globals/global.h"
#include "RageSoundDriver_PulseAudio.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Sound/RageSound.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Misc/RageTimer.h"
#include "Etterna/Singletons/PrefsManager.h"
#include <pulse/error.h>
#include <sys/time.h>
#include <sys/resource.h>

/* Register the RageSoundDriver_Pulseaudio class as sound driver "Pulse" */
REGISTER_SOUND_DRIVER_CLASS2(Pulse, PulseAudio);

/* Constructor */
RageSoundDriver_PulseAudio::RageSoundDriver_PulseAudio()
  : RageSoundDriver()
  , m_LastPosition(0)
  , m_Error(NULL)
  , m_Sem("Pulseaudio Synchronization Semaphore")
  , m_PulseMainLoop(NULL)
  , m_PulseCtx(NULL)
  , m_PulseStream(NULL)
{
	m_ss.rate = PREFSMAN->m_iSoundPreferredSampleRate;
	if (m_ss.rate == 0)
		m_ss.rate = 44100;
}

RageSoundDriver_PulseAudio::~RageSoundDriver_PulseAudio()
{
	pa_context_disconnect(m_PulseCtx);
	pa_context_unref(m_PulseCtx);
	pa_threaded_mainloop_stop(m_PulseMainLoop);
	pa_threaded_mainloop_free(m_PulseMainLoop);

	if (m_Error != NULL) {
		free(m_Error);
	}
}

/* Initialization */
std::string
RageSoundDriver_PulseAudio::Init()
{
	int error = 0;

	Locator::getLogger()->info("Pulse: pa_threaded_mainloop_new()...");
	m_PulseMainLoop = pa_threaded_mainloop_new();
	if (m_PulseMainLoop == NULL) {
		return "pa_threaded_mainloop_new() failed!";
	}

#ifdef PA_PROP_APPLICATION_NAME /* proplist available only since 0.9.11 */
	pa_proplist* plist = pa_proplist_new();
	pa_proplist_sets(plist, PA_PROP_APPLICATION_NAME, PACKAGE_NAME);
	pa_proplist_sets(plist, PA_PROP_APPLICATION_VERSION, PACKAGE_VERSION);
	pa_proplist_sets(plist, PA_PROP_MEDIA_ROLE, "game");

	Locator::getLogger()->info("Pulse: pa_context_new_with_proplist()...");

	m_PulseCtx = pa_context_new_with_proplist(
	  pa_threaded_mainloop_get_api(m_PulseMainLoop), "StepMania", plist);
	pa_proplist_free(plist);

	if (m_PulseCtx == NULL) {
		return "pa_context_new_with_proplist() failed!";
	}
#else
	Locator::getLogger()->info("Pulse: pa_context_new()...");
	m_PulseCtx = pa_context_new(pa_threaded_mainloop_get_api(m_PulseMainLoop),
								"Stepmania");
	if (m_PulseCtx == NULL) {
		return "pa_context_new() failed!";
	}
#endif

	pa_context_set_state_callback(m_PulseCtx, StaticCtxStateCb, this);

	Locator::getLogger()->info("Pulse: pa_context_connect()...");
	error = pa_context_connect(m_PulseCtx, NULL, (pa_context_flags_t)0, NULL);

	if (error < 0) {
		return ssprintf("pa_contect_connect(): %s",
						pa_strerror(pa_context_errno(m_PulseCtx)));
	}

	Locator::getLogger()->info("Pulse: pa_threaded_mainloop_start()...");
	error = pa_threaded_mainloop_start(m_PulseMainLoop);
	if (error < 0) {
		return ssprintf("pa_threaded_mainloop_start() returned %i", error);
	}

	/* Create the decode thread, this will be needed for Mix(), that we
	 * will use as soon as a stream is ready. */
	StartDecodeThread();

	/* Wait for the pulseaudio stream to be ready before returning.
	 * An error may occur, if it appends, m_Error becomes non-NULL. */
	m_Sem.Wait();

	if (m_Error == NULL) {
		return "";
	} else {
		return m_Error;
	}
}

void
RageSoundDriver_PulseAudio::m_InitStream(void)
{
	int error;
	pa_sample_spec ss;
	pa_channel_map map;

	/* init sample spec */
	ss.format = PA_SAMPLE_S16LE;
	ss.channels = 2;
	ss.rate = PREFSMAN->m_iSoundPreferredSampleRate;
	if (ss.rate == 0) {
		ss.rate = 44100;
	}

	/* init channel map */
	pa_channel_map_init_stereo(&map);

	/* check sample spec */
	if (!pa_sample_spec_valid(&ss)) {
		if (asprintf(&m_Error, "invalid sample spec!") == -1) {
			m_Error = NULL;
		}
		m_Sem.Post();
		return;
	}

	/* log the used sample spec */
	char specstring[PA_SAMPLE_SPEC_SNPRINT_MAX];
	pa_sample_spec_snprint(specstring, sizeof(specstring), &ss);
	Locator::getLogger()->info("Pulse: using sample spec: {}", specstring);

	/* create the stream */
	Locator::getLogger()->info("Pulse: pa_stream_new()...");
	m_PulseStream = pa_stream_new(m_PulseCtx, "Stepmania Audio", &ss, &map);
	if (m_PulseStream == NULL) {
		if (asprintf(&m_Error,
					 "pa_stream_new(): %s",
					 pa_strerror(pa_context_errno(m_PulseCtx))) == -1) {
			m_Error = NULL;
		}
		m_Sem.Post();
		return;
	}

	/* set the write callback, it will be called when the sound server
	 * needs data */
	pa_stream_set_write_callback(m_PulseStream, StaticStreamWriteCb, this);

	/* set the state callback, it will be called the the stream state will
	 * change */
	pa_stream_set_state_callback(m_PulseStream, StaticStreamStateCb, this);

	/* configure attributes of the stream */
	pa_buffer_attr attr;
	memset(&attr, 0x00, sizeof(attr));

	/* tlength: Target length of the buffer.
	 *
	 * "The server tries to assure that at least tlength bytes are always
	 *  available in the per-stream server-side playback buffer. It is
	 *  recommended to set this to (uint32_t) -1, which will initialize
	 *  this to a value that is deemed sensible by the server. However,
	 *  this value will default to something like 2s, i.e. for applications
	 *  that have specific latency requirements this value should be set to
	 *  the maximum latency that the application can deal with."
	 *
	 * We don't want the default here, we want a small latency.
	 * We use pa_usec_to_bytes() to convert a latency to a buffer size.
	 */
	attr.tlength = pa_usec_to_bytes(20 * PA_USEC_PER_MSEC, &ss);

	/* maxlength: Maximum length of the buffer
	 *
	 * "Setting this to (uint32_t) -1 will initialize this to the maximum
	 *  value supported by server, which is recommended."
	 *
	 * (uint32_t)-1 is NOT working here, setting it to tlength*2, like
	 * openal-soft-pulseaudio does.
	 */
	attr.maxlength = attr.tlength * 2;

	/* minreq: Minimum request
	 *
	 * "The server does not request less than minreq bytes from the client,
	 *  instead waits until the buffer is free enough to request more bytes
	 *  at once. It is recommended to set this to (uint32_t) -1, which will
	 *  initialize this to a value that is deemed sensible by the server."
	 *
	 * (uint32_t)-1 is NOT working here, setting it to 0, like
	 * openal-soft-pulseaudio does.
	 */
	attr.minreq = 0;

	/* prebuf: Pre-buffering
	 *
	 * "The server does not start with playback before at least prebuf
	 *  bytes are available in the buffer. It is recommended to set this
	 *  to (uint32_t) -1, which will initialize this to the same value as
	 *  tlength"
	 */
	attr.prebuf = (uint32_t)-1;

	/* log the used target buffer length */
	Locator::getLogger()->info("Pulse: using target buffer length of {} bytes", attr.tlength);

	/* connect the stream for playback */
	Locator::getLogger()->info("Pulse: pa_stream_connect_playback()...");
	const int flags = PA_STREAM_INTERPOLATE_TIMING
		| PA_STREAM_NOT_MONOTONIC
		| PA_STREAM_AUTO_TIMING_UPDATE;
	error = pa_stream_connect_playback(
	  m_PulseStream, NULL, &attr, static_cast<pa_stream_flags_t>(flags), NULL, NULL);
	if (error < 0) {
		if (asprintf(&m_Error,
					 "pa_stream_connect_playback(): %s",
					 pa_strerror(pa_context_errno(m_PulseCtx))) == -1) {
			m_Error = NULL;
		}
		m_Sem.Post();
		return;
	}

	m_ss = ss;
}

void
RageSoundDriver_PulseAudio::CtxStateCb(pa_context* c)
{
	switch (pa_context_get_state(m_PulseCtx)) {
		case PA_CONTEXT_CONNECTING:
			Locator::getLogger()->info("Pulse: Context connecting...");
			break;
		case PA_CONTEXT_AUTHORIZING:
			Locator::getLogger()->info("Pulse: Context authorizing...");
			break;
		case PA_CONTEXT_SETTING_NAME:
			Locator::getLogger()->info("Pulse: Context setting name...");
			break;
		case PA_CONTEXT_READY:
			Locator::getLogger()->info("Pulse: Context ready now.");
			m_InitStream();
			break;
		case PA_CONTEXT_TERMINATED:
		case PA_CONTEXT_FAILED:
			if (asprintf(&m_Error,
						 "context connection failed: %s",
						 pa_strerror(pa_context_errno(m_PulseCtx))) == -1) {
				m_Error = NULL;
			}
			m_Sem.Post();
			return;
			break;
		case PA_CONTEXT_UNCONNECTED:
		default:
			break;
	}
}

void
RageSoundDriver_PulseAudio::StreamStateCb(pa_stream* s)
{
	switch (pa_stream_get_state(m_PulseStream)) {
		case PA_STREAM_CREATING:
			Locator::getLogger()->info("Pulse: Stream creating...");
			break;
		case PA_STREAM_READY:
			Locator::getLogger()->info("Pulse: Stream ready now.");
			m_Sem.Post();
			return;
			break;
		case PA_STREAM_UNCONNECTED:
		case PA_STREAM_TERMINATED:
		case PA_STREAM_FAILED:
			if (asprintf(&m_Error,
						 "stream connection failed: %s",
						 pa_strerror(pa_context_errno(m_PulseCtx))) == -1) {
			}
			m_Sem.Post();
			return;
			break;
	}
}

int64_t
RageSoundDriver_PulseAudio::GetPosition() const
{
	pa_usec_t usec;
	if(pa_stream_get_time(m_PulseStream, &usec) < 0)
	{
		Locator::getLogger()->warn("pa_stream_get_time went backwards??");
	}
	std::size_t length = pa_usec_to_bytes(usec, &m_ss);
	return length / (sizeof(std::int16_t) * 2);
}

void
RageSoundDriver_PulseAudio::StreamWriteCb(pa_stream* s, size_t length)
{
	void* buf;
	if(pa_stream_begin_write(m_PulseStream, &buf, &length) < 0)
	{
		RageException::Throw("Pulse: pa_stream_begin_write()");
	}
	size_t nbframes = length / sizeof(int16_t); /* we use 16-bit frames */
	int64_t pos1 = m_LastPosition;
	int64_t pos2 = pos1 + nbframes / 2; /* Mix() position in stereo frames */
	this->Mix( reinterpret_cast<std::int16_t*>(buf), pos2-pos1, pos1, pos2);
	if(pa_stream_write(m_PulseStream, buf, length, nullptr, 0, PA_SEEK_RELATIVE) < 0) {
		RageException::Throw("Pulse: pa_stream_write()");
	}
	m_LastPosition = pos2;
}

/* Static wrappers, because pulseaudio is a C API, it uses callbacks.
 * So we have to write wrappers that will call our objects callbacks. */
void
RageSoundDriver_PulseAudio::StaticCtxStateCb(pa_context* c, void* user)
{
	RageSoundDriver_PulseAudio* obj = (RageSoundDriver_PulseAudio*)user;
	obj->CtxStateCb(c);
}
void
RageSoundDriver_PulseAudio::StaticStreamStateCb(pa_stream* s, void* user)
{
	RageSoundDriver_PulseAudio* obj = (RageSoundDriver_PulseAudio*)user;
	obj->StreamStateCb(s);
}
void
RageSoundDriver_PulseAudio::StaticStreamWriteCb(pa_stream* s,
												size_t length,
												void* user)
{
	RageSoundDriver_PulseAudio* obj = (RageSoundDriver_PulseAudio*)user;
	obj->StreamWriteCb(s, length);
}
