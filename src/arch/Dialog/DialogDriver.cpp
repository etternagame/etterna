#include "Etterna/Globals/global.h"
#include "DialogDriver.h"
#include "Core/Services/Locator.hpp"

std::map<istring, CreateDialogDriverFn>* RegisterDialogDriver::g_pRegistrees;
RegisterDialogDriver::RegisterDialogDriver(const istring& sName,
										   CreateDialogDriverFn pfn)
{
	if (g_pRegistrees == nullptr)
		g_pRegistrees = new std::map<istring, CreateDialogDriverFn>;

	ASSERT(g_pRegistrees->find(sName) == g_pRegistrees->end());
	(*g_pRegistrees)[sName] = pfn;
}

REGISTER_DIALOG_DRIVER_CLASS(Null);

DialogDriver*
DialogDriver::Create()
{
	std::string sDrivers = "win32,macosx,null";
	vector<std::string> asDriversToTry;
	split(sDrivers, ",", asDriversToTry, true);

	ASSERT(asDriversToTry.size() != 0);

	for (auto& Driver : asDriversToTry) {
		std::map<istring, CreateDialogDriverFn>::const_iterator iter =
		  RegisterDialogDriver::g_pRegistrees->find(istring(Driver.c_str()));

		if (iter == RegisterDialogDriver::g_pRegistrees->end())
			continue;

		DialogDriver* pRet = (iter->second)();
		DEBUG_ASSERT(pRet);
		const std::string sError = pRet->Init();

		if (sError.empty())
			return pRet;
		Locator::getLogger()->info("Couldn't load driver {}: {}", Driver, sError.c_str());
		SAFE_DELETE(pRet);
	}
	return nullptr;
}
