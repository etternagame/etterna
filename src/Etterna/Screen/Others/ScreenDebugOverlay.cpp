#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/CodeDetector.h"
#include "Etterna/Models/Misc/GameCommand.h"
#include "Etterna/Globals/GameLoop.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Singletons/InputMapper.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/NoteSkinManager.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Misc/RageInput.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "ScreenDebugOverlay.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "Etterna/Screen/Gameplay/ScreenGameplay.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenSyncOverlay.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Globals/StepMania.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/FileTypes/XmlToLua.h"

static bool g_bIsDisplayed = false;
static bool g_bIsSlow = false;
static bool g_bIsHalt = false;
static RageTimer g_HaltTimer(RageZeroTimer);
static float g_fImageScaleCurrent = 1;
static float g_fImageScaleDestination = 1;

// DebugLine theming
static const ThemeMetric<RageColor> BACKGROUND_COLOR("ScreenDebugOverlay",
													 "BackgroundColor");
static const ThemeMetric<RageColor> LINE_ON_COLOR("ScreenDebugOverlay",
												  "LineOnColor");
static const ThemeMetric<RageColor> LINE_OFF_COLOR("ScreenDebugOverlay",
												   "LineOffColor");
static const ThemeMetric<float> LINE_START_Y("ScreenDebugOverlay",
											 "LineStartY");
static const ThemeMetric<float> LINE_SPACING("ScreenDebugOverlay",
											 "LineSpacing");
static const ThemeMetric<float> LINE_BUTTON_X("ScreenDebugOverlay",
											  "LineButtonX");
static const ThemeMetric<float> LINE_FUNCTION_X("ScreenDebugOverlay",
												"LineFunctionX");
static const ThemeMetric<float> PAGE_START_X("ScreenDebugOverlay",
											 "PageStartX");
static const ThemeMetric<float> PAGE_SPACING_X("ScreenDebugOverlay",
											   "PageSpacingX");

// self-registering debug lines
// We don't use SubscriptionManager, because we want to keep the line order.
static LocalizedString ON("ScreenDebugOverlay", "on");
static LocalizedString OFF("ScreenDebugOverlay", "off");
static LocalizedString MUTE_ACTIONS_ON("ScreenDebugOverlay", "Mute actions on");
static LocalizedString MUTE_ACTIONS_OFF("ScreenDebugOverlay",
										"Mute actions off");

class IDebugLine;
static std::vector<IDebugLine*>* g_pvpSubscribers = NULL;
class IDebugLine
{
  public:
	IDebugLine()
	{
		if (g_pvpSubscribers == NULL)
			g_pvpSubscribers = new std::vector<IDebugLine*>;
		g_pvpSubscribers->push_back(this);
	}
	virtual ~IDebugLine() = default;
	enum Type
	{
		all_screens,
		gameplay_only
	};
	virtual Type GetType() const { return all_screens; }
	virtual RString GetDisplayTitle() = 0;
	virtual RString GetDisplayValue()
	{
		return IsEnabled() ? ON.GetValue() : OFF.GetValue();
	}
	virtual RString GetPageName() const { return "Main"; }
	virtual bool ForceOffAfterUse() const { return false; }
	virtual bool IsEnabled() = 0;
	virtual void DoAndLog(RString& sMessageOut)
	{
		RString s1 = GetDisplayTitle();
		RString s2 = GetDisplayValue();
		if (!s2.empty())
			s1 += " - ";
		sMessageOut = s1 + s2;
	};

	DeviceInput m_Button;
};

static bool
IsGameplay()
{
	return (SCREENMAN != nullptr) && (SCREENMAN->GetTopScreen() != nullptr) &&
		   SCREENMAN->GetTopScreen()->GetScreenType() == gameplay;
}

REGISTER_SCREEN_CLASS(ScreenDebugOverlay);

ScreenDebugOverlay::~ScreenDebugOverlay()
{
	this->RemoveAllChildren();

	FOREACH(BitmapText*, m_vptextPages, p)
	SAFE_DELETE(*p);
	FOREACH(BitmapText*, m_vptextButton, p)
	SAFE_DELETE(*p);
	m_vptextButton.clear();
	FOREACH(BitmapText*, m_vptextFunction, p)
	SAFE_DELETE(*p);
	m_vptextFunction.clear();
}

const int MAX_DEBUG_LINES = 30;
struct MapDebugToDI
{
	DeviceInput holdForDebug1;
	DeviceInput holdForDebug2;
	DeviceInput holdForSlow;
	DeviceInput holdForFast;
	DeviceInput toggleMute;
	DeviceInput debugButton[MAX_DEBUG_LINES];
	DeviceInput gameplayButton[MAX_DEBUG_LINES];
	std::map<DeviceInput, int> pageButton;
	void Clear()
	{
		holdForDebug1.MakeInvalid();
		holdForDebug2.MakeInvalid();
		holdForSlow.MakeInvalid();
		holdForFast.MakeInvalid();
		toggleMute.MakeInvalid();
		for (int i = 0; i < MAX_DEBUG_LINES; i++) {
			debugButton[i].MakeInvalid();
			gameplayButton[i].MakeInvalid();
		}
	}
};
static MapDebugToDI g_Mappings;

static LocalizedString IN_GAMEPLAY("ScreenDebugOverlay", "%s in gameplay");
static LocalizedString OR("ScreenDebugOverlay", "or");
static RString
GetDebugButtonName(const IDebugLine* pLine)
{
	RString s = INPUTMAN->GetDeviceSpecificInputString(pLine->m_Button);
	IDebugLine::Type type = pLine->GetType();
	switch (type) {
		case IDebugLine::all_screens:
			return s;
		case IDebugLine::gameplay_only:
			return ssprintf(IN_GAMEPLAY.GetValue(), s.c_str());
		default:
			FAIL_M(ssprintf("Invalid debug line type: %i", type));
	}
}

template<typename U, typename V>
static bool
GetKeyFromMap(const std::map<U, V>& m, const V& val, U& key)
{
	for (typename std::map<U, V>::const_iterator iter = m.begin(); iter != m.end();
		 ++iter) {
		if (iter->second == val) {
			key = iter->first;
			return true;
		}
	}
	return false;
}

