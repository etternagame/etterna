#ifndef SCREEN_SELECT_H
#define SCREEN_SELECT_H

#include "Etterna/Models/Misc/GameCommand.h"
#include "ScreenWithMenuElements.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
/** @brief Base class for Style, Difficulty, and Mode selection screens. */
class ScreenSelect : public ScreenWithMenuElements
{
  public:
	void Init() override;
	void BeginScreen() override;
	~ScreenSelect() override;

	void Update(float fDelta) override;
	bool Input(const InputEventPlus& input) override;
	void HandleScreenMessage(const ScreenMessage& SM) override;
	void HandleMessage(const Message& msg) override;

	bool MenuBack(const InputEventPlus& input) override;

  protected:
	virtual int GetSelectionIndex(PlayerNumber pn) = 0;
	virtual void
	UpdateSelectableChoices() = 0; // derived screens must handle this

	/**
	 * @brief The game commands available.
	 *
	 * Derived classes should look here for the choices. */
	vector<GameCommand> m_aGameCommands;

	vector<std::string> m_asSubscribedMessages;

	/** @brief Count up to the time between idle comment announcer sounds. */
	RageTimer m_timerIdleComment;
	/** @brief Count up to go to the timeout screen. */
	RageTimer m_timerIdleTimeout;

	ThemeMetric<float> IDLE_COMMENT_SECONDS;
	ThemeMetric<float> IDLE_TIMEOUT_SECONDS;
	ThemeMetric<bool> ALLOW_DISABLED_PLAYER_INPUT;
};

#endif
