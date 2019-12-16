/* RageFile - High-level file access. */

#ifndef RAGE_FILE_H
#define RAGE_FILE_H

#include "RageFileBasic.h"

struct lua_State;

/**
 * @brief High-level file access.
 *
 * This is the high-level interface, which interfaces with RageFileObj
 * implementations and RageFileManager. */
class RageFile : public RageFileBasic
{
  public:
	enum
	{
		READ = 0x1,
		WRITE = 0x2,

		/* Always write directly to the destination file; don't do a safe write.
		   (for logs) */
		STREAMED = 0x4,

		/* Flush the file to disk on close.  Combined with not streaming, this
		 * results in very safe writes, but is slow. */
		SLOW_FLUSH = 0x8
	};

	RageFile();
	~RageFile() override { Close(); }
	RageFile(const RageFile& cpy);
	RageFile* Copy() const override;

	/*
	 * Use GetRealPath to get the path this file was opened with; use that if
	 * you want a path that will probably get you the same file again.
	 *
	 * GetPath can be overridden by drivers.  Use it to get a path for display;
	 * it may give more information, such as the name of the archive the file
	 * is in.  It has no parsable meaning.
	 */
	const RString& GetRealPath() const { return m_Path; }
	RString GetPath() const;

	bool Open(const RString& path, int mode = READ);
	void Close();
	bool IsOpen() const { return m_File != NULL; }
	int GetMode() const { return m_Mode; }

	bool AtEOF() const override;
	RString GetError() const override;
	void ClearError() override;

	int Tell() const override;
	int Seek(int offset) override;
	int GetFileSize() const override;
	int GetFD() override;

	/* Raw I/O: */
	int Read(void* buffer, size_t bytes) override;
	int Read(RString& buffer, int bytes = -1) override;
	int Write(const void* buffer, size_t bytes) override;
	int Write(const RString& string) override
	{
		return Write(string.data(), string.size());
	}
	int Flush() override;

	/* These are just here to make wrappers (eg. vorbisfile, SDL_rwops) easier.
	 */
	int Write(const void* buffer, size_t bytes, int nmemb) override;
	int Read(void* buffer, size_t bytes, int nmemb) override;
	int Seek(int offset, int whence) override;

	/* Line-based I/O: */
	int GetLine(RString& out) override;
	int PutLine(const RString& str) override;

	void EnableCRC32(bool on = true) override;
	bool GetCRC32(uint32_t* iRet) override;

	// Lua
	virtual void PushSelf(lua_State* L);

  private:
	void SetError(const RString& err);

	RageFileBasic* m_File;
	RString m_Path;
	RString m_sError;
	int m_Mode;

	// Swallow up warnings. If they must be used, define them.
	RageFile& operator=(const RageFile& rhs);
};

/** @brief Convenience wrappers for reading binary files. */
namespace FileReading {
/* On error, these set sError to the error message.  If sError is already
 * non-empty, nothing happens. */
void
ReadBytes(RageFileBasic& f, void* buf, int size, RString& sError);
void
SkipBytes(RageFileBasic& f, int size, RString& sError);
void
Seek(RageFileBasic& f, int iOffset, RString& sError);
RString
ReadString(RageFileBasic& f, int size, RString& sError);
uint8_t
read_8(RageFileBasic& f, RString& sError);
int16_t
read_16_le(RageFileBasic& f, RString& sError);
uint16_t
read_u16_le(RageFileBasic& f, RString& sError);
int32_t
read_32_le(RageFileBasic& f, RString& sError);
uint32_t
read_u32_le(RageFileBasic& f, RString& sError);
};

#endif