static LocalizedString DEBUG_MENU("ScreenDebugOverlay", "Debug Menu");
void
ScreenDebugOverlay::Init()
{
	Screen::Init();

	// Init debug mappings
	// TODO: Arch-specific?
	{
		g_Mappings.Clear();

		g_Mappings.holdForDebug1 = DeviceInput(DEVICE_KEYBOARD, KEY_F3);
		g_Mappings.holdForDebug2.MakeInvalid();
		g_Mappings.holdForSlow = DeviceInput(DEVICE_KEYBOARD, KEY_ACCENT);
		g_Mappings.holdForFast = DeviceInput(DEVICE_KEYBOARD, KEY_TAB);
		g_Mappings.toggleMute = DeviceInput(DEVICE_KEYBOARD, KEY_PAUSE);

		/* TODO: Find a better way of indicating which option is which here.
		 * Maybe we should take a page from ScreenEdit's menus and make
		 * RowDefs()? */

		int i = 0;
		g_Mappings.gameplayButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_F8);
		g_Mappings.gameplayButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_F7);
		g_Mappings.gameplayButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_F6);
		i = 0;
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C1);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C2);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C3);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C4);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C5);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C6);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C7);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C8);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C9);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_C0);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cq);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cw);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Ce);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cr);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Ct);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cy);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cu);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Ci);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Co);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cp);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Ca);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cs);
		g_Mappings.debugButton[i++] = DeviceInput(DEVICE_KEYBOARD, KEY_Cd);
		g_Mappings.pageButton[DeviceInput(DEVICE_KEYBOARD, KEY_F5)] = 0;
		g_Mappings.pageButton[DeviceInput(DEVICE_KEYBOARD, KEY_F6)] = 1;
		g_Mappings.pageButton[DeviceInput(DEVICE_KEYBOARD, KEY_F7)] = 2;
		g_Mappings.pageButton[DeviceInput(DEVICE_KEYBOARD, KEY_F8)] = 3;
	}

	std::map<RString, int> iNextDebugButton;
	int iNextGameplayButton = 0;
	FOREACH(IDebugLine*, *g_pvpSubscribers, p)
	{
		RString sPageName = (*p)->GetPageName();

		DeviceInput di;
		switch ((*p)->GetType()) {
			case IDebugLine::all_screens:
				di = g_Mappings.debugButton[iNextDebugButton[sPageName]++];
				break;
			case IDebugLine::gameplay_only:
				di = g_Mappings.gameplayButton[iNextGameplayButton++];
				break;
		}
		(*p)->m_Button = di;

		if (find(m_asPages.begin(), m_asPages.end(), sPageName) ==
			m_asPages.end())
			m_asPages.push_back(sPageName);
	}

	m_iCurrentPage = 0;
	m_bForcedHidden = false;

	m_Quad.StretchTo(RectF(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
	m_Quad.SetDiffuse(BACKGROUND_COLOR);
	this->AddChild(&m_Quad);

	// if you're going to add user commands, make sure to have the overrides
	// set after parsing the metrics. -aj
	m_textHeader.SetName("HeaderText");
	m_textHeader.LoadFromFont(THEME->GetPathF("ScreenDebugOverlay", "header"));
	LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_textHeader);
	m_textHeader.SetText(DEBUG_MENU);
	this->AddChild(&m_textHeader);

	FOREACH_CONST(RString, m_asPages, s)
	{
		int iPage = s - m_asPages.begin();

		DeviceInput di;
		bool b = GetKeyFromMap(g_Mappings.pageButton, iPage, di);
		ASSERT(b);

		RString sButton = INPUTMAN->GetDeviceSpecificInputString(di);

		BitmapText* p = new BitmapText;
		p->SetName("PageText");
		p->LoadFromFont(THEME->GetPathF("ScreenDebugOverlay", "page"));
		LOAD_ALL_COMMANDS_AND_ON_COMMAND(p);
		// todo: Y value is still hardcoded. -aj
		p->SetXY(PAGE_START_X + iPage * PAGE_SPACING_X, SCREEN_TOP + 20);
		p->SetText(*s + " (" + sButton + ")");
		m_vptextPages.push_back(p);
		this->AddChild(p);
	}

	FOREACH_CONST(IDebugLine*, *g_pvpSubscribers, p)
	{
		{
			BitmapText* bt = new BitmapText;
			bt->SetName("ButtonText");
			bt->LoadFromFont(THEME->GetPathF("ScreenDebugOverlay", "line"));
			bt->SetHorizAlign(align_right);
			bt->SetText("blah");
			LOAD_ALL_COMMANDS_AND_ON_COMMAND(*bt);
			m_vptextButton.push_back(bt);
			this->AddChild(bt);
		}
		{
			BitmapText* bt = new BitmapText;
			bt->SetName("FunctionText");
			bt->LoadFromFont(THEME->GetPathF("ScreenDebugOverlay", "line"));
			bt->SetHorizAlign(align_left);
			bt->SetText("blah");
			LOAD_ALL_COMMANDS_AND_ON_COMMAND(*bt);
			m_vptextFunction.push_back(bt);
			this->AddChild(bt);
		}
	}

	this->SetVisible(false);
}

void
ScreenDebugOverlay::Update(float fDeltaTime)
{
	{
		float fRate = 1;
		if (INPUTFILTER->IsBeingPressed(g_Mappings.holdForFast)) {
			if (INPUTFILTER->IsBeingPressed(g_Mappings.holdForSlow))
				fRate = 0; // both; stop time
			else
				fRate *= 4;
		} else if (INPUTFILTER->IsBeingPressed(g_Mappings.holdForSlow)) {
			fRate /= 4;
		}

		if (g_bIsHalt)
			fRate = 0;
		else if (g_bIsSlow)
			fRate /= 4;

		GameLoop::SetUpdateRate(fRate);
	}

	bool bCenteringNeedsUpdate =
	  g_fImageScaleCurrent != g_fImageScaleDestination;
	fapproach(g_fImageScaleCurrent, g_fImageScaleDestination, fDeltaTime);
	if (bCenteringNeedsUpdate) {
		DISPLAY->ChangeCentering(
		  PREFSMAN->m_iCenterImageTranslateX,
		  PREFSMAN->m_iCenterImageTranslateY,
		  PREFSMAN->m_fCenterImageAddWidth - (int)SCREEN_WIDTH +
			(int)(g_fImageScaleCurrent * SCREEN_WIDTH),
		  PREFSMAN->m_fCenterImageAddHeight - (int)SCREEN_HEIGHT +
			(int)(g_fImageScaleCurrent * SCREEN_HEIGHT));
	}

	this->SetVisible(g_bIsDisplayed && !m_bForcedHidden);
	if (!g_bIsDisplayed)
		return;

	Screen::Update(fDeltaTime); // Is there a particular reason this needs to be
								// updated when not visible? - Mina
	UpdateText();
}

