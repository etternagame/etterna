/* RageFileBasic - simple file interface. */

#ifndef RAGE_FILE_BASIC_H
#define RAGE_FILE_BASIC_H

/* This is a simple file I/O interface.  Although most of these operations
 * are straightforward, there are several of them; most of the time, you'll
 * only want to implement RageFileObj. */
class RageFileBasic
{
public:
	virtual ~RageFileBasic() = default;

	virtual RString GetError() const = 0;
	virtual void ClearError() = 0;
	virtual bool AtEOF() const = 0;

	/* Seek to the given absolute offset.  Return to the position actually
	 * seeked to; if the position given was beyond the end of the file, the
	 * return value will be the size of the file. */
	virtual int Seek( int iOffset ) = 0;
	virtual int Seek( int offset, int whence ) = 0;
	virtual int Tell() const = 0;

	/* Read at most iSize bytes into pBuf.  Return the number of bytes read,
	 * 0 on end of stream, or -1 on error.  Note that reading less than iSize
	 * does not necessarily mean that the end of the stream has been reached;
	 * keep reading until 0 is returned. */
	virtual int Read( void *pBuffer, size_t iBytes ) = 0;
	virtual int Read( RString &buffer, int bytes = -1 ) = 0;
	virtual int Read( void *buffer, size_t bytes, int nmemb ) = 0;

	/* Write iSize bytes of data from pBuf.  Return 0 on success, -1 on error. */
	virtual int Write( const void *pBuffer, size_t iBytes ) = 0;
	virtual int Write( const RString &sString ) = 0;
	virtual int Write( const void *buffer, size_t bytes, int nmemb ) = 0;

	/* Due to buffering, writing may not happen by the end of a Write() call, so not
	 * all errors may be returned by it.  Data will be flushed when the stream (or its
	 * underlying object) is destroyed, but errors can no longer be returned.  Call
	 * Flush() to flush pending data, in order to check for errors. */
	virtual int Flush() = 0;

	/* This returns a descriptive path for the file, or "". */
	virtual RString GetDisplayPath() const { return RString(); }

	virtual RageFileBasic *Copy() const = 0;

	virtual int GetLine( RString &out ) = 0;
	virtual int PutLine( const RString &str ) = 0;

	virtual void EnableCRC32( bool on=true ) = 0;
	virtual bool GetCRC32( uint32_t *iRet ) = 0;

	virtual int GetFileSize() const = 0;

	/* If this file is backed by a file descriptor, return it. This is valid even
	 * if the file is being filtered or decompressed. If the file has no
	 * associated file descriptor, return -1. */
	virtual int GetFD() = 0;
};

class RageFileObj: public RageFileBasic
{
public:
	RageFileObj();
	RageFileObj( const RageFileObj &cpy );
	~RageFileObj() override;

	RString GetError() const override { return m_sError; }
	void ClearError() override { SetError(""); }
	
	bool AtEOF() const override { return m_bEOF; }

	int Seek( int iOffset ) override;
	int Seek( int offset, int whence ) override;
	int Tell() const override { return m_iFilePos; }

	int Read( void *pBuffer, size_t iBytes ) override;
	int Read( RString &buffer, int bytes = -1 ) override;
	int Read( void *buffer, size_t bytes, int nmemb ) override;

	int Write( const void *pBuffer, size_t iBytes ) override;
	int Write( const RString &sString ) override { return Write( sString.data(), sString.size() ); }
	int Write( const void *buffer, size_t bytes, int nmemb ) override;

	int Flush() override;

	int GetLine( RString &out ) override;
	int PutLine( const RString &str ) override;

	void EnableCRC32( bool on=true ) override;
	bool GetCRC32( uint32_t *iRet ) override;

	int GetFileSize() const override = 0;
	int GetFD() override { return -1; }
	RString GetDisplayPath() const override { return RString(); }
	RageFileBasic *Copy() const override { FAIL_M( "Copying unimplemented" ); }

protected:
	virtual int SeekInternal( int /* iOffset */ ) { FAIL_M( "Seeking unimplemented" ); }
	virtual int ReadInternal( void *pBuffer, size_t iBytes ) = 0;
	virtual int WriteInternal( const void *pBuffer, size_t iBytes ) = 0;
	virtual int FlushInternal() { return 0; }

	void EnableReadBuffering();
	void EnableWriteBuffering( int iBytes=1024*64 );

	void SetError( const RString &sError ) { m_sError = sError; }
	RString m_sError;

private:
	int FillReadBuf();
	void ResetReadBuf();
	int EmptyWriteBuf();

	bool m_bEOF;
	int m_iFilePos;

	/*
	 * If read buffering is enabled, m_pReadBuffer is the buffer, m_pReadBuf is the
	 * current read position in the buffer, and m_iReadBufAvail is the number of
	 * bytes at m_pReadBuf.  Note that read buffering is only enabled if:
	 *
	 *  - GetLine() is called (which requires buffering to efficiently search for newlines);
	 *  - or EnableReadBuffering() is called
	 *
	 * Once buffering is enabled, it stays enabled for the life of the object.
	 *
	 * If buffering is not enabled, this buffer will not be allocated, keeping the
	 * size overhead of each file down.  Layered RageFileBasic implementations, which
	 * read from other RageFileBasics, should generally not use buffering, in order
	 * to avoid reads being passed through several buffers, which is only a waste of
	 * memory.
	 */
	enum { BSIZE = 1024 };
	char *m_pReadBuffer;
	char *m_pReadBuf;
	int  m_iReadBufAvail;

	/*
	 * If write buffering is enabled, m_pWriteBuffer will be allocated, and m_iWriteBufferPos
	 * is the file position of the start of the buffer.
	 */
	char *m_pWriteBuffer;
	int m_iWriteBufferPos;
	int m_iWriteBufferSize;
	int m_iWriteBufferUsed;

	/* If EnableCRC32() is called, a CRC32 will be calculated as the file is read.
	 * This is only meaningful if EnableCRC32() is called at the very start of the
	 * file, and no seeking is performed. */
	bool m_bCRC32Enabled;
	uint32_t m_iCRC32;
	
	// Swallow up warnings. If they must be used, define them.
	RageFileObj& operator=(const RageFileObj& rhs);
};

#endif

/*
 * Copyright (c) 2003-2004 Glenn Maynard, Chris Danford, Steve Checkoway
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

