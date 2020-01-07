﻿/* OptionsBinding - little helpers so that SongOptions and PlayerOptions can
 * more easily have similar interfaces. */

// No .cpp file because it's just some #defines, and not in a makefile because
// it doesn't need to be. DefaultNilArgs would be in here, but then there would
// need to be a .cpp file, and DefaultNilArgs is more widely useful.

#ifndef OptionsBinding_H
#define OptionsBinding_H
// Functions are designed to combine Get and Set into one, to be less clumsy to
// use. -Kyz If a valid arg is passed, the value is set. The previous value is
// returned.
// OPTIONAL_RETURN_SELF exists to provide optional chaining support.
// Example:  local a= player_options:Twirl(5, 1, true):Roll(5,
// true):Dizzy(true):Twirl()
#define OPTIONAL_RETURN_SELF(option_index)                                     \
	if (lua_isboolean(L, option_index) && lua_toboolean(L, option_index)) {    \
		p->PushSelf(L);                                                        \
		return 1;                                                              \
	}
#define FLOAT_INTERFACE(func_name, member, valid)                              \
	static int func_name(T* p, lua_State* L)                                   \
	{                                                                          \
		int original_top = lua_gettop(L);                                      \
		lua_pushnumber(L, p->m_f##member);                                     \
		lua_pushnumber(L, p->m_Speedf##member);                                \
		if (lua_isnumber(L, 1) && original_top >= 1) {                         \
			float v = FArg(1);                                                 \
			if (!valid) {                                                      \
				luaL_error(L, "Invalid value %f", v);                          \
			}                                                                  \
			p->m_f##member = v;                                                \
		}                                                                      \
		if (original_top >= 2 && lua_isnumber(L, 2)) {                         \
			p->m_Speedf##member = FArgGTEZero(L, 2);                           \
		}                                                                      \
		OPTIONAL_RETURN_SELF(original_top);                                    \
		return 2;                                                              \
	}
#define FLOAT_NO_SPEED_INTERFACE(func_name, member, valid)                     \
	static int func_name(T* p, lua_State* L)                                   \
	{                                                                          \
		int original_top = lua_gettop(L);                                      \
		lua_pushnumber(L, p->m_f##member);                                     \
		if (lua_isnumber(L, 1) && original_top >= 1) {                         \
			float v = FArg(1);                                                 \
			if (!valid) {                                                      \
				luaL_error(L, "Invalid value %f", v);                          \
			}                                                                  \
			p->m_f##member = v;                                                \
		}                                                                      \
		OPTIONAL_RETURN_SELF(original_top);                                    \
		return 1;                                                              \
	}
#define INT_INTERFACE(func_name, member)                                       \
	static int func_name(T* p, lua_State* L)                                   \
	{                                                                          \
		int original_top = lua_gettop(L);                                      \
		lua_pushnumber(L, p->m_##member);                                      \
		if (lua_isnumber(L, 1) && original_top >= 1) {                         \
			p->m_##member = IArg(1);                                           \
		}                                                                      \
		OPTIONAL_RETURN_SELF(original_top);                                    \
		return 1;                                                              \
	}
// BOOL_INTERFACE can't use OPTIONAL_RETURN_SELF because it pushes a bool.
// If it did original_top, then "foo(true)" would chain when it shouldn't.
#define BOOL_INTERFACE(func_name, member)                                      \
	static int func_name(T* p, lua_State* L)                                   \
	{                                                                          \
		int original_top = lua_gettop(L);                                      \
		lua_pushboolean(L, p->m_b##member);                                    \
		if (lua_isboolean(L, 1) && original_top >= 1) {                        \
			p->m_b##member = BArg(1);                                          \
		}                                                                      \
		if (original_top >= 2 && lua_isboolean(L, 2)) {                        \
			p->PushSelf(L);                                                    \
			return 1;                                                          \
		}                                                                      \
		return 1;                                                              \
	}
#define ENUM_INTERFACE(func_name, member, enum_name)                           \
	static int func_name(T* p, lua_State* L)                                   \
	{                                                                          \
		int original_top = lua_gettop(L);                                      \
		Enum::Push(L, p->m_##member);                                          \
		if (!lua_isnil(L, 1) && original_top >= 1) {                           \
			p->m_##member = Enum::Check<enum_name>(L, 1);                      \
		}                                                                      \
		OPTIONAL_RETURN_SELF(original_top);                                    \
		return 1;                                                              \
	}
// Walk the table to make sure all entries are valid before setting.
#define FLOAT_TABLE_INTERFACE(func_name, member, valid)                        \
	static int func_name(T* p, lua_State* L)                                   \
	{                                                                          \
		int original_top = lua_gettop(L);                                      \
		lua_createtable(L, p->m_##member.size(), 0);                           \
		for (size_t n = 0; n < p->m_##member.size(); ++n) {                    \
			lua_pushnumber(L, p->m_##member[n]);                               \
			lua_rawseti(L, -2, n + 1);                                         \
		}                                                                      \
		if (lua_istable(L, 1) && original_top >= 1) {                          \
			size_t size = lua_objlen(L, 1);                                    \
			if (valid(L, 1)) {                                                 \
				p->m_##member.clear();                                         \
				p->m_##member.reserve(size);                                   \
				for (size_t n = 1; n <= size; ++n) {                           \
					lua_pushnumber(L, n);                                      \
					lua_gettable(L, 1);                                        \
					float v = FArg(-1);                                        \
					p->m_##member.push_back(v);                                \
					lua_pop(L, 1);                                             \
				}                                                              \
			}                                                                  \
		}                                                                      \
		OPTIONAL_RETURN_SELF(original_top);                                    \
		return 1;                                                              \
	}

#endif
