#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Fonts/FontCharAliases.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Models/Misc/Preference.h"
#include "RageUtil/Misc/RageInput.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenPrompt.h"
#include "ScreenTextEntry.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Singletons/InputFilter.h"
#include "Core/Platform/Platform.hpp"

#include <algorithm>
#include <utility>

static const char* g_szKeys[NUM_KeyboardRow][KEYS_PER_ROW] = {
	{ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M" },
	{ "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" },
	{ "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m" },
	{ "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z" },
	{ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "", "", "" },
	{ "!", "@", "#", "$", "%", "^", "&", "(", ")", "[", "]", "{", "}" },
	{ "+", "-", "=", "_", ",", ".", "'", R"(")", ":", "", "", "", "" },
	{ "",
	  "",
	  "Space",
	  "",
	  "",
	  "Backspace",
	  "",
	  "",
	  "Cancel",
	  "",
	  "",
	  "Done",
	  "" },
};

std::string ScreenTextEntry::s_sLastAnswer = "";
bool ScreenTextEntry::s_bMustResetInputRedirAtClose = false;
bool ScreenTextEntry::s_bResetInputRedirTo = false;


// Settings:
namespace {
std::string g_sQuestion;
std::string g_sInitialAnswer;
int g_iMaxInputLength;
bool (*g_pValidate)(const std::string& sAnswer, std::string& sErrorOut);
void (*g_pOnOK)(const std::string& sAnswer);
void (*g_pOnCancel)();
bool g_bPassword = false;
bool (*g_pValidateAppend)(const std::string& sAnswerBeforeChar,
						  const std::string& sAppend);
std::string (*g_pFormatAnswerForDisplay)(const std::string& sAnswer);

// Lua bridge
LuaReference g_ValidateFunc;
LuaReference g_OnOKFunc;
LuaReference g_OnCancelFunc;
LuaReference g_ValidateAppendFunc;
LuaReference g_FormatAnswerForDisplayFunc;
};

// Lua bridges
static bool
ValidateFromLua(const std::string& sAnswer,
				std::string& sErrorOut,
				const LuaReference& func)
{

	if (func.IsNil() || !func.IsSet()) {
		return true;
	}
	Lua* L = LUA->Get();

	func.PushSelf(L);

	// Argument 1 (answer):
	lua_pushstring(L, sAnswer.c_str());

	// Argument 2 (error out):
	lua_pushstring(L, sErrorOut.c_str());

	bool valid = false;

	std::string error = "Lua error in ScreenTextEntry Validate: ";
	if (LuaHelpers::RunScriptOnStack(L, error, 2, 2, true)) {
		if (!lua_isstring(L, -1) || !lua_isboolean(L, -2)) {
			LuaHelpers::ReportScriptError("Lua error: ScreenTextEntry Validate "
										  "did not return 'bool, string'.");
		} else {
			std::string ErrorFromLua;
			LuaHelpers::Pop(L, ErrorFromLua);
			if (!ErrorFromLua.empty()) {
				sErrorOut = ErrorFromLua;
			}
			LuaHelpers::Pop(L, valid);
		}
	}
	lua_settop(L, 0);
	LUA->Release(L);
	return valid;
}
static bool
ValidateFromLua(const std::string& sAnswer, std::string& sErrorOut)
{
	return ValidateFromLua(sAnswer, sErrorOut, g_ValidateFunc);
}

static void
OnOKFromLua(const std::string& sAnswer, const LuaReference& func)
{
	if (func.IsNil() || !func.IsSet()) {
		return;
	}
	Lua* L = LUA->Get();

	func.PushSelf(L);
	// Argument 1 (answer):
	lua_pushstring(L, sAnswer.c_str());
	std::string error = "Lua error in ScreenTextEntry OnOK: ";
	LuaHelpers::RunScriptOnStack(L, error, 1, 0, true);

	LUA->Release(L);
}

static void
OnOKFromLua(const std::string& sAnswer)
{
	OnOKFromLua(sAnswer, g_OnOKFunc);
}

static void
OnCancelFromLua(const LuaReference& func)
{
	if (func.IsNil() || !func.IsSet()) {
		return;
	}
	Lua* L = LUA->Get();

	func.PushSelf(L);
	std::string error = "Lua error in ScreenTextEntry OnCancel: ";
	LuaHelpers::RunScriptOnStack(L, error, 0, 0, true);

	LUA->Release(L);
}

static void
OnCancelFromLua()
{
	OnCancelFromLua(g_OnCancelFunc);
}

static bool
ValidateAppendFromLua(const std::string& sAnswerBeforeChar,
					  const std::string& sAppend,
					  const LuaReference& func)
{
	if (func.IsNil() || !func.IsSet()) {
		return true;
	}
	Lua* L = LUA->Get();

	func.PushSelf(L);

	// Argument 1 (AnswerBeforeChar):
	lua_pushstring(L, sAnswerBeforeChar.c_str());

	// Argument 2 (Append):
	lua_pushstring(L, sAppend.c_str());

	bool append = false;

	std::string error = "Lua error in ScreenTextEntry ValidateAppend: ";
	if (LuaHelpers::RunScriptOnStack(L, error, 2, 1, true)) {
		if (!lua_isboolean(L, -1)) {
			LuaHelpers::ReportScriptError(
			  "\"ValidateAppend\" did not return a boolean.");
		} else {
			LuaHelpers::Pop(L, append);
		}
	}
	lua_settop(L, 0);
	LUA->Release(L);
	return append;
}

static bool
ValidateAppendFromLua(const std::string& sAnswerBeforeChar,
					  const std::string& sAppend)
{
	return ValidateAppendFromLua(
	  sAnswerBeforeChar, sAppend, g_ValidateAppendFunc);
}

static std::string
FormatAnswerForDisplayFromLua(const std::string& sAnswer,
							  const LuaReference& func)
{
	if (func.IsNil() || !func.IsSet()) {
		return sAnswer;
	}
	Lua* L = LUA->Get();

	func.PushSelf(L);
	// Argument 1 (Answer):
	lua_pushstring(L, sAnswer.c_str());

	std::string answer;
	std::string error = "Lua error in ScreenTextEntry FormatAnswerForDisplay: ";
	if (LuaHelpers::RunScriptOnStack(L, error, 1, 1, true)) {
		if (!lua_isstring(L, -1)) {
			LuaHelpers::ReportScriptError(
			  "\"FormatAnswerForDisplay\" did not return a string.");
		} else {
			LuaHelpers::Pop(L, answer);
		}
	}
	lua_settop(L, 0);
	LUA->Release(L);
	return answer;
}

static std::string
FormatAnswerForDisplayFromLua(const std::string& sAnswer)
{
	return FormatAnswerForDisplayFromLua(sAnswer, g_FormatAnswerForDisplayFunc);
}

void
ScreenTextEntry::SetTextEntrySettings(
  std::string sQuestion,
  std::string sInitialAnswer,
  int iMaxInputLength,
  bool (*Validate)(const std::string& sAnswer, std::string& sErrorOut),
  void (*OnOK)(const std::string& sAnswer),
  void (*OnCancel)(),
  bool bPassword,
  bool (*ValidateAppend)(const std::string& sAnswerBeforeChar,
						 const std::string& sAppend),
  std::string (*FormatAnswerForDisplay)(const std::string& sAnswer))
{
	g_sQuestion = std::move(sQuestion);
	g_sInitialAnswer = std::move(sInitialAnswer);
	g_iMaxInputLength = iMaxInputLength;
	g_pValidate = Validate;
	g_pOnOK = OnOK;
	g_pOnCancel = OnCancel;
	g_bPassword = bPassword;
	g_pValidateAppend = ValidateAppend;
	g_pFormatAnswerForDisplay = FormatAnswerForDisplay;
}

void
ScreenTextEntry::SetTextEntrySettings(
  std::string question,
  std::string initialAnswer,
  int maxInputLength,
  const LuaReference& validateFunc,
  const LuaReference& onOKFunc,
  const LuaReference& onCancelFunc,
  const LuaReference& validateAppendFunc,
  const LuaReference& formatAnswerForDisplayFunc,
  bool (*Validate)(const std::string& sAnswer, std::string& sErrorOut),
  void (*OnOK)(const std::string& sAnswer),
  void (*OnCancel)(),
  bool password,
  bool (*ValidateAppend)(const std::string& sAnswerBeforeChar,
						 const std::string& sAppend),
  std::string (*FormatAnswerForDisplay)(const std::string& sAnswer))
{
	sQuestion = std::move(question);
	sInitialAnswer = std::move(initialAnswer);
	iMaxInputLength = maxInputLength;
	pValidate = Validate;
	pOnOK = OnOK;
	pOnCancel = OnCancel;
	pValidateAppend = ValidateAppend;
	bPassword = password;
	pFormatAnswerForDisplay = FormatAnswerForDisplay;

	ValidateFunc = validateFunc;
	OnOKFunc = onOKFunc;
	OnCancelFunc = onCancelFunc;
	ValidateAppendFunc = validateAppendFunc;
	FormatAnswerForDisplayFunc = formatAnswerForDisplayFunc;
}

void
ScreenTextEntry::TextEntry(
  ScreenMessage smSendOnPop,
  std::string sQuestion,
  std::string sInitialAnswer,
  int iMaxInputLength,
  bool (*Validate)(const std::string& sAnswer, std::string& sErrorOut),
  void (*OnOK)(const std::string& sAnswer),
  void (*OnCancel)(),
  bool bPassword,
  bool (*ValidateAppend)(const std::string& sAnswerBeforeChar,
						 const std::string& sAppend),
  std::string (*FormatAnswerForDisplay)(const std::string& sAnswer))
{
	g_sQuestion = std::move(sQuestion);
	g_sInitialAnswer = std::move(sInitialAnswer);
	g_iMaxInputLength = iMaxInputLength;
	g_pValidate = Validate;
	g_pOnOK = OnOK;
	g_pOnCancel = OnCancel;
	g_bPassword = bPassword;
	g_pValidateAppend = ValidateAppend;
	g_pFormatAnswerForDisplay = FormatAnswerForDisplay;

	SCREENMAN->AddNewScreenToTop("ScreenTextEntry", std::move(smSendOnPop));
}

static LocalizedString INVALID_FLOAT(
  "ScreenTextEntry",
  "\"%s\" is an invalid floating point value.");
bool
ScreenTextEntry::FloatValidate(const std::string& sAnswer,
							   std::string& sErrorOut)
{
	float f;
	if (StringToFloat(sAnswer, f))
		return true;
	sErrorOut = ssprintf(INVALID_FLOAT.GetValue(), sAnswer.c_str());
	return false;
}

static LocalizedString INVALID_INT("ScreenTextEntry",
								   "\"%s\" is an invalid integer value.");
bool
ScreenTextEntry::IntValidate(const std::string& sAnswer, std::string& sErrorOut)
{
	int f;
	if (sAnswer >> f)
		return true;
	sErrorOut = ssprintf(INVALID_INT.GetValue(), sAnswer.c_str());
	return false;
}

bool ScreenTextEntry::s_bCancelledLast = false;

/* Handle UTF-8. Right now, we need to at least be able to backspace a whole
 * UTF-8 character. Better would be to operate in wchar_t.
 *
 * XXX: Don't allow internal-use codepoints (above 0xFFFF); those are subject to
 * change and shouldn't be written to disk. */
REGISTER_SCREEN_CLASS(ScreenTextEntry);
REGISTER_SCREEN_CLASS(ScreenTextEntryVisual);

void
ScreenTextEntry::Init()
{
	ScreenWithMenuElements::Init();

	MESSAGEMAN->Broadcast("BeginTextEntry");

	m_textQuestion.LoadFromFont(THEME->GetPathF(m_sName, "question"));
	m_textQuestion.SetName("Question");
	LOAD_ALL_COMMANDS(m_textQuestion);
	this->AddChild(&m_textQuestion);

	m_textAnswer.LoadFromFont(THEME->GetPathF(m_sName, "answer"));
	m_textAnswer.SetName("Answer");
	LOAD_ALL_COMMANDS(m_textAnswer);
	this->AddChild(&m_textAnswer);

	m_bShowAnswerCaret = false;
	// m_iCaretLocation = 0;

	m_sndType.Load(THEME->GetPathS(m_sName, "type"), true);
	m_sndBackspace.Load(THEME->GetPathS(m_sName, "backspace"), true);
}

void
ScreenTextEntry::BeginScreen()
{
	if (sInitialAnswer != "")
		m_sAnswer = StringToWString(sInitialAnswer);
	else
		m_sAnswer = StringToWString(g_sInitialAnswer);

	ScreenWithMenuElements::BeginScreen();

	if (s_bMustResetInputRedirAtClose) {
		s_bResetInputRedirTo = SCREENMAN->get_input_redirected(PLAYER_1);
		SCREENMAN->set_input_redirected(PLAYER_1, false);
	}

	if (sQuestion != "")
		m_textQuestion.SetText(sQuestion);
	else
		m_textQuestion.SetText(g_sQuestion);
	SET_XY(m_textQuestion);
	SET_XY(m_textAnswer);

	UpdateAnswerText();
}

static LocalizedString ANSWER_CARET("ScreenTextEntry", "AnswerCaret");
static LocalizedString ANSWER_BLANK("ScreenTextEntry", "AnswerBlank");
void
ScreenTextEntry::UpdateAnswerText()
{
	std::string s;
	if (g_bPassword || bPassword)
		s = std::string(m_sAnswer.size(), '*');
	else
		s = WStringToString(m_sAnswer);

	bool bAnswerFull = static_cast<int>(s.length()) >=
					   std::max(g_iMaxInputLength, iMaxInputLength);

	if (!FormatAnswerForDisplayFunc.IsNil() &&
		FormatAnswerForDisplayFunc.IsSet())
		FormatAnswerForDisplayFromLua(s, FormatAnswerForDisplayFunc);
	else if (pFormatAnswerForDisplay != nullptr)
		s = pFormatAnswerForDisplay(s);
	else if (g_pFormatAnswerForDisplay)
		s = g_pFormatAnswerForDisplay(s);

	// Handle caret drawing
	// m_iCaretLocation = s.length()
	if (m_bShowAnswerCaret && !bAnswerFull)
		s += ANSWER_CARET; // was '_'
	else {
		s += ANSWER_BLANK; // was "  "
	}

	FontCharAliases::ReplaceMarkers(s);
	m_textAnswer.SetText(s);
}

void
ScreenTextEntry::Update(float fDelta)
{
	ScreenWithMenuElements::Update(fDelta);

	if (m_timerToggleCursor.PeekDeltaTime() > 0.25f) {
		m_timerToggleCursor.Touch();
		m_bShowAnswerCaret = !m_bShowAnswerCaret;
		UpdateAnswerText();
	}
}

bool
ScreenTextEntry::Input(const InputEventPlus& input)
{
	if (IsTransitioning())
		return false;

	bool bHandled = false;
	if (input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_BACK)) {
		switch (input.type) {
			case IET_FIRST_PRESS:
			case IET_REPEAT:
				BackspaceInAnswer();
				bHandled = true;
			default:
				break;
		}
	} else if (input.type == IET_FIRST_PRESS) {
		wchar_t c = INPUTMAN->DeviceInputToChar(input.DeviceI, true);
		// Detect Ctrl+V
		auto ctrlPressed = INPUTFILTER->IsControlPressed();
		auto vPressed =
		  input.DeviceI.button == KEY_CV || input.DeviceI.button == KEY_Cv;
		if (vPressed && ctrlPressed) {
			TryAppendToAnswer(Core::Platform::getClipboard());

			TextEnteredDirectly(); // XXX: This doesn't seem appropriate but
								   // there's no TextPasted()
			bHandled = true;
		} else if (c >= L' ') {
			// todo: handle caps lock -aj
			auto str = WStringToString(std::wstring() + c);
			TryAppendToAnswer(str);

			TextEnteredDirectly();
			bHandled = true;
		}
	}

	return ScreenWithMenuElements::Input(input) || bHandled;
}

