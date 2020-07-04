#include "Etterna/Globals/global.h"
#include "GetSysInfo.h"

#if defined(HAVE_SYS_UTSNAME_H)
#include <sys/utsname.h>
#endif

void
GetKernel(std::string& sys, int& iVersion)
{
	struct utsname uts;
	uname(&uts);

	sys = uts.sysname;
	iVersion = 0;

	int iMajor = 0, iMinor = 0, iRevision = 0;
	if (sscanf(uts.release, "%d.%d.%d", &iMajor, &iMinor, &iRevision) >= 2)
		iVersion = (iMajor * 10000) + (iMinor * 100) + (iRevision);
}
