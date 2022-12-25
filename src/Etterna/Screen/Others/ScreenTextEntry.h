#ifndef SCREEN_TEXT_ENTRY_H
#define SCREEN_TEXT_ENTRY_H

#include <utility>

#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "RageUtil/Sound/RageSound.h"
#include "ScreenWithMenuElements.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

/** @brief The list of possible keyboard rows. */
enum KeyboardRow
{
	R1,
	R2,
	R3,
	R4,
	R5,
	R6,
	R7,
	KEYBOARD_ROW_SPECIAL,
	NUM_KeyboardRow,
	KeyboardRow_Invalid
};
/** @brief A special foreach loop for the KeyboardRow enum. */
#define FOREACH_KeyboardRow(i) FOREACH_ENUM(KeyboardRow, i)
/** @brief The maximum number of keys per row. */
const int KEYS_PER_ROW = 13;
/** @brief The list of very special keys inside some rows. */
enum KeyboardRowSpecialKey
{
	SPACEBAR = 2,  /**< The space bar key. */
	BACKSPACE = 5, /**< The backspace key. */
	CANCEL = 8,
	DONE = 11
};

/** @brief Displays a text entry box over the top of another screen. */
class ScreenTextEntry : public ScreenWithMenuElements
{
  public:
	void SetTextEntrySettings(
	  std::string sQuestion,
	  std::string sInitialAnswer,
	  int iMaxInputLength,
	  bool (*Validate)(const std::string& sAnswer,
					   std::string& sErrorOut) = nullptr,
	  void (*OnOK)(const std::string& sAnswer) = nullptr,
	  void (*OnCancel)() = nullptr,
	  bool bPassword = false,
	  bool (*ValidateAppend)(const std::string& sAnswerBeforeChar,
							 const std::string& sAppend) = nullptr,
	  std::string (*FormatAnswerForDisplay)(const std::string& sAnswer) =
		nullptr);
	void SetTextEntrySettings(
	  std::string sQuestion,
	  std::string sInitialAnswer,
	  int iMaxInputLength,
	  const LuaReference&,
	  const LuaReference&,
	  const LuaReference&,
	  const LuaReference&,
	  const LuaReference&,
	  bool (*Validate)(const std::string& sAnswer,
					   std::string& sErrorOut) = nullptr,
	  void (*OnOK)(const std::string& sAnswer) = nullptr,
	  void (*OnCancel)() = nullptr,
	  bool bPassword = false,
	  bool (*ValidateAppend)(const std::string& sAnswerBeforeChar,
							 const std::string& sAppend) = nullptr,
	  std::string (*FormatAnswerForDisplay)(const std::string& sAnswer) =
		nullptr);
	static void TextEntry(
	  ScreenMessage smSendOnPop,
	  std::string sQuestion,
	  std::string sInitialAnswer,
	  int iMaxInputLength,
	  bool (*Validate)(const std::string& sAnswer,
					   std::string& sErrorOut) = nullptr,
	  void (*OnOK)(const std::string& sAnswer) = nullptr,
	  void (*OnCancel)() = nullptr,
	  bool bPassword = false,
	  bool (*ValidateAppend)(const std::string& sAnswerBeforeChar,
							 const std::string& sAppend) = nullptr,
	  std::string (*FormatAnswerForDisplay)(const std::string& sAnswer) =
		nullptr);
	static void Password(ScreenMessage smSendOnPop,
						 const std::string& sQuestion,
						 void (*OnOK)(const std::string& sPassword) = nullptr,
						 void (*OnCancel)() = nullptr)
	{
		TextEntry(std::move(smSendOnPop),
				  sQuestion,
				  "",
				  255,
				  nullptr,
				  OnOK,
				  OnCancel,
				  true);
	}

