#ifndef RAGE_DRIVER_H
#define RAGE_DRIVER_H

#include "RageUtil/Utils/RageUtil.h"

class RageDriver
{
  public:
	virtual ~RageDriver() = default;
};

using CreateRageDriverFn = RageDriver* (*)();

/* This is created and accessed during C++ static initialization; it must be a
 * POD. */
struct DriverList
{
	void Add(const istring& sName, CreateRageDriverFn pfn);
	RageDriver* Create(const std::string& sDriverName);
	std::map<istring, CreateRageDriverFn>* m_pRegistrees;
};

struct RegisterRageDriver
{
	RegisterRageDriver(DriverList* pDriverList,
					   const istring& sName,
					   CreateRageDriverFn pfn);
};

#endif