void
ScreenDebugOverlay::UpdateText()
{
	FOREACH_CONST(RString, m_asPages, s)
	{
		int iPage = s - m_asPages.begin();
		m_vptextPages[iPage]->PlayCommand(
		  (iPage == m_iCurrentPage) ? "GainFocus" : "LoseFocus");
	}

	// todo: allow changing of various spacing/location things -aj
	int iOffset = 0;
	FOREACH_CONST(IDebugLine*, *g_pvpSubscribers, p)
	{
		RString sPageName = (*p)->GetPageName();

		int i = p - g_pvpSubscribers->begin();

		float fY = LINE_START_Y + iOffset * LINE_SPACING;

		BitmapText& txt1 = *m_vptextButton[i];
		BitmapText& txt2 = *m_vptextFunction[i];
		if (sPageName != GetCurrentPageName()) {
			txt1.SetVisible(false);
			txt2.SetVisible(false);
			continue;
		}
		txt1.SetVisible(true);
		txt2.SetVisible(true);
		++iOffset;

		txt1.SetX(LINE_BUTTON_X);
		txt1.SetY(fY);

		txt2.SetX(LINE_FUNCTION_X);
		txt2.SetY(fY);

		RString s1 = (*p)->GetDisplayTitle();
		RString s2 = (*p)->GetDisplayValue();

		bool bOn = (*p)->IsEnabled();

		txt1.SetDiffuse(bOn ? LINE_ON_COLOR : LINE_OFF_COLOR);
		txt2.SetDiffuse(bOn ? LINE_ON_COLOR : LINE_OFF_COLOR);

		RString sButton = GetDebugButtonName(*p);
		if (!sButton.empty())
			sButton += ": ";
		txt1.SetText(sButton);
		if (!s2.empty())
			s1 += " - ";
		txt2.SetText(s1 + s2);
	}

	if (g_bIsHalt) {
		/* More than once I've paused the game accidentally and wasted time
		 * figuring out why, so warn. */
		if (g_HaltTimer.Ago() >= 5.0f) {
			g_HaltTimer.Touch();
			LOG->Warn("Game halted");
		}
	}
}

template<typename U, typename V>
static bool
GetValueFromMap(const std::map<U, V>& m, const U& key, V& val)
{
	typename std::map<U, V>::const_iterator it = m.find(key);
	if (it == m.end())
		return false;
	val = it->second;
	return true;
}

bool
ScreenDebugOverlay::Input(const InputEventPlus& input)
{
	if (input.DeviceI == g_Mappings.holdForDebug1 ||
		input.DeviceI == g_Mappings.holdForDebug2) {
		bool bHoldingNeither =
		  (!g_Mappings.holdForDebug1.IsValid() ||
		   !INPUTFILTER->IsBeingPressed(g_Mappings.holdForDebug1)) &&
		  (!g_Mappings.holdForDebug2.IsValid() ||
		   !INPUTFILTER->IsBeingPressed(g_Mappings.holdForDebug2));
		bool bHoldingBoth =
		  (!g_Mappings.holdForDebug1.IsValid() ||
		   INPUTFILTER->IsBeingPressed(g_Mappings.holdForDebug1)) &&
		  (!g_Mappings.holdForDebug2.IsValid() ||
		   INPUTFILTER->IsBeingPressed(g_Mappings.holdForDebug2));
		if (bHoldingNeither)
			m_bForcedHidden = false;

		if (bHoldingBoth)
			g_bIsDisplayed = true;
		else
			g_bIsDisplayed = false;
	}
	if (input.DeviceI == g_Mappings.toggleMute) {
		PREFSMAN->m_MuteActions.Set(!PREFSMAN->m_MuteActions);
		SCREENMAN->SystemMessage(PREFSMAN->m_MuteActions
								   ? MUTE_ACTIONS_ON.GetValue()
								   : MUTE_ACTIONS_OFF.GetValue());
	}

	int iPage = 0;
	if (g_bIsDisplayed &&
		GetValueFromMap(g_Mappings.pageButton, input.DeviceI, iPage)) {
		if (input.type != IET_FIRST_PRESS)
			return true; // eat the input but do nothing
		m_iCurrentPage = iPage;
		CLAMP(m_iCurrentPage, 0, (int)m_asPages.size() - 1);
		return true;
	}

	FOREACH_CONST(IDebugLine*, *g_pvpSubscribers, p)
	{
		RString sPageName = (*p)->GetPageName();

		int i = p - g_pvpSubscribers->begin();

		// Gameplay buttons are available only in gameplay. Non-gameplay buttons
		// are only available when the screen is displayed.
		IDebugLine::Type type = (*p)->GetType();
		switch (type) {
			case IDebugLine::all_screens:
				if (!g_bIsDisplayed)
					continue;
				if (sPageName != GetCurrentPageName())
					continue;
				break;
			case IDebugLine::gameplay_only:
				if (!IsGameplay())
					continue;
				break;
			default:
				FAIL_M(ssprintf("Invalid debug line type: %i", type));
		}

		if (input.DeviceI == (*p)->m_Button) {
			if (input.type != IET_FIRST_PRESS)
				return true; // eat the input but do nothing

			// do the action
			RString sMessage;
			(*p)->DoAndLog(sMessage);
			if (!sMessage.empty())
				LOG->Trace("DEBUG: %s", sMessage.c_str());
			if ((*p)->ForceOffAfterUse())
				m_bForcedHidden = true;

			// update text to show the effect of what changed above
			UpdateText();

			// show what changed
			BitmapText& bt = *m_vptextButton[i];
			bt.FinishTweening();
			bt.PlayCommand("Toggled");

			return true;
		}
	}

	return Screen::Input(input);
}

