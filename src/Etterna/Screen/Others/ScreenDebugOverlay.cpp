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
#include "Core/Services/Locator.hpp"
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
#include "Etterna/Globals/rngthing.h"
#include "Etterna/Models/Misc/Foreach.h"

#include <map>

#include "RageUtil/File/RageFileManager.h"
#include "Core/Platform/Platform.hpp"

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

static Preference<bool> g_debugMenuButtonToggles("DebugMenuButtonToggles",
												 false);

// self-registering debug lines
// We don't use SubscriptionManager, because we want to keep the line order.
static LocalizedString ON("ScreenDebugOverlay", "on");
static LocalizedString OFF("ScreenDebugOverlay", "off");
static LocalizedString MUTE_ACTIONS_ON("ScreenDebugOverlay", "Mute actions on");
static LocalizedString MUTE_ACTIONS_OFF("ScreenDebugOverlay",
										"Mute actions off");

class IDebugLine;
static std::vector<IDebugLine*>* g_pvpSubscribers = nullptr;

class IDebugLine
{
  public:
	IDebugLine()
	{
		if (g_pvpSubscribers == nullptr)
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
	virtual std::string GetDisplayTitle() = 0;

	virtual std::string GetDisplayValue()
	{
		return IsEnabled() ? ON.GetValue() : OFF.GetValue();
	}

	virtual std::string GetPageName() const { return "Main"; }
	virtual bool ForceOffAfterUse() const { return false; }
	virtual bool IsEnabled() = 0;

	virtual void DoAndLog(std::string& sMessageOut)
	{
		std::string s1 = GetDisplayTitle();
		std::string s2 = GetDisplayValue();
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

static std::string
GetDebugButtonName(const IDebugLine* pLine)
{
	std::string s = INPUTMAN->GetDeviceSpecificInputString(pLine->m_Button);
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
	for (typename std::map<U, V>::const_iterator iter = m.begin();
		 iter != m.end();
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

	std::map<std::string, int> iNextDebugButton;
	int iNextGameplayButton = 0;
	FOREACH(IDebugLine*, *g_pvpSubscribers, p)
	{
		std::string sPageName = (*p)->GetPageName();

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

	FOREACH_CONST(std::string, m_asPages, s)
	{
		int iPage = s - m_asPages.begin();

		DeviceInput di;
		bool b = GetKeyFromMap(g_Mappings.pageButton, iPage, di);
		ASSERT(b);

		std::string sButton = INPUTMAN->GetDeviceSpecificInputString(di);

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
		  PREFSMAN->m_fCenterImageAddWidth - static_cast<int>(SCREEN_WIDTH) +
			static_cast<int>(g_fImageScaleCurrent * SCREEN_WIDTH),
		  PREFSMAN->m_fCenterImageAddHeight - static_cast<int>(SCREEN_HEIGHT) +
			static_cast<int>(g_fImageScaleCurrent * SCREEN_HEIGHT));
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
	FOREACH_CONST(std::string, m_asPages, s)
	{
		int iPage = s - m_asPages.begin();
		m_vptextPages[iPage]->PlayCommand(
		  (iPage == m_iCurrentPage) ? "GainFocus" : "LoseFocus");
	}

	// todo: allow changing of various spacing/location things -aj
	int iOffset = 0;
	FOREACH_CONST(IDebugLine*, *g_pvpSubscribers, p)
	{
		std::string sPageName = (*p)->GetPageName();

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
		txt2.SetMaxWidth((SCREEN_WIDTH - LINE_FUNCTION_X) / txt2.GetZoom());

		std::string s1 = (*p)->GetDisplayTitle();
		std::string s2 = (*p)->GetDisplayValue();

		bool bOn = (*p)->IsEnabled();

		txt1.SetDiffuse(bOn ? LINE_ON_COLOR : LINE_OFF_COLOR);
		txt2.SetDiffuse(bOn ? LINE_ON_COLOR : LINE_OFF_COLOR);

		std::string sButton = GetDebugButtonName(*p);
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
			Locator::getLogger()->warn("Game halted");
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

		if (bHoldingBoth) {
			if (g_debugMenuButtonToggles && input.type != IET_REPEAT) {
				// if the button should toggle, when pressing
				// just flip the state
				g_bIsDisplayed = !g_bIsDisplayed;
			} else {
				// if it doesnt toggle, hold it on
				g_bIsDisplayed = true;
			}
		} else {
			if (!g_debugMenuButtonToggles) {
				// if it shouldnt toggle, hide on release
				g_bIsDisplayed = false;
			}
		}
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
		CLAMP(m_iCurrentPage, 0, static_cast<int>(m_asPages.size()) - 1);
		return true;
	}

	FOREACH_CONST(IDebugLine*, *g_pvpSubscribers, p)
	{
		std::string sPageName = (*p)->GetPageName();

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
			std::string sMessage;
			(*p)->DoAndLog(sMessage);
			if (!sMessage.empty())
				Locator::getLogger()->info("DEBUG: {}", sMessage.c_str());
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

void
ChangeGlobalOffset(float fDelta)
{
	Preference<float>* pRet =
	  Preference<float>::GetPreferenceByName("GlobalOffsetSeconds");
	float fSecs = pRet->Get();
	fSecs += fDelta;
	CLAMP(fSecs, -5.0f, 5.0f);
	pRet->Set(fSecs);
}

void
ResetGlobalOffset()
{
	Preference<float>* pRet =
	  Preference<float>::GetPreferenceByName("GlobalOffsetSeconds");
	pRet->Set(0.0f);
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
static LocalizedString GLOBAL_OFFSET_UP("ScreenDebugOverlay",
										"Global Offset Up");
static LocalizedString GLOBAL_OFFSET_DOWN("ScreenDebugOverlay",
										  "Global Offset Down");
static LocalizedString GLOBAL_OFFSET_RESET("ScreenDebugOverlay",
										   "Global Offset Reset");
static LocalizedString KEY_CONFIG("ScreenDebugOverlay", "Key Config");
static LocalizedString CHART_FOLDER("ScreenDebugOverlay", "Chart Folder");
static LocalizedString CHART_KEY("ScreenDebugOverlay", "Chartkey");
static LocalizedString VOLUME_UP("ScreenDebugOverlay", "Volume Up");
static LocalizedString VOLUME_DOWN("ScreenDebugOverlay", "Volume Down");
static LocalizedString UPTIME("ScreenDebugOverlay", "Uptime");
static LocalizedString FORCE_CRASH("ScreenDebugOverlay", "Force Crash");
static LocalizedString SLOW("ScreenDebugOverlay", "Slow");
static LocalizedString CPU("ScreenDebugOverlay", "CPU");
static LocalizedString REPLAY("ScreenDebugOverlay", "REPLAY");
static LocalizedString SONG("ScreenDebugOverlay", "Song");
static LocalizedString MACHINE("ScreenDebugOverlay", "Machine");
static LocalizedString RENDER_SKIPS("ScreenDebugOverlay", "Rendering Skips");
static LocalizedString EASTER_EGGS("ScreenDebugOverlay", "Easter Eggs");
static LocalizedString OSU_LIFTS("ScreenDebugOverlay", "Osu Lifts");
static LocalizedString PITCH_RATES("ScreenDebugOverlay", "Pitch Rates");
static LocalizedString FULLSCREEN("ScreenDebugOverlay", "Fullscreen");

class DebugLineAutoplay : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return AUTO_PLAY.GetValue() + " (+Shift = AI) (+Alt = hide)";
	}

	std::string GetDisplayValue() override
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

	Type GetType() const override { return gameplay_only; }

	bool IsEnabled() override
	{
		return GamePreferences::m_AutoPlay.Get() != PC_HUMAN;
	}

	void DoAndLog(std::string& sMessageOut) override
	{
		ASSERT(GAMESTATE->GetMasterPlayerNumber() != PLAYER_INVALID);
		PlayerController pc = GAMESTATE->m_pPlayerState->m_PlayerController;
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
		GAMESTATE->m_pPlayerState->m_PlayerController =
		  GamePreferences::m_AutoPlay;
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
	std::string GetDisplayTitle() override { return ASSIST.GetValue(); }
	Type GetType() const override { return gameplay_only; }

	std::string GetDisplayValue() override
	{
		SongOptions so;
		so.m_bAssistClap = GAMESTATE->m_SongOptions.GetSong().m_bAssistClap;
		so.m_bAssistMetronome =
		  GAMESTATE->m_SongOptions.GetSong().m_bAssistMetronome;
		if (so.m_bAssistClap || so.m_bAssistMetronome)
			return so.GetLocalizedString();
		return OFF.GetValue();
	}

	bool IsEnabled() override
	{
		return GAMESTATE->m_SongOptions.GetSong().m_bAssistClap ||
			   GAMESTATE->m_SongOptions.GetSong().m_bAssistMetronome;
	}

	void DoAndLog(std::string& sMessageOut) override
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
	std::string GetDisplayTitle() override { return AUTOSYNC.GetValue(); }

	std::string GetDisplayValue() override
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
			default:
				FAIL_M(ssprintf("Invalid autosync type: %i", type));
		}
	}

	Type GetType() const override { return gameplay_only; }

	bool IsEnabled() override
	{
		return GAMESTATE->m_SongOptions.GetSong().m_AutosyncType !=
			   AutosyncType_Off;
	}

	void DoAndLog(std::string& sMessageOut) override
	{
		int as = GAMESTATE->m_SongOptions.GetSong().m_AutosyncType + 1;
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
	std::string GetDisplayTitle() override { return SLOW.GetValue(); }
	bool IsEnabled() override { return g_bIsSlow; }

	void DoAndLog(std::string& sMessageOut) override
	{
		g_bIsSlow = !g_bIsSlow;
		SetSpeed();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineHalt : public IDebugLine
{
	std::string GetDisplayTitle() override { return HALT.GetValue(); }
	bool IsEnabled() override { return g_bIsHalt; }

	void DoAndLog(std::string& sMessageOut) override
	{
		g_bIsHalt = !g_bIsHalt;
		g_HaltTimer.Touch();
		SetSpeed();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineStats : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return RENDERING_STATS.GetValue();
	}

	bool IsEnabled() override { return PREFSMAN->m_bShowStats.Get(); }

	void DoAndLog(std::string& sMessageOut) override
	{
		PREFSMAN->m_bShowStats.Set(!PREFSMAN->m_bShowStats);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineSkips : public IDebugLine
{
	std::string GetDisplayTitle() override { return RENDER_SKIPS.GetValue(); }
	std::string GetPageName() const override { return "Misc"; }

	bool IsEnabled() override
	{
		return PREFSMAN->m_bShowSkips && PREFSMAN->m_bShowStats;
	}

	void DoAndLog(std::string& sMessageOut) override
	{
		if (PREFSMAN->m_bShowStats) {
			PREFSMAN->m_bShowSkips.Set(!PREFSMAN->m_bShowSkips);
			IDebugLine::DoAndLog(sMessageOut);
		}
	}
};

class DebugLineVsync : public IDebugLine
{
	std::string GetDisplayTitle() override { return VSYNC.GetValue(); }
	bool IsEnabled() override { return PREFSMAN->m_bVsync.Get(); }

	void DoAndLog(std::string& sMessageOut) override
	{
		PREFSMAN->m_bVsync.Set(!PREFSMAN->m_bVsync);
		StepMania::ApplyGraphicOptions();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineAllowMultitexture : public IDebugLine
{
	std::string GetDisplayTitle() override { return MULTITEXTURE.GetValue(); }
	bool IsEnabled() override { return PREFSMAN->m_bAllowMultitexture.Get(); }

	void DoAndLog(std::string& sMessageOut) override
	{
		PREFSMAN->m_bAllowMultitexture.Set(!PREFSMAN->m_bAllowMultitexture);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineShowMasks : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return SCREEN_SHOW_MASKS.GetValue();
	}

	bool IsEnabled() override { return GetPref()->Get(); }
	std::string GetPageName() const override { return "Theme"; }

	void DoAndLog(std::string& sMessageOut) override
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

class DebugLineProfileSlot : public IDebugLine
{
	std::string GetDisplayTitle() override { return PROFILE.GetValue(); }

	std::string GetDisplayValue() override
	{
		switch (g_ProfileSlot) {
			case ProfileSlot_Player1:
				return "Player 1";
			case ProfileSlot_Player2:
				return "Player 2";
			default:
				return std::string();
		}
	}

	bool IsEnabled() override { return true; }
	std::string GetPageName() const override { return "Profiles"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		enum_add(g_ProfileSlot, +1);
		if (g_ProfileSlot == NUM_ProfileSlot)
			g_ProfileSlot = ProfileSlot_Player1;

		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineClearProfileStats : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return CLEAR_PROFILE_STATS.GetValue();
	}

	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }
	std::string GetPageName() const override { return "Profiles"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		Profile* pProfile = PROFILEMAN->GetProfile(g_ProfileSlot);
		pProfile->ClearStats();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineSendNotesEnded : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return SEND_NOTES_ENDED.GetValue();
	}

	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override
	{
		SCREENMAN->PostMessageToTopScreen(SM_NotesEnded, 0);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineResetKeyMapping : public IDebugLine
{
	std::string GetDisplayTitle() override { return RESET_KEY_MAP.GetValue(); }
	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override
	{
		INPUTMAPPER->ResetMappingsToDefault();
		INPUTMAPPER->SaveMappingsToDisk();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineMuteActions : public IDebugLine
{
	std::string GetDisplayTitle() override { return MUTE_ACTIONS.GetValue(); }
	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return PREFSMAN->m_MuteActions; }

	void DoAndLog(std::string& sMessageOut) override
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
	std::string GetDisplayTitle() override { return RELOAD.GetValue(); }

	std::string GetDisplayValue() override
	{
		return SCREENMAN && SCREENMAN->GetTopScreen()
				 ? SCREENMAN->GetTopScreen()->GetName()
				 : std::string();
	}

	bool IsEnabled() override { return true; }
	std::string GetPageName() const override { return "Theme"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		std::string sScreenName = SCREENMAN->GetScreen(0)->GetName();
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
	std::string GetDisplayTitle() override { return RESTART.GetValue(); }

	std::string GetDisplayValue() override
	{
		return SCREENMAN && SCREENMAN->GetTopScreen()
				 ? SCREENMAN->GetTopScreen()->GetName()
				 : std::string();
	}

	bool IsEnabled() override { return true; }
	bool ForceOffAfterUse() const override { return true; }
	std::string GetPageName() const override { return "Theme"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		SCREENMAN->GetTopScreen()->BeginScreen();
		IDebugLine::DoAndLog(sMessageOut);
		sMessageOut = "";
	}
};

class DebugLineCurrentScreenOn : public IDebugLine
{
	std::string GetDisplayTitle() override { return SCREEN_ON.GetValue(); }

	std::string GetDisplayValue() override
	{
		return SCREENMAN && SCREENMAN->GetTopScreen()
				 ? SCREENMAN->GetTopScreen()->GetName()
				 : std::string();
	}

	bool IsEnabled() override { return true; }
	bool ForceOffAfterUse() const override { return true; }
	std::string GetPageName() const override { return "Theme"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		SCREENMAN->GetTopScreen()->PlayCommand("On");
		IDebugLine::DoAndLog(sMessageOut);
		sMessageOut = "";
	}
};

class DebugLineCurrentScreenOff : public IDebugLine
{
	std::string GetDisplayTitle() override { return SCREEN_OFF.GetValue(); }

	std::string GetDisplayValue() override
	{
		return SCREENMAN && SCREENMAN->GetTopScreen()
				 ? SCREENMAN->GetTopScreen()->GetName()
				 : std::string();
	}

	bool IsEnabled() override { return true; }
	bool ForceOffAfterUse() const override { return true; }
	std::string GetPageName() const override { return "Theme"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		SCREENMAN->GetTopScreen()->PlayCommand("Off");
		IDebugLine::DoAndLog(sMessageOut);
		sMessageOut = "";
	}
};

class DebugLineReloadTheme : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return RELOAD_THEME_AND_TEXTURES.GetValue();
	}

	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }
	std::string GetPageName() const override { return "Theme"; }

	void DoAndLog(std::string& sMessageOut) override
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
	std::string GetDisplayTitle() override
	{
		return RELOAD_OVERLAY_SCREENS.GetValue();
	}

	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }
	std::string GetPageName() const override { return "Theme"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		SCREENMAN->ReloadOverlayScreensAfterInputFinishes();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineToggleErrors : public IDebugLine
{
	std::string GetDisplayTitle() override { return TOGGLE_ERRORS.GetValue(); }
	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return PREFSMAN->m_show_theme_errors; }
	std::string GetPageName() const override { return "Theme"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		PREFSMAN->m_show_theme_errors.Set(!PREFSMAN->m_show_theme_errors);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineShowRecentErrors : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return SHOW_RECENT_ERRORS.GetValue();
	}

	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }
	std::string GetPageName() const override { return "Theme"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		Message msg("ToggleScriptError");
		MESSAGEMAN->Broadcast(msg);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineClearErrors : public IDebugLine
{
	std::string GetDisplayTitle() override { return CLEAR_ERRORS.GetValue(); }
	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }
	std::string GetPageName() const override { return "Theme"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		Message msg("ClearScriptError");
		MESSAGEMAN->Broadcast(msg);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineConvertXML : public IDebugLine
{
	std::string GetDisplayTitle() override { return CONVERT_XML.GetValue(); }
	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }
	std::string GetPageName() const override { return "Theme"; }

	void DoAndLog(std::string& sMessageOut) override
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
	std::string GetDisplayTitle() override { return WRITE_PROFILES.GetValue(); }
	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }
	std::string GetPageName() const override { return "Profiles"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		auto pn = static_cast<PlayerNumber>(g_ProfileSlot);
		GAMESTATE->SaveCurrentSettingsToProfile(pn);
		GAMESTATE->SavePlayerProfile();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineWritePreferences : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return WRITE_PREFERENCES.GetValue();
	}

	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override
	{
		PREFSMAN->SavePrefsToDisk();
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineMenuTimer : public IDebugLine
{
	std::string GetDisplayTitle() override { return MENU_TIMER.GetValue(); }
	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return PREFSMAN->m_bMenuTimer.Get(); }

	void DoAndLog(std::string& sMessageOut) override
	{
		PREFSMAN->m_bMenuTimer.Set(!PREFSMAN->m_bMenuTimer);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineFlushLog : public IDebugLine
{
	std::string GetDisplayTitle() override { return FLUSH_LOG.GetValue(); }
	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override
	{
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLinePullBackCamera : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return PULL_BACK_CAMERA.GetValue();
	}

	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return g_fImageScaleDestination != 1; }

	void DoAndLog(std::string& sMessageOut) override
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
	std::string GetDisplayTitle() override { return VOLUME_UP.GetValue(); }

	std::string GetDisplayValue() override
	{
		return ssprintf("%.0f%%", GetPref()->Get() * 100);
	}

	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override
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
	std::string GetDisplayTitle() override { return VOLUME_DOWN.GetValue(); }
	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override
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
	std::string GetDisplayTitle() override
	{
		return VISUAL_DELAY_UP.GetValue();
	}

	std::string GetDisplayValue() override
	{
		return ssprintf("%.03f", GetPref()->Get());
	}

	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override
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
	std::string GetDisplayTitle() override
	{
		return VISUAL_DELAY_DOWN.GetValue();
	}

	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override
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

class DebugLineUptime : public IDebugLine
{
	std::string GetDisplayTitle() override { return UPTIME.GetValue(); }

	std::string GetDisplayValue() override
	{
		return SecondsToMMSSMsMsMs(RageTimer::GetTimeSinceStart());
	}

	bool IsEnabled() override { return false; }

	void DoAndLog(std::string& sMessageOut) override {}
};

class DebugLineEasterEggs : public IDebugLine
{
	std::string GetDisplayTitle() override { return EASTER_EGGS.GetValue(); }
	std::string GetPageName() const override { return "Misc"; }
	bool IsEnabled() override { return PREFSMAN->m_bEasterEggs; }

	void DoAndLog(std::string& sMessageOut) override
	{
		PREFSMAN->m_bEasterEggs.Set(!PREFSMAN->m_bEasterEggs);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineOsuLifts : public IDebugLine
{
	std::string GetDisplayTitle() override { return OSU_LIFTS.GetValue(); }
	std::string GetPageName() const override { return "Misc"; }
	bool IsEnabled() override { return PREFSMAN->LiftsOnOsuHolds; }

	void DoAndLog(std::string& sMessageOut) override
	{
		PREFSMAN->LiftsOnOsuHolds.Set(!PREFSMAN->LiftsOnOsuHolds);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLinePitchRates : public IDebugLine
{
	std::string GetDisplayTitle() override { return PITCH_RATES.GetValue(); }
	std::string GetPageName() const override { return "Misc"; }
	bool IsEnabled() override { return PREFSMAN->EnablePitchRates; }

	void DoAndLog(std::string& sMessageOut) override
	{
		PREFSMAN->EnablePitchRates.Set(!PREFSMAN->EnablePitchRates);
		IDebugLine::DoAndLog(sMessageOut);
	}
};

class DebugLineFullscreen : public IDebugLine
{
	std::string GetDisplayTitle() override { return FULLSCREEN.GetValue(); }
	std::string GetDisplayValue() override { return std::string(); }
	std::string GetPageName() const override { return "Misc"; }
	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override
	{
#if !defined(__APPLE__)
		GameLoop::setToggleWindowed();
		IDebugLine::DoAndLog(sMessageOut);
#endif
	}
};

class DebugLineGlobalOffsetUp : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return GLOBAL_OFFSET_UP.GetValue();
	}

	std::string GetDisplayValue() override
	{
		return ssprintf("%.03f", GetPref()->Get());
	}

	std::string GetPageName() const override { return "Misc"; }
	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override
	{
		ChangeGlobalOffset(+0.001f);
		IDebugLine::DoAndLog(sMessageOut);
	}

	Preference<float>* GetPref()
	{
		return Preference<float>::GetPreferenceByName("GlobalOffsetSeconds");
	}
};

class DebugLineGlobalOffsetDown : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return GLOBAL_OFFSET_DOWN.GetValue();
	}

	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }
	std::string GetPageName() const override { return "Misc"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		ChangeGlobalOffset(-0.001f);
		IDebugLine::DoAndLog(sMessageOut);
		sMessageOut += " - " + ssprintf("%.03f", GetPref()->Get());
	}

	Preference<float>* GetPref()
	{
		return Preference<float>::GetPreferenceByName("GlobalOffsetSeconds");
	}
};

class DebugLineGlobalOffsetReset : public IDebugLine
{
	std::string GetDisplayTitle() override
	{
		return GLOBAL_OFFSET_RESET.GetValue();
	}

	std::string GetDisplayValue() override { return std::string(); }
	bool IsEnabled() override { return true; }
	std::string GetPageName() const override { return "Misc"; }

	void DoAndLog(std::string& sMessageOut) override
	{
		ResetGlobalOffset();
		IDebugLine::DoAndLog(sMessageOut);
		sMessageOut += " - " + ssprintf("%.03f", GetPref()->Get());
	}

	Preference<float>* GetPref()
	{
		return Preference<float>::GetPreferenceByName("GlobalOffsetSeconds");
	}
};

class DebugLineKeyConfig : public IDebugLine
{
	std::string GetDisplayTitle() override { return KEY_CONFIG.GetValue(); }
	std::string GetDisplayValue() override { return std::string(); }
	std::string GetPageName() const override { return "Misc"; }
	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override
	{
		SCREENMAN->PopAllScreens();
		SCREENMAN->set_input_redirected(PLAYER_1, false);
		GAMESTATE->Reset();
		SCREENMAN->SetNewScreen("ScreenMapControllers");
	}
};

class DebugLineChartFolder: public IDebugLine
{
	std::string GetDisplayTitle() override { return CHART_FOLDER.GetValue(); }
	std::string GetDisplayValue() override { return std::string();	}
	std::string GetPageName() const override { return "Misc"; }
	bool IsEnabled() override { return GAMESTATE->m_pCurSong != nullptr; }

	void DoAndLog(std::string& sMessageOut) override
	{
		Song* s = GAMESTATE->m_pCurSong;
		if (s != nullptr) {
			auto d = s->GetSongDir();
			auto b = SONGMAN->WasLoadedFromAdditionalSongs(s);
			auto p = FILEMAN->ResolveSongFolder(d, b);

			Core::Platform::openFolder(p);
			IDebugLine::DoAndLog(sMessageOut);
			sMessageOut += " - Opened " + s->m_sSongFileName;
		}
	}
};

class DebugLineChartkey : public IDebugLine
{
	std::string GetDisplayTitle() override { return CHART_KEY.GetValue(); }
	std::string GetDisplayValue() override
	{
		auto c = GAMESTATE->m_pCurSteps;
		if (c != nullptr)
			return c->GetChartKey();
		return std::string("None");
	}
	std::string GetPageName() const override { return "Misc"; }
	bool IsEnabled() override { return true; }

	void DoAndLog(std::string& sMessageOut) override {}
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
DECLARE_ONE(DebugLineUptime);
DECLARE_ONE(DebugLineResetKeyMapping);
DECLARE_ONE(DebugLineMuteActions);
DECLARE_ONE(DebugLineSkips);
DECLARE_ONE(DebugLineEasterEggs);
DECLARE_ONE(DebugLineOsuLifts);
DECLARE_ONE(DebugLinePitchRates);
DECLARE_ONE(DebugLineFullscreen);
DECLARE_ONE(DebugLineGlobalOffsetDown);
DECLARE_ONE(DebugLineGlobalOffsetUp);
DECLARE_ONE(DebugLineGlobalOffsetReset);
DECLARE_ONE(DebugLineKeyConfig);
DECLARE_ONE(DebugLineChartFolder);
DECLARE_ONE(DebugLineChartkey);
