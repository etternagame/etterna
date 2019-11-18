#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "RageUtil/Misc/RageLog.h"
#include "ScreenMessage.h"
#include <map>

const ScreenMessage SM_Invalid = "";
AutoScreenMessage(SM_None);
AutoScreenMessage(SM_MenuTimer);
AutoScreenMessage(SM_DoneFadingIn);
AutoScreenMessage(SM_BeginFadingOut);
AutoScreenMessage(SM_GoToNextScreen);
AutoScreenMessage(SM_GoToPrevScreen);
AutoScreenMessage(SM_GainFocus);
AutoScreenMessage(SM_LoseFocus);
AutoScreenMessage(SM_Pause);
AutoScreenMessage(SM_Success);
AutoScreenMessage(SM_Failure);
AutoScreenMessage(SM_GoToDisconnectScreen);

static map<RString, ScreenMessage>* m_pScreenMessages;

ScreenMessage
ScreenMessageHelpers::ToScreenMessage(const RString& sName)
{
	if (m_pScreenMessages == NULL)
		m_pScreenMessages = new map<RString, ScreenMessage>;

	if (m_pScreenMessages->find(sName) == m_pScreenMessages->end())
		(*m_pScreenMessages)[sName] = (ScreenMessage)sName;

	return (*m_pScreenMessages)[sName];
}

RString
ScreenMessageHelpers::ScreenMessageToString(ScreenMessage SM)
{
	FOREACHM(RString, ScreenMessage, *m_pScreenMessages, it)
	if (SM == it->second)
		return (*it).first;

	return RString();
}