// DebugLine helpers
static void
SetSpeed()
{
	// PauseMusic( g_bIsHalt );
}

void
ChangeVolume(float fDelta)
{
	Preference<float>* pRet =
	  Preference<float>::GetPreferenceByName("SoundVolume");
	float fVol = pRet->Get();
	fVol += fDelta;
	CLAMP(fVol, 0.0f, 1.0f);
	pRet->Set(fVol);
	SOUNDMAN->SetMixVolume();
}

void
ChangeVisualDelay(float fDelta)
{
	Preference<float>* pRet =
	  Preference<float>::GetPreferenceByName("VisualDelaySeconds");
	float fSecs = pRet->Get();
	fSecs += fDelta;
	CLAMP(fSecs, -1.0f, 1.0f);
	pRet->Set(fSecs);
}

// DebugLines
static LocalizedString AUTO_PLAY("ScreenDebugOverlay", "AutoPlay");
static LocalizedString ASSIST("ScreenDebugOverlay", "Assist");
static LocalizedString AUTOSYNC("ScreenDebugOverlay", "Autosync");
static LocalizedString COIN_MODE("ScreenDebugOverlay", "CoinMode");
static LocalizedString HALT("ScreenDebugOverlay", "Halt");
static LocalizedString RENDERING_STATS("ScreenDebugOverlay", "Rendering Stats");
static LocalizedString VSYNC("ScreenDebugOverlay", "Vsync");
static LocalizedString MULTITEXTURE("ScreenDebugOverlay", "Multitexture");
static LocalizedString SCREEN_TEST_MODE("ScreenDebugOverlay",
										"Screen Test Mode");
static LocalizedString SCREEN_SHOW_MASKS("ScreenDebugOverlay", "Show Masks");
static LocalizedString PROFILE("ScreenDebugOverlay", "Profile");
static LocalizedString CLEAR_PROFILE_STATS("ScreenDebugOverlay",
										   "Clear Profile Stats");
static LocalizedString FILL_PROFILE_STATS("ScreenDebugOverlay",
										  "Fill Profile Stats");
static LocalizedString SEND_NOTES_ENDED("ScreenDebugOverlay",
										"Send Notes Ended");
static LocalizedString RESET_KEY_MAP("ScreenDebugOverlay",
									 "Reset key mapping to default");
static LocalizedString MUTE_ACTIONS("ScreenDebugOverlay", "Mute actions");
static LocalizedString RELOAD("ScreenDebugOverlay", "Reload");
static LocalizedString RESTART("ScreenDebugOverlay", "Restart");
static LocalizedString SCREEN_ON("ScreenDebugOverlay", "Send On To Screen");
static LocalizedString SCREEN_OFF("ScreenDebugOverlay", "Send Off To Screen");
static LocalizedString RELOAD_OVERLAY_SCREENS("ScreenDebugOverlay",
											  "Reload Overlay Screens");
static LocalizedString TOGGLE_ERRORS("ScreenDebugOverlay", "Toggle Errors");
static LocalizedString SHOW_RECENT_ERRORS("ScreenDebugOverlay",
										  "Show Recent Errors");
static LocalizedString CLEAR_ERRORS("ScreenDebugOverlay", "Clear Errors");
static LocalizedString CONVERT_XML("ScreenDebugOverlay", "Convert XML");
static LocalizedString RELOAD_THEME_AND_TEXTURES("ScreenDebugOverlay",
												 "Reload Theme and Textures");
static LocalizedString WRITE_PROFILES("ScreenDebugOverlay", "Write Profiles");
static LocalizedString WRITE_PREFERENCES("ScreenDebugOverlay",
										 "Write Preferences");
static LocalizedString MENU_TIMER("ScreenDebugOverlay", "Menu Timer");
static LocalizedString FLUSH_LOG("ScreenDebugOverlay", "Flush Log");
static LocalizedString PULL_BACK_CAMERA("ScreenDebugOverlay",
										"Pull Back Camera");
static LocalizedString VISUAL_DELAY_UP("ScreenDebugOverlay", "Visual Delay Up");
static LocalizedString VISUAL_DELAY_DOWN("ScreenDebugOverlay",
										 "Visual Delay Down");
static LocalizedString VOLUME_UP("ScreenDebugOverlay", "Volume Up");
static LocalizedString VOLUME_DOWN("ScreenDebugOverlay", "Volume Down");
static LocalizedString UPTIME("ScreenDebugOverlay", "Uptime");
static LocalizedString FORCE_CRASH("ScreenDebugOverlay", "Force Crash");
static LocalizedString SLOW("ScreenDebugOverlay", "Slow");
static LocalizedString CPU("ScreenDebugOverlay", "CPU");
static LocalizedString REPLAY("ScreenDebugOverlay", "REPLAY");
static LocalizedString SONG("ScreenDebugOverlay", "Song");
static LocalizedString MACHINE("ScreenDebugOverlay", "Machine");
static LocalizedString SYNC_TEMPO("ScreenDebugOverlay", "Tempo");

