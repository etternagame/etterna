#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/EnumHelper.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "LuaManager.h"
#include "MessageManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Misc/RageThreads.h"
#include "RageUtil/Utils/RageUtil.h"

#include <map>
#include <set>

MessageManager* MESSAGEMAN =
  nullptr; // global and accessible from anywhere in our program

static const char* MessageIDNames[] = {
	"CurrentGameChanged",
	"CurrentStyleChanged",
	"PlayModeChanged",
	"CoinsChanged",
	"CurrentSongChanged",
	"CurrentStepsChanged",
	"GameplayLeadInChanged",
	"GameplayModeChanged",
	"EditStepsTypeChanged",
	"EditCourseDifficultyChanged",
	"EditSourceStepsChanged",
	"EditSourceStepsTypeChanged",
	"PreferredStepsTypeChanged",
	"PreferredDifficultyP1Changed",
	"PreferredDifficultyP2Changed",
	"PreferredCourseDifficultyP1Changed",
	"PreferredCourseDifficultyP2Changed",
	"EditCourseEntryIndexChanged",
	"EditLocalProfileIDChanged",
	"NoteCrossed",
	"NoteWillCrossIn400Ms",
	"NoteWillCrossIn800Ms",
	"NoteWillCrossIn1200Ms",
	"CardRemovedP1",
	"CardRemovedP2",
	"BeatCrossed",
	"MenuUpP1",
	"MenuUpP2",
	"MenuDownP1",
	"MenuDownP2",
	"MenuLeftP1",
	"MenuLeftP2",
	"MenuRightP1",
	"MenuRightP2",
	"MenuStartP1",
	"MenuStartP2",
	"MenuSelectionChanged",
	"PlayerJoined",
	"PlayerUnjoined",
	"AutosyncChanged",
	"PreferredSongGroupChanged",
	"PreferredCourseGroupChanged",
	"SortOrderChanged",
	"LessonTry1",
	"LessonTry2",
	"LessonTry3",
	"LessonCleared",
	"LessonFailed",
	"StorageDevicesChanged",
	"AutoJoyMappingApplied",
	"ScreenChanged",
	"SongModified",
	"ScoreMultiplierChangedP1",
	"ScoreMultiplierChangedP2",
	"StarPowerChangedP1",
	"StarPowerChangedP2",
	"CurrentComboChangedP1",
	"CurrentComboChangedP2",
	"StarMeterChangedP1",
	"StarMeterChangedP2",
	"LifeMeterChangedP1",
	"LifeMeterChangedP2",
	"UpdateScreenHeader",
	"LeftClick",
	"RightClick",
	"MiddleClick",
	"MouseWheelUp",
	"MouseWheelDown",
};
XToString(MessageID);

static RageMutex g_Mutex("MessageManager");

typedef std::set<IMessageSubscriber*> SubscribersSet;
static std::map<std::string, SubscribersSet> g_MessageToSubscribers;

Message::Message(const std::string& s)
{
	m_sName = s;
	m_pParams = new LuaTable;
	m_bBroadcast = false;
}

Message::Message(const MessageID id)
{
	m_sName = MessageIDToString(id);
	m_pParams = new LuaTable;
	m_bBroadcast = false;
}

Message::Message(const std::string& s, const LuaReference& params)
{
	m_sName = s;
	m_bBroadcast = false;
	auto* L = LUA->Get();
	m_pParams = new LuaTable; // XXX: creates an extra table
	params.PushSelf(L);
	m_pParams->SetFromStack(L);
	LUA->Release(L);
	//	m_pParams = new LuaTable( params );
}

Message::~Message()
{
	delete m_pParams;
}

void
Message::PushParamTable(lua_State* L)
{
	m_pParams->PushSelf(L);
}

void
Message::SetParamTable(const LuaReference& params)
{
	auto* L = LUA->Get();
	params.PushSelf(L);
	m_pParams->SetFromStack(L);
	LUA->Release(L);
}

const LuaReference&
Message::GetParamTable() const
{
	return *m_pParams;
}

void
Message::GetParamFromStack(lua_State* L, const std::string& sName) const
{
	m_pParams->Get(L, sName);
}

void
Message::SetParamFromStack(lua_State* L, const std::string& sName)
{
	m_pParams->Set(L, sName);
}

