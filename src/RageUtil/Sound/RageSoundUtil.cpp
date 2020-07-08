#include "Etterna/Globals/global.h"
#include "RageSoundUtil.h"
#include "RageUtil/Utils/RageUtil.h"

#include <algorithm>

void
RageSoundUtil::Attenuate(float* pBuf, int iSamples, float fVolume)
{
	while (iSamples--) {
		*pBuf *= fVolume;
		++pBuf;
	}
}

/* Pan buffer left or right; fPos is -1...+1.  Buffer is assumed to be stereo.
 */
void
RageSoundUtil::Pan(float* buffer, int frames, float fPos)
{
	if (fPos == 0)
		return; /* no change */

	bool bSwap = fPos < 0;
	if (bSwap)
		fPos = -fPos;

	float fLeftFactors[2] = { 1 - fPos, 0 };
	float fRightFactors[2] = { SCALE(fPos, 0, 1, 0.5f, 0),
							   SCALE(fPos, 0, 1, 0.5f, 1) };

	if (bSwap) {
		std::swap(fLeftFactors[0], fRightFactors[0]);
		std::swap(fLeftFactors[1], fRightFactors[1]);
	}

	for (int samp = 0; samp < frames; ++samp) {
		float l = buffer[0] * fLeftFactors[0] + buffer[1] * fLeftFactors[1];
		float r = buffer[0] * fRightFactors[0] + buffer[1] * fRightFactors[1];
		buffer[0] = l;
		buffer[1] = r;
		buffer += 2;
	}
}

void
RageSoundUtil::Fade(float* pBuffer,
					int iFrames,
					int iChannels,
					float fStartVolume,
					float fEndVolume)
{
	/* If the whole buffer is full volume, skip. */
	if (fStartVolume > .9999f && fEndVolume > .9999f)
		return;

	for (int iFrame = 0; iFrame < iFrames; ++iFrame) {
		float fVolPercent = SCALE(iFrame, 0, iFrames, fStartVolume, fEndVolume);

		fVolPercent = std::clamp(fVolPercent, 0.f, 1.f);
		for (int i = 0; i < iChannels; ++i) {
			*pBuffer *= fVolPercent;
			pBuffer++;
		}
	}
}

/* Dupe mono to stereo in-place; do it in reverse, to handle overlap. */
void
RageSoundUtil::ConvertMonoToStereoInPlace(float* data, int iFrames)
{
	float* input = data;
	float* output = input;
	input += iFrames;
	output += iFrames * 2;
	while (iFrames--) {
		input -= 1;
		output -= 2;
		output[0] = input[0];
		output[1] = input[0];
	}
}

void
RageSoundUtil::ConvertNativeInt16ToFloat(const int16_t* pFrom,
										 float* pTo,
										 int iSamples)
{
	for (int i = 0; i < iSamples; ++i) {
		pTo[i] = pFrom[i] / 32768.0f;
	}
}

void
RageSoundUtil::ConvertFloatToNativeInt16(const float* pFrom,
										 int16_t* pTo,
										 int iSamples)
{
	for (int i = 0; i < iSamples; ++i) {
		int iOut = lround(pFrom[i] * 32768.0f);
		pTo[i] = std::clamp(iOut, -32768, 32767);
	}
}
