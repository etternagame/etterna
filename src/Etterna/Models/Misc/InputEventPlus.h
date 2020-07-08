#ifndef INPUT_EVENT_PLUS_H
#define INPUT_EVENT_PLUS_H

#include "GameInput.h"
#include "Etterna/Singletons/InputFilter.h"
#include "PlayerNumber.h"
/** @brief Holds a device input plus Game/Menu translations. */
class InputEventPlus
{
  public:
	InputEventPlus()
	  : pn(PLAYER_INVALID)
	{
	}
	DeviceInput DeviceI;
	GameInput GameI;
	InputEventType type{ IET_FIRST_PRESS };
	GameButton MenuI{ GameButton_Invalid };
	PlayerNumber pn;
	MultiPlayer mp{ MultiPlayer_Invalid };
	DeviceInputList InputList;
};

struct AlternateMapping
{
	GameInput inpMain;
	GameInput inpAlt;
};

#endif
