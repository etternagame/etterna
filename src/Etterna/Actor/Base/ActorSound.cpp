#include "Etterna/Globals/global.h"
#include "ActorSound.h"
#include "ActorUtil.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/FileTypes/XmlFile.h"

REGISTER_ACTOR_CLASS_WITH_NAME(ActorSound, Sound);

void
ActorSound::Load(const std::string& sPath)
{
	m_Sound.Load(sPath, true);
}

void
ActorSound::Update(float dt)
{
	Actor::Update(dt);
	if (m_Sound.pendingPlayBackCall) {
		auto* L = LUA->Get();
		m_Sound.ExecutePlayBackCallback(L);
		LUA->Release(L);
	}
}

void
ActorSound::Play()
{
	m_Sound.Play(m_is_action);
}

void
ActorSound::Pause(bool bPause)
{
	m_Sound.Pause(bPause);
}

void
ActorSound::Stop()
{
	m_Sound.Stop();
}

void
ActorSound::LoadFromNode(const XNode* pNode)
{
	RageSoundLoadParams params;
	pNode->GetAttrValue("SupportPan", params.m_bSupportPan);
	pNode->GetAttrValue("SupportRateChanging", params.m_bSupportRateChanging);
	pNode->GetAttrValue("IsAction", m_is_action);

	auto bPrecache = true;
	pNode->GetAttrValue("Precache", bPrecache);

	Actor::LoadFromNode(pNode);

	std::string sFile;
	if (ActorUtil::GetAttrPath(pNode, "File", sFile))
		m_Sound.Load(sFile, bPrecache, &params);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ActorSound. */
class LunaActorSound : public Luna<ActorSound>
{
  public:
	static int load(T* p, lua_State* L)
	{
		p->Load(SArg(1));
		COMMON_RETURN_SELF;
	}
	static int play(T* p, lua_State* L)
	{
		p->Play();
		COMMON_RETURN_SELF;
	}
	static int pause(T* p, lua_State* L)
	{
		p->Pause(BArg(1));
		COMMON_RETURN_SELF;
	}
	static int stop(T* p, lua_State* L)
	{
		p->Stop();
		COMMON_RETURN_SELF;
	}
	static int get(T* p, lua_State* L)
	{
		p->PushSound(L);
		return 1;
	}
	static int set_is_action(T* p, lua_State* L)
	{
		p->m_is_action = BArg(1);
		COMMON_RETURN_SELF;
	}
	DEFINE_METHOD(get_is_action, m_is_action);

	LunaActorSound()
	{
		ADD_METHOD(load);
		ADD_METHOD(play);
		ADD_METHOD(pause);
		ADD_METHOD(stop);
		ADD_METHOD(get);
		ADD_GET_SET_METHODS(is_action);
	}
};

LUA_REGISTER_DERIVED_CLASS(ActorSound, Actor)
// lua end
