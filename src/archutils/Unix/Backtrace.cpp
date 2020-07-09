#include "Etterna/Globals/global.h"
#include "Backtrace.h"
#include "RageUtil/Utils/RageUtil.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <cerrno>
#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif

#if defined(BACKTRACE_METHOD_X86_LINUX)
#include "archutils/Common/PthreadHelpers.h"

#if defined(__linux__) && !defined(ANDROID)
#include <limits.h>
#endif

#include <algorithm> // Literally just used for a single "min" call within PointsToValidCall

static const char*
itoa(unsigned n)
{
	static char ret[32];
	char* p = ret;
	for (int div = 1000000000; div > 0; div /= 10) {
		*p++ = (n / div) + '0';
		n %= div;
	}
	*p = 0;
	p = ret;
	while (p[0] == '0' && p[1])
		++p;
	return p;
}

static intptr_t
xtoi(const char* hex)
{
	intptr_t ret = 0;
	for (;;) {
		int val = -1;
		if (*hex >= '0' && *hex <= '9') {
			val = *hex - '0';
		} else if (*hex >= 'A' && *hex <= 'F') {
			val = *hex - 'A' + 10;
		} else if (*hex >= 'a' && *hex <= 'f') {
			val = *hex - 'a' + 10;
		} else {
			break;
		}
		hex++;

		ret *= 16;
		ret += val;
	}
	return ret;
}

enum
{
	READABLE_ONLY = 1,
	EXECUTABLE_ONLY = 2
};
static int
get_readable_ranges(const void** starts,
					const void** ends,
					int size,
					int type = READABLE_ONLY)
{
	char path[PATH_MAX] = "/proc/";
	strcat(path, itoa(getpid()));
	strcat(path, "/maps");

	int fd = open(path, O_RDONLY);
	if (fd == -1)
		return false;

	/*
	 * Format:
	 *
	 * 402dd000-402de000 rw-p 00010000 03:03 16815669   /lib/libnsl-2.3.1.so
	 * or
	 * bfffb000-c0000000 rwxp ffffc000 00:00 0
	 */
	char file[1024];
	int file_used = 0;
	bool eof = false;
	int got = 0;
	while (!eof && got < size - 1) {
		int ret = read(fd, file + file_used, sizeof(file) - file_used);
		if (ret == -1)
			return false;
		if (ret < int(sizeof(file)) - file_used)
			eof = true;

		file_used += ret;

		/* Parse lines. */
		while (got < size - 1) {
			char* p = (char*)memchr(file, '\n', file_used);
			if (p == NULL)
				break;
			*p++ = 0;

			char line[1024];
			strcpy(line, file);
			memmove(file, p, file_used);
			file_used -= p - file;

			/* Search for the hyphen. */
			char* hyphen = strchr(line, '-');
			if (hyphen == NULL)
				continue; /* Parse error. */

			/* Search for the space. */
			char* space = strchr(hyphen, ' ');
			if (space == NULL)
				continue; /* Parse error. */

			/* " rwxp".  If space[1] isn't 'r', then the block isn't readable.
			 */
			if (type & READABLE_ONLY)
				if (strlen(space) < 2 || space[1] != 'r')
					continue;
			/* " rwxp".  If space[3] isn't 'x', then the block isn't readable.
			 */
			if (type & EXECUTABLE_ONLY)
				if (strlen(space) < 4 || space[3] != 'x')
					continue;

			/* If, for some reason, either end is NULL, skip it; that's our
			 * terminator. */
			const void* start = (const void*)xtoi(line);
			const void* end = (const void*)xtoi(hyphen + 1);
			if (start != NULL && end != NULL) {
				*starts++ = start;
				*ends++ = end;
			}

			++got;
		}

		if (file_used == sizeof(file)) {
			/* Line longer than the buffer.  Weird; bail. */
			break;
		}
	}

	close(fd);

	*starts++ = NULL;
	*ends++ = NULL;

	return got;
}

/* If the address is readable (eg. reading it won't cause a segfault), return
 * the block it's in.  Otherwise, return -1. */
