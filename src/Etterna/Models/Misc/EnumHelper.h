#ifndef ENUM_HELPER_H
#define ENUM_HELPER_H

#include "Etterna/Models/Lua/LuaReference.h"
#include <memory>

#include "lua.hpp"

/** @brief A general foreach loop for enumerators, going up to a max value. */
#define FOREACH_ENUM_N(e, max, var)                                            \
	for (e var = (e)0; (var) < (max); enum_add<e>((var), +1))
/** @brief A general foreach loop for enumerators. */
#define FOREACH_ENUM(e, var)                                                   \
	for (e var = (e)0; (var) < NUM_##e; enum_add<e>((var), +1))

auto
CheckEnum(lua_State* L,
		  LuaReference& table,
		  int iPos,
		  int iInvalid,
		  const char* szType,
		  bool bAllowInvalid,
		  bool bAllowAnything = false) -> int;

template<typename T>
struct EnumTraits
{
	static LuaReference StringToEnum;
	static LuaReference EnumToString;
	static T Invalid;
	static const char* szName;
};
template<typename T>
LuaReference EnumTraits<T>::StringToEnum;
template<typename T>
LuaReference EnumTraits<T>::EnumToString;
/** @brief Lua helpers for Enumerators. */

auto
EnumToString(int iVal,
			 int iMax,
			 const char** szNameArray,
			 std::unique_ptr<std::string>* pNameCache)
  -> const std::string&; // XToString helper

#define XToString(X)                                                           \
                                                                               \
	const std::string& X##ToString(X x);                                       \
                                                                               \
	COMPILE_ASSERT(NUM_##X == ARRAYLEN(X##Names));                             \
                                                                               \
	const std::string& X##ToString(X x)                                        \
                                                                               \
	{                                                                          \
		static std::unique_ptr<std::string> as_##X##Name[NUM_##X + 2];         \
		return EnumToString(x, NUM_##X, X##Names, as_##X##Name);               \
	}                                                                          \
                                                                               \
	namespace StringConversion {                                               \
	template<>                                                                 \
	std::string ToString<X>(const X& value)                                    \
	{                                                                          \
		return X##ToString(value);                                             \
	}                                                                          \
	}

#define XToLocalizedString(X)                                                  \
                                                                               \
	const std::string& X##ToLocalizedString(X x);                              \
                                                                               \
	const std::string& X##ToLocalizedString(X x)                               \
                                                                               \
	{                                                                          \
		static std::unique_ptr<LocalizedString> g_##X##Name[NUM_##X];          \
		if (g_##X##Name[0].get() == nullptr) {                                 \
			for (unsigned i = 0; i < NUM_##X; ++i) {                           \
				std::unique_ptr<LocalizedString> ap(                           \
				  new LocalizedString(#X, X##ToString((X)i)));                 \
				g_##X##Name[i] = std::move(ap);                                \
			}                                                                  \
		}                                                                      \
		return g_##X##Name[x]->GetValue();                                     \
	}

#define StringToX(X)                                                           \
                                                                               \
	X StringTo##X(const std::string&);                                         \
                                                                               \
	X StringTo##X(const std::string& s)                                        \
                                                                               \
	{                                                                          \
		for (unsigned i = 0; i < ARRAYLEN(X##Names); ++i)                      \
			if (!CompareNoCase(s, X##Names[i]))                                \
				return (X)i;                                                   \
		return X##_Invalid;                                                    \
	}                                                                          \
                                                                               \
	namespace StringConversion                                                 \
                                                                               \
	{                                                                          \
	template<>                                                                 \
	bool FromString<X>(const std::string& sValue, X& out)                      \
	{                                                                          \
		out = StringTo##X(sValue);                                             \
		return out != X##_Invalid;                                             \
	}                                                                          \
	}

/* Disable warnings about instantiations of static member variables of the
 * templated class not being defined. These are all defined in different
 * translation units by LuaXType, a macro that does a whole lot of stuff
 * including initializing them.
 *
 * Because it happens in a different translation unit, there's not really a
 * great way to do this better. You could state that external instantiations
 * happen elsewhere with a `extern template struct EnumTraits<T>;` for every T,
 * but the issue is that this header file doesn't know about the existence of
 * those classes.
 */

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-var-template"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundefined-var-template"
#elif defined(_MSC_VER)
// TODO: Does anything need to be here?
#endif

namespace Enum {
template<typename T>
static auto
Check(lua_State* L,
	  int iPos,
	  bool bAllowInvalid = false,
	  bool bAllowAnything = false) -> T
{
	return static_cast<T>(CheckEnum(L,
									EnumTraits<T>::StringToEnum,
									iPos,
									EnumTraits<T>::Invalid,
									EnumTraits<T>::szName,
									bAllowInvalid,
									bAllowAnything));
}
#if defined(__clang__)
#pragma clang pop
#elif defined(__GNUC__)
#pragma GCC pop
#elif defined(_MSC_VER)

#endif

template<typename T>
static void
Push(lua_State* L, T iVal)
{
	/* Enum_Invalid values are nil in Lua. */
	if (iVal == EnumTraits<T>::Invalid) {
		lua_pushnil(L);
		return;
	}

	/* Look up the string value. */
	EnumTraits<T>::EnumToString.PushSelf(L);
	lua_rawgeti(L, -1, iVal + 1);
	lua_remove(L, -2);
}

void
SetMetatable(lua_State* L,
			 LuaReference& EnumTable,
			 LuaReference& EnumIndexTable,
			 const char* szName);
} // namespace Enum

#define LuaDeclareType(X)

#define LuaXType(X)                                                            \
                                                                               \
	template struct EnumTraits<X>;                                             \
                                                                               \
	static void Lua##X(lua_State* L)                                           \
                                                                               \
	{                                                                          \
		lua_newtable(L);                                                       \
		FOREACH_ENUM(X, i)                                                     \
		{                                                                      \
			std::string s = X##ToString(i);                                    \
			lua_pushstring(L, ((#X "_") + s).c_str());                         \
			lua_rawseti(L, -2, i + 1); /* 1-based */                           \
		}                                                                      \
		EnumTraits<X>::EnumToString.SetFromStack(L);                           \
		EnumTraits<X>::EnumToString.PushSelf(L);                               \
		lua_setglobal(L, #X);                                                  \
		lua_newtable(L);                                                       \
		FOREACH_ENUM(X, i)                                                     \
		{                                                                      \
			std::string s = X##ToString(i);                                    \
			lua_pushstring(L, ((#X "_") + s).c_str());                         \
			lua_pushnumber(L, i); /* 0-based */                                \
			lua_rawset(L, -3);                                                 \
			/* Compatibility with old, case-insensitive values */              \
			s = make_lower(s);                                                 \
			lua_pushstring(L, s.c_str());                                      \
			lua_pushnumber(L, i); /* 0-based */                                \
			lua_rawset(L, -3);                                                 \
			/* Compatibility with old, raw values */                           \
			lua_pushnumber(L, i);                                              \
			lua_rawseti(L, -2, i);                                             \
		}                                                                      \
		EnumTraits<X>::StringToEnum.SetFromStack(L);                           \
		EnumTraits<X>::StringToEnum.PushSelf(L);                               \
		Enum::SetMetatable(                                                    \
		  L, EnumTraits<X>::EnumToString, EnumTraits<X>::StringToEnum, #X);    \
	}                                                                          \
                                                                               \
	REGISTER_WITH_LUA_FUNCTION(Lua##X);                                        \
                                                                               \
	template<>                                                                 \
	X EnumTraits<X>::Invalid = X##_Invalid;                                    \
                                                                               \
	template<>                                                                 \
	const char* EnumTraits<X>::szName = #X;                                    \
                                                                               \
	namespace LuaHelpers                                                       \
                                                                               \
	{                                                                          \
	template<>                                                                 \
	bool FromStack<X>(lua_State * L, X& Object, int iOffset)                   \
	{                                                                          \
		Object = Enum::Check<X>(L, iOffset, true);                             \
		return Object != EnumTraits<X>::Invalid;                               \
	}                                                                          \
	}                                                                          \
                                                                               \
	namespace LuaHelpers                                                       \
                                                                               \
	{                                                                          \
	template<>                                                                 \
	void Push<X>(lua_State * L, const X& Object)                               \
	{                                                                          \
		Enum::Push<X>(L, Object);                                              \
	}                                                                          \
	}

#endif
