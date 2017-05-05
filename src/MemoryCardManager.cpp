#include "global.h"
#include "MemoryCardManager.h"
#include "LuaManager.h"

MemoryCardManager*	MEMCARDMAN = NULL;	// global and accessible from anywhere in our program

MemoryCardManager::MemoryCardManager()
{

	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring( L, "MEMCARDMAN" );
		this->PushSelf( L );
		lua_settable( L, LUA_GLOBALSINDEX );
		LUA->Release( L );
	}
}

MemoryCardManager::~MemoryCardManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal( "MEMCARDMAN" );
}


// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the MemoryCardManager. */ 
class LunaMemoryCardManager: public Luna<MemoryCardManager>
{
public:
	static int GetCardState( T* p, lua_State *L )
	{
		lua_pushstring(L, "MemoryCardState_none");
		return 1;
	}
	static int GetName( T* p, lua_State *L )
	{
		lua_pushnil(L);
		return 1;
	}

	LunaMemoryCardManager()
	{
		ADD_METHOD( GetCardState );
		ADD_METHOD( GetName );
	}
};

LUA_REGISTER_CLASS( MemoryCardManager )
// lua end


/*
 * (c) 2003-2005 Chris Danford, Glenn Maynard
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
