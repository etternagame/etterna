#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "ScreenSelectLanguage.h"
#include "Core/Services/Locator.hpp"
#include "Core/Platform/Platform.hpp"
#include "Etterna/Models/Misc/Foreach.h"

REGISTER_SCREEN_CLASS(ScreenSelectLanguage);

void
ScreenSelectLanguage::Init()
{
	// fill m_aGameCommands before calling Init()
	vector<std::string> vs;
	THEME->GetLanguages(vs);
	SortStringArray(vs, true);

	FOREACH_CONST(std::string, vs, s)
	{
		const LanguageInfo* pLI = GetLanguageInfo(*s);

		GameCommand gc;
		gc.m_iIndex = s - vs.begin();
		gc.m_sName = *s;
		gc.m_bInvalid = false;
		if (pLI)
			gc.m_sText =
			  THEME->GetString("NativeLanguageNames", pLI->szEnglishName);
		else
			gc.m_sText = *s;

		m_aGameCommands.push_back(gc);
	}

	ScreenSelectMaster::Init();
}

std::string
ScreenSelectLanguage::GetDefaultChoice()
{
	return Core::Platform::getLanguage();
}

void
ScreenSelectLanguage::BeginScreen()
{
	ScreenSelectMaster::BeginScreen();
}

bool
ScreenSelectLanguage::MenuStart(const InputEventPlus& input)
{
	int iIndex = this->GetSelectionIndex(input.pn);
	std::string sLangCode = m_aGameCommands[iIndex].m_sName;
	PREFSMAN->m_sLanguage.Set(sLangCode);
	PREFSMAN->SavePrefsToDisk();
	THEME->SwitchThemeAndLanguage(THEME->GetCurThemeName(),
								  PREFSMAN->m_sLanguage,
								  PREFSMAN->m_bPseudoLocalize);

	m_soundStart.Play(true);
	this->PostScreenMessage(SM_BeginFadingOut, 0);
	return true;
}

bool
ScreenSelectLanguage::MenuBack(const InputEventPlus& input)
{
	return false; // ignore the press
}
