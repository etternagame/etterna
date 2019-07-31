#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "RageUtil/File/RageFile.h"
#include "RageLog.h"
#include "RageThreads.h"
#include "RageTimer.h"
#include "RageUtil/Utils/RageUtil.h"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <map>
RageLog* LOG; // global and accessible from anywhere in the program

/*
 * We have a couple log types and a couple logs.
 *
 * Traces are for very verbose debug information.  Use them as much as you want.
 *
 * Warnings are for things that shouldn't happen, but can be dealt with.  (If
 * they can't be dealt with, use RageException::Throw, which will also send a
 * warning.)
 *
 * Info is for important bits of information.  These should be used selectively.
 * Try to keep Info text dense; lots of information is fine, but try to keep
 * it to a reasonable total length.
 *
 * log.txt receives all logs.  This file can get rather large.
 *
 * info.txt receives warnings and infos.  This file should be fairly small;
 * small enough to be mailed without having to be edited or zipped, and small
 * enough to be very easily read.
 */

/* Map data names to logged data.
 *
 * This lets us keep individual bits of changing information to be present
 * in crash logs.  For example, we want to know which file was being processed
 * if we crash during a texture load.  However, we don't want to log every
 * load, since there are a huge number, even for log.txt.  We only want to
 * know the current one, if any.
 *
 * So, when a texture begins loading, we do:
 * LOG->MapLog("TextureManager::Load", "Loading foo.png");
 * and when it finishes loading without crashing,
 * LOG->UnmapLog("TextureManager::Load");
 *
 * Each time a mapped log changes, we update a block of static text to be put
 * in info.txt, so we see "Loading foo.png".
 *
 * The identifier is never displayed, so we can use a simple local object to
 * map/unmap, using any mechanism to generate unique IDs. */
static map<RString, RString> LogMaps;

#define LOG_PATH "Logs/log.txt"
#define INFO_PATH "Logs/info.txt"
#define TIME_PATH "Logs/timelog.txt"
#define USER_PATH "Logs/userlog.txt"

/* staticlog gets info.txt
 * crashlog gets log.txt */
enum
{
	/* If this is set, the message will also be written to info.txt. (info and
	   warnings) */
	WRITE_TO_INFO = 0x01,

	/* If this is set, the message will also be written to userlog.txt. (user
	   warnings only) */
	WRITE_TO_USER_LOG = 0x02,

	/* Whether this line should be loud when written to log.txt (warnings). */
	WRITE_LOUD = 0x04,
	WRITE_TO_TIME = 0x08
};

RageLog::RageLog()
{
	try {
		g_fileLog = SetLogger("Log", LOG_PATH);
		g_fileInfo = SetLogger("Info", INFO_PATH);
		g_fileUserLog = SetLogger("Userlog", USER_PATH);
		g_fileTimeLog = SetLogger("Timelog", TIME_PATH);
		spdlog::set_pattern("[%T.%e] [%P:%t] [%l]:  %v");

	} catch (const spdlog::spdlog_ex& ex) {
		sm_crash(ex.what());
	}
}

RageLog::~RageLog()
{
	/* Add the mapped log data to info.txt. */
	const RString AdditionalLog = GetAdditionalLog();
	vector<RString> AdditionalLogLines;
	split(AdditionalLog, "\n", AdditionalLogLines);
	for (unsigned i = 0; i < AdditionalLogLines.size(); ++i) {
		Trim(AdditionalLogLines[i]);
		this->Info("%s", AdditionalLogLines[i].c_str());
	}

	Flush();
	SetShowLogOutput(false);
	spdlog::shutdown();
}

shared_ptr<spdlog::logger>
RageLog::SetLogger(const char* name, const char* path)
{
	shared_ptr<spdlog::logger> out;
	vector<spdlog::sink_ptr> sinks;
	try {
		sinks.push_back(
		  std::make_shared<spdlog::sinks::basic_file_sink_mt>(path));
		sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
		out = make_shared<spdlog::logger>(name, begin(sinks), end(sinks));
		spdlog::register_logger(out);
	} catch (const spdlog::spdlog_ex& ex) {
		sm_crash(ex.what());
	}
	return out;
}

void
RageLog::SetLogToDisk(bool b)
{
	if (m_bLogToDisk == b)
		return;

	m_bLogToDisk = b;
}

void
RageLog::SetInfoToDisk(bool b)
{
	if (m_bInfoToDisk == b)
		return;

	m_bInfoToDisk = b;
}

void
RageLog::SetUserLogToDisk(bool b)
{
	if (m_bUserLogToDisk == b)
		return;

	m_bUserLogToDisk = b;
}

