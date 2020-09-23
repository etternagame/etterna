#include "Etterna/Globals/global.h"
#include "ActorProxy.h"
#include "Etterna/Actor/Base/ActorUtil.h"

REGISTER_ACTOR_CLASS(ActorProxy);

ActorProxy::ActorProxy()
{
	m_pActorTarget = nullptr;
}

bool
ActorProxy::EarlyAbortDraw() const
{
	return m_pActorTarget == nullptr || Actor::EarlyAbortDraw();
}

void
ActorProxy::DrawPrimitives()
{
	if (m_pActorTarget != nullptr) {
		bool bVisible = m_pActorTarget->GetVisible();
		m_pActorTarget->SetVisible(true);
		m_pActorTarget->Draw();
		m_pActorTarget->SetVisible(bVisible);
	}
}

void
ActorProxy::LoadFromNode(const XNode* pNode)
{
	Actor::LoadFromNode(pNode);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ActorProxy. */
class LunaActorProxy : public Luna<ActorProxy>
{
  public:
	static int SetTarget(T* p, lua_State* L)
	{
		Actor* pTarget = Luna<Actor>::check(L, 1);
		p->SetTarget(pTarget);
		COMMON_RETURN_SELF;
	}

	static int GetTarget(T* p, lua_State* L)
	{
		Actor* pTarget = p->GetTarget();
		if (pTarget != nullptr)
			pTarget->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	LunaActorProxy()
	{
		ADD_METHOD(SetTarget);
		ADD_METHOD(GetTarget);
	}
};

LUA_REGISTER_DERIVED_CLASS(ActorProxy, Actor)
// lua end
