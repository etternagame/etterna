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
	const vector<InputEventPlus>& GetQueue(GameController c) const
	{
		return m_aQueue[c];
	}
	void ClearQueue(GameController c);

  protected:
	vector<InputEventPlus> m_aQueue[NUM_GameController];
};

struct InputQueueCode
{
  public:
	bool Load(std::string sButtonsNames);
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
		vector<GameButton> m_aButtonsToHold;
		vector<GameButton> m_aButtonsToNotHold;
		vector<GameButton> m_aButtonsToPress;

		bool m_InputTypes[NUM_InputEventType];
		bool m_bAllowIntermediatePresses{ false };
	};
	vector<ButtonPress> m_aPresses;

	float m_fMaxSecondsBack = 0.f;
};

extern InputQueue*
  INPUTQUEUE; // global and accessible from anywhere in our program

#endif
