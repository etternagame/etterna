#ifndef GLOBAL_H
#define GLOBAL_H

#include "config.hpp"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#pragma warning(disable : 4996)
#pragma warning(disable : 4267)
#pragma warning(disable : 4244)
/** @brief This macro is for INT8_MIN, etc. */
#define __STDC_LIMIT_MACROS
/** @brief This macro is for INT64_C, etc. */
#define __STDC_CONSTANT_MACROS

/* Platform-specific fixes. */
#ifdef _WIN32
#include "archutils/Win32/arch_setup.h"
#elif defined(__APPLE__)
#include "archutils/Darwin/arch_setup.h"
#elif defined(__unix__)
#include "archutils/Unix/arch_setup.h"
#endif

#if defined(HAVE_STDINT_H) /* need to define int64_t if so */
#include <cstdint>
#endif
#if defined(HAVE_INTTYPES_H)
#include <cinttypes>
#endif

/* Branch optimizations: */
#if defined(__GNUC__)
#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#if defined(NEED_CSTDLIB_WORKAROUND)
#define llabs ::llabs
#endif

#ifdef ASSERT
#undef ASSERT
#endif

/**
 * @brief Define a macro to tell the compiler that a function doesn't return.
 *
 * This just improves compiler warnings.  This should be placed near the
 * beginning of the function prototype (although it looks better near the end,
 * VC only accepts it at the beginning). */
#if defined(_MSC_VER)
#define NORETURN __declspec(noreturn)
#elif defined(__GNUC__) &&                                                     \
  (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5))
#define NORETURN __attribute__((__noreturn__))
#else
#define NORETURN
#endif

/**
 * @brief A crash has occurred, and we're not getting out of it easily.
 *
 * For most users, this will result in a crashinfo.txt file being generated.
 * For anyone that is using a debug build, a debug break will be thrown to
 * allow viewing the current process.
 * @param reason the crash reason as determined by prior function calls.
 * @return nothing: there is no escape without quitting the program.
 */
void NORETURN
sm_crash(const char* reason = "Internal error");

/**
 * @brief Assertion that sets an optional message and brings up the crash
 * handler, so we get a backtrace.
 *
 * This should probably be used instead of throwing an exception in most
 * cases we expect never to happen (but not in cases that we do expect,
 * such as DSound init failure.) */
#define FAIL_M(MESSAGE)                                                        \
	do {                                                                       \
		sm_crash(std::string(MESSAGE).c_str());                                \
	} while (0)
#define ASSERT_M(COND, MESSAGE)                                                \
	do {                                                                       \
		if (unlikely(!(COND))) {                                               \
			FAIL_M(std::string(MESSAGE).c_str());                              \
		}                                                                      \
	} while (0)

#if !defined(CO_EXIST_WITH_MFC)
#define ASSERT(COND) ASSERT_M((COND), "Assertion '" #COND "' failed")
#endif

/** @brief Use this to catch switching on invalid values */
#define DEFAULT_FAIL(i)                                                        \
	default:                                                                   \
		FAIL_M(ssprintf("%s = %i", #i, (i)))

void
ShowWarningOrTrace(const char* file,
				   int line,
				   const char* message,
				   bool bWarning); // don't pull in LOG here

#ifdef DEBUG
// No reason to kill the program. A lot of these don't produce a crash in NDEBUG
// so why stop?
// TODO: These should have something you can hook a breakpoint on.
#define DEBUG_ASSERT_M(COND, MESSAGE)                                          \
	if (unlikely(!(COND)))                                                     \
	WARN(MESSAGE)
#define DEBUG_ASSERT(COND) DEBUG_ASSERT_M(COND, "Debug assert failed")
#else
/** @brief A dummy define to keep things going smoothly. */
#define DEBUG_ASSERT(x)
/** @brief A dummy define to keep things going smoothly. */
#define DEBUG_ASSERT_M(x, y)
#endif

/* Use SM_UNIQUE_NAME to get the line number concatenated to x. This is useful
 * for generating unique identifiers in other macros.  */
#define SM_UNIQUE_NAME3(x, line) x##line
#define SM_UNIQUE_NAME2(x, line) SM_UNIQUE_NAME3(x, line)
#define SM_UNIQUE_NAME(x) SM_UNIQUE_NAME2(x, __LINE__)

template<bool>
struct CompileAssert;
template<>
struct CompileAssert<true>
{
};
template<int>
struct CompileAssertDecl
{
};

// Ignore "unused-local-typedef" warnings for COMPILE_ASSERT
#if defined(__clang__)
#define COMPILE_ASSERT_PRE                                                     \
	_Pragma("clang diagnostic push")                                           \
	  _Pragma("clang diagnostic ignored \"-Wunused-local-typedef\"")
#define COMPILE_ASSERT_POST _Pragma("clang diagnostic pop")
#else
#define COMPILE_ASSERT_PRE
#define COMPILE_ASSERT_POST
#endif
#define COMPILE_ASSERT(COND)                                                   \
	COMPILE_ASSERT_PRE                                                         \
	typedef CompileAssertDecl<sizeof(CompileAssert<!!(COND)>)>                 \
	  CompileAssertInst COMPILE_ASSERT_POST

#include "RageUtil/Misc/RageException.h"
/* Don't include our own headers here, since they tend to change often. */

#endif
