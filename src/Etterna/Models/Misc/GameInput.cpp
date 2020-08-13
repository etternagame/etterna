#include "Etterna/Globals/global.h"
#include "GameInput.h"
#include "Etterna/Singletons/InputMapper.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ThemeManager.h"

static const char* GameControllerNames[] = {
	"1",
	"2",
};
XToString(GameController);
StringToX(GameController);
LuaXType(GameController);

std::string
GameButtonToString(const InputScheme* pInputs, GameButton i)
{
	return pInputs->GetGameButtonName(i);
}

std::string
GameButtonToLocalizedString(const InputScheme* pInputs, GameButton i)
{
	return THEME->GetString("GameButton", GameButtonToString(pInputs, i));
}

GameButton
StringToGameButton(const InputScheme* pInputs, const std::string& s)
{
	FOREACH_GameButtonInScheme(pInputs, i)
	{
		if (s == GameButtonToString(pInputs, i))
			return i;
	}
	return GameButton_Invalid;
}

std::string
GameInput::ToString(const InputScheme* pInputs) const
{
	return GameControllerToString(controller) + std::string("_") +
		   GameButtonToString(pInputs, button);
}

bool
GameInput::FromString(const InputScheme* pInputs, const std::string& s)
{
	char szController[32] = "";
	char szButton[32] = "";

	if (2 != sscanf(s.c_str(), "%31[^_]_%31[^_]", szController, szButton)) {
		controller = GameController_Invalid;
		return false;
	}

	controller = StringToGameController(szController);
	button = StringToGameButton(pInputs, szButton);
	return true;
};