static int
find_address(const void* p, const void** starts, const void** ends)
{
	for (int i = 0; starts[i]; ++i) {
		/* Found it. */
		if (starts[i] <= p && p < ends[i])
			return i;
	}

	return -1;
}

static void* SavedStackPointer = NULL;

void
InitializeBacktrace()
{
	static bool bInitialized = false;
	if (bInitialized)
		return;
	bInitialized = true;

	/* We might have a different stack in the signal handler.  Record a pointer
	 * that lies in the real stack, so we can look it up later. */
	SavedStackPointer = __builtin_frame_address(0);
}

/* backtrace() for x86 Linux, tested with kernel 2.4.18, glibc 2.3.1. */
const void *g_ReadableBegin[1024], *g_ReadableEnd[1024];
const void *g_ExecutableBegin[1024], *g_ExecutableEnd[1024];
/* Indexes in g_ReadableBegin of the stack(s), or -1: */
int g_StackBlock1, g_StackBlock2;

/* This matches the layout of the stack.  The frame pointer makes the
 * stack a linked list. */
struct StackFrame
{
	const StackFrame* link;
	const void* return_address;
};

/* Return true if the given pointer is in readable memory, and on the stack. */
bool
IsOnStack(const void* p)
{
	int val = find_address(p, g_ReadableBegin, g_ReadableEnd);
	return val != -1 && (val == g_StackBlock1 || val == g_StackBlock2);
}

/* Return true if the given pointer is in executable memory. */
bool
IsExecutableAddress(const void* p)
{
	int val = find_address(p, g_ExecutableBegin, g_ExecutableEnd);
	return val != -1;
}

/* Return true if the given stack frame is in readable memory. */
bool
IsReadableFrame(const StackFrame* frame)
{
	if (!IsOnStack(&frame->link))
		return false;
	if (!IsOnStack(&frame->return_address))
		return false;
	return true;
}

/* The following from VirtualDub: */
/* ptr points to a return address, and does not have to be word-aligned. */
static bool
PointsToValidCall(const void* ptr)
{
	const char* buf = (char*)ptr;

	/* We're reading buf backwards, between buf[-7] and buf[-1].  Find out how
	 * far we can read. */
	int len = 7;
	while (len) {
		int val = find_address(buf - len, g_ReadableBegin, g_ReadableEnd);
		if (val != -1)
			break;
		--len;
	}

	// Permissible CALL sequences that we care about:
	//
	//	E8 xx xx xx xx			CALL near relative
	//	FF (group 2)			CALL near absolute indirect
	//
	// Minimum sequence is 2 bytes (call eax).
	// Maximum sequence is 7 bytes (call dword ptr [eax+disp32]).

	if (len >= 5 && buf[-5] == '\xe8')
		return true;

	// FF 14 xx					CALL [reg32+reg32*scale]
	if (len >= 3 && buf[-3] == '\xff' && buf[-2] == '\x14')
		return true;

	// FF 15 xx xx xx xx		CALL disp32
	if (len >= 6 && buf[-6] == '\xff' && buf[-5] == '\x15')
		return true;

	// FF 00-3F(!14/15)			CALL [reg32]
	if (len >= 2 && buf[-2] == '\xff' && (unsigned char)buf[-1] < '\x40')
		return true;

	// FF D0-D7					CALL reg32
	if (len >= 2 && buf[-2] == '\xff' && char(buf[-1] & 0xF8) == '\xd0')
		return true;

	// FF 50-57 xx				CALL [reg32+reg32*scale+disp8]
	if (len >= 3 && buf[-3] == '\xff' && char(buf[-2] & 0xF8) == '\x50')
		return true;

	// FF 90-97 xx xx xx xx xx	CALL [reg32+reg32*scale+disp32]
	if (len >= 7 && buf[-7] == '\xff' && char(buf[-6] & 0xF8) == '\x90')
		return true;

	return false;
}

/* Return true if frame appears to be a legitimate, readable stack frame. */
bool
IsValidFrame(const StackFrame* frame)
{
	if (!IsReadableFrame(frame))
		return false;

	/* The frame link should only go upwards. */
	if (frame->link <= frame)
		return false;

	/* The link should be on the stack. */
	if (!IsOnStack(frame->link))
		return false;

	/* The return address should be in a readable, executable page. */
	if (!IsExecutableAddress(frame->return_address))
		return false;

	/* The return address should follow a CALL opcode. */
	if (!PointsToValidCall(frame->return_address))
		return false;

	return true;
}