void
RageLog::SetFlushing(bool b)
{
	m_bFlush = b;
}

/* Enable or disable display of output to stdout, or a console window in
 * Windows. */
void
RageLog::SetShowLogOutput(bool show)
{
	m_bShowLogOutput = show;

#ifdef _WIN32
	if (m_bShowLogOutput) {
		// create a new console window and attach standard handles
		AllocConsole();
		freopen("CONOUT$", "wb", stdout);
		freopen("CONOUT$", "wb", stderr);
	} else {
		FreeConsole();
	}
#endif
}

void
RageLog::Flush()
{
	g_fileLog->flush();
	g_fileInfo->flush();
	g_fileTimeLog->flush();
	g_fileUserLog->flush();
}

#define NEWLINE "\n"

static char staticlog[1024 * 32] = "";
static unsigned staticlog_size = 0;
void
RageLog::AddToInfo(const RString& str)
{
	static bool limit_reached = false;
	if (limit_reached)
		return;

	unsigned len = str.size() + strlen(NEWLINE);
	if (staticlog_size + len > sizeof(staticlog)) {
		const RString txt(NEWLINE "Staticlog limit reached" NEWLINE);

		const unsigned pos =
		  min(staticlog_size, sizeof(staticlog) - txt.size());
		memcpy(staticlog + pos, txt.data(), txt.size());
		limit_reached = true;
		return;
	}

	memcpy(staticlog + staticlog_size, str.data(), str.size());
	staticlog_size += str.size();
	memcpy(staticlog + staticlog_size, NEWLINE, strlen(NEWLINE));
	staticlog_size += strlen(NEWLINE);
}

const char*
RageLog::GetInfo()
{
	staticlog[sizeof(staticlog) - 1] = 0;
	return staticlog;
}

static const int BACKLOG_LINES = 10;
static char backlog[BACKLOG_LINES][1024];
static int backlog_start = 0, backlog_cnt = 0;
void
RageLog::AddToRecentLogs(const RString& str)
{
	unsigned len = str.size();
	if (len > sizeof(backlog[backlog_start]) - 1)
		len = sizeof(backlog[backlog_start]) - 1;

	strncpy(backlog[backlog_start], str, len);
	backlog[backlog_start][len] = 0;

	backlog_start++;
	if (backlog_start > backlog_cnt)
		backlog_cnt = backlog_start;
	backlog_start %= BACKLOG_LINES;
}

const char*
RageLog::GetRecentLog(int n)
{
	if (n >= BACKLOG_LINES || n >= backlog_cnt)
		return NULL;

	if (backlog_cnt == BACKLOG_LINES) {
		n += backlog_start;
		n %= BACKLOG_LINES;
	}
	/* Make sure it's terminated: */
	backlog[n][sizeof(backlog[n]) - 1] = 0;

	return backlog[n];
}

static char g_AdditionalLogStr[10240] = "";
static int g_AdditionalLogSize = 0;

void
RageLog::UpdateMappedLog()
{
	RString str;
	FOREACHM_CONST(RString, RString, LogMaps, i)
	str += ssprintf("%s" NEWLINE, i->second.c_str());

	g_AdditionalLogSize = min(sizeof(g_AdditionalLogStr), str.size() + 1);
	memcpy(g_AdditionalLogStr, str.c_str(), g_AdditionalLogSize);
	g_AdditionalLogStr[sizeof(g_AdditionalLogStr) - 1] = 0;
}

const char*
RageLog::GetAdditionalLog()
{
	int size = min(g_AdditionalLogSize, (int)sizeof(g_AdditionalLogStr) - 1);
	g_AdditionalLogStr[size] = 0;
	return g_AdditionalLogStr;
}

void
RageLog::MapLog(const RString& key, const char* fmt, ...)
{
	RString s;

	va_list va;
	va_start(va, fmt);
	s += vssprintf(fmt, va);
	va_end(va);

	LogMaps[key] = s;
	UpdateMappedLog();
}

void
RageLog::UnmapLog(const RString& key)
{
	LogMaps.erase(key);
	UpdateMappedLog();
}

void
ShowWarningOrTrace(const char* file,
				   int line,
				   const char* message,
				   bool bWarning)
{
	/* Ignore everything up to and including the first "src/". */
	const char* temp = strstr(file, "src/");
	if (temp != nullptr)
		file = temp + 4;

	if (LOG != nullptr)
		if (bWarning)
			LOG->Warn("%s:%i: %s", file, line, message);
		else
			LOG->Trace("%s:%i: %s", file, line, message);
	else
		fprintf(stderr, "%s:%i: %s\n", file, line, message);
}

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
