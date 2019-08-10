#ifndef CRASH_HANDLER_INTERNAL_H
#define CRASH_HANDLER_INTERNAL_H

#define BACKTRACE_MAX_SIZE 100

struct CrashInfo
{
	char m_CrashReason[1024 * 8];

	const void* m_BacktracePointers[BACKTRACE_MAX_SIZE];

	enum
	{
		MAX_BACKTRACE_THREADS = 32
	};
	const void* m_AlternateThreadBacktrace[MAX_BACKTRACE_THREADS]
										  [BACKTRACE_MAX_SIZE];
	char m_AlternateThreadName[MAX_BACKTRACE_THREADS][128];

	CrashInfo()
	{
		m_CrashReason[0] = 0;
		memset(
		  m_AlternateThreadBacktrace, 0, sizeof(m_AlternateThreadBacktrace));
		memset(m_AlternateThreadName, 0, sizeof(m_AlternateThreadName));
		m_BacktracePointers[0] = NULL;
	}
};

#define CHILD_MAGIC_PARAMETER "--private-do-crash-handler"

#endif
