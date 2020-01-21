#ifndef ARCH_SETUP_UNIX_H
#define ARCH_SETUP_UNIX_H

#if !defined(_STDC_C99) && !defined(__C99FEATURES__)
#define __C99FEATURES__
#endif

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#if defined(HAVE_STDINT_H) /* need to define int64_t if so */
#include <stdint.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "archutils/Common/gcc_byte_swaps.h"
#endif
