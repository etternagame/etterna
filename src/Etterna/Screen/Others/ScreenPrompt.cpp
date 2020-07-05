#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenPrompt.h"
#include "Etterna/Singletons/ThemeManager.h"

PromptAnswer ScreenPrompt::s_LastAnswer = ANSWER_YES;
bool ScreenPrompt::s_bCancelledLast = false;

#define ANSWER_TEXT(s) THEME->GetString(m_sName, s)

static const char* PromptAnswerNames[] = {
	"Yes",
	"No",
	"Cancel",
};
XToString(PromptAnswer);

// Settings:
namespace {
std::string g_sText;
PromptType g_PromptType;
PromptAnswer g_defaultAnswer;
void (*g_pOnYes)(void*);
void (*g_pOnNo)(void*);
void* g_pCallbackData;
};

void
ScreenPrompt::SetPromptSettings(const std::string& sText,
								PromptType type,
								PromptAnswer defaultAnswer,
								void (*OnYes)(void*),
								void (*OnNo)(void*),
								void* pCallbackData)
{
	g_sText = sText;
	g_PromptType = type;
	g_defaultAnswer = defaultAnswer;
	g_pOnYes = OnYes;
	g_pOnNo = OnNo;
	g_pCallbackData = pCallbackData;
}

void
ScreenPrompt::Prompt(ScreenMessage smSendOnPop,
					 const std::string& sText,
					 PromptType type,
					 PromptAnswer defaultAnswer,
					 void (*OnYes)(void*),
					 void (*OnNo)(void*),
					 void* pCallbackData)
{
	SetPromptSettings(sText, type, defaultAnswer, OnYes, OnNo, pCallbackData);

	SCREENMAN->AddNewScreenToTop("ScreenPrompt", smSendOnPop);
}

REGISTER_SCREEN_CLASS(ScreenPrompt);

void
ScreenPrompt::Init()
{
	ScreenWithMenuElements::Init();

	m_textQuestion.LoadFromFont(THEME->GetPathF(m_sName, "question"));
	m_textQuestion.SetName("Question");
	LOAD_ALL_COMMANDS(m_textQuestion);
	this->AddChild(&m_textQuestion);

	m_sprCursor.Load(THEME->GetPathG(m_sName, "cursor"));
	m_sprCursor->SetName("Cursor");
	LOAD_ALL_COMMANDS(m_sprCursor);
	this->AddChild(m_sprCursor);

	for (int i = 0; i < NUM_PromptAnswer; i++) {
		m_textAnswer[i].LoadFromFont(THEME->GetPathF(m_sName, "answer"));
		// The name of the actor isn't set because it is not known at this point
		// how many answers there will be, and the name depends on the number of
		// answers as a clumsy way of letting the themer set different positions
		// for different answer groups.  The name is set in BeginScreen, and
		// then the commands are loaded. -Kyz (At least, that seems like the
		// explanation to me, reading the code years after the author left)
		this->AddChild(&m_textAnswer[i]);
	}

	m_sndChange.Load(THEME->GetPathS(m_sName, "change"), true);
}

void
ScreenPrompt::BeginScreen()
{
	ScreenWithMenuElements::BeginScreen();

	m_Answer = g_defaultAnswer;
	ENUM_CLAMP(m_Answer, PromptAnswer(0), PromptAnswer(g_PromptType));

	m_textQuestion.SetText(g_sText);
	SET_XY(m_textQuestion);
	m_textQuestion.PlayCommand("On");

	m_sprCursor->PlayCommand("On");

	for (int i = 0; i <= g_PromptType; i++) {
		std::string sElem = ssprintf("Answer%dOf%d", i + 1, g_PromptType + 1);
		m_textAnswer[i].SetName(sElem);
		LOAD_ALL_COMMANDS(m_textAnswer[i]);
		// Side note:  Because LOAD_ALL_COMMANDS occurs here, InitCommand will
		// not be run for the actors.  People can just use OnCommand instead.
		std::string sAnswer = PromptAnswerToString((PromptAnswer)i);
		// FRAGILE
		if (g_PromptType == PROMPT_OK)
			sAnswer = "OK";

		m_textAnswer[i].SetText(ANSWER_TEXT(sAnswer));
		m_textAnswer[i].PlayCommand("On");
		SET_XY(m_textAnswer[i]);
	}

	for (int i = g_PromptType + 1; i < NUM_PromptAnswer; i++)
		m_textAnswer[i].SetText("");

	PositionCursor();
}

