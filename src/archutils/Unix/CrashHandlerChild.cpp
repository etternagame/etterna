#define __USE_GNU
#include "Etterna/Globals/global.h"

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sstream>

#include "Backtrace.h"
#include "BacktraceNames.h"

#include "RageUtil/Utils/RageUtil.h"
#include "CrashHandler.h"
#include "CrashHandlerInternal.h"
#include "Etterna/Globals/ProductInfo.h"
#include "Core/Services/Locator.hpp"


#ifdef __APPLE__
#include "archutils/Darwin/Crash.h"
#endif

#include "Core/Misc/AppInfo.hpp"

bool
child_read(int fd, void* p, int size);

const char* g_pCrashHandlerArgv0 = NULL;

static void
output_stack_trace(FILE* out, const void** BacktracePointers)
{
	if (BacktracePointers[0] == BACKTRACE_METHOD_NOT_AVAILABLE) {
		fprintf(out, "No backtrace method available.\n");
		return;
	}

	if (!BacktracePointers[0]) {
		fprintf(out, "Backtrace was empty.\n");
		return;
	}

	for (int i = 0; BacktracePointers[i]; ++i) {
		BacktraceNames bn;
		bn.FromAddr(const_cast<void*>(BacktracePointers[i]));
		bn.Demangle();

		/* Don't show the main module name. */
		if (bn.File == g_pCrashHandlerArgv0 && !bn.Symbol.empty())
			bn.File = "";

		if (bn.Symbol == "__libc_start_main")
			break;

		fprintf(out, "%s\n", bn.Format().c_str());
	}
}

bool
child_read(int fd, void* p, int size)
{
	char* buf = (char*)p;
	int got = 0;
	while (got < size) {
		int ret = read(fd, buf + got, size - got);
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			fprintf(stderr,
					"Crash handler: error communicating with parent: %s\n",
					strerror(errno));
			return false;
		}

		if (ret == 0) {
			fprintf(stderr, "Crash handler: EOF communicating with parent.\n");
			return false;
		}

		got += ret;
	}

	return true;
}

/* Once we get here, we should be * safe to do whatever we want;
 * heavyweights like malloc and std::string are OK. (Don't crash!) */
