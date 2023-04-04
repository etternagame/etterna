#include "Etterna/Globals/global.h"
#include "CommandLineActions.h"
#include "Etterna/Models/Misc/DateTime.h"
#include "Etterna/FileTypes/IniFile.h"
#include "LuaManager.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Screen/Others/ScreenInstallOverlay.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/FileTypes/XmlFileUtil.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "Core/Misc/AppInfo.hpp"
#include <fmt/format.h>
// only used for Version()
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#endif

/** @brief The directory where languages should be installed. */
const std::string INSTALLER_LANGUAGES_DIR = "Themes/_Installer/Languages/";

std::vector<CommandLineActions::CommandLineArgs> CommandLineActions::ToProcess;

static void
Nsis()
{
	RageFile out;
	if (!out.Open("nsis_strings_temp.inc", RageFile::WRITE))
		RageException::Throw("Error opening file for write.");

	std::vector<std::string> vs;
	FILEMAN->GetDirListing(
	  INSTALLER_LANGUAGES_DIR + "*.ini", vs, ONLY_FILE, false);
	for (auto& s : vs) {
		std::string sThrowAway, sLangCode;
		splitpath(s, sThrowAway, sLangCode, sThrowAway);
		const LanguageInfo* pLI = GetLanguageInfo(sLangCode);

		std::string sLangNameUpper = make_upper(pLI->szEnglishName);

		IniFile ini;
		if (!ini.ReadFile(INSTALLER_LANGUAGES_DIR + s))
			RageException::Throw("Error opening file for read.");
		FOREACH_CONST_Child(&ini, child)
		{
			FOREACH_CONST_Attr(child, attr)
			{
				std::string sName = attr->first;
				std::string sValue = attr->second->GetValue<std::string>();
				s_replace(sValue, "\\n", "$\\n");
				std::string sLine = ssprintf("LangString %s ${LANG_%s} \"%s\"",
											 sName.c_str(),
											 sLangNameUpper.c_str(),
											 sValue.c_str());
				out.PutLine(sLine);
			}
		}
	}
}
static void
LuaInformation()
{
	XNode* pNode = LuaHelpers::GetLuaInformation();
	pNode->AppendAttr("xmlns", "http://www.stepmania.com");
	pNode->AppendAttr("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	pNode->AppendAttr("xsi:schemaLocation", "http://www.stepmania.com Lua.xsd");

	pNode->AppendChild("Version", fmt::format("{} {}", Core::AppInfo::APP_TITLE, Core::AppInfo::APP_VERSION));
	pNode->AppendChild("Date", DateTime::GetNowDate().GetString());

	XmlFileUtil::SaveToFile(pNode, "Lua.xml", "Lua.xsl");

	delete pNode;
}

/**
 * @brief Print out version information.
 *
 * HACK: This function is primarily needed for Windows users.
 * Mac OS X and Linux print out version information on the command line
 * regardless of any preferences (tested by shakesoda on Mac). -aj */
static void
Version()
{
#ifdef _WIN32
	std::string sProductID = fmt::format("{}{}", Core::AppInfo::APP_TITLE, Core::AppInfo::APP_VERSION).c_str();
	std::string sVersion = ssprintf("build %s", Core::AppInfo::GIT_HASH);

	AllocConsole();
	freopen("CONOUT$", "wb", stdout);
	freopen("CONOUT$", "wb", stderr);

	fprintf(stdout,
			"Version Information:\n%s %s\n",
			sProductID.c_str(),
			sVersion.c_str());
	fprintf(stdout, "Press any key to exit.");
	_getch();
#endif // WIN32
}

void
CommandLineActions::Handle(LoadingWindow* pLW)
{
	CommandLineArgs args;
	for (int i = 0; i < g_argc; ++i)
		args.argv.push_back(g_argv[i]);
	ToProcess.push_back(args);

	bool bExitAfter = false;
	if (GetCommandlineArgument("ExportNsisStrings")) {
		Nsis();
		bExitAfter = true;
	}
	if (GetCommandlineArgument("ExportLuaInformation")) {
		LuaInformation();
		bExitAfter = true;
	}
	if (GetCommandlineArgument("version")) {
		Version();
		bExitAfter = true;
	}
	if (bExitAfter)
		exit(0);
}
