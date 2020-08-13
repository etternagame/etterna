#include "Etterna/Globals/global.h"
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

static std::map<std::string, ScreenMessage>* m_pScreenMessages;

ScreenMessage
ScreenMessageHelpers::ToScreenMessage(const std::string& sName)
{
	if (m_pScreenMessages == nullptr)
		m_pScreenMessages = new std::map<std::string, ScreenMessage>;

	if (m_pScreenMessages->find(sName) == m_pScreenMessages->end())
		(*m_pScreenMessages)[sName] = (ScreenMessage)sName;

	return (*m_pScreenMessages)[sName];
}

std::string
ScreenMessageHelpers::ScreenMessageToString(ScreenMessage SM)
{
	for (auto& it : *m_pScreenMessages)
		if (SM == it.second)
			return it.first;

	return std::string();
}
