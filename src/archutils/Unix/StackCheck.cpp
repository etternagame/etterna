#include "Etterna/Globals/global.h"

/* Newer g++ versions (around 4.2) output __stack_chk_fail calls.  This is
 * grossly backwards-incompatible.  It can be disabled, but you'd have to
 * disable it in any statically linked libraries: all Ubuntu static libraries
 * have this enabled.  Work around it by declaring the symbol manually. */
extern "C" void
__stack_chk_fail(void) __attribute__((__weak__)) __attribute__((__noreturn__));
extern "C" void
__stack_chk_fail(void)
{
	abort();
}
