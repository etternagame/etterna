#include "global.h"
#include "ActorProxy.h"
#include "ActorUtil.h"

REGISTER_ACTOR_CLASS( ActorProxy );

ActorProxy::ActorProxy()
{
	m_pActorTarget = nullptr;
}

bool ActorProxy::EarlyAbortDraw() const
{
	return m_pActorTarget == NULL || Actor::EarlyAbortDraw();
}

void ActorProxy::DrawPrimitives()
{
	if( m_pActorTarget != nullptr )
	{
		bool bVisible = m_pActorTarget->GetVisible();
		m_pActorTarget->SetVisible( true );
		m_pActorTarget->Draw();
		m_pActorTarget->SetVisible( bVisible );
	}
}

void ActorProxy::LoadFromNode( const XNode* pNode )
{
	Actor::LoadFromNode( pNode );
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the ActorProxy. */ 
class LunaActorProxy: public Luna<ActorProxy>
{
public:
	static int SetTarget( T* p, lua_State *L )
	{
		Actor *pTarget = Luna<Actor>::check( L, 1 );
		p->SetTarget( pTarget );
		COMMON_RETURN_SELF;
	}

	static int GetTarget( T* p, lua_State *L )
	{
		Actor *pTarget = p->GetTarget();
		if( pTarget != nullptr )
			pTarget->PushSelf( L );
		else
			lua_pushnil( L );
		return 1;
	}

	LunaActorProxy()
	{
		ADD_METHOD( SetTarget );
		ADD_METHOD( GetTarget );
	}
};

LUA_REGISTER_DERIVED_CLASS( ActorProxy, Actor )
// lua end

/*
 * (c) 2006 Glenn Maynard
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
