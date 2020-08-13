/* ControllerStateDisplay - Show the button state of a controller. */

#ifndef ControllerStateDisplay_H
#define ControllerStateDisplay_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "GameInput.h"
#include "PlayerNumber.h"
#include "RageUtil/Misc/RageInput.h"

enum ControllerStateButton
{
	ControllerStateButton_UpLeft,
	ControllerStateButton_UpRight,
	ControllerStateButton_Center,
	ControllerStateButton_DownLeft,
	ControllerStateButton_DownRight,
	NUM_ControllerStateButton
};

/*
enum ControllerDanceButton
{
	ControllerDanceButton_Up,
	ControllerDanceButton_Down,
	ControllerDanceButton_Left,
	ControllerDanceButton_Right,
	ControllerPumpButton_UpLeft,
	ControllerPumpButton_UpRight,
	NUM_ControllerDanceButton
};

enum ControllerPumpButton
{
	ControllerPumpButton_UpLeft,
	ControllerPumpButton_UpRight,
	ControllerPumpButton_Center,
	ControllerPumpButton_DownLeft,
	ControllerPumpButton_DownRight,
	NUM_ControllerPumpButton
};

enum ControllerKB7Button
{
	ControllerKB7Button_Key1,
	ControllerKB7Button_Key2,
	ControllerKB7Button_Key3,
	ControllerKB7Button_Key4,
	ControllerKB7Button_Key5,
	ControllerKB7Button_Key6,
	ControllerKB7Button_Key7,
	NUM_ControllerKB7Button
};
*/

class ControllerStateDisplay : public ActorFrame
{
  public:
	ControllerStateDisplay();
	void LoadMultiPlayer(const std::string& sType, MultiPlayer mp);
	void LoadGameController(const std::string& sType, GameController gc);
	void Update(float fDelta) override;
	[[nodiscard]] bool IsLoaded() const { return m_bIsLoaded; }

	[[nodiscard]] ControllerStateDisplay* Copy() const override;

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	void LoadInternal(const std::string& sType,
					  MultiPlayer mp,
					  GameController gc);
	MultiPlayer m_mp;

	bool m_bIsLoaded;
	AutoActor m_sprFrame;
	struct Button
	{
		Button()
		{
			di.MakeInvalid();
			gi.MakeInvalid();
		}

		AutoActor spr;
		DeviceInput di;
		GameInput gi;
	};
	Button m_Buttons[NUM_ControllerStateButton];

	InputDeviceState m_idsLast;
};

#endif