void
ScreenTextEntry::TryAppendToAnswer(const std::string& s)
{
	{
		std::wstring sNewAnswer = m_sAnswer + StringToWString(s);
		if (static_cast<int>(sNewAnswer.length()) >
			std::max(g_iMaxInputLength, iMaxInputLength)) {
			SCREENMAN->PlayInvalidSound();
			return;
		}
	}

	if (!ValidateAppendFunc.IsNil() && ValidateAppendFunc.IsSet()) {
		ValidateAppendFromLua(
		  WStringToString(m_sAnswer), s, ValidateAppendFunc);
	} else if (pValidateAppend != nullptr) {
		if (!pValidateAppend(WStringToString(m_sAnswer), s)) {
			SCREENMAN->PlayInvalidSound();
			return;
		}
	} else if (g_pValidateAppend &&
			   !g_pValidateAppend(WStringToString(m_sAnswer), s)) {
		SCREENMAN->PlayInvalidSound();
		return;
	}

	std::wstring sNewAnswer = m_sAnswer + StringToWString(s);
	m_sAnswer = sNewAnswer;
	m_sndType.Play(true);
	UpdateAnswerText();
}

void
ScreenTextEntry::BackspaceInAnswer()
{
	if (m_sAnswer.empty()) {
		SCREENMAN->PlayInvalidSound();
		return;
	}
	m_sAnswer.erase(m_sAnswer.end() - 1);
	m_sndBackspace.Play(true);
	UpdateAnswerText();
}

