/* RageLog - Manages logging. */

#ifndef RAGE_LOG_H
#define RAGE_LOG_H

#include "RageUtil/Utils/RageUtil.h"
#include "spdlog/spdlog.h"

class RageLog
{
  public:
	RageLog();
	~RageLog();
	void Flush();

	template<typename... Args>
	void Trace(const char* fmt, Args... args)
	{
		RString sBuff = ssprintf(fmt, args...);

		AddToRecentLogs(sBuff);
		g_fileLog->trace(sBuff.c_str());
		if (m_bFlush)
			Flush();
		// Write(0, sBuff);
	}

	/* Use this for more important information; it'll always be included
	 * in crash dumps. */
	template<typename... Args>
	void Info(const char* fmt, Args... args)
	{
		RString sBuff = ssprintf(fmt, args...);

		g_fileLog->info(sBuff.c_str());
		AddToInfo(sBuff);
		AddToRecentLogs(sBuff);
		g_fileLog->flush();
		// Write(WRITE_TO_INFO, sBuff);
	}

	template<typename... Args>
	void Warn(const char* fmt, Args... args)
	{
		RString sBuff = ssprintf(fmt, args...);

		AddToRecentLogs(sBuff);
		g_fileLog->warn(sBuff.c_str());
		if (m_bFlush)
			Flush();
		// Write(WRITE_TO_INFO | WRITE_LOUD, sBuff);
	}

	template<typename... Args>
	void Time(const char* fmt, Args... args)
	{
		RString sBuff = ssprintf(fmt, args...);

		AddToRecentLogs(sBuff);
		g_fileTimeLog->info(sBuff.c_str());
		if (m_bFlush)
			Flush();
		// Write(WRITE_TO_TIME, sBuff);
	}

	template<typename... Args>
	void UserLog(const RString& sType,
				 const RString& sElement,
				 const char* fmt,
				 Args... args)
	{
		RString sBuf = ssprintf(fmt, args...);

		if (!sType.empty())
			sBuf = ssprintf(
			  "%s \"%s\" %s", sType.c_str(), sElement.c_str(), sBuf.c_str());

		AddToRecentLogs(sBuf);
		g_fileUserLog->info(sBuf.c_str());
		if (m_bFlush)
			Flush();
		// Write(WRITE_TO_USER_LOG, sBuf);
	}

	void MapLog(const RString& key, const char* fmt, ...) PRINTF(3, 4);
	void UnmapLog(const RString& key);

	static const char* GetAdditionalLog();
	static const char* GetInfo();
	/* Returns NULL if past the last recent log. */
	static const char* GetRecentLog(int n);

	void SetShowLogOutput(bool show); // enable or disable logging to stdout
	void SetLogToDisk(bool b);		  // enable or disable logging to file
	void SetInfoToDisk(bool b);	// enable or disable logging info.txt to file
	void SetUserLogToDisk(bool b); // enable or disable logging user.txt to file
	void SetFlushing(bool b);	  // enable or disable flushing

  private:
	bool m_bLogToDisk{ false };
	bool m_bInfoToDisk{ false };
	bool m_bUserLogToDisk{ false };
	bool m_bFlush{ false };
	bool m_bShowLogOutput{ false };
	shared_ptr<spdlog::logger> SetLogger(const char* name, const char* path);
	void UpdateMappedLog();
	void AddToInfo(const RString& buf);
	void AddToRecentLogs(const RString& buf);
	shared_ptr<spdlog::logger> g_fileLog, g_fileInfo, g_fileUserLog,
	  g_fileTimeLog, g_consoleLog;
};

extern RageLog* LOG; // global and accessible from anywhere in our program
#endif

/*
 * Copyright (c) 2001-2004 Chris Danford, Glenn Maynard
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
