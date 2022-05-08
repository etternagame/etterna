#include "Etterna/Globals/global.h"
#include "RageSoundDriver_AU.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/PrefsManager.h"
#include "archutils/Darwin/DarwinThreadHelpers.h"
#include <CoreServices/CoreServices.h>
#include <AudioToolbox/AudioServices.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioComponent.h>

REGISTER_SOUND_DRIVER_CLASS2(AudioUnit, AU);

static const UInt32 kFramesPerPacket = 1;
static const UInt32 kChannelsPerFrame = 2;
static const UInt32 kBitsPerChannel = 32;
static const UInt32 kBytesPerPacket = kChannelsPerFrame * kBitsPerChannel / 8;
static const UInt32 kBytesPerFrame = kBytesPerPacket;
static const UInt32 kFormatFlags =
  kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsFloat;

#define WERROR(str, num, extra...)                                             \
	str ": '{}' ({}).", ##extra, FourCCToString(num).c_str(), (num)
#define ERROR(str, num, extra...) (ssprintf(WERROR(str, (num), ##extra)))

static inline std::string
FourCCToString(uint32_t num)
{
	std::string s(4, '?');
	char c;

	c = (num >> 24) & 0xFF;
	if (c >= '\x20' && c <= '\x7e')
		s[0] = c;
	c = (num >> 16) & 0xFF;
	if (c >= '\x20' && c <= '\x7e')
		s[1] = c;
	c = (num >> 8) & 0xFF;
	if (c >= '\x20' && c <= '\x7e')
		s[2] = c;
	c = num & 0xFF;
	if (c >= '\x20' && c <= '\x7e')
		s[3] = c;

	return s;
}

RageSoundDriver_AU::RageSoundDriver_AU()
  : m_OutputUnit(NULL)
  , m_iSampleRate(0)
  , m_bDone(false)
  , m_bStarted(false)
  , m_pIOThread(NULL)
  , m_pNotificationThread(NULL)
  , m_Semaphore("Sound")
{
}

static void
SetSampleRate(AudioUnit au, Float64 desiredRate)
{
	AudioDeviceID OutputDevice;
	OSStatus error;
	UInt32 size = sizeof(AudioDeviceID);

	if ((error = AudioUnitGetProperty(au,
									  kAudioOutputUnitProperty_CurrentDevice,
									  kAudioUnitScope_Global,
									  0,
									  &OutputDevice,
									  &size))) {
		Locator::getLogger()->warn(WERROR("No output device", error));
		return;
	}

	Float64 rate = 0.0;
	size = sizeof(Float64);
	if ((error = AudioDeviceGetProperty(OutputDevice,
										0,
										false,
										kAudioDevicePropertyNominalSampleRate,
										&size,
										&rate))) {
		Locator::getLogger()->warn(
		  WERROR("Couldn't get the device's sample rate", error));
		return;
	}
	if (rate == desiredRate)
		return;

	if ((error = AudioDeviceGetPropertyInfo(
		   OutputDevice,
		   0,
		   false,
		   kAudioDevicePropertyAvailableNominalSampleRates,
		   &size,
		   NULL))) {
		Locator::getLogger()->warn(
		  WERROR("Couldn't get available nominal sample rates info", error));
		return;
	}

	const int num = size / sizeof(AudioValueRange);
	AudioValueRange* ranges = new AudioValueRange[num];

	if ((error = AudioDeviceGetProperty(
		   OutputDevice,
		   0,
		   false,
		   kAudioDevicePropertyAvailableNominalSampleRates,
		   &size,
		   ranges))) {
		Locator::getLogger()->warn(
		  WERROR("Couldn't get available nominal sample rates", error));
		delete[] ranges;
		return;
	}

	Float64 bestRate = 0.0;
	for (int i = 0; i < num; ++i) {
		if (desiredRate >= ranges[i].mMinimum &&
			desiredRate <= ranges[i].mMaximum) {
			bestRate = desiredRate;
			break;
		}
		/* XXX: If the desired rate is supported by the device, then change it,
		 * if not then we should select the "best" rate. I don't really know
		 * what such a best rate would be. The rate closest to the desired
		 * value? A multiple of 2? For now give up if the desired sample rate
		 * isn't available. */
	}
	delete[] ranges;
	if (bestRate == 0.0)
		return;

	if ((error = AudioDeviceSetProperty(OutputDevice,
										NULL,
										0,
										false,
										kAudioDevicePropertyNominalSampleRate,
										sizeof(Float64),
										&bestRate))) {
		Locator::getLogger()->warn(
		  WERROR("Couldn't set the device's sample rate", error));
	}
}