/* This x86 backtracer attempts to walk the stack frames.  If we come to a
 * place that doesn't look like a valid frame, we'll look forward and try
 * to find one again. */
static void
do_backtrace(const void** buf, size_t size, const BacktraceContext* ctx)
{
	/* Read /proc/pid/maps to find the address range of the stack. */
	get_readable_ranges(g_ReadableBegin, g_ReadableEnd, 1024);
	get_readable_ranges(g_ExecutableBegin,
						g_ExecutableEnd,
						1024,
						READABLE_ONLY | EXECUTABLE_ONLY);

	/* Find the stack memory blocks. */
	g_StackBlock1 = find_address(ctx->sp, g_ReadableBegin, g_ReadableEnd);
	g_StackBlock2 =
	  find_address(SavedStackPointer, g_ReadableBegin, g_ReadableEnd);

	/* Put eip at the top of the backtrace. */
	/* XXX: We want EIP even if it's not valid, but we can't put NULL on the
	 * list, since it's NULL-terminated.  Hmm. */
	unsigned i = 0;
	if (i < size - 1 && ctx->ip) // -1 for NULL
		buf[i++] = ctx->ip;

	/* If we did a CALL to an invalid address (eg. call a NULL callback), then
	 * we won't have a stack frame for the function that called it (since the
	 * stack frame is set up by the called function), but if esp hasn't been
	 * changed after the CALL, the return address will be esp[0].  Grab it. */
	if (IsOnStack(ctx->sp)) {
		const void* p = ((const void**)ctx->sp)[0];
		if (IsExecutableAddress(p) && PointsToValidCall(p) &&
			i < size - 1) // -1 for NULL
			buf[i++] = p;
	}

#if 0
	/* ebp is usually the frame pointer. */
	const StackFrame *frame = (StackFrame *) ctx->bp;

	/* If ebp doesn't point to a valid stack frame, we're probably in
	 * -fomit-frame-pointer code.  Ignore it; use esp instead.  It probably
	 * won't point to a stack frame, but it should at least give us a starting
	 * point in the stack. */
	if( !IsValidFrame( frame ) )
		frame = (StackFrame *) ctx->sp;
#endif

	/* Actually, let's just use esp.  Even if ebp points to a valid stack frame,
	 * there might be -fomit-frame-pointer calls in front of it, and we want to
	 * get those. */
	const StackFrame* frame = (StackFrame*)ctx->sp;

	while (i < size - 1) // -1 for NULL
	{
		/* Make sure that this frame address is readable, and is on the stack.
		 */
		if (!IsReadableFrame(frame))
			break;

		if (!IsValidFrame(frame)) {
			/* We've lost the frame.  We might have crashed while in a call in
			 * -fomit-frame-pointer code.  Iterate through the stack word by
			 * word.  If a word is possibly a valid return address, record it.
			 * This is important; if we don't do this, we'll lose too many stack
			 * frames at the top of the trace.  This can have false positives,
			 * and introduce garbage into the trace, but we should eventually
			 * find a real stack frame. */
			void** p = (void**)frame;
			if (IsExecutableAddress(*p) && PointsToValidCall(*p))
				buf[i++] = *p;

			/* The frame pointer is invalid.  Just move forward one word. */
			frame = (StackFrame*)(((char*)frame) + 4);
			continue;
		}

		/* Valid frame.  Store the return address, and hop forward. */
		buf[i++] = frame->return_address;
		frame = frame->link;
	}

	buf[i] = NULL;
}

