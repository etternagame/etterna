#include "newRageSoundReader_MP3.h"
#include "mpg123.h"
#include <io.h>


newRageSoundReader_MP3::newRageSoundReader_MP3()
{
}


newRageSoundReader_MP3::~newRageSoundReader_MP3()
{
}

RageSoundReader_FileReader::OpenResult newRageSoundReader_MP3::Open(RageFileBasic *pFile)
{
	return OPEN_OK;
}

int newRageSoundReader_MP3::GetLength() const
{
	return 0;
}

newRageSoundReader_MP3 *newRageSoundReader_MP3::Copy() const 
{
	newRageSoundReader_MP3 *ret = new newRageSoundReader_MP3;
	RageFileBasic *pFile = m_pFile->Copy();
	pFile->Seek(0);
	ret->Open(pFile);
	return ret;
}

int newRageSoundReader_MP3::SetPosition(int iFrame)
{
	return 0;
}

int newRageSoundReader_MP3::Read(float *pBuf, int iFrames)
{
	return 0;
}