class DebugLineAutoplay : public IDebugLine
{
	RString GetDisplayTitle() override
	{
		return AUTO_PLAY.GetValue() + " (+Shift = AI) (+Alt = hide)";
	}
	RString GetDisplayValue() override
	{
		PlayerController pc = GamePreferences::m_AutoPlay.Get();
		switch (pc) {
			case PC_HUMAN:
				return OFF.GetValue();
				break;
			case PC_AUTOPLAY:
				return ON.GetValue();
				break;
			case PC_CPU:
				return CPU.GetValue();
				break;
			case PC_REPLAY:
				return REPLAY.GetValue();
				break;
			default:
				FAIL_M(ssprintf("Invalid PlayerController: %i", pc));
		}
	}
	Type GetType() const override { return IDebugLine::gameplay_only; }
	bool IsEnabled() override
	{
		return GamePreferences::m_AutoPlay.Get() != PC_HUMAN;
	}
	void DoAndLog(RString& sMessageOut) override
	{
		ASSERT(GAMESTATE->GetMasterPlayerNumber() != PLAYER_INVALID);
		PlayerController pc =
		  GAMESTATE->m_pPlayerState
			->m_PlayerController;
		bool bHoldingShift =
		  INPUTFILTER->IsBeingPressed(
			DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT)) ||
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT));
		if (bHoldingShift)
			pc = (pc == PC_CPU) ? PC_HUMAN : PC_CPU;
		else
			pc = (pc == PC_AUTOPLAY) ? PC_HUMAN : PC_AUTOPLAY;
		if (GamePreferences::m_AutoPlay != PC_REPLAY)
			GamePreferences::m_AutoPlay.Set(pc);
		GAMESTATE->m_pPlayerState
		  ->m_PlayerController = GamePreferences::m_AutoPlay;
		FOREACH_MultiPlayer(p) GAMESTATE->m_pMultiPlayerState[p]
		  ->m_PlayerController = GamePreferences::m_AutoPlay;

		// Hide Autoplay if Alt is held down
		bool bHoldingAlt =
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LALT)) ||
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RALT));
		ScreenSyncOverlay::SetShowAutoplay(!bHoldingAlt);

		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineAssist : public IDebugLine
{
	RString GetDisplayTitle() override { return ASSIST.GetValue(); }
	Type GetType() const override { return gameplay_only; }
	RString GetDisplayValue() override
	{
		SongOptions so;
		so.m_bAssistClap = GAMESTATE->m_SongOptions.GetSong().m_bAssistClap;
		so.m_bAssistMetronome =
		  GAMESTATE->m_SongOptions.GetSong().m_bAssistMetronome;
		if (so.m_bAssistClap || so.m_bAssistMetronome)
			return so.GetLocalizedString();
		else
			return OFF.GetValue();
	}
	bool IsEnabled() override
	{
		return GAMESTATE->m_SongOptions.GetSong().m_bAssistClap ||
			   GAMESTATE->m_SongOptions.GetSong().m_bAssistMetronome;
	}
	void DoAndLog(RString& sMessageOut) override
	{
		ASSERT(GAMESTATE->GetMasterPlayerNumber() != PLAYER_INVALID);
		bool bHoldingShift =
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT));
		bool b;
		if (bHoldingShift)
			b = !GAMESTATE->m_SongOptions.GetSong().m_bAssistMetronome;
		else
			b = !GAMESTATE->m_SongOptions.GetSong().m_bAssistClap;
		if (bHoldingShift)
			SO_GROUP_ASSIGN(GAMESTATE->m_SongOptions,
							ModsLevel_Preferred,
							m_bAssistMetronome,
							b);
		else
			SO_GROUP_ASSIGN(
			  GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bAssistClap, b);

		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineAutosync : public IDebugLine
{
	RString GetDisplayTitle() override { return AUTOSYNC.GetValue(); }
	RString GetDisplayValue() override
	{
		AutosyncType type = GAMESTATE->m_SongOptions.GetSong().m_AutosyncType;
		switch (type) {
			case AutosyncType_Off:
				return OFF.GetValue();
				break;
			case AutosyncType_Song:
				return SONG.GetValue();
				break;
			case AutosyncType_Machine:
				return MACHINE.GetValue();
				break;
			case AutosyncType_Tempo:
				return SYNC_TEMPO.GetValue();
				break;
			default:
				FAIL_M(ssprintf("Invalid autosync type: %i", type));
		}
	}
	Type GetType() const override { return IDebugLine::gameplay_only; }
	bool IsEnabled() override
	{
		return GAMESTATE->m_SongOptions.GetSong().m_AutosyncType !=
			   AutosyncType_Off;
	}
	void DoAndLog(RString& sMessageOut) override
	{
		int as = GAMESTATE->m_SongOptions.GetSong().m_AutosyncType + 1;
		bool bAllowSongAutosync = true;
		if (!bAllowSongAutosync &&
			(as == AutosyncType_Song || as == AutosyncType_Tempo))
			as = AutosyncType_Machine;
		wrap(as, NUM_AutosyncType);
		SO_GROUP_ASSIGN(GAMESTATE->m_SongOptions,
						ModsLevel_Song,
						m_AutosyncType,
						AutosyncType(as));
		MESSAGEMAN->Broadcast(Message_AutosyncChanged);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineSlow : public IDebugLine
{
	RString GetDisplayTitle() override { return SLOW.GetValue(); }
	bool IsEnabled() override { return g_bIsSlow; }
	void DoAndLog(RString& sMessageOut) override
	{
		g_bIsSlow = !g_bIsSlow;
		SetSpeed();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineHalt : public IDebugLine
{
	RString GetDisplayTitle() override { return HALT.GetValue(); }
	bool IsEnabled() override { return g_bIsHalt; }
	void DoAndLog(RString& sMessageOut) override
	{
		g_bIsHalt = !g_bIsHalt;
		g_HaltTimer.Touch();
		SetSpeed();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineStats : public IDebugLine
{
	RString GetDisplayTitle() override { return RENDERING_STATS.GetValue(); }
	bool IsEnabled() override { return PREFSMAN->m_bShowStats.Get(); }
	void DoAndLog(RString& sMessageOut) override
	{
		PREFSMAN->m_bShowStats.Set(!PREFSMAN->m_bShowStats);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineVsync : public IDebugLine
{
	RString GetDisplayTitle() override { return VSYNC.GetValue(); }
	bool IsEnabled() override { return PREFSMAN->m_bVsync.Get(); }
	void DoAndLog(RString& sMessageOut) override
	{
		PREFSMAN->m_bVsync.Set(!PREFSMAN->m_bVsync);
		StepMania::ApplyGraphicOptions();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineAllowMultitexture : public IDebugLine
{
	RString GetDisplayTitle() override { return MULTITEXTURE.GetValue(); }
	bool IsEnabled() override { return PREFSMAN->m_bAllowMultitexture.Get(); }
	void DoAndLog(RString& sMessageOut) override
	{
		PREFSMAN->m_bAllowMultitexture.Set(!PREFSMAN->m_bAllowMultitexture);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineShowMasks : public IDebugLine
{
	RString GetDisplayTitle() override { return SCREEN_SHOW_MASKS.GetValue(); }
	bool IsEnabled() override { return GetPref()->Get(); }
	RString GetPageName() const override { return "Theme"; }
	void DoAndLog(RString& sMessageOut) override
	{
		GetPref()->Set(!GetPref()->Get());
		IDebugLine::DoAndLog(sMessageOut);
	}

	Preference<bool>* GetPref()
	{
		return Preference<bool>::GetPreferenceByName("ShowMasks");
	}
};

static ProfileSlot g_ProfileSlot = ProfileSlot_Player1;
static bool
IsSelectProfilePersistent()
{
	return PROFILEMAN->IsPersistentProfile(
	  static_cast<PlayerNumber>(g_ProfileSlot));
}

class DebugLineProfileSlot : public IDebugLine
{
	RString GetDisplayTitle() override { return PROFILE.GetValue(); }
	RString GetDisplayValue() override
	{
		switch (g_ProfileSlot) {
			case ProfileSlot_Player1:
				return "Player 1";
			case ProfileSlot_Player2:
				return "Player 2";
			default:
				return RString();
		}
	}
	bool IsEnabled() override { return IsSelectProfilePersistent(); }
	RString GetPageName() const override { return "Profiles"; }
	void DoAndLog(RString& sMessageOut) override
	{
		enum_add(g_ProfileSlot, +1);
		if (g_ProfileSlot == NUM_ProfileSlot)
			g_ProfileSlot = ProfileSlot_Player1;

		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineClearProfileStats : public IDebugLine
{
	RString GetDisplayTitle() override
	{
		return CLEAR_PROFILE_STATS.GetValue();
	}
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return IsSelectProfilePersistent(); }
	RString GetPageName() const override { return "Profiles"; }
	void DoAndLog(RString& sMessageOut) override
	{
		Profile* pProfile = PROFILEMAN->GetProfile(g_ProfileSlot);
		pProfile->ClearStats();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

static HighScore
MakeRandomHighScore(float fPercentDP)
{
	HighScore hs;
	hs.SetName("FAKE");
	auto g =
	  static_cast<Grade> SCALE(RandomInt(6), 0, 4, Grade_Tier01, Grade_Tier06);
	if (g == Grade_Tier06)
		g = Grade_Failed;
	hs.SetGrade(g);
	hs.SetScore(RandomInt(100 * 1000));
	hs.SetPercentDP(fPercentDP);
	hs.SetAliveSeconds(randomf(30.0f, 100.0f));
	PlayerOptions po;
	po.ChooseRandomModifiers();
	hs.SetModifiers(po.GetString());
	hs.SetDateTime(DateTime::GetNowDateTime());
	hs.SetPlayerGuid(Profile::MakeGuid());
	hs.SetMachineGuid(Profile::MakeGuid());
	hs.SetProductID(RandomInt(10));
	FOREACH_ENUM(TapNoteScore, tns)
	hs.SetTapNoteScore(tns, RandomInt(100));
	FOREACH_ENUM(HoldNoteScore, hns)
	hs.SetHoldNoteScore(hns, RandomInt(100));
	RadarValues rv;
	FOREACH_ENUM(RadarCategory, rc)
	{
		rv[rc] = static_cast<int>(randomf(0, 1));
	}
	hs.SetRadarValues(rv);

	return hs;
}

static void
FillProfileStats(Profile* pProfile)
{
	pProfile->InitSongScores();

	static int s_iCount = 0;
	// Choose a percent for all scores. This is useful for testing unlocks
	// where some elements are unlocked at a certain percent complete.
	float fPercentDP = s_iCount != 0 ? randomf(0.6f, 1.0f) : 1.0f;
	s_iCount = (s_iCount + 1) % 2;

	int iCount = 20;

	std::vector<Song*> vpAllSongs = SONGMAN->GetAllSongs();
	FOREACH(Song*, vpAllSongs, pSong)
	{
		std::vector<Steps*> vpAllSteps = (*pSong)->GetAllSteps();
		FOREACH(Steps*, vpAllSteps, pSteps)
		{
			if (random_up_to(5)) {
				pProfile->IncrementStepsPlayCount(*pSong, *pSteps);
			}
			for (int i = 0; i < iCount; i++) {
				int iIndex = 0;
				pProfile->AddStepsHighScore(
				  *pSong, *pSteps, MakeRandomHighScore(fPercentDP), iIndex);
			}
		}
	}
	SCREENMAN->ZeroNextUpdate();
}

class DebugLineFillProfileStats : public IDebugLine
{
	RString GetDisplayTitle() override { return FILL_PROFILE_STATS.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return IsSelectProfilePersistent(); }
	RString GetPageName() const override { return "Profiles"; }
	void DoAndLog(RString& sMessageOut) override
	{
		Profile* pProfile = PROFILEMAN->GetProfile(g_ProfileSlot);
		FillProfileStats(pProfile);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineSendNotesEnded : public IDebugLine
{
	RString GetDisplayTitle() override { return SEND_NOTES_ENDED.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return true; }
	void DoAndLog(RString& sMessageOut) override
	{
		SCREENMAN->PostMessageToTopScreen(SM_NotesEnded, 0);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineResetKeyMapping : public IDebugLine
{
	RString GetDisplayTitle() override { return RESET_KEY_MAP.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return true; }
	void DoAndLog(RString& sMessageOut) override
	{
		INPUTMAPPER->ResetMappingsToDefault();
		INPUTMAPPER->SaveMappingsToDisk();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineMuteActions : public IDebugLine
{
	RString GetDisplayTitle() override { return MUTE_ACTIONS.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return PREFSMAN->m_MuteActions; }
	void DoAndLog(RString& sMessageOut) override
	{
		PREFSMAN->m_MuteActions.Set(!PREFSMAN->m_MuteActions);
		SCREENMAN->SystemMessage(PREFSMAN->m_MuteActions
								   ? MUTE_ACTIONS_ON.GetValue()
								   : MUTE_ACTIONS_OFF.GetValue());
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineReloadCurrentScreen : public IDebugLine
{
	RString GetDisplayTitle() override { return RELOAD.GetValue(); }
	RString GetDisplayValue() override
	{
		return SCREENMAN && SCREENMAN->GetTopScreen()
				 ? SCREENMAN->GetTopScreen()->GetName()
				 : RString();
	}
	bool IsEnabled() override { return true; }
	RString GetPageName() const override { return "Theme"; }
	void DoAndLog(RString& sMessageOut) override
	{
		RString sScreenName = SCREENMAN->GetScreen(0)->GetName();
		SCREENMAN->PopAllScreens();

		SOUND->StopMusic();
		// StepMania::ResetGame();

		SCREENMAN->SetNewScreen(sScreenName);
		IDebugLine::DoAndLog(sMessageOut);
		sMessageOut = "";
	}
};

class DebugLineRestartCurrentScreen : public IDebugLine
{
	RString GetDisplayTitle() override { return RESTART.GetValue(); }
	RString GetDisplayValue() override
	{
		return SCREENMAN && SCREENMAN->GetTopScreen()
				 ? SCREENMAN->GetTopScreen()->GetName()
				 : RString();
	}
	bool IsEnabled() override { return true; }
	bool ForceOffAfterUse() const override { return true; }
	RString GetPageName() const override { return "Theme"; }
	void DoAndLog(RString& sMessageOut) override
	{
		SCREENMAN->GetTopScreen()->BeginScreen();
		IDebugLine::DoAndLog(sMessageOut);
		sMessageOut = "";
	}
};

class DebugLineCurrentScreenOn : public IDebugLine
{
	RString GetDisplayTitle() override { return SCREEN_ON.GetValue(); }
	RString GetDisplayValue() override
	{
		return SCREENMAN && SCREENMAN->GetTopScreen()
				 ? SCREENMAN->GetTopScreen()->GetName()
				 : RString();
	}
	bool IsEnabled() override { return true; }
	bool ForceOffAfterUse() const override { return true; }
	RString GetPageName() const override { return "Theme"; }
	void DoAndLog(RString& sMessageOut) override
	{
		SCREENMAN->GetTopScreen()->PlayCommand("On");
		IDebugLine::DoAndLog(sMessageOut);
		sMessageOut = "";
	}
};

class DebugLineCurrentScreenOff : public IDebugLine
{
	RString GetDisplayTitle() override { return SCREEN_OFF.GetValue(); }
	RString GetDisplayValue() override
	{
		return SCREENMAN && SCREENMAN->GetTopScreen()
				 ? SCREENMAN->GetTopScreen()->GetName()
				 : RString();
	}
	bool IsEnabled() override { return true; }
	bool ForceOffAfterUse() const override { return true; }
	RString GetPageName() const override { return "Theme"; }
	void DoAndLog(RString& sMessageOut) override
	{
		SCREENMAN->GetTopScreen()->PlayCommand("Off");
		IDebugLine::DoAndLog(sMessageOut);
		sMessageOut = "";
	}
};

class DebugLineReloadTheme : public IDebugLine
{
	RString GetDisplayTitle() override
	{
		return RELOAD_THEME_AND_TEXTURES.GetValue();
	}
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return true; }
	RString GetPageName() const override { return "Theme"; }
	void DoAndLog(RString& sMessageOut) override
	{
		THEME->ReloadMetrics();
		TEXTUREMAN->ReloadAll();
		NOTESKIN->RefreshNoteSkinData(GAMESTATE->m_pCurGame);
		CodeDetector::RefreshCacheItems();
		// HACK: Don't update text below. Return immediately because this screen
		// was just destroyed as part of the theme reload.
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineReloadOverlayScreens : public IDebugLine
{
	RString GetDisplayTitle() override
	{
		return RELOAD_OVERLAY_SCREENS.GetValue();
	}
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return true; }
	RString GetPageName() const override { return "Theme"; }
	void DoAndLog(RString& sMessageOut) override
	{
		SCREENMAN->ReloadOverlayScreensAfterInputFinishes();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineToggleErrors : public IDebugLine
{
	RString GetDisplayTitle() override { return TOGGLE_ERRORS.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return PREFSMAN->m_show_theme_errors; }
	RString GetPageName() const override { return "Theme"; }
	void DoAndLog(RString& sMessageOut) override
	{
		PREFSMAN->m_show_theme_errors.Set(!PREFSMAN->m_show_theme_errors);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineShowRecentErrors : public IDebugLine
{
	RString GetDisplayTitle() override { return SHOW_RECENT_ERRORS.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return true; }
	RString GetPageName() const override { return "Theme"; }
	void DoAndLog(RString& sMessageOut) override
	{
		Message msg("ToggleScriptError");
		MESSAGEMAN->Broadcast(msg);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineClearErrors : public IDebugLine
{
	RString GetDisplayTitle() override { return CLEAR_ERRORS.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return true; }
	RString GetPageName() const override { return "Theme"; }
	void DoAndLog(RString& sMessageOut) override
	{
		Message msg("ClearScriptError");
		MESSAGEMAN->Broadcast(msg);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineConvertXML : public IDebugLine
{
	RString GetDisplayTitle() override { return CONVERT_XML.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return true; }
	RString GetPageName() const override { return "Theme"; }
	void DoAndLog(RString& sMessageOut) override
	{
		Song* cur_song = GAMESTATE->m_pCurSong;
		if (cur_song != nullptr) {
			convert_xmls_in_dir(cur_song->GetSongDir() + "/");
			IDebugLine::DoAndLog(sMessageOut);
		}
	}
};

class DebugLineWriteProfiles : public IDebugLine
{
	RString GetDisplayTitle() override { return WRITE_PROFILES.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return IsSelectProfilePersistent(); }
	RString GetPageName() const override { return "Profiles"; }
	void DoAndLog(RString& sMessageOut) override
	{
		auto pn = static_cast<PlayerNumber>(g_ProfileSlot);
		GAMESTATE->SaveCurrentSettingsToProfile(pn);
		GAMESTATE->SavePlayerProfile(pn);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineWritePreferences : public IDebugLine
{
	RString GetDisplayTitle() override { return WRITE_PREFERENCES.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return true; }
	void DoAndLog(RString& sMessageOut) override
	{
		PREFSMAN->SavePrefsToDisk();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineMenuTimer : public IDebugLine
{
	RString GetDisplayTitle() override { return MENU_TIMER.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return PREFSMAN->m_bMenuTimer.Get(); }
	void DoAndLog(RString& sMessageOut) override
	{
		PREFSMAN->m_bMenuTimer.Set(!PREFSMAN->m_bMenuTimer);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineFlushLog : public IDebugLine
{
	RString GetDisplayTitle() override { return FLUSH_LOG.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return true; }
	void DoAndLog(RString& sMessageOut) override
	{
		LOG->Flush();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLinePullBackCamera : public IDebugLine
{
	RString GetDisplayTitle() override { return PULL_BACK_CAMERA.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return g_fImageScaleDestination != 1; }
	void DoAndLog(RString& sMessageOut) override
	{
		if (g_fImageScaleDestination == 1)
			g_fImageScaleDestination = 0.5f;
		else
			g_fImageScaleDestination = 1;
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineVolumeUp : public IDebugLine
{
	RString GetDisplayTitle() override { return VOLUME_UP.GetValue(); }
	RString GetDisplayValue() override
	{
		return ssprintf("%.0f%%", GetPref()->Get() * 100);
	}
	bool IsEnabled() override { return true; }
	void DoAndLog(RString& sMessageOut) override
	{
		ChangeVolume(+0.1f);
		IDebugLine::DoAndLog(sMessageOut);
	}
	Preference<float>* GetPref()
	{
		return Preference<float>::GetPreferenceByName("SoundVolume");
	}
};

class DebugLineVolumeDown : public IDebugLine
{
	RString GetDisplayTitle() override { return VOLUME_DOWN.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return true; }
	void DoAndLog(RString& sMessageOut) override
	{
		ChangeVolume(-0.1f);
		IDebugLine::DoAndLog(sMessageOut);
		sMessageOut += " - " + ssprintf("%.0f%%", GetPref()->Get() * 100);
	}
	Preference<float>* GetPref()
	{
		return Preference<float>::GetPreferenceByName("SoundVolume");
	}
};

class DebugLineVisualDelayUp : public IDebugLine
{
	RString GetDisplayTitle() override { return VISUAL_DELAY_UP.GetValue(); }
	RString GetDisplayValue() override
	{
		return ssprintf("%.03f", GetPref()->Get());
	}
	bool IsEnabled() override { return true; }
	void DoAndLog(RString& sMessageOut) override
	{
		ChangeVisualDelay(+0.001f);
		IDebugLine::DoAndLog(sMessageOut);
	}
	Preference<float>* GetPref()
	{
		return Preference<float>::GetPreferenceByName("VisualDelaySeconds");
	}
};

class DebugLineVisualDelayDown : public IDebugLine
{
	RString GetDisplayTitle() override { return VISUAL_DELAY_DOWN.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return true; }
	void DoAndLog(RString& sMessageOut) override
	{
		ChangeVisualDelay(-0.001f);
		IDebugLine::DoAndLog(sMessageOut);
		sMessageOut += " - " + ssprintf("%.03f", GetPref()->Get());
	}
	Preference<float>* GetPref()
	{
		return Preference<float>::GetPreferenceByName("VisualDelaySeconds");
	}
};

class DebugLineForceCrash : public IDebugLine
{
	RString GetDisplayTitle() override { return FORCE_CRASH.GetValue(); }
	RString GetDisplayValue() override { return RString(); }
	bool IsEnabled() override { return false; }
	void DoAndLog(RString& sMessageOut) override { FAIL_M("DebugLineCrash"); }
};

class DebugLineUptime : public IDebugLine
{
	RString GetDisplayTitle() override { return UPTIME.GetValue(); }
	RString GetDisplayValue() override
	{
		return SecondsToMMSSMsMsMs(RageTimer::GetTimeSinceStart());
	}
	bool IsEnabled() override { return false; }
	void DoAndLog(RString& sMessageOut) override {}
};

/* #ifdef out the lines below if you don't want them to appear on certain
 * platforms.  This is easier than #ifdefing the whole DebugLine definitions
 * that can span pages.
 */

#define DECLARE_ONE(x) static x g_##x
DECLARE_ONE(DebugLineAutoplay);
DECLARE_ONE(DebugLineAssist);
DECLARE_ONE(DebugLineAutosync);
DECLARE_ONE(DebugLineSlow);
DECLARE_ONE(DebugLineHalt);
DECLARE_ONE(DebugLineStats);
DECLARE_ONE(DebugLineVsync);
DECLARE_ONE(DebugLineAllowMultitexture);
DECLARE_ONE(DebugLineShowMasks);
DECLARE_ONE(DebugLineProfileSlot);
DECLARE_ONE(DebugLineClearProfileStats);
DECLARE_ONE(DebugLineFillProfileStats);
DECLARE_ONE(DebugLineSendNotesEnded);
DECLARE_ONE(DebugLineReloadCurrentScreen);
DECLARE_ONE(DebugLineRestartCurrentScreen);
DECLARE_ONE(DebugLineCurrentScreenOn);
DECLARE_ONE(DebugLineCurrentScreenOff);
DECLARE_ONE(DebugLineReloadTheme);
DECLARE_ONE(DebugLineReloadOverlayScreens);
DECLARE_ONE(DebugLineToggleErrors);
DECLARE_ONE(DebugLineShowRecentErrors);
DECLARE_ONE(DebugLineClearErrors);
DECLARE_ONE(DebugLineConvertXML);
DECLARE_ONE(DebugLineWriteProfiles);
DECLARE_ONE(DebugLineWritePreferences);
DECLARE_ONE(DebugLineMenuTimer);
DECLARE_ONE(DebugLineFlushLog);
DECLARE_ONE(DebugLinePullBackCamera);
DECLARE_ONE(DebugLineVolumeDown);
DECLARE_ONE(DebugLineVolumeUp);
DECLARE_ONE(DebugLineVisualDelayDown);
DECLARE_ONE(DebugLineVisualDelayUp);
DECLARE_ONE(DebugLineForceCrash);
DECLARE_ONE(DebugLineUptime);
DECLARE_ONE(DebugLineResetKeyMapping);
DECLARE_ONE(DebugLineMuteActions);

/*
 * (c) 2001-2005 Chris Danford, Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
