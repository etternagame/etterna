#include "Etterna/Globals/global.h"
#include "DialogDriver.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "RageUtil/Misc/RageLog.h"

std::map<istring, CreateDialogDriverFn>* RegisterDialogDriver::g_pRegistrees;
RegisterDialogDriver::RegisterDialogDriver(const istring& sName,
										   CreateDialogDriverFn pfn)
{
	if (g_pRegistrees == NULL)
		g_pRegistrees = new std::map<istring, CreateDialogDriverFn>;

	ASSERT(g_pRegistrees->find(sName) == g_pRegistrees->end());
	(*g_pRegistrees)[sName] = pfn;
}

REGISTER_DIALOG_DRIVER_CLASS(Null);

DialogDriver*
DialogDriver::Create()
{
	RString sDrivers = "win32,macosx,null";
	std::vector<RString> asDriversToTry;
	split(sDrivers, ",", asDriversToTry, true);

	ASSERT(asDriversToTry.size() != 0);

	FOREACH_CONST(RString, asDriversToTry, Driver)
	{
		std::map<istring, CreateDialogDriverFn>::const_iterator iter =
		  RegisterDialogDriver::g_pRegistrees->find(istring(*Driver));

		if (iter == RegisterDialogDriver::g_pRegistrees->end())
			continue;

		DialogDriver* pRet = (iter->second)();
		DEBUG_ASSERT(pRet);
		const RString sError = pRet->Init();

		if (sError.empty())
			return pRet;
		if (LOG)
			LOG->Info(
			  "Couldn't load driver %s: %s", Driver->c_str(), sError.c_str());
		SAFE_DELETE(pRet);
	}
	return NULL;
}
