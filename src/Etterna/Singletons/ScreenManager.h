#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "Etterna/Models/Misc/PlayerNumber.h"
#include "RageUtil/Sound/RageSound.h"
#include "Etterna/Screen/Others/ScreenMessage.h"

class Actor;
class Screen;
struct Menu;
struct lua_State;
class InputEventPlus;
/** @brief Manager/container for Screens. */
class ScreenManager
{
  public:
	ScreenManager();
	~ScreenManager();

	// pass these messages along to the current state
	void Update(float fDeltaTime);
	void Draw();
	void Input(const InputEventPlus& input);

	// Main screen stack management
	void SetNewScreen(const RString& sName);
	void AddNewScreenToTop(const RString& sName,
						   ScreenMessage SendOnPop = SM_None);
	/**
	 * @brief Create and cache the requested Screen.
	 *
	 * This is so that the next call to SetNewScreen for this Screen
	 * will be very quick.
	 * @param sScreenName the Screen to prepare. */
	void PrepareScreen(const RString& sScreenName);
	void GroupScreen(const RString& sScreenName);
	void PersistantScreen(const RString& sScreenName);
	void PopTopScreen(ScreenMessage SM);
	void PopAllScreens();
	Screen* GetTopScreen();
	Screen* GetScreen(int iPosition);
	bool AllowOperatorMenuButton() const;

	bool IsScreenNameValid(RString const& name) const;

	// System messages
	void SystemMessage(const RString& sMessage);
	void SystemMessageNoAnimate(const RString& sMessage);
	void HideSystemMessage();

	// Screen messages
	void PostMessageToTopScreen(ScreenMessage SM, float fDelay);
	void SendMessageToTopScreen(ScreenMessage SM);

	void RefreshCreditsMessages();
	void ThemeChanged();
	void ReloadOverlayScreens();
	void ReloadOverlayScreensAfterInputFinishes();

	/**
	 * @brief Is this Screen in the main Screen stack, but not the bottommost
	 * Screen?
	 *
	 * If this function returns true, the screen should exit by popping
	 * itself, not by loading another Screen.
	 * @param pScreen the Screen to check.
	 * @return true if it's on the stack while not on the bottom, or false
	 * otherwise. */
	bool IsStackedScreen(const Screen* pScreen) const;

	bool get_input_redirected(PlayerNumber pn);
	void set_input_redirected(PlayerNumber pn, bool redir);

	// Lua
	void PushSelf(lua_State* L);

	void PlaySharedBackgroundOffCommand();
	void ZeroNextUpdate();

  private:
	// Screen loads, removals, and concurrent prepares are delayed until the
	// next update.
	RString m_sDelayedScreen;
	RString m_sDelayedConcurrentPrepare;
	ScreenMessage m_OnDonePreparingScreen;
	ScreenMessage m_PopTopScreen;

	// Set this to true anywhere we create of delete objects.  These
	// operations take a long time, and will cause a skip on the next update.
	bool m_bZeroNextUpdate;

	// This exists so the debug overlay can reload the overlay screens without
	// seg faulting. It's "AfterInput" because the debug overlay carries out
	// actions in Input.
	bool m_bReloadOverlayScreensAfterInput;

	// m_input_redirected exists to allow the theme to prevent input being
	// passed to the normal Screen::Input function, on a per-player basis.
	// Input is still passed to lua callbacks, so it's intended for the case
	// where someone has a custom menu on a screen and needs to disable normal
	// input for navigating the custom menu to work. -Kyz
	std::vector<bool> m_input_redirected;

	Screen* MakeNewScreen(const RString& sName);
	void LoadDelayedScreen();
	bool ActivatePreparedScreenAndBackground(const RString& sScreenName);
	ScreenMessage PopTopScreenInternal(bool bSendLoseFocus = true);

	// Keep these sounds always loaded, because they could be
	// played at any time.  We want to eliminate SOUND->PlayOnce
  public:
	void PlayStartSound();
	void PlayCoinSound();
	void PlayCancelSound();
	void PlayInvalidSound();
	void PlayScreenshotSound();

  private:
	RageSound m_soundStart;
	/** @brief The sound played when a coin has been put into the machine. */
	RageSound m_soundCoin;
	RageSound m_soundCancel;
	RageSound m_soundInvalid;
	/** @brief The sound played when a Player wishes to take a picture of their
	 * Score. */
	RageSound m_soundScreenshot;
};

extern ScreenManager*
  SCREENMAN; // global and accessible from anywhere in our program

#endif
