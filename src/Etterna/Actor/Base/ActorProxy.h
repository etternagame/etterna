#ifndef ACTOR_PROXY_H
#define ACTOR_PROXY_H

#include "Actor.h"

struct lua_State;
/** @brief Renders another actor. */
class ActorProxy : public Actor
{
  public:
	ActorProxy();

	virtual bool EarlyAbortDraw() const;
	virtual void DrawPrimitives();

	void LoadFromNode(const XNode* pNode);
	virtual ActorProxy* Copy() const;

	Actor* GetTarget() { return m_pActorTarget; }
	void SetTarget(Actor* pTarget) { m_pActorTarget = pTarget; }

	// Lua
	virtual void PushSelf(lua_State* L);

  private:
	Actor* m_pActorTarget;
};

#endif
