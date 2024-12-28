#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageSoundReader_SpeedChange_Good.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Misc/RageMath.h"
#include "Etterna/Singletons/GameState.h"
#include "fft.h"

#ifndef __aarch64__
	#include <xmmintrin.h>
#else
	// Use sse2neon to transparently provide ARM Neon equivalents of x86_64 SIMD intrinsics
	#include "sse2neon.h"
#endif

//
//
// This whole thing is just overlap-add with big windows
//
//

static Preference<bool> g_StepmaniaUnpitchRates("StepmaniaUnpitchRates", false);

static const double BaseWindowsizeInMilliseconds = 266.0;
static const double MaxTimingAdjustStep = 20.0;
static const double FFTNonsenseBelowRate = 0.6;

static float
rsqrt(float x)
{
	return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x)));
}

static double
RoundPositive(double dValue)
{
	return double(int64_t(dValue + 0.5));
}

static uint32_t
RoundUpToPowerOfTwo(uint32_t x)
{
	x -= 1;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x += 1;
	return x;
}

static double
Clamp(double dValue, double dLow, double dHigh)
{
	CLAMP(dValue, dLow, dHigh);
	return dValue;
}

// std::complex is brutally slow in debug
struct complex { float real, imag; };
static complex operator+(complex a, float b) { return { a.real + b, a.imag }; }
static complex operator+(float a, complex b) { return { a + b.real, b.imag }; }
static complex operator+(complex a, complex b) { return { a.real+b.real, a.imag + b.imag }; }
static complex operator*(complex a, float b) { return { a.real * b, a.imag * b }; }
static complex operator*(float a, complex b) { return { a * b.real, a * b.imag }; }
static complex operator*(complex a, complex b) { return { a.real*b.real - a.imag*b.imag, a.real*b.imag + a.imag*b.real }; }
static complex& operator+=(complex& self, float other) { self = self + other; return self; }
static complex& operator+=(complex& self, complex other) { self = self + other; return self; }
static complex& operator*=(complex& self, float other) { self = self * other; return self; }
static complex& operator*=(complex& self, complex other) { self = self * other; return self; }
static float norm(complex a) { return a.real*a.real + a.imag*a.imag; }

struct Xorshift32
{
	uint32_t x = 1;
	float Next()
	{
		uint32_t x = this->x;
		x ^= x << 13;
		x ^= x >> 17;
		x ^= x << 5;
		this->x = x;
		return (float)((x >> 8) * 0x1p-24f);
	}
};

struct SpeedChangeFFT
{

	uint32_t iSize;
	std::unique_ptr<void, decltype(&mufft_free)> pInput;
	std::unique_ptr<void, decltype(&mufft_free)> pOutput;
	std::unique_ptr<void, decltype(&mufft_free)> pPhases;
	std::unique_ptr<mufft_plan_1d, decltype(&mufft_free_plan_1d)> pForward;
	std::unique_ptr<mufft_plan_1d, decltype(&mufft_free_plan_1d)> pBackward;

	static SpeedChangeFFT Make(uint32_t iWindowSize)
	{
		uint32_t iFFTSize = RoundUpToPowerOfTwo(iWindowSize);

		SpeedChangeFFT junk = {
			iFFTSize,
			{ mufft_alloc(iFFTSize * sizeof(float)),       mufft_free },
			{ mufft_alloc((iFFTSize + 2) * sizeof(float)), mufft_free },

			{ mufft_alloc((iFFTSize + 2) * sizeof(float)), mufft_free },

			{ mufft_create_plan_1d_r2c(iFFTSize, 0), mufft_free_plan_1d },
			{ mufft_create_plan_1d_c2r(iFFTSize, 0), mufft_free_plan_1d }
		};

		return junk;
	}
};

static float
SmoothStep(float  x)
{
	CLAMP(x, 0.0f, 1.0f);
	return x*x*(3.0f - 2.0f*x);
}

