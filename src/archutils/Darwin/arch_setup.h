#ifndef ARCH_SETUP_DARWIN_H
#define ARCH_SETUP_DARWIN_H

// Replace the main function.
extern "C" int
sm_main(int argc, char* argv[]);

#define HAVE_CXA_DEMANGLE
#define HAVE_PTHREAD_COND_TIMEDWAIT
/* This must be defined to 1 because autoconf's AC_CHECK_DECLS macro decides to
 * define this in all cases. If only they could be consistent... */
#define HAVE_DECL_SIGUSR1 1

#define __STDC_FORMAT_MACROS
#define CRASH_HANDLER

#define GL_GET_ERROR_IS_SLOW
// CGFlushDrawable() performs a glFlush() and the docs say not to call glFlush()
#define NO_GL_FLUSH

#define BACKTRACE_METHOD_X86_DARWIN
#define BACKTRACE_LOOKUP_METHOD_DLADDR

#ifndef MACOSX
#define MACOSX
#endif
#ifndef __MACOSX__
#define __MACOSX__
#endif

#include <libkern/OSByteOrder.h>
#define ArchSwap32(n) OSSwapInt32((n))
#define ArchSwap24(n) (ArchSwap32((n)) >> 8)
#define ArchSwap16(n) OSSwapInt16((n))
#define HAVE_BYTE_SWAPS

#endif
