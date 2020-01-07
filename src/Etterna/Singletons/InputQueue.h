#ifndef INPUT_QUEUE_H
#define INPUT_QUEUE_H

#include "Etterna/Models/Misc/GameInput.h"
#include "InputFilter.h"
#include <chrono>

class InputEventPlus;

/** @brief Stores a list of the most recently pressed MenuInputs for each
 * player. */
class InputQueue
{
  public:
	InputQueue();

	void RememberInput(const InputEventPlus& gi);
	bool WasPressedRecently(
	  GameController c,
	  GameButton button,
	  const std::chrono::steady_clock::time_point& OldestTimeAllowed,
	  InputEventPlus* pIEP = nullptr);
	const std::vector<InputEventPlus>& GetQueue(GameController c) const
	{
		return m_aQueue[c];
	}
	void ClearQueue(GameController c);

  protected:
	std::vector<InputEventPlus> m_aQueue[NUM_GameController];
};

struct InputQueueCode
{
  public:
	bool Load(RString sButtonsNames);
	bool EnteredCode(GameController controller) const;

	InputQueueCode()
	  : m_aPresses()
	{
	}

  private:
	struct ButtonPress
	{
		ButtonPress()
		  : m_aButtonsToHold()
		  , m_aButtonsToNotHold()
		  , m_aButtonsToPress()
		{
			memset(m_InputTypes, 0, sizeof(m_InputTypes));
			m_InputTypes[IET_FIRST_PRESS] = true;
		}
		std::vector<GameButton> m_aButtonsToHold;
		std::vector<GameButton> m_aButtonsToNotHold;
		std::vector<GameButton> m_aButtonsToPress;

		bool m_InputTypes[NUM_InputEventType];
		bool m_bAllowIntermediatePresses{ false };
	};
	std::vector<ButtonPress> m_aPresses;

	float m_fMaxSecondsBack;
};

extern InputQueue*
  INPUTQUEUE; // global and accessible from anywhere in our program

#endif