static float
Lerp(float t, float a, float b)
{
	return (1.0f - t)*a + t*b;
}

static void
ComputeRandomPhases(SpeedChangeFFT *Junk, uint32_t iSeed, float *pBuffer, int64_t iSamples, int64_t iNumChannels)
{
	ASSERT(uint64_t(iSamples) < Junk->iSize);

	float *pTime = (float *)Junk->pInput.get();
	complex *pFreq = (complex *)Junk->pPhases.get();
	float *pInputCursor = pBuffer;
	for (int64_t i = 0; i < iSamples; i++) {
		float fSum = 0.0f;
		for (int64_t i = 0; i < iNumChannels; i++) {
			fSum += *pInputCursor;
			pInputCursor++;
		}
		pTime[i] = fSum;
	}

	for (int64_t i = iSamples; i < Junk->iSize; i++) {
		pTime[i] = 0.0f;
	}

	mufft_execute_plan_1d(Junk->pForward.get(), pFreq, pTime);

	Xorshift32 RNG1 = { iSeed };
	Xorshift32 RNG2 = { ~iSeed };

	complex RandomPhasor1 = { 1.0f, 0.0f };
	complex RandomPhasor2 = { 1.0f, 0.0f };
	int64_t iNyquistBin = Junk->iSize / 2;
	for (int64_t i = 1; i < iNyquistBin; ++i) {
		float x = 2.0f * RNG1.Next() - 1.0f;
		float y = 2.0f * RNG2.Next() - 1.0f;
		complex fl = pFreq[i];
		complex fr = pFreq[i+1];
		float nc = fl.real*fr.real + fl.imag*fr.imag;
		if (nc > 0) {
			RandomPhasor1 *= complex{ x, y };
			RandomPhasor1 *= rsqrt(norm(RandomPhasor1));
			pFreq[i] = RandomPhasor1;
		} else {
			RandomPhasor2 += complex{ x, y };
			RandomPhasor2 *= rsqrt(norm(RandomPhasor2));
			pFreq[i] = RandomPhasor2;
		}
	}
	pFreq[0] = { 1.0f, 0.0f };
	pFreq[iNyquistBin] = { 1.0f, 0.0f };
}

static void
ApplyPhases(SpeedChangeFFT *Junk, uint32_t iSeed, float fMix, float *pBuffer, int64_t iSamples, int64_t iStrideInFloats, int64_t iSampleRate)
{
	ASSERT(uint64_t(iSamples) < Junk->iSize);

	int64_t iNyquistBin = Junk->iSize / 2;

	float *pTime = (float *)Junk->pInput.get();
	complex *pFreq = (complex *)Junk->pOutput.get();
	complex *pPhases = (complex *)Junk->pPhases.get();
	float *pInputCursor = pBuffer;
	for (int64_t i = 0; i < iSamples; i++) {
		pTime[i] = *pInputCursor;
		pInputCursor += iStrideInFloats;
	}

	for (int64_t i = iSamples; i < Junk->iSize; i++) {
		pTime[i] = 0.0f;
	}

	mufft_execute_plan_1d(Junk->pForward.get(), pFreq, pTime);

	// Makes it easier to listen to this stuff for a long time if we duck
	// some parts of the spectrum
	float fSibilanceStart = (6500.0 / (iSampleRate / 2)) * iNyquistBin;
	float fSibilanceEnd = (7500.0 / (iSampleRate / 2)) * iNyquistBin;
	float fShelfEnd = (12000.0 / (iSampleRate / 2)) * iNyquistBin;
	ASSERT(fSibilanceEnd < iNyquistBin);

	float fSibilanceBinInto01 = 1.0f / (fSibilanceEnd - fSibilanceStart); 
	float fShelfBinInto01 = 1.0f / (fShelfEnd - fSibilanceStart);

	float fScale = sqrtf(0.5f) / iNyquistBin;
	for (int64_t i = 1; i < iNyquistBin; ++i) {
		float fDuck = 1.0f;
		if (i >= fSibilanceStart && i < fSibilanceEnd) {
			float x = (float(i) - fSibilanceStart) * fSibilanceBinInto01;
			float a = 0.5f + 2.0f*(x - 0.5f)*(x - 0.5f);
			float b = 1.0f - 2.0f*(x - 0.5f)*(x - 0.5f);
			fDuck = 2.0f*a*a*b;
		}
		if (i >= fSibilanceStart) {
			float x = (float(i) - fSibilanceStart) * fShelfBinInto01;
			fDuck *= 0.75f + 0.25f*SmoothStep(1.0f - x);
		}

		pFreq[i] *= pPhases[i] * (fDuck * fScale);
	}
	pFreq[0] *= fScale;
	pFreq[iNyquistBin] *= fScale;

	mufft_execute_plan_1d(Junk->pBackward.get(), pTime, pFreq);

	pInputCursor = pBuffer;
	float fScale0 = sqrtf(1.0f - fMix);
	float fScale1 = sqrtf(fMix);
	for (int64_t i = 0; i < iSamples; i++) {
		*pInputCursor = fScale0 * pInputCursor[0] + fScale1 * pTime[i];
		pInputCursor += iStrideInFloats;
	}
}

