#ifndef GCC_BYTE_SWAPS_H
#define GCC_BYTE_SWAPS_H

#if defined(__i386__)

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

inline uint32_t
ArchSwap32(uint32_t n)
{
	asm("xchg %b0, %h0\n"
		"rorl $16, %0\n"
		"xchg %b0, %h0"
		: "=q"(n)
		: "0"(n));
	return n;
}

inline uint32_t
ArchSwap24(uint32_t n)
{
	asm("xchg %b0, %h0\n"
		"rorl $16, %0\n"
		"xchg %b0, %h0\n"
		"shrl $8, %0\n"
		: "=q"(n)
		: "0"(n));
	return n;
}

inline uint16_t
ArchSwap16(uint16_t n)
{
	asm("xchg %b0, %h0\n" : "=q"(n) : "0"(n));
	return n;
}

#define HAVE_BYTE_SWAPS
#endif

#endif
