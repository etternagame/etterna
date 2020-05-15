#include "Etterna/Globals/global.h"
#include "Dialog.h"
#include "DialogDriver.h"
#if !defined(SMPACKAGE)
#include "Etterna/Singletons/PrefsManager.h"
#endif
#include "RageUtil/Utils/RageUtil.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Misc/RageThreads.h"

#if !defined(SMPACKAGE)
static Preference<std::string> g_sIgnoredDialogs("IgnoredDialogs", "");
#endif

DialogDriver*
MakeDialogDriver()
{
	std::string sDrivers = "win32,cocoa,null";
	vector<std::string> asDriversToTry;
	split(sDrivers, ",", asDriversToTry, true);

	ASSERT(!asDriversToTry.empty());

	std::string sDriver;
	DialogDriver* pRet = nullptr;

	for (unsigned i = 0; pRet == nullptr && i < asDriversToTry.size(); ++i) {
		sDriver = asDriversToTry[i];

#ifdef USE_DIALOG_DRIVER_COCOA
		if (!CompareNoCase(asDriversToTry[i], "Cocoa"))
			pRet = new DialogDriver_MacOSX;
#endif
#ifdef USE_DIALOG_DRIVER_WIN32
		if (!CompareNoCase(asDriversToTry[i], "Win32"))
			pRet = new DialogDriver_Win32;
#endif
#ifdef USE_DIALOG_DRIVER_NULL
		if (!CompareNoCase(asDriversToTry[i], "Null"))
			pRet = new DialogDriver_Null;
#endif

		if (pRet == nullptr) {
			continue;
		}

		std::string sError = pRet->Init();
		if (!sError.empty()) {
			Locator::getLogger()->info("Couldn't load driver {}: {}}", asDriversToTry[i], sError);
			SAFE_DELETE(pRet);
		}
	}

	return pRet;
}

static DialogDriver* g_pImpl = nullptr;
static DialogDriver_Null g_NullDriver;
static bool g_bWindowed =
  true; // Start out true so that we'll show errors before DISPLAY is init'd.

static bool
DialogsEnabled()
{
	return g_bWindowed;
}

void
Dialog::Init()
{
	if (g_pImpl != nullptr)
		return;

	g_pImpl = DialogDriver::Create();

	// DialogDriver_Null should have worked, at least.
	ASSERT(g_pImpl != NULL);
}

void
Dialog::Shutdown()
{
	delete g_pImpl;
	g_pImpl = nullptr;
}

static bool
MessageIsIgnored(const std::string& sID)
{
#if !defined(SMPACKAGE)
	vector<std::string> asList;
	split(g_sIgnoredDialogs, ",", asList);
	for (unsigned i = 0; i < asList.size(); ++i)
		if (!CompareNoCase(sID, asList[i]))
			return true;
#endif
	return false;
}

void
Dialog::IgnoreMessage(const std::string& sID)
{
// We can't ignore messages before PREFSMAN is around.
#if !defined(SMPACKAGE)
	if (PREFSMAN == nullptr) {
		if (!sID.empty())
			Locator::getLogger()->warn("Dialog: message \"{}\" set ID too early for ignorable messages", sID);
		return;
	}

	if (sID.empty())
		return;

	if (MessageIsIgnored(sID))
		return;

	vector<std::string> asList;
	split(g_sIgnoredDialogs, ",", asList);
	asList.push_back(sID);
	g_sIgnoredDialogs.Set(join(",", asList));
	PREFSMAN->SavePrefsToDisk();
#endif
}

void
Dialog::Error(const std::string& sMessage, const std::string& sID)
{
	Dialog::Init();

    Locator::getLogger()->trace("Dialog: \"{}\" [{}]", sMessage, sID);

	if (!sID.empty() && MessageIsIgnored(sID))
		return;

	RageThread::SetIsShowingDialog(true);

	g_pImpl->Error(sMessage, sID);

	RageThread::SetIsShowingDialog(false);
}

void
Dialog::SetWindowed(bool bWindowed)
{
	g_bWindowed = bWindowed;
}

void
Dialog::OK(const std::string& sMessage, const std::string& sID)
{
	Dialog::Init();

    Locator::getLogger()->trace("Dialog: \"{}\" [{}]", sMessage, sID);

	if (!sID.empty() && MessageIsIgnored(sID))
		return;

	RageThread::SetIsShowingDialog(true);

	// only show Dialog if windowed
	if (DialogsEnabled())
		g_pImpl->OK(sMessage, sID); // call derived version
	else
		g_NullDriver.OK(sMessage, sID);

	RageThread::SetIsShowingDialog(false);
}

Dialog::Result
Dialog::OKCancel(const std::string& sMessage, const std::string& sID)
{
	Dialog::Init();

    Locator::getLogger()->trace("Dialog: \"{}\" [{}]", sMessage, sID);

    if (sID != "" && MessageIsIgnored(sID))
		return g_NullDriver.OKCancel(sMessage, sID);

	RageThread::SetIsShowingDialog(true);

	// only show Dialog if windowed
	Dialog::Result ret;
	if (DialogsEnabled())
		ret = g_pImpl->OKCancel(sMessage, sID); // call derived version
	else
		ret = g_NullDriver.OKCancel(sMessage, sID);

	RageThread::SetIsShowingDialog(false);

	return ret;
}

Dialog::Result
Dialog::AbortRetryIgnore(const std::string& sMessage, const std::string& sID)
{
	Dialog::Init();

    Locator::getLogger()->trace("Dialog: \"{}\" [{}]", sMessage, sID);

	if (sID != "" && MessageIsIgnored(sID))
		return g_NullDriver.AbortRetryIgnore(sMessage, sID);

	RageThread::SetIsShowingDialog(true);

	// only show Dialog if windowed
	Dialog::Result ret;
	if (DialogsEnabled())
		ret = g_pImpl->AbortRetryIgnore(sMessage, sID); // call derived version
	else
		ret = g_NullDriver.AbortRetryIgnore(sMessage, sID);

	RageThread::SetIsShowingDialog(false);

	return ret;
}

Dialog::Result
Dialog::AbortRetry(const std::string& sMessage, const std::string& sID)
{
	Dialog::Init();

    Locator::getLogger()->trace("Dialog: \"{}\" [{}]", sMessage, sID);

	if (sID != "" && MessageIsIgnored(sID))
		return g_NullDriver.AbortRetry(sMessage, sID);

	RageThread::SetIsShowingDialog(true);

	// only show Dialog if windowed
	Dialog::Result ret;
	if (DialogsEnabled())
		ret = g_pImpl->AbortRetry(sMessage, sID); // call derived version
	else
		ret = g_NullDriver.AbortRetry(sMessage, sID);

	RageThread::SetIsShowingDialog(false);

	return ret;
}

Dialog::Result
Dialog::YesNo(const std::string& sMessage, const std::string& sID)
{
	Dialog::Init();

    Locator::getLogger()->trace("Dialog: \"{}\" [{}]", sMessage, sID);

	if (sID != "" && MessageIsIgnored(sID))
		return g_NullDriver.YesNo(sMessage, sID);

	RageThread::SetIsShowingDialog(true);

	// only show Dialog if windowed
	Dialog::Result ret;
	if (DialogsEnabled())
		ret = g_pImpl->YesNo(sMessage, sID); // call derived version
	else
		ret = g_NullDriver.YesNo(sMessage, sID);

	RageThread::SetIsShowingDialog(false);

	return ret;
}
