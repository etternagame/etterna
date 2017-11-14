/* RageFileDriverTimeOut - manipulate files with a forced timeout. */

#ifndef RAGE_FILE_DRIVER_TIMEOUT_H
#define RAGE_FILE_DRIVER_TIMEOUT_H

#include "RageFileDriver.h"

class ThreadedFileWorker;

class RageFileDriverTimeout: public RageFileDriver
{
public:
	RageFileDriverTimeout( const RString &path );
	~RageFileDriverTimeout() override;

	RageFileBasic *Open( const RString &path, int mode, int &err ) override;
	void FlushDirCache( const RString &sPath ) override;
	bool Move( const RString &sOldPath, const RString &sNewPath ) override;
	bool Remove( const RString &sPath ) override;

	static void SetTimeout( float fSeconds );
	static void ResetTimeout() { SetTimeout( -1 ); }

private:
	RageFileDriver *m_pChild;
	ThreadedFileWorker *m_pWorker;
};


#endif

/*
 * Copyright (c) 2005 Glenn Maynard
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
