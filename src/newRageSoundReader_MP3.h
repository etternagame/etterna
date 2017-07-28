
#ifndef RAGE_SOUND_READER_MP3_H
#define RAGE_SOUND_READER_MP3_H


#include "RageSoundReader_FileReader.h"
#include "RageFile.h"

class newRageSoundReader_MP3 :
	public RageSoundReader_FileReader
{
public:
	OpenResult Open(RageFileBasic *pFile); 
	void Close();
	int GetLength() const;
	int SetPosition(int iFrame);
	int Read(float *pBuf, int iSample);
	int GetSampleRate() const { return 0; };
	unsigned GetNumChannels() const { return 0; };
	int GetNextSourceFrame() const { return 0; };
	newRageSoundReader_MP3();
	~newRageSoundReader_MP3();
	newRageSoundReader_MP3(const newRageSoundReader_MP3 &); /* not defined; don't use */
	newRageSoundReader_MP3 *Copy() const;
private:
	int SampleRate;
};

#endif