bool
ScreenPrompt::Input(const InputEventPlus& input)
{
	if (IsTransitioning())
		return false;

	if (input.type == IET_RELEASE)
		return false;

	if (input.DeviceI.device == DEVICE_KEYBOARD) {
		switch (input.DeviceI.button) {
			case KEY_LEFT:
				return this->MenuLeft(input);
			case KEY_RIGHT:
				return this->MenuRight(input);
			default:
				break;
		}
	}

	return ScreenWithMenuElements::Input(input);
}

bool
ScreenPrompt::CanGoRight()
{
	switch (g_PromptType) {
		case PROMPT_OK:
			return false;
		case PROMPT_YES_NO:
			return m_Answer < ANSWER_NO;
		case PROMPT_YES_NO_CANCEL:
			return m_Answer < ANSWER_CANCEL;
		default:
			FAIL_M(ssprintf("Invalid PromptType: %i", g_PromptType));
	}
}

void
ScreenPrompt::Change(int dir)
{
	m_textAnswer[m_Answer].StopEffect();
	m_Answer = static_cast<PromptAnswer>(m_Answer + dir);
	ASSERT(m_Answer >= 0 && m_Answer < NUM_PromptAnswer);

	PositionCursor();

	m_sndChange.Play(true);
}

bool
ScreenPrompt::MenuLeft(const InputEventPlus& input)
{
	if (CanGoLeft()) {
		Change(-1);
		return true;
	}
	return false;
}

bool
ScreenPrompt::MenuRight(const InputEventPlus& input)
{
	if (CanGoRight()) {
		Change(+1);
		return true;
	}
	return false;
}

bool
ScreenPrompt::MenuStart(const InputEventPlus& input)
{
	if (m_Out.IsTransitioning() || m_Cancel.IsTransitioning())
		return false;

	End(false);
	return true;
}

bool
ScreenPrompt::MenuBack(const InputEventPlus& input)
{
	if (m_Out.IsTransitioning() || m_Cancel.IsTransitioning())
		return false;

	switch (g_PromptType) {
		case PROMPT_OK:
		case PROMPT_YES_NO:
			// don't allow cancel
			break;
		case PROMPT_YES_NO_CANCEL:
			End(true);
			return true;
	}
	return false;
}

void
ScreenPrompt::End(bool bCancelled)
{
	switch (m_Answer) {
		case ANSWER_YES:
			m_smSendOnPop = SM_Success;
			break;
		case ANSWER_NO:
			m_smSendOnPop = SM_Failure;
			break;
		default:
			break;
	}

	if (bCancelled) {
		Cancel(SM_GoToNextScreen);
	} else {
		SCREENMAN->PlayStartSound();
		StartTransitioningScreen(SM_GoToNextScreen);
	}

	switch (m_Answer) {
		case ANSWER_YES:
			if (g_pOnYes != nullptr)
				g_pOnYes(g_pCallbackData);
			break;
		case ANSWER_NO:
			if (g_pOnNo != nullptr)
				g_pOnNo(g_pCallbackData);
			break;
		default:
			break;
	}

	s_LastAnswer = bCancelled ? ANSWER_CANCEL : m_Answer;
	s_bCancelledLast = bCancelled;
}

void
ScreenPrompt::PositionCursor()
{
	BitmapText& bt = m_textAnswer[m_Answer];
	m_sprCursor->SetXY(bt.GetX(), bt.GetY());
}

void
ScreenPrompt::TweenOffScreen()
{
	m_textQuestion.PlayCommand("Off");
	m_sprCursor->PlayCommand("Off");
	for (int i = 0; i <= g_PromptType; i++)
		m_textAnswer[i].PlayCommand("Off");

	ScreenWithMenuElements::TweenOffScreen();
}