RageSoundReader_SpeedChange_Good::Window
RageSoundReader_SpeedChange_Good::Window::Make(int iSampleRate, double dRate) {
	// These are exponents in a pow of the window. Rates < 0.5 need dShape < 1 as the window will be applied twice,
	// once before the FFT and then again after
	double dShape = (dRate >= 1.0)                  ? 3.0 :
					(dRate >= FFTNonsenseBelowRate) ? 2.0 :
												 sqrt(0.5);
	double dWindowScale = 1.0;
	if (dRate >= 2.0) dWindowScale = 0.8;
	if ((dRate >= FFTNonsenseBelowRate) && (dRate < 1.0)) dWindowScale = 1.25;

	double dWindowSize = RoundPositive((BaseWindowsizeInMilliseconds * (double)iSampleRate * dWindowScale) / 1000.0);

	int64_t iSourceStep = 0;
	if (dRate >= FFTNonsenseBelowRate) {
		if (dRate >= 0.85) {
			iSourceStep = RoundPositive(dWindowSize * 0.5);
		} else {
			iSourceStep = RoundPositive(dWindowSize * (1.0 / 3.0));
		}
	} else {
		double dUncorrectedDestStep = 0.0;
		if (dRate < 0.25)  {
			dWindowSize *= Clamp(SCALE(dRate, 0.25, 0.15, 1.0, 8.0), 1.0, 8.0);
			dUncorrectedDestStep = dWindowSize * SCALE(dRate, 0.25, 0.05, 0.125, 0.06125);
		} else {
			dUncorrectedDestStep = dWindowSize * (1.0 / 3.0);
		}

		iSourceStep = RoundPositive(dUncorrectedDestStep * dRate);
	}

	Window W = {};
	W.iSize = int64_t(dWindowSize);
	W.iSourceStep = iSourceStep;
	W.dDestStep = double(iSourceStep) / dRate;

	W.Buffer.resize(W.iSize);
	double dN = W.iSize+1;
	for (int64_t i = 0; i < W.iSize; ++i) {
		double t = (i + 1) / dN;
		if (dRate >= FFTNonsenseBelowRate)
			t = sqrt(t);
        t = 2.0 * t - 1.0;
		W.Buffer[i] = (float)pow(0.5 + 0.5*cos(PI*t), dShape);
	}

	if (dRate < FFTNonsenseBelowRate) {
		W.Junk = std::make_shared<SpeedChangeFFT>(SpeedChangeFFT::Make(W.iSize));
	}

	return W;
}

RageSoundReader_SpeedChange_Good::RageSoundReader_SpeedChange_Good(
  RageSoundReader* pSource)
  : RageSoundReader_Filter(pSource)
  , m_ReadAhead({ pSource->GetNumChannels() })
  , m_Mixed({ pSource->GetNumChannels() })
{
	SetSpeedRatio(1.0f);
}