#if defined(__i386__)
void
GetSignalBacktraceContext(BacktraceContext* ctx, const ucontext_t* uc)
{
	ctx->ip = (void*)uc->uc_mcontext.gregs[REG_EIP];
	ctx->bp = (void*)uc->uc_mcontext.gregs[REG_EBP];
	ctx->sp = (void*)uc->uc_mcontext.gregs[REG_ESP];
	ctx->pid = GetCurrentThreadId();
}
#elif defined(__x86_64__)
void
GetSignalBacktraceContext(BacktraceContext* ctx, const ucontext_t* uc)
{
	ctx->ip = (void*)uc->uc_mcontext.gregs[REG_RIP];
	ctx->bp = (void*)uc->uc_mcontext.gregs[REG_RBP];
	ctx->sp = (void*)uc->uc_mcontext.gregs[REG_RSP];
	ctx->pid = GetCurrentThreadId();
}
#else
#error
#endif

void
GetBacktrace(const void** buf, size_t size, const BacktraceContext* ctx)
{
	InitializeBacktrace();

	BacktraceContext CurrentCtx;
	if (ctx == NULL) {
		ctx = &CurrentCtx;

		CurrentCtx.ip = NULL;
		CurrentCtx.bp = __builtin_frame_address(0);
		CurrentCtx.sp = __builtin_frame_address(0);
		CurrentCtx.pid = GetCurrentThreadId();
	}

	do_backtrace(buf, size, ctx);
}
#elif defined(BACKTRACE_METHOD_X86_DARWIN)
#include <mach/mach.h>

static vm_address_t g_StackPointer = 0;
struct Frame
{
	const Frame* link;
	const void* return_address;
};
#define PROT_RW (VM_PROT_READ | VM_PROT_WRITE)
#define PROT_EXE (VM_PROT_READ | VM_PROT_EXECUTE)

/* Returns the starting address and the protection. Pass in mach_task_self() and
 * the starting address. */
static bool
GetRegionInfo(mach_port_t machPort,
			  const void* address,
			  vm_address_t& startOut,
			  vm_prot_t& protectionOut)
{
	struct vm_region_basic_info_64 info;
	mach_msg_type_number_t infoCnt = VM_REGION_BASIC_INFO_COUNT_64;
	mach_port_t unused;
	vm_size_t size = 0;
	vm_address_t start = vm_address_t(address);
	kern_return_t ret = vm_region_64(machPort,
									 &start,
									 &size,
									 VM_REGION_BASIC_INFO_64,
									 (vm_region_info_t)&info,
									 &infoCnt,
									 &unused);

	if (ret != KERN_SUCCESS || start >= (vm_address_t)address ||
		(vm_address_t)address >= start + size) {
		return false;
	}
	startOut = start;
	protectionOut = info.protection;
	return true;
}

void
InitializeBacktrace()
{
	static bool bInitialized = false;

	if (bInitialized)
		return;
	vm_prot_t protection;
	if (!GetRegionInfo(mach_task_self(),
					   __builtin_frame_address(0),
					   g_StackPointer,
					   protection) ||
		protection != PROT_RW) {
		g_StackPointer = 0;
	}
	bInitialized = true;
}

void
GetSignalBacktraceContext(BacktraceContext* ctx, const ucontext_t* uc)
{
#if !defined(__APPLE__)
	ctx->ip = (void*)uc->uc_mcontext->ss.eip;
	ctx->bp = (void*)uc->uc_mcontext->ss.ebp;
	ctx->sp = (void*)uc->uc_mcontext->ss.esp;
#elif defined(__i386__)
	ctx->ip = (void*)uc->uc_mcontext->__ss.__eip;
	ctx->bp = (void*)uc->uc_mcontext->__ss.__ebp;
	ctx->sp = (void*)uc->uc_mcontext->__ss.__esp;
#elif defined(__x86_64__)
	ctx->ip = (void*)uc->uc_mcontext->__ss.__rip;
	ctx->bp = (void*)uc->uc_mcontext->__ss.__rbp;
	ctx->sp = (void*)uc->uc_mcontext->__ss.__rsp;
#endif
}

