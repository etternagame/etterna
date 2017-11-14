#ifndef RAGE_FILE_DRIVER_DIRECT_H
#define RAGE_FILE_DRIVER_DIRECT_H

#include "RageFile.h"
#include "RageFileDriver.h"

/** @brief File driver for accessing a regular filesystem. */
class RageFileDriverDirect: public RageFileDriver
{
public:
	RageFileDriverDirect( const RString &sRoot );

	RageFileBasic *Open( const RString &sPath, int iMode, int &iError ) override;
	bool Move( const RString &sOldPath, const RString &sNewPath ) override;
	bool Remove( const RString &sPath ) override;
	bool Remount( const RString &sPath ) override;

private:
	RString m_sRoot;
};

class RageFileDriverDirectReadOnly: public RageFileDriverDirect
{
public:
	RageFileDriverDirectReadOnly( const RString &sRoot );
	RageFileBasic *Open( const RString &sPath, int iMode, int &iError ) override;
	bool Move( const RString &sOldPath, const RString &sNewPath ) override;
	bool Remove( const RString &sPath ) override;
};

/** @brief This driver handles direct file access. */

class RageFileObjDirect: public RageFileObj
{
public:
	RageFileObjDirect( const RString &sPath, int iFD, int iMode );
	~RageFileObjDirect() override;
	int ReadInternal( void *pBuffer, size_t iBytes ) override;
	int WriteInternal( const void *pBuffer, size_t iBytes ) override;
	int FlushInternal() override;
	int SeekInternal( int offset ) override;
	RageFileObjDirect *Copy() const override;
	RString GetDisplayPath() const override { return m_sPath; }
	int GetFileSize() const override;
	int GetFD() override;
	
private:
	bool FinalFlush();
	
	int m_iFD;
	int m_iMode;
	RString m_sPath; /* for Copy */
	
	/*
	 * When not streaming to disk, we write to a temporary file, and rename to the
	 * real file on completion.  If any write, this is aborted.  When streaming to
	 * disk, allow recovering from errors.
	 */
	bool m_bWriteFailed;
	bool WriteFailed() const { return !(m_iMode & RageFile::STREAMED) && m_bWriteFailed; }
	
	// unused
	RageFileObjDirect& operator=(const RageFileObjDirect& rhs) = delete;
	RageFileObjDirect(const RageFileObjDirect& rhs) = delete;
};


#endif

/*
 * Copyright (c) 2003-2004 Glenn Maynard
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