bool
ScreenTextEntry::MenuStart(const InputEventPlus& input)
{
	// HACK: Only allow the screen to end on the Enter key.-aj
	if (input.DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_ENTER) &&
		input.type == IET_FIRST_PRESS) {
		End(false);
		return true;
	}
	return false;
}

void
ScreenTextEntry::End(bool bCancelled)
{
	if (bCancelled) {
		if (!OnCancelFunc.IsNil() && OnCancelFunc.IsSet()) {
			OnCancelFromLua(OnCancelFunc);
		} else if (pOnCancel != nullptr)
			pOnCancel();
		if (g_pOnCancel != nullptr)
			g_pOnCancel();

		Cancel(SM_GoToNextScreen);
		// TweenOffScreen();
		MESSAGEMAN->Broadcast("CancelTextEntry");
	} else {
		std::string sAnswer = WStringToString(m_sAnswer);
		std::string sError;

		if (!ValidateFunc.IsNil() && ValidateFunc.IsSet()) {
			bool bValidAnswer = ValidateFromLua(sAnswer, sError, ValidateFunc);
			if (!bValidAnswer)
				return;
		} else if (pValidate != nullptr) {
			bool bValidAnswer = pValidate(sAnswer, sError);
			if (!bValidAnswer) {
				ScreenPrompt::Prompt(SM_None, sError);
				return; // don't end this screen.
			}
		} else if (g_pValidate != nullptr) {
			bool bValidAnswer = g_pValidate(sAnswer, sError);
			if (!bValidAnswer) {
				ScreenPrompt::Prompt(SM_None, sError);
				return; // don't end this screen.
			}
		}

		std::string ret = WStringToString(m_sAnswer);
		FontCharAliases::ReplaceMarkers(ret);
		if (!OnOKFunc.IsNil() && OnOKFunc.IsSet()) {
			OnOKFromLua(ret, OnOKFunc);
		} else if (pOnOK != nullptr) {
			pOnOK(ret);
		} else if (g_pOnOK != nullptr) {
			g_pOnOK(ret);
		}

		MESSAGEMAN->Broadcast("CompleteTextEntry");

		StartTransitioningScreen(SM_GoToNextScreen);
		SCREENMAN->PlayStartSound();
	}

	s_bCancelledLast = bCancelled;
	s_sLastAnswer = bCancelled ? std::string("") : WStringToString(m_sAnswer);
	if (s_bMustResetInputRedirAtClose) {
		s_bMustResetInputRedirAtClose = false;
		SCREENMAN->set_input_redirected(PLAYER_1, s_bResetInputRedirTo);
	}
}

