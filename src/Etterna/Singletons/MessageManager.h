#ifndef MessageManager_H
#define MessageManager_H

#include "LuaManager.h"
struct lua_State;
class LuaTable;
class LuaReference;

/** @brief The various messages available to watch for. */
enum MessageID
{
	Message_CurrentGameChanged,
	Message_CurrentStyleChanged,
	Message_PlayModeChanged,
	Message_CoinsChanged,
	Message_CurrentSongChanged,
	Message_CurrentStepsChanged,
	Message_GameplayLeadInChanged,
	Message_GameplayModeChanged,
	Message_EditStepsTypeChanged,
	Message_EditCourseDifficultyChanged,
	Message_EditSourceStepsChanged,
	Message_EditSourceStepsTypeChanged,
	Message_PreferredStepsTypeChanged,
	Message_PreferredDifficultyP1Changed,
	Message_PreferredDifficultyP2Changed,
	Message_PreferredCourseDifficultyP1Changed,
	Message_PreferredCourseDifficultyP2Changed,
	Message_EditCourseEntryIndexChanged,
	Message_EditLocalProfileIDChanged,
	Message_NoteCrossed,
	Message_NoteWillCrossIn400Ms,
	Message_NoteWillCrossIn800Ms,
	Message_NoteWillCrossIn1200Ms,
	Message_CardRemovedP1,
	Message_CardRemovedP2,
	Message_BeatCrossed,
	Message_MenuUpP1,
	Message_MenuUpP2,
	Message_MenuDownP1,
	Message_MenuDownP2,
	Message_MenuLeftP1,
	Message_MenuLeftP2,
	Message_MenuRightP1,
	Message_MenuRightP2,
	Message_MenuStartP1,
	Message_MenuStartP2,
	Message_MenuSelectionChanged,
	Message_PlayerJoined,
	Message_PlayerUnjoined,
	Message_AutosyncChanged,
	Message_PreferredSongGroupChanged,
	Message_PreferredCourseGroupChanged,
	Message_SortOrderChanged,
	Message_LessonTry1,
	Message_LessonTry2,
	Message_LessonTry3,
	Message_LessonCleared,
	Message_LessonFailed,
	Message_StorageDevicesChanged,
	Message_AutoJoyMappingApplied,
	Message_ScreenChanged,
	Message_SongModified,
	Message_ScoreMultiplierChangedP1,
	Message_ScoreMultiplierChangedP2,
	Message_StarPowerChangedP1,
	Message_StarPowerChangedP2,
	Message_CurrentComboChangedP1,
	Message_CurrentComboChangedP2,
	Message_StarMeterChangedP1,
	Message_StarMeterChangedP2,
	Message_LifeMeterChangedP1,
	Message_LifeMeterChangedP2,
	Message_UpdateScreenHeader,
	Message_LeftClick,
	Message_RightClick,
	Message_MiddleClick,
	Message_MouseWheelUp,
	Message_MouseWheelDown,
	NUM_MessageID, // leave this at the end
	MessageID_Invalid
};
auto
MessageIDToString(MessageID m) -> const std::string&;

struct Message
{
	explicit Message(const std::string& s);
	explicit Message(MessageID id);
	Message(const std::string& s, const LuaReference& params);
	~Message();

	void SetName(const std::string& sName) { m_sName = sName; }
	[[nodiscard]] auto GetName() const -> std::string { return m_sName; }

	[[nodiscard]] auto IsBroadcast() const -> bool { return m_bBroadcast; }
	void SetBroadcast(const bool b) { m_bBroadcast = b; }

	void PushParamTable(lua_State* L);
	[[nodiscard]] auto GetParamTable() const -> const LuaReference&;
	void SetParamTable(const LuaReference& params);

	void GetParamFromStack(lua_State* L, const std::string& sName) const;
	void SetParamFromStack(lua_State* L, const std::string& sName);

	template<typename T>
	auto GetParam(const std::string& sName, T& val) const -> bool
	{
		Lua* L = LUA->Get();
		GetParamFromStack(L, sName);
		bool bRet = LuaHelpers::Pop(L, val);
		LUA->Release(L);
		return bRet;
	}

	template<typename T>
	void SetParam(const std::string& sName, const T& val)
	{
		Lua* L = LUA->Get();
		LuaHelpers::Push(L, val);
		SetParamFromStack(L, sName);
		LUA->Release(L);
	}

	template<typename T>
	void SetParam(const std::string& sName, const std::vector<T>& val)
	{
		Lua* L = LUA->Get();
		LuaHelpers::CreateTableFromArray(val, L);
		SetParamFromStack(L, sName);
		LUA->Release(L);
	}

	auto operator==(const std::string& s) const -> bool { return m_sName == s; }
	auto operator==(const MessageID id) const -> bool
	{
		return MessageIDToString(id) == m_sName;
	}

  private:
	std::string m_sName;
	LuaTable* m_pParams;
	bool m_bBroadcast;