std::string
RageSoundDriver_AU::Init()
{
	AudioComponentDescription desc;

	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;

	AudioComponent comp = AudioComponentFindNext(NULL, &desc);

	if (comp == NULL)
		return "Failed to find the default output unit.";

	OSStatus error = AudioComponentInstanceNew(comp, &m_OutputUnit);

	if (error != noErr || m_OutputUnit == NULL)
		return ERROR("Could not open the default output unit", error);

	// Set up a callback function to generate output to the output unit
	AURenderCallbackStruct input;
	input.inputProc = Render;
	input.inputProcRefCon = this;

	error = AudioUnitSetProperty(m_OutputUnit,
								 kAudioUnitProperty_SetRenderCallback,
								 kAudioUnitScope_Input,
								 0,
								 &input,
								 sizeof(input));
	if (error != noErr)
		return ERROR("Failed to set render callback", error);

	AudioStreamBasicDescription streamFormat;

	streamFormat.mSampleRate = PREFSMAN->m_iSoundPreferredSampleRate;
	streamFormat.mFormatID = kAudioFormatLinearPCM;
	streamFormat.mFormatFlags = kFormatFlags;
	streamFormat.mBytesPerPacket = kBytesPerPacket;
	streamFormat.mFramesPerPacket = kFramesPerPacket;
	streamFormat.mBytesPerFrame = kBytesPerFrame;
	streamFormat.mChannelsPerFrame = kChannelsPerFrame;
	streamFormat.mBitsPerChannel = kBitsPerChannel;

	if (streamFormat.mSampleRate <= 0.0)
		streamFormat.mSampleRate = 44100.0;
	m_iSampleRate = int(streamFormat.mSampleRate);
	m_TimeScale = streamFormat.mSampleRate / AudioGetHostClockFrequency();

	// Try to set the hardware sample rate.
	SetSampleRate(m_OutputUnit, streamFormat.mSampleRate);

	error = AudioUnitSetProperty(m_OutputUnit,
								 kAudioUnitProperty_StreamFormat,
								 kAudioUnitScope_Input,
								 0,
								 &streamFormat,
								 sizeof(AudioStreamBasicDescription));
	if (error != noErr)
		return ERROR("Failed to set AU stream format", error);
	UInt32 renderQuality = kRenderQuality_Max;

	error = AudioUnitSetProperty(m_OutputUnit,
								 kAudioUnitProperty_RenderQuality,
								 kAudioUnitScope_Global,
								 0,
								 &renderQuality,
								 sizeof(renderQuality));
	if (error != noErr)
		Locator::getLogger()->warn(
		  WERROR("Failed to set the maximum render quality", error));

	// Initialize the AU.
	if ((error = AudioUnitInitialize(m_OutputUnit)))
		return ERROR("Could not initialize the AudioUnit", error);

	StartDecodeThread();

	if ((error = AudioOutputUnitStart(m_OutputUnit)))
		return ERROR("Could not start the AudioUnit", error);
	m_bStarted = true;
	return std::string();
}

RageSoundDriver_AU::~RageSoundDriver_AU()
{
	if (!m_OutputUnit)
		return;
	if (m_bStarted) {
		m_bDone = true;
		m_Semaphore.Wait();
	}
	AudioUnitUninitialize(m_OutputUnit);
	AudioComponentInstanceDispose(m_OutputUnit);
	delete m_pIOThread;
	delete m_pNotificationThread;
}

int64_t
RageSoundDriver_AU::GetPosition() const
{
	return int64_t(m_TimeScale * AudioGetCurrentHostTime());
}

void
RageSoundDriver_AU::SetupDecodingThread()
{
	/* Increase the scheduling precedence of the decoder thread. */
	const std::string sError = SetThreadPrecedence(0.75f);
	if (!sError.empty())
		Locator::getLogger()->warn(
		  "Could not set precedence of the decoding thread: {}",
		  sError.c_str());
}

