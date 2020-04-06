#include "Etterna/Globals/global.h"
#include "RunningUnderValgrind.h"

#if defined(__i386__) && defined(__GNUC__)
bool
RunningUnderValgrind()
{
	/* Valgrind crashes and burns on pthread_mutex_timedlock. */
	static int under_valgrind = -1;
	if (under_valgrind == -1) {
		unsigned int magic[8] = { 0x00001001, 0, 0, 0, 0, 0, 0, 0 };
		asm("mov %1, %%eax\n"
			"mov $0, %%edx\n"
			"rol $29, %%eax\n"
			"rol $3, %%eax\n"
			"ror $27, %%eax\n"
			"ror $5, %%eax\n"
			"rol $13, %%eax\n"
			"rol $19, %%eax\n"
			"mov %%edx, %0\t"
			: "=r"(under_valgrind)
			: "r"(magic)
			: "eax", "edx");
	}

	return under_valgrind != 0;
}
#else
bool
RunningUnderValgrind()
{
	return false;
}
#endif
