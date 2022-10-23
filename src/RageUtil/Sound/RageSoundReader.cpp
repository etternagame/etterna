#include "Etterna/Globals/global.h"
#include "Core/Services/Locator.hpp"
#include "RageSoundReader.h"

/* Read(), handling the STREAM_LOOPED and empty return cases. */
int
RageSoundReader::RetriedRead(float* pBuffer,
							 int iFrames,
							 int* iSourceFrame,
							 float* fRate)
{
	if (iFrames == 0)
		return 0;

	/* pReader may return 0, which means "try again immediately".  As a
	 * failsafe, only try this a finite number of times.  Use a high number,
	 * because in principle each filter in the stack may cause this. */
	int iTries = 100;
	while (--iTries) {
		if (fRate)
			*fRate = this->GetStreamToSourceRatio();
		if (iSourceFrame)
			*iSourceFrame = this->GetNextSourceFrame();

		int iGotFrames = this->Read(pBuffer, iFrames);

		if (iGotFrames == STREAM_LOOPED)
			iGotFrames = 0;

		if (iGotFrames != 0)
			return iGotFrames;
	}

	Locator::getLogger()->warn("Read() busy looping");

	/* Pretend we got EOF. */
	return END_OF_FILE;
}
