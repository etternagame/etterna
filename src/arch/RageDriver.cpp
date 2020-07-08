#include "Etterna/Globals/global.h"
#include "RageDriver.h"

void
DriverList::Add(const istring& sName, CreateRageDriverFn pfn)
{
	if (m_pRegistrees == nullptr)
		m_pRegistrees = new std::map<istring, CreateRageDriverFn>;

	ASSERT(m_pRegistrees->find(sName) == m_pRegistrees->end());
	(*m_pRegistrees)[sName] = pfn;
}

RageDriver*
DriverList::Create(const std::string& sDriverName)
{
	if (m_pRegistrees == nullptr)
		return nullptr;

	std::map<istring, CreateRageDriverFn>::const_iterator iter =
	  m_pRegistrees->find(istring(sDriverName.c_str()));
	if (iter == m_pRegistrees->end())
		return nullptr;
	return (iter->second)();
}

RegisterRageDriver::RegisterRageDriver(DriverList* pDriverList,
									   const istring& sName,
									   CreateRageDriverFn pfn)
{
	pDriverList->Add(sName, pfn);
}
