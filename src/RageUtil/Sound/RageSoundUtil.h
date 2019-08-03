#ifndef RAGE_SOUND_UTIL_H
#define RAGE_SOUND_UTIL_H

/** @brief Simple utilities that operate on sound buffers. */
namespace RageSoundUtil {
void
Attenuate(float* pBuf, int iSamples, float fVolume);
void
Pan(float* pBuffer, int iFrames, float fPos);
void
Fade(float* pBuffer,
	 int iFrames,
	 int iChannels,
	 float fStartVolume,
	 float fEndVolume);
void
ConvertMonoToStereoInPlace(float* pBuffer, int iFrames);
void
ConvertNativeInt16ToFloat(const int16_t* pFrom, float* pTo, int iSamples);
void
ConvertFloatToNativeInt16(const float* pFrom, int16_t* pTo, int iSamples);
};

#endif
