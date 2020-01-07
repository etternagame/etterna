#include "Etterna/Globals/global.h"
#include "RageDriver.h"

void
DriverList::Add(const istring& sName, CreateRageDriverFn pfn)
{
	if (m_pRegistrees == NULL)
		m_pRegistrees = new std::map<istring, CreateRageDriverFn>;

	ASSERT(m_pRegistrees->find(sName) == m_pRegistrees->end());
	(*m_pRegistrees)[sName] = pfn;
}

RageDriver*
DriverList::Create(const RString& sDriverName)
{
	if (m_pRegistrees == NULL)
		return NULL;

	std::map<istring, CreateRageDriverFn>::const_iterator iter =
	  m_pRegistrees->find(istring(sDriverName));
	if (iter == m_pRegistrees->end())
		return NULL;
	return (iter->second)();
}

RegisterRageDriver::RegisterRageDriver(DriverList* pDriverList,
									   const istring& sName,
									   CreateRageDriverFn pfn)
{
	pDriverList->Add(sName, pfn);
}