/* The following from VirtualDub: */
/* ptr points to a return address, and does not have to be word-aligned. */
static bool
PointsToValidCall(vm_address_t start, const void* ptr)
{
	const char* buf = (const char*)ptr;

	/* We're reading buf backwards, between buf[-7] and buf[-1].  Find out how
	 * far we can read. */
	const int len = std::min(intptr_t(ptr) - start, 7UL);

	// Permissible CALL sequences that we care about:
	//
	//	E8 xx xx xx xx			CALL near relative
	//	FF (group 2)			CALL near absolute indirect
	//
	// Minimum sequence is 2 bytes (call eax).
	// Maximum sequence is 7 bytes (call dword ptr [eax+disp32]).

	if (len >= 5 && buf[-5] == '\xe8')
		return true;

	// FF 14 xx					CALL [reg32+reg32*scale]
	if (len >= 3 && buf[-3] == '\xff' && buf[-2] == '\x14')
		return true;

	// FF 15 xx xx xx xx		CALL disp32
	if (len >= 6 && buf[-6] == '\xff' && buf[-5] == '\x15')
		return true;

	// FF 00-3F(!14/15)			CALL [reg32]
	if (len >= 2 && buf[-2] == '\xff' && (unsigned char)buf[-1] < '\x40')
		return true;

	// FF D0-D7					CALL reg32
	if (len >= 2 && buf[-2] == '\xff' && char(buf[-1] & 0xF8) == '\xd0')
		return true;

	// FF 50-57 xx				CALL [reg32+reg32*scale+disp8]
	if (len >= 3 && buf[-3] == '\xff' && char(buf[-2] & 0xF8) == '\x50')
		return true;

	// FF 90-97 xx xx xx xx xx	CALL [reg32+reg32*scale+disp32]
	if (len >= 7 && buf[-7] == '\xff' && char(buf[-6] & 0xF8) == '\x90')
		return true;

	return false;
}

void
GetBacktrace(const void** buf, size_t size, const BacktraceContext* ctx)
{
	InitializeBacktrace();

	if (g_StackPointer == 0) {
		buf[0] = BACKTRACE_METHOD_NOT_AVAILABLE;
		buf[1] = NULL;
		return;
	}

	BacktraceContext CurrentCtx;
	if (ctx == NULL) {
		ctx = &CurrentCtx;

		CurrentCtx.ip = NULL;
		CurrentCtx.bp = __builtin_frame_address(0);
		CurrentCtx.sp = __builtin_frame_address(0);
	}

	mach_port_t self = mach_task_self();
	vm_address_t stackPointer = 0;
	vm_prot_t protection = 0;
	vm_address_t start = 0;

	size_t i = 0;
	if (i < size - 1 && ctx->ip)
		buf[i++] = ctx->ip;

	if (GetRegionInfo(self, ctx->sp, stackPointer, protection) &&
		protection == (VM_PROT_READ | VM_PROT_WRITE)) {
		const void* p = *(const void**)ctx->sp;
		if (GetRegionInfo(self, p, start, protection) &&
			(protection & (VM_PROT_READ | VM_PROT_EXECUTE)) ==
			  (VM_PROT_READ | VM_PROT_EXECUTE) &&
			PointsToValidCall(start, p) && i < size - 1) {
			buf[i++] = p;
		}
	}

	GetRegionInfo(self, ctx->sp, stackPointer, protection);
	if (protection != PROT_RW) {
		/* There isn't much we can do if this is the case. The stack should be
		 * read/write and since it isn't, give up. */
		buf[i] = NULL;
		return;
	}
	const Frame* frame = (Frame*)ctx->sp;

	while (i < size - 1) {
		// Make sure this is on the stack
		if (!GetRegionInfo(self, frame, start, protection) ||
			protection != PROT_RW)
			break;
		if ((start != g_StackPointer && start != stackPointer) ||
			uintptr_t(frame) - uintptr_t(start) < sizeof(Frame))
			break;

		/* The stack pointer is always 16 byte aligned _before_ the call. Thus a
		 * valid frame should look like the follwoing. |                  | |
		 * Caller's frame  |
		 * -------------------- 16 byte boundary
		 * |     Linkage      | This is return_address
		 * - - - - - - - - - -
		 * |   Saved %ebp     | This is link
		 * - - - - - - - - - -
		 * |    Rest of       |
		 * |  Callee's frame  |
		 *
		 * Therefore, frame + 8 should be on a 16 byte boundary, the frame link
		 * should be at a higher address, the link should be on the stack and it
		 * should be RW. The return address should be EXE and point to a valid
		 * call (well, just after). */
		if ((((uintptr_t)frame + 8) & 0xF) != 0 || // boundary
			frame->link <= frame ||				   // the frame link goes up
			!GetRegionInfo(self, frame->link, start, protection) ||
			(start != g_StackPointer &&
			 start != stackPointer) || // the link is on the stack
			protection != PROT_RW ||   // RW
			!GetRegionInfo(self, frame->return_address, start, protection) ||
			protection != PROT_EXE ||						  // EXE
			!PointsToValidCall(start, frame->return_address)) // follows a CALL
		{
			/* This is not a valid frame but we might be in code compiled with
			 * -fomit-frame-pointer so look at each address on the stack that is
			 * 4 bytes below a 16 byte boundary. */
			if ((((uintptr_t)frame + 4) & 0xF) == 0) {
				void* p = *(void**)frame;
				if (GetRegionInfo(self, p, start, protection) &&
					protection == PROT_EXE && PointsToValidCall(start, p)) {
					buf[i++] = p;
				}
			}
			frame = (Frame*)(intptr_t(frame) + 4);
			continue;
		}
		// Valid.
		buf[i++] = frame->return_address;
		frame = frame->link;
	}

	buf[i] = NULL;
}
#undef PROT_RW
#undef PROT_EXE