	struct TextEntrySettings
	{
		TextEntrySettings()
		  : smSendOnPop()
		  , sQuestion("")
		  , sInitialAnswer("")
		  , Validate()
		  , OnOK()
		  , OnCancel()
		  , ValidateAppend()
		  , FormatAnswerForDisplay()
		{
		}
		ScreenMessage smSendOnPop;
		std::string sQuestion;
		std::string sInitialAnswer;
		int iMaxInputLength{ 0 };
		/** @brief Is there a password involved with this setting?
		 *
		 * This parameter doesn't have to be used. */
		bool bPassword{ false };
		LuaReference
		  Validate; // (std::string sAnswer, std::string sErrorOut; optional)
		LuaReference OnOK;					 // (std::string sAnswer; optional)
		LuaReference OnCancel;				 // (optional)
		LuaReference ValidateAppend;		 // (std::string sAnswerBeforeChar,
											 // std::string sAppend; optional)
		LuaReference FormatAnswerForDisplay; // (std::string sAnswer; optional)

		// see BitmapText.cpp Attribute::FromStack()  and
		// OptionRowHandler.cpp LoadInternal() for ideas on how to implement the
		// main part, and ImportOption() from OptionRowHandler.cpp for
		// functions.
		void FromStack(lua_State* L);
	};
	void LoadFromTextEntrySettings(const TextEntrySettings& settings);

	static bool FloatValidate(const std::string& sAnswer,
							  std::string& sErrorOut);
	static bool IntValidate(const std::string& sAnswer, std::string& sErrorOut);

	std::string sQuestion{ "" };
	std::string sInitialAnswer{ "" };
	int iMaxInputLength{ 0 };
	bool (*pValidate)(const std::string& sAnswer,
					  std::string& sErrorOut){ nullptr };
	void (*pOnOK)(const std::string& sAnswer){ nullptr };
	void (*pOnCancel)(){ nullptr };
	bool bPassword{ false };
	bool (*pValidateAppend)(const std::string& sAnswerBeforeChar,
							const std::string& sAppend){ nullptr };
	std::string (*pFormatAnswerForDisplay)(const std::string& sAnswer){
		nullptr
	};

	// Lua bridge
	LuaReference ValidateFunc;
	LuaReference OnOKFunc;
	LuaReference OnCancelFunc;
	LuaReference ValidateAppendFunc;
	LuaReference FormatAnswerForDisplayFunc;

	void Init() override;
	void BeginScreen() override;

	void Update(float fDelta) override;
	bool Input(const InputEventPlus& input) override;

	static std::string s_sLastAnswer;
	static bool s_bCancelledLast;

	static bool s_bMustResetInputRedirAtClose;
	static bool s_bResetInputRedirTo;
	virtual void End(bool bCancelled);

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	void TryAppendToAnswer(const std::string& s);
	void BackspaceInAnswer();
	virtual void TextEnteredDirectly() {}


  private:
	bool MenuStart(const InputEventPlus& input) override;
	bool MenuBack(const InputEventPlus& input) override;

	void UpdateAnswerText();

	std::wstring m_sAnswer;
	bool m_bShowAnswerCaret = false;
	// todo: allow Left/Right to change caret location -aj
	// int			m_iCaretLocation;

	BitmapText m_textQuestion;
	BitmapText m_textAnswer;

	RageSound m_sndType;
	RageSound m_sndBackspace;

	RageTimer m_timerToggleCursor;
};

/** @brief Displays a text entry box and keyboard over the top of another
 * screen. */
class ScreenTextEntryVisual : public ScreenTextEntry
{
  public:
	~ScreenTextEntryVisual() override;
	void Init() override;
	void BeginScreen() override;

  protected:
	void MoveX(int iDir);
	void MoveY(int iDir);
	void PositionCursor();

	void TextEnteredDirectly() override;

	bool MenuLeft(const InputEventPlus& input) override;
	bool MenuRight(const InputEventPlus& input) override;
	bool MenuUp(const InputEventPlus& input) override;
	bool MenuDown(const InputEventPlus& input) override;

	bool MenuStart(const InputEventPlus& input) override;

	int m_iFocusX;
	KeyboardRow m_iFocusY;

	AutoActor m_sprCursor;
	BitmapText* m_ptextKeys[NUM_KeyboardRow][KEYS_PER_ROW];

	RageSound m_sndChange;

	ThemeMetric<float> ROW_START_X;
	ThemeMetric<float> ROW_START_Y;
	ThemeMetric<float> ROW_END_X;
	ThemeMetric<float> ROW_END_Y;
};

#endif
