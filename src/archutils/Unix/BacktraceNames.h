#ifndef BACKTRACE_NAMES_H
#define BACKTRACE_NAMES_H

struct BacktraceNames
{
	RString Symbol, File;
	intptr_t Address;
	int Offset;
	void FromAddr(void* const p);
	void FromString(RString str);
	void Demangle();
	RString Format() const;
	BacktraceNames()
	  : Address(0)
	  , Offset(0)
	{
	}
};

#endif
