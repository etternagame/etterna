#ifndef SCREEN_H
#define SCREEN_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Misc/CodeSet.h"
#include "Etterna/Models/Misc/EnumHelper.h"
#include "ScreenMessage.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

#include <functional>
#include <list>

class InputEventPlus;
class Screen;
using CreateScreenFn = Screen* (*)(const std::string&);

/**
 * @brief Allow registering the screen for easier access.
 *
 * Each Screen class should have a REGISTER_SCREEN_CLASS in its CPP file.
 */
struct RegisterScreenClass
{
	RegisterScreenClass(const std::string& sClassName, CreateScreenFn pfn);
};

#define REGISTER_SCREEN_CLASS(className)                                       \
	static Screen* Create##className(const std::string& sName)                 \
	{                                                                          \
		LuaThreadVariable var("LoadingScreen", sName);                         \
		Screen* pRet = new className;                                          \
		pRet->SetName(sName);                                                  \
		Screen::InitScreen(pRet);                                              \
		return pRet;                                                           \
	}                                                                          \
	static RegisterScreenClass register_##className(#className,                \
													Create##className)

/** @brief The different types of screens available. */
enum ScreenType
{
	attract,
	/**< The attract/demo mode, inviting players to play. */
	game_menu,
	/**< The menu screens, where options can be set before playing.
	 */
	gameplay,
	/**< The gameplay screen, where the actual game takes place. */
	evaluation,
	system_menu,
	/**< The system/operator menu, where special options are set.
	 */
	NUM_ScreenType,
	/**< The number of screen types. */
	ScreenType_Invalid
};

const std::string&
ScreenTypeToString(ScreenType st);
LuaDeclareType(ScreenType);

/** @brief Class that holds a screen-full of Actors. */
class Screen : public ActorFrame
{
  public:
	static void InitScreen(Screen* pScreen);

	~Screen() override;

	/**
	 * @brief This is called immediately after construction,
	 * to allow initializing after all derived classes exist.
	 *
	 * Don't call it directly; use InitScreen instead. */
	virtual void Init();

	/** @brief This is called immediately before the screen is used. */
	virtual void BeginScreen();

	/** @brief This is called when the screen is popped. */
	virtual void EndScreen();

	void Update(float fDeltaTime) override;
	virtual void UpdateTimedFunctions(float fDeltaTime);
	virtual bool Input(const InputEventPlus& input);
	virtual void HandleScreenMessage(const ScreenMessage& SM);
	void SetLockInputSecs(const float f) { m_fLockInputSecs = f; }

	/**
	 * @brief Put the specified message onto the screen for a specified time.
	 * @param SM the message to put on the screen.
	 * @param fDelay The length of time it stays up. */
	void PostScreenMessage(const ScreenMessage& SM, float fDelay);
	/** @brief Clear the entire message queue. */
	void ClearMessageQueue();
	/**
	 * @brief Clear the message queue of a specific ScreenMessage.
	 * @param SM the specific ScreenMessage to get out of the Queue. */
	void ClearMessageQueue(const ScreenMessage& SM);

	virtual ScreenType GetScreenType() const
	{
		return ALLOW_OPERATOR_MENU_BUTTON ? game_menu : system_menu;
	}

	bool AllowOperatorMenuButton() const { return ALLOW_OPERATOR_MENU_BUTTON; }

	/**
	 * @brief Determine if we allow extra players to join in on this screen.
	 * @return false, for players should never be able to join while in
	 * progress. */
	virtual bool AllowLateJoin() const { return false; }

	// Lua
	void PushSelf(lua_State* L) override;

	std::vector<std::pair<std::function<void()>, float>> delayedFunctions;
	void SetTimeout(const std::function<void()>& f, float ms);
	std::list<std::tuple<std::function<void()>, float, float, int>>
	  delayedPeriodicFunctions; // This is a list to allow safe iterators
	std::vector<int> delayedPeriodicFunctionIdsToDelete;
	void SetInterval(const std::function<void()>& f, float ms, int fRemove);

	bool b_PreviewNoteFieldIsActive = false;

  protected:
	/** @brief Holds the messages sent to a Screen. */
	struct QueuedScreenMessage
	{
		/** @brief The message being held. */
		ScreenMessage SM;
		/** @brief How long the message is up. */
		float fDelayRemaining;
	};

	/** @brief The list of messages that are sent to a Screen. */
	std::vector<QueuedScreenMessage> m_QueuedMessages;
	static bool SortMessagesByDelayRemaining(const QueuedScreenMessage& m1,
											 const QueuedScreenMessage& m2);

	InputQueueCodeSet m_Codes;

	/** @brief Do we allow the operator menu button to be pressed here? */
	ThemeMetric<bool> ALLOW_OPERATOR_MENU_BUTTON;
	/** @brief Do we handle the back button being pressed here? */
	ThemeMetric<bool> HANDLE_BACK_BUTTON;
	ThemeMetric<float> REPEAT_RATE;
	ThemeMetric<float> REPEAT_DELAY;

	/**
	 * @brief The next screen to go to once this screen is done.
	 *
	 * If this is blank, the NextScreen metric will be used. */
	std::string m_sNextScreen;
	std::string m_sPrevScreen;
	ScreenMessage m_smSendOnPop;

	float m_fLockInputSecs = 0.F;

	// If currently between BeginScreen/EndScreen calls:
	bool m_bRunning = false;

  public:
	std::string GetNextScreenName() const;
	std::string GetPrevScreen() const;
	void SetNextScreenName(std::string const& name);
	void SetPrevScreenName(std::string const& name);

	bool PassInputToLua(const InputEventPlus& input);
	void AddInputCallbackFromStack(lua_State* L);
	void RemoveInputCallback(lua_State* L);
	virtual bool AllowCallbackInput() { return true; }

	// let subclass override if they want
	virtual bool MenuUp(const InputEventPlus&) { return false; }
	virtual bool MenuDown(const InputEventPlus&) { return false; }
	virtual bool MenuLeft(const InputEventPlus&) { return false; }
	virtual bool MenuRight(const InputEventPlus&) { return false; }
	virtual bool MenuStart(const InputEventPlus&) { return false; }
	virtual bool MenuSelect(const InputEventPlus&) { return false; }
	virtual bool MenuBack(const InputEventPlus&) { return false; }
	virtual bool MenuCoin(const InputEventPlus&) { return false; }

  private:
	// void* is the key so that we can use lua_topointer to find the callback
	// to remove when removing a callback.
	using callback_key_t = const void*;
	std::map<callback_key_t, LuaReference> m_InputCallbacks;
	std::vector<callback_key_t> orderedcallbacks;
	std::vector<callback_key_t> m_DelayedCallbackRemovals;
	bool m_CallingInputCallbacks = false;
	void InternalRemoveCallback(callback_key_t key);
};

#endif