MessageManager::MessageManager()
{
	m_Logging = false;
	// Register with Lua.
	{
		auto* L = LUA->Get();
		lua_pushstring(L, "MESSAGEMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

MessageManager::~MessageManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("MESSAGEMAN");
}

void
MessageManager::Subscribe(IMessageSubscriber* pSubscriber,
						  const std::string& sMessage)
{
	LockMut(g_Mutex);

	auto& subs = g_MessageToSubscribers[sMessage];
#ifdef DEBUG
	SubscribersSet::iterator iter = subs.find(pSubscriber);
	ASSERT_M(iter == subs.end(),
			 ssprintf("already subscribed to '%s'", sMessage.c_str()));
#endif
	subs.insert(pSubscriber);
}

void
MessageManager::Subscribe(IMessageSubscriber* pSubscriber, MessageID m)
{
	Subscribe(pSubscriber, MessageIDToString(m));
}

void
MessageManager::Unsubscribe(IMessageSubscriber* pSubscriber,
							const std::string& sMessage)
{
	LockMut(g_Mutex);

	auto& subs = g_MessageToSubscribers[sMessage];
	auto iter = subs.find(pSubscriber);
	ASSERT(iter != subs.end());
	subs.erase(iter);
}

void
MessageManager::Unsubscribe(IMessageSubscriber* pSubscriber, MessageID m)
{
	Unsubscribe(pSubscriber, MessageIDToString(m));
}

void
MessageManager::Broadcast(Message& msg) const
{
	// GAMESTATE is created before MESSAGEMAN, and has several
	// BroadcastOnChangePtr members, so they all broadcast when they're
	// initialized.
	if (this != nullptr && m_Logging) {
		Locator::getLogger()->trace("MESSAGEMAN:Broadcast: {}", msg.GetName().c_str());
	}
	msg.SetBroadcast(true);

	LockMut(g_Mutex);

	std::map<std::string, SubscribersSet>::const_iterator iter =
	  g_MessageToSubscribers.find(msg.GetName());
	if (iter == g_MessageToSubscribers.end())
		return;

	for (const auto& p : iter->second) {
		auto* pSub = p;
		pSub->HandleMessage(msg);
	}
}

void
MessageManager::Broadcast(const std::string& sMessage) const
{
	ASSERT(!sMessage.empty());
	Message msg(sMessage);
	Broadcast(msg);
}

void
MessageManager::Broadcast(MessageID m) const
{
	Broadcast(MessageIDToString(m));
}

bool
MessageManager::IsSubscribedToMessage(IMessageSubscriber* pSubscriber,
									  const std::string& sMessage) const
{
	auto& subs = g_MessageToSubscribers[sMessage];
	return subs.find(pSubscriber) != subs.end();
}

void
IMessageSubscriber::ClearMessages(const std::string& sMessage)
{
}

MessageSubscriber::MessageSubscriber(const MessageSubscriber& cpy)
  : IMessageSubscriber(cpy)
{
	FOREACH_CONST(std::string, cpy.m_vsSubscribedTo, msg)
	this->SubscribeToMessage(*msg);
}

MessageSubscriber&
MessageSubscriber::operator=(const MessageSubscriber& cpy)
{
	if (&cpy == this)
		return *this;

	UnsubscribeAll();

	FOREACH_CONST(std::string, cpy.m_vsSubscribedTo, msg)
	this->SubscribeToMessage(*msg);

	return *this;
}

void
MessageSubscriber::SubscribeToMessage(const std::string& sMessageName)
{
	MESSAGEMAN->Subscribe(this, sMessageName);
	m_vsSubscribedTo.push_back(sMessageName);
}

void
MessageSubscriber::SubscribeToMessage(MessageID message)
{
	MESSAGEMAN->Subscribe(this, message);
	m_vsSubscribedTo.push_back(MessageIDToString(message));
}

void
MessageSubscriber::UnsubscribeAll()
{
	FOREACH_CONST(std::string, m_vsSubscribedTo, s)
	MESSAGEMAN->Unsubscribe(this, *s);
	m_vsSubscribedTo.clear();
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the MessageManager. */
class LunaMessageManager : public Luna<MessageManager>
{
  public:
	static int Broadcast(T* p, lua_State* L)
	{
		if (!lua_istable(L, 2) && !lua_isnoneornil(L, 2))
			luaL_typerror(L, 2, "table or nil");

		LuaReference ParamTable;
		lua_pushvalue(L, 2);
		ParamTable.SetFromStack(L);

		Message msg(SArg(1), ParamTable);
		p->Broadcast(msg);
		COMMON_RETURN_SELF;
	}
	static int SetLogging(T* p, lua_State* L)
	{
		p->SetLogging(lua_toboolean(L, -1) != 0);
		COMMON_RETURN_SELF;
	}

	LunaMessageManager()
	{
		ADD_METHOD(Broadcast);
		ADD_METHOD(SetLogging);
	}
};

LUA_REGISTER_CLASS(MessageManager)
// lua end