bool
ScreenTextEntry::MenuBack(const InputEventPlus& input)
{
	if (input.type != IET_FIRST_PRESS)
		return false;
	End(true);
	return true;
}

void
ScreenTextEntry::TextEntrySettings::FromStack(lua_State* L)
{
	if (lua_type(L, 1) != LUA_TTABLE) {
		Locator::getLogger()->error("ScreenTextEntry FromStack: not a table");
		return;
	}

	lua_pushvalue(L, 1);
	const int iTab = lua_gettop(L);

	// Get ScreenMessage
	lua_getfield(L, iTab, "SendOnPop");
	const char* pStr = lua_tostring(L, -1);
	if (pStr == nullptr)
		smSendOnPop = SM_None;
	else
		smSendOnPop = ScreenMessageHelpers::ToScreenMessage(pStr);
	lua_settop(L, iTab);

	// Get Question
	lua_getfield(L, iTab, "Question");
	pStr = lua_tostring(L, -1);
	if (pStr == nullptr) {
		LuaHelpers::ReportScriptError(
		  "ScreenTextEntry \"Question\" entry is not a string.");
		pStr = "";
	}
	sQuestion = pStr;
	lua_settop(L, iTab);

	// Get Initial Answer
	lua_getfield(L, iTab, "InitialAnswer");
	pStr = lua_tostring(L, -1);
	if (pStr == nullptr)
		pStr = "";
	sInitialAnswer = pStr;
	lua_settop(L, iTab);

	// Get Max Input Length
	lua_getfield(L, iTab, "MaxInputLength");
	iMaxInputLength = lua_tointeger(L, -1);
	lua_settop(L, iTab);

	// Get Password
	lua_getfield(L, iTab, "Password");
	bPassword = !(lua_toboolean(L, -1) == 0);
	lua_settop(L, iTab);

#define SET_FUNCTION_MEMBER(memname)                                           \
	lua_getfield(L, iTab, #memname);                                           \
	if (lua_isfunction(L, -1)) {                                               \
		(memname).SetFromStack(L);                                             \
	} else if (!lua_isnil(L, -1)) {                                            \
		LuaHelpers::ReportScriptError("ScreenTextEntry \"" #memname            \
									  "\" is not a function.");                \
	}                                                                          \
	lua_settop(L, iTab);

	SET_FUNCTION_MEMBER(Validate);
	SET_FUNCTION_MEMBER(OnOK);
	SET_FUNCTION_MEMBER(OnCancel);
	SET_FUNCTION_MEMBER(ValidateAppend);
	SET_FUNCTION_MEMBER(FormatAnswerForDisplay);
#undef SET_FUNCTION_MEMBER
}

void
ScreenTextEntry::LoadFromTextEntrySettings(const TextEntrySettings& settings)
{
	g_ValidateFunc = settings.Validate;
	g_OnOKFunc = settings.OnOK;
	g_OnCancelFunc = settings.OnCancel;
	g_ValidateAppendFunc = settings.ValidateAppend;
	g_FormatAnswerForDisplayFunc = settings.FormatAnswerForDisplay;
	g_sInitialAnswer = settings.sInitialAnswer;
	g_bPassword = settings.bPassword;
	g_iMaxInputLength = settings.iMaxInputLength;

	// set functions
	SetTextEntrySettings(settings.sQuestion,
						 settings.sInitialAnswer,
						 settings.iMaxInputLength,
						 settings.Validate,
						 settings.OnOK,
						 settings.OnCancel,
						 settings.ValidateAppend,
						 settings.FormatAnswerForDisplay,
						 ValidateFromLua, // Validate
						 OnOKFromLua,	  // OnOK
						 OnCancelFromLua, // OnCancel
						 settings.bPassword,
						 ValidateAppendFromLua,		   // ValidateAppend
						 FormatAnswerForDisplayFromLua // FormatAnswerForDisplay
	);

	// Hack: reload screen with new info
	BeginScreen();
}

/** @brief Allow Lua to have access to the ScreenTextEntry. */
class LunaScreenTextEntry : public Luna<ScreenTextEntry>
{
  public:
	static int Load(T* p, lua_State* L)
	{
		ScreenTextEntry::TextEntrySettings settings;
		settings.FromStack(L);
		p->LoadFromTextEntrySettings(settings);
		return 0;
	}
	static int End(T* p, lua_State* L)
	{
		bool bCancelled = false;
		if (!lua_isnoneornil(L, 1)) {
			bCancelled = BArg(1);
		}

		p->End(bCancelled);
		return 0;
	}

	LunaScreenTextEntry() {
		ADD_METHOD(Load);
		ADD_METHOD(End);
	}
};
LUA_REGISTER_DERIVED_CLASS(ScreenTextEntry, ScreenWithMenuElements)
// lua end

// begin ScreenTextEntryVisual
void
ScreenTextEntryVisual::Init()
{
	ROW_START_X.Load(m_sName, "RowStartX");
	ROW_START_Y.Load(m_sName, "RowStartY");
	ROW_END_X.Load(m_sName, "RowEndX");
	ROW_END_Y.Load(m_sName, "RowEndY");

	ScreenTextEntry::Init();

	m_sprCursor.Load(THEME->GetPathG(m_sName, "cursor"));
	m_sprCursor->SetName("Cursor");
	LOAD_ALL_COMMANDS(m_sprCursor);
	this->AddChild(m_sprCursor);

	// Init keyboard
	{
		BitmapText text;
		text.LoadFromFont(THEME->GetPathF(m_sName, "keyboard"));
		text.SetName("Keys");
		ActorUtil::LoadAllCommands(text, m_sName);
		text.PlayCommand("Init");

		FOREACH_KeyboardRow(r)
		{
			for (int x = 0; x < KEYS_PER_ROW; ++x) {
				BitmapText*& pbt = m_ptextKeys[r][x];
				pbt = text.Copy();
				this->AddChild(pbt);

				std::string s = g_szKeys[r][x];
				if (!s.empty() && r == KEYBOARD_ROW_SPECIAL)
					s = THEME->GetString(m_sName, s);
				pbt->SetText(s);
			}
		}
	}

	m_sndChange.Load(THEME->GetPathS(m_sName, "change"), true);
}

ScreenTextEntryVisual::~ScreenTextEntryVisual()
{
	FOREACH_KeyboardRow(r) for (int x = 0; x < KEYS_PER_ROW; ++x)
	  SAFE_DELETE(m_ptextKeys[r][x]);
}

void
ScreenTextEntryVisual::BeginScreen()
{
	ScreenTextEntry::BeginScreen();

	m_iFocusX = 0;
	m_iFocusY = static_cast<KeyboardRow>(0);

	FOREACH_KeyboardRow(r)
	{
		for (int x = 0; x < KEYS_PER_ROW; ++x) {
			BitmapText& bt = *m_ptextKeys[r][x];
			float fX =
			  roundf(SCALE(x, 0, KEYS_PER_ROW - 1, ROW_START_X, ROW_END_X));
			float fY =
			  roundf(SCALE(r, 0, NUM_KeyboardRow - 1, ROW_START_Y, ROW_END_Y));
			bt.SetXY(fX, fY);
		}
	}

	PositionCursor();
}

void
ScreenTextEntryVisual::PositionCursor()
{
	BitmapText& bt = *m_ptextKeys[m_iFocusY][m_iFocusX];
	m_sprCursor->SetXY(bt.GetX(), bt.GetY());
	m_sprCursor->PlayCommand(m_iFocusY == KEYBOARD_ROW_SPECIAL ? "SpecialKey"
															   : "RegularKey");
}

void
ScreenTextEntryVisual::TextEnteredDirectly()
{
	// If the user enters text with a keyboard, jump to DONE, so enter ends the
	// screen.
	m_iFocusY = KEYBOARD_ROW_SPECIAL;
	m_iFocusX = DONE;

	PositionCursor();
}

void
ScreenTextEntryVisual::MoveX(int iDir)
{
	std::string sKey;
	do {
		m_iFocusX += iDir;
		wrap(m_iFocusX, KEYS_PER_ROW);

		sKey = g_szKeys[m_iFocusY][m_iFocusX];
	} while (sKey == "");

	m_sndChange.Play(true);
	PositionCursor();
}

void
ScreenTextEntryVisual::MoveY(int iDir)
{
	std::string sKey;
	do {
		m_iFocusY = enum_add2(m_iFocusY, +iDir);
		wrap(*ConvertValue<int>(&m_iFocusY), NUM_KeyboardRow);

		// HACK: Round to nearest option so that we always stop
		// on KEYBOARD_ROW_SPECIAL.
		if (m_iFocusY == KEYBOARD_ROW_SPECIAL) {
			for (int i = 0; true; i++) {
				sKey = g_szKeys[m_iFocusY][m_iFocusX];
				if (sKey != "")
					break;

				// UGLY: Probe one space to the left before looking to the right
				m_iFocusX += (i == 0) ? -1 : +1;
				wrap(m_iFocusX, KEYS_PER_ROW);
			}
		}

		sKey = g_szKeys[m_iFocusY][m_iFocusX];
	} while (sKey == "");

	m_sndChange.Play(true);
	PositionCursor();
}

bool
ScreenTextEntryVisual::MenuLeft(const InputEventPlus& input)
{
	if (input.type != IET_FIRST_PRESS)
		return false;
	MoveX(-1);
	return true;
}
bool
ScreenTextEntryVisual::MenuRight(const InputEventPlus& input)
{
	if (input.type != IET_FIRST_PRESS)
		return false;
	MoveX(+1);
	return true;
}
bool
ScreenTextEntryVisual::MenuUp(const InputEventPlus& input)
{
	if (input.type != IET_FIRST_PRESS)
		return false;
	MoveY(-1);
	return true;
}
bool
ScreenTextEntryVisual::MenuDown(const InputEventPlus& input)
{
	if (input.type != IET_FIRST_PRESS)
		return false;
	MoveY(+1);
	return true;
}

bool
ScreenTextEntryVisual::MenuStart(const InputEventPlus& input)
{
	if (input.type != IET_FIRST_PRESS)
		return false;
	if (m_iFocusY == KEYBOARD_ROW_SPECIAL) {
		switch (m_iFocusX) {
			case SPACEBAR:
				TryAppendToAnswer(" ");
				break;
			case BACKSPACE:
				BackspaceInAnswer();
				break;
			case CANCEL:
				End(true);
				break;
			case DONE:
				End(false);
				break;
			default:
				return false;
		}
	} else {
		TryAppendToAnswer(g_szKeys[m_iFocusY][m_iFocusX]);
	}
	return true;
}