void
RageSoundReader_SpeedChange_Good::SetSpeedRatio(float fRatio)
{
	m_fRate = fRatio;
	m_Window = Window::Make(GetSampleRate(), (double)fRatio);
}

int
RageSoundReader_SpeedChange_Good::Read(float* pBuf, int iFrames)
{
	if ((m_fRate == 1.0f) && (m_Mixed.Frames() == 0)) {
		return m_pSource->Read(pBuf, iFrames);
	}

	int64_t iNumChannels = m_pSource->GetNumChannels();
	int64_t iNextSourceFrame = m_pSource->GetNextSourceFrame();
	double dSampleRate = double(m_pSource->GetSampleRate());

	int64_t iWindowFrames = m_Window.iSize;
	double dRate = double(m_fRate);

	bool bUseFFT = (dRate < FFTNonsenseBelowRate);
	bool bAttemptToAlignWndowsToBeats = (dRate > 1.0);

	int64_t iMixedFramesMinimum = 2*iFrames;
	while (m_Mixed.Frames() < iMixedFramesMinimum && !m_bDraining) {
		DEBUG_ASSERT(m_ReadAhead.iReadPosition == 0);

		int64_t iReadAheadPosition = m_ReadAhead.iWritePosition;
		int64_t iSourceStepFrames = m_Window.iSourceStep;
		int64_t iSourceFramesToRead = std::max(int64_t(0), iWindowFrames - m_ReadAhead.Frames());
		m_ReadAhead.Extend(iSourceFramesToRead);

		double dAdjustScale = 1.0;
		if (bAttemptToAlignWndowsToBeats) {
			// For high rates (> 2 or so) we want the peak of each window to lie over beats because
			// theres enough window overlap that not aligning to beats causes arhythmic attenuation
			// of drum hits which sounds terrible. But it doesn't need to be accurate for this to work,
			// so just slurp the current bpm and beat from GAMESTATE, and assume that it's close
			// enough to the current music time according to the current iNextSourceFrame sample
			//
			// This doesn't move the window for this current window we're about to mix, it adjusts the
			// step so the next window is better aligned than this one. The dest step is also scaled
			// so the rate remains constant over all
			//
			// This is an unsynchronised read--another thread updates this--but the outcome of racing
			// is the read of BPS and SongBeat might be torn. This doesn't matter. It can only be so
			// wrong, and we cap the max step well below what is audible
			double dCurrentTPS = 2.0 * double(GAMESTATE->m_Position.m_fCurBPS);
			double dCurrentFractionalTick = 2.0 * double(GAMESTATE->m_Position.m_fSongBeat);

			double dNearestTick = RoundPositive(dCurrentFractionalTick);
			double dCurrentSecond = double(iNextSourceFrame) / dSampleRate;
			double dNearestTickSecond = dCurrentSecond + (dNearestTick - dCurrentFractionalTick) / dCurrentTPS;
			int64_t iNearestTickFrame = int64_t(RoundPositive(dNearestTickSecond * dSampleRate));
			int64_t iMaxTimingAdjustFrames = RoundPositive((MaxTimingAdjustStep * dSampleRate) / 1000.0);
			int64_t iNextWindowPeak = iNextSourceFrame - iReadAheadPosition + iSourceStepFrames + iWindowFrames / 4;

			int64_t iTimingAdjustment = Clamp(iNextWindowPeak - iNearestTickFrame, -iMaxTimingAdjustFrames, iMaxTimingAdjustFrames);
			int64_t iAdjustedStep = iSourceStepFrames - iTimingAdjustment;

			dAdjustScale = double(iAdjustedStep) / double(iSourceStepFrames);
			iSourceStepFrames = iAdjustedStep;
		}

		while (iSourceFramesToRead > 0) {
			int iRead = m_pSource->RetriedRead(m_ReadAhead.Samples.data() + m_ReadAhead.iWritePosition*iNumChannels, iSourceFramesToRead);

			if (iRead > 0) {
				iSourceFramesToRead -= iRead;
				m_ReadAhead.iWritePosition += iRead;
			} else if (iRead == END_OF_FILE) {
				m_bDraining = true;
				break;
			} else if (iRead < 0) {
				// Return error code immediately. At this point the only state that could have changed
				// is some smaples may have been read into m_ReadAhead
				return iRead;
			}
		}

		// Get dDestStep as an integer for this step. eg if dDestStep was 1000.2, then we'd step an extra
		// sample once every 5 frames. Easier than fractional delay !!
		double dCurrentPos = m_dPos;
		double dDestStep = m_Window.dDestStep * dAdjustScale;
		m_dPos += dDestStep;
		double dJitter = (int64_t(dCurrentPos) + int64_t(dDestStep)) != int64_t(m_dPos) ? 1.0 : 0.0;

		int64_t iDestStepFrames = int64_t(dDestStep + dJitter);
		int64_t iFramesToMix = std::min(m_ReadAhead.Frames(), iWindowFrames);
		iSourceStepFrames = std::min(m_ReadAhead.Frames(), iSourceStepFrames);

		if (bUseFFT) {
			// FFT sounds impressive but all this does is smear out detail on rates below 0.5
			m_Copy = m_ReadAhead.Samples;

			for (int64_t iChannel = 0; iChannel < iNumChannels; ++iChannel) {
				for (int64_t iSample = iChannel, iFrame = 0; iFrame < iFramesToMix; iSample += iNumChannels, iFrame += 1) {
					m_ReadAhead.Samples[iSample] *= m_Window.Buffer[iFrame];
				}
			}

			uint32_t iSeed = uint32_t(iNextSourceFrame);
			ComputeRandomPhases(m_Window.Junk.get(), iSeed, m_ReadAhead.Samples.data(), iFramesToMix, iNumChannels);

			float fMix = Clamp(SCALE(m_fRate, 0.15f, float(FFTNonsenseBelowRate), 1.0f, 0.0f), 0.0f, 1.0f);
			for (int64_t iChannel = 0; iChannel < iNumChannels; ++iChannel) {
				ApplyPhases(m_Window.Junk.get(), iSeed, fMix, m_ReadAhead.Samples.data() + iChannel, iFramesToMix, iNumChannels, dSampleRate);
			}
		}

		int64_t iOffsetFrame = m_Mixed.iWritePosition;
		int64_t iOffsetSample = m_Mixed.iWritePosition * iNumChannels;
		m_Mixed.Extend(iWindowFrames);
		m_Scale.resize(iOffsetFrame + iWindowFrames);

		for (int64_t iChannel = 0; iChannel < iNumChannels; ++iChannel) {
			for (int64_t iSample = iChannel, iFrame = 0; iFrame < iFramesToMix; iSample += iNumChannels, iFrame += 1) {
				m_Mixed.Samples[iOffsetSample + iSample] += m_ReadAhead.Samples[iSample] * m_Window.Buffer[iFrame];
			}
		}
		for (int64_t iFrame = 0; iFrame < iFramesToMix; ++iFrame) {
			m_Scale[iOffsetFrame + iFrame] += m_Window.Buffer[iFrame] * m_Window.Buffer[iFrame];
		}

		if (bUseFFT) {
			m_ReadAhead.Samples = m_Copy;
		}

		m_ReadAhead.iReadPosition += iSourceStepFrames;
		m_Mixed.iWritePosition += iDestStepFrames;

		m_ReadAhead.Shift();

		if (m_Mixed.Frames() >= iMixedFramesMinimum) {
			m_Scale.erase(m_Scale.begin(), m_Scale.begin() + m_Mixed.iReadPosition);
			m_Mixed.Shift();
		}
	}

	int64_t iMixedFramesToWrite = std::min(m_Mixed.Frames(), int64_t(iFrames));
	int64_t iFramesWrote = iMixedFramesToWrite;

	int64_t iOffsetFrame = m_Mixed.iReadPosition;
	int64_t iOffsetSample = m_Mixed.iReadPosition * iNumChannels;
	for (int64_t iChannel = 0; iChannel < iNumChannels; ++iChannel) {
		for (int64_t iSample = iChannel, iFrame = 0; iFrame < iMixedFramesToWrite; iSample += iNumChannels, iFrame += 1) {
			pBuf[iSample] = m_Mixed.Samples[iOffsetSample + iSample] * rsqrt(m_Scale[iOffsetFrame + iFrame]);
		}
	}

	m_Mixed.iReadPosition += iMixedFramesToWrite;

	if (m_bDraining) {
		int64_t iReadAheadFramesToWrite = std::min(m_ReadAhead.Frames(), iFrames - iMixedFramesToWrite);
		int64_t iOffsetFrame = m_ReadAhead.iReadPosition;
		int64_t iOffsetSample = m_ReadAhead.iReadPosition * iNumChannels;
		for (int64_t iChannel = 0; iChannel < iNumChannels; ++iChannel) {
			for (int64_t iSample = iChannel, iFrame = 0; iFrame < iReadAheadFramesToWrite; iSample += iNumChannels, iFrame += 1) {
				pBuf[iSample] = m_ReadAhead.Samples[iOffsetSample + iSample];
			}
		}

		m_ReadAhead.iReadPosition += iReadAheadFramesToWrite;
		m_ReadAhead.Shift();

		iFramesWrote += iReadAheadFramesToWrite;
		m_bDraining = (m_ReadAhead.Frames() > 0);
	}

	ASSERT_M(m_ReadAhead.iReadPosition < m_ReadAhead.iWritePosition,
			 "RA readpos < writepos. to ignore this, set preference "
			 "StepmaniaUnpitchRates=1");
	ASSERT_M(m_Mixed.iReadPosition < m_Mixed.iWritePosition,
			 "M readpos < writepos. to ignore this, set preference "
			 "StepmaniaUnpitchRates=1");

	if (iFramesWrote == 0) {
		return END_OF_FILE;
	}

	return iFramesWrote;
}