	auto operator=(const Message& rhs) -> Message&; // don't use
	/* Work around a gcc bug where HandleMessage( Message("Init") ) fails
	 * because the copy ctor is private. The copy ctor is not even used so I
	 * have no idea why it being private is an issue. Also, if the Message
	 * object were constructed implicitly (remove explicit above), it works:
	 * HandleMessage( "Init" ). Leaving this undefined but public changes a
	 * compile time error into a link time error. Hmm.*/
  public:
	Message(const Message& rhs); // don't use
};

class IMessageSubscriber
{
  public:
	virtual ~IMessageSubscriber() = default;
	virtual void HandleMessage(const Message& msg) = 0;
	void ClearMessages(const std::string& sMessage = "");

  private:
	friend class MessageManager;
};

class MessageSubscriber : public IMessageSubscriber
{
  public:
	MessageSubscriber() = default;
	MessageSubscriber(const MessageSubscriber& cpy);
	auto operator=(const MessageSubscriber& cpy) -> MessageSubscriber&;

	//
	// Messages
	//
	void SubscribeToMessage(
	  MessageID message); // will automatically unsubscribe
	void SubscribeToMessage(
	  const std::string& sMessageName); // will automatically unsubscribe

	void UnsubscribeAll();

  private:
	std::vector<std::string> m_vsSubscribedTo;
};

/** @brief Deliver messages to any part of the program as needed. */
class MessageManager
{
  public:
	MessageManager();
	~MessageManager();

	static void Subscribe(IMessageSubscriber* pSubscriber,
				   const std::string& sMessage);
	static void Subscribe(IMessageSubscriber* pSubscriber, MessageID m);
	static void Unsubscribe(IMessageSubscriber* pSubscriber,
					 const std::string& sMessage);
	static void Unsubscribe(IMessageSubscriber* pSubscriber, MessageID m);
	void Broadcast(Message& msg) const;
	void Broadcast(const std::string& sMessage) const;
	void Broadcast(MessageID m) const;
	static auto IsSubscribedToMessage(IMessageSubscriber* pSubscriber,
							   const std::string& sMessage) -> bool;

	static auto IsSubscribedToMessage(IMessageSubscriber* pSubscriber,
							   const MessageID message) -> bool
	{
		return IsSubscribedToMessage(pSubscriber, MessageIDToString(message));
	}

	void SetLogging(bool set) { m_Logging = set; }
	bool m_Logging;

	// Lua
	void PushSelf(lua_State* L);
};

extern MessageManager*
  MESSAGEMAN; // global and accessible from anywhere in our program

template<class T>
class BroadcastOnChange
{
	MessageID mSendWhenChanged;
	T val;

  public:
	explicit BroadcastOnChange(const MessageID m)
	{
		val = T();
		mSendWhenChanged = m;
	}
	[[nodiscard]] auto Get() const -> T { return val; }
	void Set(T t)
	{
		val = t;
		MESSAGEMAN->Broadcast(MessageIDToString(mSendWhenChanged));
	}
	operator T() const { return val; }
	auto operator==(const T& other) const -> bool { return val == other; }
	auto operator!=(const T& other) const -> bool { return val != other; }
};

/** @brief Utilities for working with Lua. */
namespace LuaHelpers {
template<class T>
void
Push(lua_State* L, const BroadcastOnChange<T>& Object)
{
	LuaHelpers::Push<T>(L, Object.Get());
}
}

template<class T>
class BroadcastOnChangePtr
{
	MessageID mSendWhenChanged;
	T* val;

  public:
	explicit BroadcastOnChangePtr(const MessageID m)
	{
		mSendWhenChanged = m;
		val = nullptr;
	}
	[[nodiscard]] auto Get() const -> T* { return val; }
	void Set(T* t)
	{
		val = t;
		if (MESSAGEMAN) {
			MESSAGEMAN->Broadcast(MessageIDToString(mSendWhenChanged));
		}
	}

	/* This is only intended to be used for setting temporary values; always
	 * restore the original value when finished, so listeners don't get confused
	 * due to missing a message. */
	void SetWithoutBroadcast(T* t) { val = t; }
	operator T*() const { return val; }
	auto operator->() const -> T* { return val; }
};

template<class T>
class BroadcastOnChangePtrWithSelf
{
	MessageID mSendWhenChanged;
	T* val;

  public:
	explicit BroadcastOnChangePtrWithSelf(const MessageID m)
	{
		mSendWhenChanged = m;
		val = nullptr;
	}
	[[nodiscard]] auto Get() const -> T* { return val; }
	void Set(T* t)
	{
		val = t;
		if (MESSAGEMAN) {
			Message msg(MessageIDToString(mSendWhenChanged));
			msg.SetParam("ptr", t);
			MESSAGEMAN->Broadcast(msg);
		}
	}

	/* This is only intended to be used for setting temporary values; always
	 * restore the original value when finished, so listeners don't get confused
	 * due to missing a message. */
	void SetWithoutBroadcast(T* t) { val = t; }
	operator T*() const { return val; }
	auto operator->() const -> T* { return val; }
};

#endif
