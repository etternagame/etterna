#include "Etterna/Globals/global.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageUtil/Misc/RageException.h"
#include "archutils/Unix/EmergencyShutdown.h"
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <assert.h>

/* We can define this symbol to catch failed assert() calls.  This is only used
 * for library code that uses assert(); internally we always use ASSERT, which
 * does this for all platforms, not just glibc. */

extern "C" void
__assert_fail(const char* assertion,
			  const char* file,
			  unsigned int line,
			  const char* function) throw()
{
	const RString error =
	  ssprintf("Assertion failure: %s: %s", function, assertion);

	Checkpoints::SetCheckpoint(file, line, error);
	sm_crash(assertion);
}

extern "C" void
__assert_perror_fail(int errnum,
					 const char* file,
					 unsigned int line,
					 const char* function) throw()
{
	const RString error =
	  ssprintf("Assertion failure: %s: %s", function, strerror(errnum));

	Checkpoints::SetCheckpoint(file, line, error);
	sm_crash(strerror(errnum));
}

/* Catch unhandled C++ exceptions.  Note that this works in g++ even with
 * -fno-exceptions, in which case it'll be called if any exceptions are thrown
 * at all. */
#include <cxxabi.h>
void
UnexpectedExceptionHandler()
{
	type_info* pException = abi::__cxa_current_exception_type();
	char const* pName = pException->name();
	int iStatus = -1;
	char* pDem = abi::__cxa_demangle(pName, 0, 0, &iStatus);

	const RString error =
	  ssprintf("Unhandled exception: %s", iStatus ? pName : pDem);
	sm_crash(error);
}

void
InstallExceptionHandler()
{
	set_terminate(UnexpectedExceptionHandler);
}