int
RageSoundReader_SpeedChange_Good::SetPosition(int iFrame)
{
	m_ReadAhead = { GetNumChannels() };
	m_Mixed = { GetNumChannels() };
	m_Scale.clear();

	m_dPos = 0.0;
	return m_pSource->SetPosition(iFrame);
}

bool
RageSoundReader_SpeedChange_Good::SetProperty(const std::string& sProperty,
										 float fValue)
{
	if (sProperty == "Speed") {
		SetSpeedRatio(fValue);
		return true;
	}

	return RageSoundReader_Filter::SetProperty(sProperty, fValue);
}

int
RageSoundReader_SpeedChange_Good::GetNextSourceFrame() const
{
	if (m_Mixed.Frames() == 0) {
		return RageSoundReader_Filter::GetNextSourceFrame();
	} else {
		double dRate = double(m_fRate);
		int64_t iCurrent = RageSoundReader_Filter::GetNextSourceFrame();
		iCurrent -= m_ReadAhead.Frames();
		iCurrent -= int64_t(m_Mixed.Frames() * dRate);

		// For very low rates the beat shifts towards the centre of the window
		int iStretchCorrection = 0;
		if (m_fRate < 0.75f) {
			iStretchCorrection = int(SCALE(Clamp(dRate, 0.25, 0.75), 0.25, 0.75, m_Window.iSize / 2.0, m_Window.iSize / 4.0));
		}

		return iCurrent + iStretchCorrection;
	}
}

float
RageSoundReader_SpeedChange_Good::GetStreamToSourceRatio() const
{
	return m_fRate * RageSoundReader_Filter::GetStreamToSourceRatio();
}
