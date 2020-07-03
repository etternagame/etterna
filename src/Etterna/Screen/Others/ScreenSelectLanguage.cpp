#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "ScreenSelectLanguage.h"
#include "arch/ArchHooks/ArchHooks.h"
#include "Etterna/Models/Misc/Foreach.h"

REGISTER_SCREEN_CLASS(ScreenSelectLanguage);

void
ScreenSelectLanguage::Init()
{
	// fill m_aGameCommands before calling Init()
	vector<RString> vs;
	THEME->GetLanguages(vs);
	SortRStringArray(vs, true);

	FOREACH_CONST(RString, vs, s)
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

RString
ScreenSelectLanguage::GetDefaultChoice()
{
	return HOOKS->GetPreferredLanguage();
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
	RString sLangCode = m_aGameCommands[iIndex].m_sName;
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
