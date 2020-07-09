#include "Etterna/Globals/global.h"
#include "RageFileDriverSlice.h"

#include <algorithm>

RageFileDriverSlice::RageFileDriverSlice(RageFileBasic* pFile,
										 int iOffset,
										 int iFileSize)
{
	m_pFile = pFile;
	m_iOffset = iOffset;
	m_iFileSize = iFileSize;
	m_iFilePos = 0;
	m_bFileOwned = false;
}

RageFileDriverSlice::RageFileDriverSlice(const RageFileDriverSlice& cpy)
  : RageFileObj(cpy)
{
	m_pFile = cpy.m_pFile->Copy();
	m_iOffset = cpy.m_iOffset;
	m_iFileSize = cpy.m_iFileSize;
	m_iFilePos = cpy.m_iFilePos;
	m_bFileOwned = true;
}

RageFileDriverSlice::~RageFileDriverSlice()
{
	if (m_bFileOwned)
		delete m_pFile;
}

RageFileDriverSlice*
RageFileDriverSlice::Copy() const
{
	auto* pRet = new RageFileDriverSlice(*this);
	return pRet;
}

int
RageFileDriverSlice::ReadInternal(void* buf, size_t bytes)
{
	/* Make sure we're reading from the right place.  We might have been
	 * constructed with a file not pointing to iOffset. */
	m_pFile->Seek(m_iFilePos + m_iOffset);

	const int bytes_left = m_iFileSize - this->m_iFilePos;
	const int got =
	  m_pFile->Read(buf, std::min(static_cast<int>(bytes), bytes_left));
	if (got == -1) {
		SetError(m_pFile->GetError());
		return -1;
	}

	m_iFilePos += got;

	return got;
}

int
RageFileDriverSlice::SeekInternal(int offset)
{
	ASSERT(offset >= 0);
	offset = std::min(offset, m_iFileSize);

	int ret = m_pFile->Seek(m_iOffset + offset);
	if (ret == -1) {
		SetError(m_pFile->GetError());
		return -1;
	}
	ret -= m_iOffset;
	ASSERT(ret >= 0);
	m_iFilePos = ret;

	return ret;
}