float
RageSoundDriver_AU::GetPlayLatency() const
{
	OSStatus error;
	UInt32 bufferSize;
	AudioDeviceID OutputDevice;
	UInt32 size = sizeof(AudioDeviceID);
	Float64 sampleRate;

	if ((error = AudioUnitGetProperty(m_OutputUnit,
									  kAudioOutputUnitProperty_CurrentDevice,
									  kAudioUnitScope_Global,
									  0,
									  &OutputDevice,
									  &size))) {
		Locator::getLogger()->warn(WERROR("No output device", error));
		return 0.0f;
	}

	size = sizeof(Float64);
	if ((error = AudioDeviceGetProperty(OutputDevice,
										0,
										false,
										kAudioDevicePropertyNominalSampleRate,
										&size,
										&sampleRate))) {
		Locator::getLogger()->warn(
		  WERROR("Couldn't get the device sample rate", error));
		return 0.0f;
	}

	size = sizeof(UInt32);
	if ((error = AudioDeviceGetProperty(OutputDevice,
										0,
										false,
										kAudioDevicePropertyBufferFrameSize,
										&size,
										&bufferSize))) {
		Locator::getLogger()->warn(
		  WERROR("Couldn't determine buffer size", error));
		bufferSize = 0;
	}

	UInt32 frames;

	size = sizeof(UInt32);
	if ((error = AudioDeviceGetProperty(OutputDevice,
										0,
										false,
										kAudioDevicePropertyLatency,
										&size,
										&frames))) {
		Locator::getLogger()->warn(
		  WERROR("Couldn't get device latency", error));
		frames = 0;
	}

	bufferSize += frames;
	size = sizeof(UInt32);
	if ((error = AudioDeviceGetProperty(OutputDevice,
										0,
										false,
										kAudioDevicePropertySafetyOffset,
										&size,
										&frames))) {
		Locator::getLogger()->warn(
		  WERROR("Couldn't get device safety offset", error));
		frames = 0;
	}
	bufferSize += frames;
	size = sizeof(UInt32);

	do {
		if ((error = AudioDeviceGetPropertyInfo(OutputDevice,
												0,
												false,
												kAudioDevicePropertyStreams,
												&size,
												NULL))) {
			Locator::getLogger()->warn(WERROR("Device has no streams", error));
			break;
		}
		int num = size / sizeof(AudioStreamID);
		if (num == 0) {
			Locator::getLogger()->warn("Device has no streams.");
			break;
		}
		AudioStreamID* streams = new AudioStreamID[num];

		if ((error = AudioDeviceGetProperty(OutputDevice,
											0,
											false,
											kAudioDevicePropertyStreams,
											&size,
											streams))) {
			Locator::getLogger()->warn(
			  WERROR("Cannot get device's streams", error));
			delete[] streams;
			break;
		}
		if ((error = AudioStreamGetProperty(
			   streams[0], 0, kAudioDevicePropertyLatency, &size, &frames))) {
			Locator::getLogger()->warn(
			  WERROR("Stream does not report latency", error));
			frames = 0;
		}
		delete[] streams;
		bufferSize += frames;
	} while (false);

	return float(bufferSize / sampleRate);
}

OSStatus
RageSoundDriver_AU::Render(void* inRefCon,
						   AudioUnitRenderActionFlags* ioActionFlags,
						   const AudioTimeStamp* inTimeStamp,
						   UInt32 inBusNumber,
						   UInt32 inNumberFrames,
						   AudioBufferList* ioData)
{
	RageSoundDriver_AU* This = (RageSoundDriver_AU*)inRefCon;

	if (unlikely(This->m_pIOThread == NULL))
		This->m_pIOThread = new RageThreadRegister("HAL I/O thread");

	AudioBuffer& buf = ioData->mBuffers[0];
	int64_t now = int64_t(This->m_TimeScale * AudioGetCurrentHostTime());
	int64_t next = int64_t(This->m_TimeScale * inTimeStamp->mHostTime);

	This->Mix((float*)buf.mData, inNumberFrames, next, now);
	if (unlikely(This->m_bDone)) {
		AudioOutputUnitStop(This->m_OutputUnit);
		This->m_Semaphore.Post();
	}
	return noErr;
}

/*
 * (c) 2004-2007 Steve Checkoway
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
