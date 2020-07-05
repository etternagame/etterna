#ifndef BACKTRACE_NAMES_H
#define BACKTRACE_NAMES_H

struct BacktraceNames
{
	std::string Symbol, File;
	intptr_t Address;
	int Offset;
	void FromAddr(void* const p);
	void FromString(std::string str);
	void Demangle();
	std::string Format() const;
	BacktraceNames()
	  : Address(0)
	  , Offset(0)
	{
	}
};

#endif
