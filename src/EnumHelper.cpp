﻿#include "global.h"
#include "EnumHelper.h"
#include "LuaManager.h"
#include "RageUtil.h"
#include "RageLog.h"

int CheckEnum( lua_State *L, LuaReference &table, int iPos, int iInvalid, const char *szType, bool bAllowInvalid, bool bAllowAnything )
{
	if( lua_isnoneornil(L, iPos) )
	{
		if( bAllowInvalid )
			return iInvalid;

		LuaHelpers::Push( L, ssprintf("Expected %s; got nil", szType) );
		lua_error( L );
	}

	iPos = LuaHelpers::AbsIndex( L, iPos );

	table.PushSelf( L );
	lua_pushvalue( L, iPos );
	lua_gettable( L, -2 );

	// If not found, check case-insensitively for legacy compatibility
	if( lua_isnil(L, -1) && lua_isstring(L, iPos) ) {
		RString sLower;

		// Get rid of nil value on stack
		lua_pop( L, 1 );

		// Get the string and lowercase it
		lua_pushvalue( L, iPos );
		LuaHelpers::Pop( L, sLower );
		sLower.MakeLower();

		// Try again to read the value
		table.PushSelf( L );
		LuaHelpers::Push( L, sLower );
		lua_gettable( L, -2 );
	}

	// If the result is nil, then a string was passed that is not a member of this enum.  Throw
	// an error.  To specify the invalid value, pass nil.  That way, typos will throw an error,
	// and not silently result in nil, or an out-of-bounds value.
	if( unlikely(lua_isnil(L, -1)) )
	{
		RString sGot;
		if( lua_isstring(L, iPos) )
		{
			/* We were given a string, but it wasn't a valid value for this enum.  Show
			 * the string. */
			lua_pushvalue( L, iPos );
			LuaHelpers::Pop( L, sGot );
			sGot = ssprintf( "\"%s\"", sGot.c_str() );
		}
		else
		{
			/* We didn't get a string.  Show the type. */
			if (!luaL_callmeta(L, iPos, "__type"))
					lua_pushstring(L, luaL_typename(L, iPos));
			LuaHelpers::Pop( L, sGot );
		}
		LuaHelpers::Push( L, ssprintf("Expected %s; got %s", szType, sGot.c_str() ) );
		// There are a couple places where CheckEnum is used outside of a
		// function called from lua.  If we use lua_error from one of them,
		// StepMania crashes out completely.  bAllowAnything allows those places
		// to avoid crashing over theme mistakes.
		if(bAllowAnything)
		{
			RString errmsg;
			LuaHelpers::Pop(L, errmsg);
			LuaHelpers::ReportScriptError(errmsg);
			lua_pop(L, 2);
			return iInvalid;
		}
		lua_error( L );
	}
	int iRet = lua_tointeger( L, -1 );
	lua_pop( L, 2 );
	return iRet;
}

// szNameArray is of size iMax; pNameCache is of size iMax+2.
const RString &EnumToString( int iVal, int iMax, const char **szNameArray, unique_ptr<RString> *pNameCache )
{
	if( unlikely(pNameCache[0].get() == NULL) )
	{
		for( int i = 0; i < iMax; ++i )
		{
			unique_ptr<RString> ap(new RString(szNameArray[i]));
			pNameCache[i] = std::move(ap);
		}

		unique_ptr<RString> ap(new RString);
		pNameCache[iMax + 1] = std::move(ap);
	}

	// iMax+1 is "Invalid".  iMax+0 is the NUM_ size value, which can not be converted
	// to a string.
	// Maybe we should assert on _Invalid?  It seems better to make 
	// the caller check that they're supplying a valid enum value instead of 
	// returning an inconspicuous garbage value (empty string). -Chris
	if (iVal < 0)
		FAIL_M(ssprintf("Value %i cannot be negative for enums! Enum hint: %s", iVal, szNameArray[0]));
	if (iVal == iMax)
		FAIL_M(ssprintf("Value %i cannot be a string with value %i! Enum hint: %s", iVal, iMax, szNameArray[0]));
	if (iVal > iMax+1)
		FAIL_M(ssprintf("Value %i is past the invalid value %i! Enum hint: %s", iVal, iMax, szNameArray[0]));
	return *pNameCache[iVal];
}

namespace
{
	int GetName( lua_State *L )
	{
		luaL_checktype( L, 1, LUA_TTABLE );

		/* Look up the reverse table. */
		luaL_getmetafield( L, 1, "name" );

		/* If there was no metafield, then we were called on the wrong type. */
		if( lua_isnil(L, -1) )
			luaL_typerror( L, 1, "enum" );

		return 1;
	}
	
	int Reverse( lua_State *L )
	{
		luaL_checktype( L, 1, LUA_TTABLE );

		/* Look up the reverse table.  If there is no metafield, then we were
		 * called on the wrong type. */
		if( !luaL_getmetafield(L, 1, "reverse") )
			luaL_typerror( L, 1, "enum" );

		return 1;
	}
}

static const luaL_Reg EnumLib[] = {
	{ "GetName", GetName },
	{ "Reverse", Reverse },
	{ NULL, NULL }
};

static void PushEnumMethodTable( lua_State *L )
{
	luaL_register( L, "Enum", EnumLib );
}

/* Set up the enum table on the stack, and pop the table. */
void Enum::SetMetatable( lua_State *L, LuaReference &EnumTable, LuaReference &EnumIndexTable, const char *szName )
{
	EnumTable.PushSelf( L );
	{
		lua_newtable( L );
		EnumIndexTable.PushSelf( L );
		lua_setfield( L, -2, "reverse" );

		lua_pushstring( L, szName );
		lua_setfield( L, -2, "name" );

		PushEnumMethodTable( L );
		lua_setfield( L, -2, "__index" );

		lua_pushliteral( L, "Enum" );
		LuaHelpers::PushValueFunc( L, 1 );
		lua_setfield( L, -2, "__type" ); // for luaL_pushtype
	}
	lua_setmetatable( L, -2 );
	lua_pop( L, 2 );
}

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
