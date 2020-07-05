/* ScreenPrompt - Displays a prompt on top of another screen. */

#ifndef SCREEN_PROMPT_H
#define SCREEN_PROMPT_H

#include "Etterna/Actor/Base/BitmapText.h"
#include "RageUtil/Sound/RageSound.h"
#include "ScreenWithMenuElements.h"

enum PromptType
{
	PROMPT_OK,
	PROMPT_YES_NO,
	PROMPT_YES_NO_CANCEL
};

enum PromptAnswer
{
	ANSWER_YES,
	ANSWER_NO,
	ANSWER_CANCEL,
	NUM_PromptAnswer
};

class ScreenPrompt : public ScreenWithMenuElements
{
  public:
	static void SetPromptSettings(const std::string& sText,
								  PromptType type = PROMPT_OK,
								  PromptAnswer defaultAnswer = ANSWER_NO,
								  void (*OnYes)(void*) = nullptr,
								  void (*OnNo)(void*) = nullptr,
								  void* pCallbackData = nullptr);
	static void Prompt(ScreenMessage smSendOnPop,
					   const std::string& sText,
					   PromptType type = PROMPT_OK,
					   PromptAnswer defaultAnswer = ANSWER_NO,
					   void (*OnYes)(void*) = nullptr,
					   void (*OnNo)(void*) = nullptr,
					   void* pCallbackData = nullptr);

	void Init() override;
	void BeginScreen() override;
	bool Input(const InputEventPlus& input) override;

	static PromptAnswer s_LastAnswer;
	static bool s_bCancelledLast;

	// Lua
	// virtual void PushSelf( lua_State *L );

  protected:
	bool CanGoLeft() { return m_Answer > 0; }
	bool CanGoRight();
	void Change(int dir);
	bool MenuLeft(const InputEventPlus& input) override;
	bool MenuRight(const InputEventPlus& input) override;
	bool MenuBack(const InputEventPlus& input) override;
	bool MenuStart(const InputEventPlus& input) override;

	virtual void End(bool bCancelled);
	void PositionCursor();

	void TweenOffScreen() override;

	BitmapText m_textQuestion;
	AutoActor m_sprCursor;
	BitmapText m_textAnswer[NUM_PromptAnswer];
	PromptAnswer m_Answer;

	RageSound m_sndChange;
};

#endif