static void
child_process()
{
	/* 1. Read the CrashData. */
	CrashData crash;
	if (!child_read(3, &crash, sizeof(CrashData)))
		return;

	/* 2. Read info. */
	int size;
	if (!child_read(3, &size, sizeof(size)))
		return;
	char* Info = new char[size];
	if (!child_read(3, Info, size))
		return;

	/* 3. Read AdditionalLog. */
	if (!child_read(3, &size, sizeof(size)))
		return;

	char* AdditionalLog = new char[size];
	if (!child_read(3, AdditionalLog, size))
		return;

	/* 4. Read RecentLogs. */
	int cnt = 0;
	if (!child_read(3, &cnt, sizeof(cnt)))
		return;
	char* Recent[1024];
	for (int i = 0; i < cnt; ++i) {
		if (!child_read(3, &size, sizeof(size)))
			return;
		Recent[i] = new char[size];
		if (!child_read(3, Recent[i], size))
			return;
	}

	/* 5. Read CHECKPOINTs. */
	if (!child_read(3, &size, sizeof(size)))
		return;

	char* temp = new char[size];
	if (!child_read(3, temp, size))
		return;

	vector<std::string> Checkpoints;
	split(temp, "$$", Checkpoints);
	delete[] temp;

	/* 6. Read the crashed thread's name. */
	if (!child_read(3, &size, sizeof(size)))
		return;
	temp = new char[size];
	if (!child_read(3, temp, size))
		return;
	const std::string CrashedThread(temp);
	delete[] temp;

	/* Wait for the child to either finish cleaning up or die. */
	fd_set rs;
	struct timeval timeout = { 5, 0 }; // 5 seconds

	FD_ZERO(&rs);
	FD_SET(3, &rs);
	int ret = select(4, &rs, NULL, NULL, &timeout);

	if (ret == 0) {
		fputs("Timeout exceeded.\n", stderr);
	} else if ((ret == -1 && errno != EPIPE) || ret != 1) {
		fprintf(stderr,
				"Unexpected return from select() result: %d (%s)\n",
				ret,
				strerror(errno));
		// Keep going.
	} else {
		char x;

		// No need to check FD_ISSET( 3, &rs ) because it was the only
		// descriptor in the set.
		ret = read(3, &x, sizeof(x));
		if (ret > 0) {
			fprintf(stderr, "Unexpected child read() result: %i\n", ret);
			/* keep going */
		} else if ((ret == -1 && errno != EPIPE) || ret != 0) {
			/* We expect an EOF or EPIPE.  What happened? */
			fprintf(stderr,
					"Unexpected child read() result: %i (%s)\n",
					ret,
					strerror(errno));
			/* keep going */
		}
	}

	std::string sCrashInfoPath = "/tmp";
#ifdef __APPLE__
	sCrashInfoPath = CrashHandler::GetLogsDirectory();
#else
	const char* home = getenv("HOME");
	if (home)
		sCrashInfoPath = home;
#endif
	sCrashInfoPath += "/crashinfo.txt";

	FILE* CrashDump = fopen(sCrashInfoPath.c_str(), "w+");
	if (CrashDump == NULL) {
		std::stringstream ss;
		ss << "Couldn't open " << sCrashInfoPath << ": %s\n";
		std::string strrrr = ss.str();
		fprintf(stderr,
				strrrr.c_str(),
				strerror(errno));
		exit(1);
	}

	fprintf(CrashDump, "%s%s crash report", PRODUCT_FAMILY, Core::AppInfo::APP_VERSION);
	fprintf(CrashDump, " (build %s)", Core::AppInfo::GIT_HASH);
	fprintf(CrashDump, "\n");
	fprintf(CrashDump, "--------------------------------------\n");
	fprintf(CrashDump, "\n");

	std::string reason;
	switch (crash.type) {
		case CrashData::SIGNAL: {
			reason = ssprintf("%s - %s",
							  SignalName(crash.signal),
							  SignalCodeName(crash.signal, crash.si.si_code));

			/* Linux puts the PID that sent the signal in si_addr for SI_USER.
			 */
			if (crash.si.si_code == SI_USER) {
				reason += ssprintf(" from pid %li", (long)crash.si.si_addr);
			} else {
				switch (crash.signal) {
					case SIGILL:
					case SIGFPE:
					case SIGSEGV:
					case SIGBUS:
						reason += ssprintf(" at 0x%0*lx",
										   int(sizeof(void*) * 2),
										   (unsigned long)crash.si.si_addr);
				}
				break;
			}
		}
		case CrashData::FORCE_CRASH:
			crash.reason[sizeof(crash.reason) - 1] = 0;
			reason = crash.reason;
			break;
	}

	fprintf(CrashDump, "Architecture:   %s\n", Locator::getArchHooks()->GetArchName().c_str());
	fprintf(CrashDump, "Crash reason:   %s\n", reason.c_str());
	fprintf(CrashDump, "Crashed thread: %s\n\n", CrashedThread.c_str());

	fprintf(CrashDump, "Checkpoints:\n");
	for (unsigned i = 0; i < Checkpoints.size(); ++i)
		fputs(Checkpoints[i].c_str(), CrashDump);
	fprintf(CrashDump, "\n");

	for (int i = 0; i < CrashData::MAX_BACKTRACE_THREADS; ++i) {
		if (!crash.BacktracePointers[i][0])
			break;
		fprintf(CrashDump, "Thread: %s\n", crash.m_ThreadName[i]);
		output_stack_trace(CrashDump, crash.BacktracePointers[i]);
		fprintf(CrashDump, "\n");
	}

	fprintf(CrashDump, "Static log:\n");
	fprintf(CrashDump, "%s", Info);
	fprintf(CrashDump, "%s", AdditionalLog);
	fprintf(CrashDump, "\nPartial log:\n");
	for (int i = 0; i < cnt; ++i)
		fprintf(CrashDump, "%s\n", Recent[i]);
	fprintf(CrashDump, "\n");
	fprintf(CrashDump, "-- End of report\n");
	fclose(CrashDump);

#ifdef __APPLE__
	CrashHandler::InformUserOfCrash(sCrashInfoPath);
#else
	/* stdout may have been inadvertently closed by the crash in the parent;
	 * write to /dev/tty instead. */
	FILE* tty = fopen("/dev/tty", "w");
	if (tty == NULL)
		tty = stderr;

	std::stringstream ssa;
	ssa << "\n" << PRODUCT_ID << " has crashed. Debug information has been output to\n\n    " << sCrashInfoPath;
	std::string soup = ssa.str();

	fputs(soup.c_str(), tty);
#endif
}

void
CrashHandler::CrashHandlerHandleArgs(int argc, char* argv[])
{
	g_pCrashHandlerArgv0 = argv[0];
	if (argc == 2 && !strcmp(argv[1], CHILD_MAGIC_PARAMETER)) {
		child_process();
		exit(0);
	}
}