#elif defined(BACKTRACE_METHOD_POWERPC_DARWIN)
struct Frame
{
	Frame* stackPointer;
	long conditionReg;
	void* linkReg;
};

void
GetSignalBacktraceContext(BacktraceContext* ctx, const ucontext_t* uc)
{
	ctx->PC = (const void*)uc->uc_mcontext->ss.srr0;
	ctx->FramePtr = (const Frame*)uc->uc_mcontext->ss.r1;
}

void
InitializeBacktrace()
{
}

void
GetBacktrace(const void** buf, size_t size, const BacktraceContext* ctx)
{
	BacktraceContext CurrentCtx;
	if (ctx == NULL) {
		ctx = &CurrentCtx;

		/* __builtin_frame_address is broken on OS X; it sometimes returns bogus
		 * results. */
		register void* r1 __asm__("r1");
		CurrentCtx.FramePtr = (const Frame*)r1;
		CurrentCtx.PC = NULL;
	}

	const Frame* frame = ctx->FramePtr;

	unsigned i = 0;
	if (ctx->PC && i < size - 1)
		buf[i++] = ctx->PC;

	while (frame && i < size - 1) // -1 for NULL
	{
		if (frame->linkReg)
			buf[i++] = frame->linkReg;

		frame = frame->stackPointer;
	}

	buf[i] = NULL;
}

#elif defined(BACKTRACE_METHOD_PPC_LINUX)
#include <asm/ptrace.h>

struct Frame
{
	Frame* stackPointer;
	void* linkReg;
};

void
GetSignalBacktraceContext(BacktraceContext* ctx, const ucontext_t* uc)
{
	// Wow, this is an ugly structure...
	ctx->PC = (void*)uc->uc_mcontext.uc_regs->gregs[PT_NIP];
	ctx->FramePtr = (const Frame*)uc->uc_mcontext.uc_regs->gregs[PT_R1];
}

void
InitializeBacktrace()
{
}

void
GetBacktrace(const void** buf, size_t size, const BacktraceContext* ctx)
{
	BacktraceContext CurrentCtx;

	if (ctx == NULL) {
		ctx = &CurrentCtx;

		register void* r1 __asm__("1");
		CurrentCtx.FramePtr = (const Frame*)r1;
		CurrentCtx.PC = NULL;
	}

	const Frame* frame = (const Frame*)ctx->FramePtr;
	unsigned i = 0;

	if (ctx->PC && i < size - 1)
		buf[i++] = ctx->PC;

	while (frame && i < size - 1) {
		if (frame->linkReg)
			buf[i++] = frame->linkReg;
	}
	buf[i] = NULL;
}

#else

#warning Undefined BACKTRACE_METHOD_*
void
InitializeBacktrace()
{
}

void
GetBacktrace(const void** buf, size_t size, const BacktraceContext* ctx)
{
	buf[0] = BACKTRACE_METHOD_NOT_AVAILABLE;
	buf[1] = NULL;
}

#endif
