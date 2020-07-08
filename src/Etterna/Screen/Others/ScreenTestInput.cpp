#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Singletons/InputMapper.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Misc/RageInput.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenTestInput.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/Foreach.h"

class DeviceList : public BitmapText
{
  public:
	void Update(float fDeltaTime) override
	{
		// Update devices text
		this->SetText(INPUTMAN->GetDisplayDevicesString());

		BitmapText::Update(fDeltaTime);
	}

	DeviceList* Copy() const override;
};

REGISTER_ACTOR_CLASS(DeviceList);

static LocalizedString CONTROLLER("ScreenTestInput", "Controller");
static LocalizedString SECONDARY("ScreenTestInput", "secondary");
static LocalizedString NOT_MAPPED("ScreenTestInput", "not mapped");
class InputList : public BitmapText
{
	InputList* Copy() const override;

	void Update(float fDeltaTime) override
	{
		// Update input texts
		vector<std::string> asInputs;

		vector<DeviceInput> DeviceInputs;
		INPUTFILTER->GetPressedButtons(DeviceInputs);
		FOREACH(DeviceInput, DeviceInputs, di)
		{
			if (!di->bDown && di->level == 0.0f)
				continue;

			std::string sTemp;
			sTemp += INPUTMAN->GetDeviceSpecificInputString(*di);
			if (di->level == 1.0f)
				sTemp += ssprintf(" - 1 ");
			else
				sTemp += ssprintf(" - %.3f ", di->level);

			GameInput gi;
			if (INPUTMAPPER->DeviceToGame(*di, gi)) {
				std::string sName = GameButtonToLocalizedString(
				  INPUTMAPPER->GetInputScheme(), gi.button);
				sTemp += ssprintf(" - %s %d %s",
								  CONTROLLER.GetValue().c_str(),
								  gi.controller + 1,
								  sName.c_str());

				if (!PREFSMAN->m_bOnlyDedicatedMenuButtons) {
					GameButton mb =
					  INPUTMAPPER->GetInputScheme()->GameButtonToMenuButton(
						gi.button);
					if (mb != GameButton_Invalid && mb != gi.button) {
						std::string sGameButtonString =
						  GameButtonToLocalizedString(
							INPUTMAPPER->GetInputScheme(), mb);
						sTemp += ssprintf(" - (%s %s)",
										  sGameButtonString.c_str(),
										  SECONDARY.GetValue().c_str());
					}
				}
			} else {
				sTemp += " - " + NOT_MAPPED.GetValue();
			}

			std::string sComment = INPUTFILTER->GetButtonComment(*di);
			if (sComment != "")
				sTemp += " - " + sComment;

			asInputs.push_back(sTemp);
		}

		this->SetText(join("\n", asInputs));

		BitmapText::Update(fDeltaTime);
	}
};

REGISTER_ACTOR_CLASS(InputList);

REGISTER_SCREEN_CLASS(ScreenTestInput);

bool
ScreenTestInput::Input(const InputEventPlus& input)
{
	std::string sMessage = input.DeviceI.ToString();
	bool bHandled = false;
	switch (input.type) {
		case IET_FIRST_PRESS:
		case IET_RELEASE: {
			switch (input.type) {
				case IET_FIRST_PRESS:
					sMessage += "Pressed";
					break;
				case IET_RELEASE:
					sMessage += "Released";
					break;
				default:
					break;
			}
			MESSAGEMAN->Broadcast(sMessage);
			bHandled = true;
			break;
		}
		default:
			break;
	}

	return Screen::Input(input) || bHandled; // default handler
}

bool
ScreenTestInput::MenuStart(const InputEventPlus& input)
{
	return MenuBack(input);
}

bool
ScreenTestInput::MenuBack(const InputEventPlus& input)
{
	if (input.type != IET_REPEAT)
		return false; // ignore

	if (IsTransitioning())
		return false;
	SCREENMAN->PlayStartSound();
	StartTransitioningScreen(SM_GoToPrevScreen);
	return true;
}
