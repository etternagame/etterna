#include "Etterna/Globals/global.h"
#include "ArchHooks.h"
#include "Etterna/Models/Lua/LuaReference.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Misc/RageThreads.h"
#include "arch/arch_default.h"
#include "Etterna/Singletons/PrefsManager.h"

#ifdef __APPLE__
#include "../../archutils/Darwin/MouseDevice.h"

int
ArchHooks::GetWindowWidth()
{
	return MACWindowWidth();
}

int
ArchHooks::GetWindowHeight()
{
	return MACWindowHeight();
}
#else
int
ArchHooks::GetWindowWidth()
{
	return (static_cast<int>(PREFSMAN->m_iDisplayHeight *
							 PREFSMAN->m_fDisplayAspectRatio));
}

int
ArchHooks::GetWindowHeight()
{
	return PREFSMAN->m_iDisplayHeight;
}
#endif

bool ArchHooks::g_bQuitting = false;
bool ArchHooks::g_bToggleWindowed = false;
// Keep from pulling RageThreads.h into ArchHooks.h
static RageMutex g_Mutex("ArchHooks");
ArchHooks* HOOKS = NULL; // global and accessible from anywhere in our program

ArchHooks::ArchHooks()
  : m_bHasFocus(true)
  , m_bFocusChanged(false)
{
}

bool
ArchHooks::GetAndClearToggleWindowed()
{
	LockMut(g_Mutex);
	bool bToggle = g_bToggleWindowed;

	g_bToggleWindowed = false;
	return bToggle;
}

void
ArchHooks::SetToggleWindowed()
{
	LockMut(g_Mutex);
	g_bToggleWindowed = true;
}

void
ArchHooks::SetHasFocus(bool bHasFocus)
{
	if (bHasFocus == m_bHasFocus)
		return;
	m_bHasFocus = bHasFocus;

	LOG->Trace("App %s focus", bHasFocus ? "has" : "doesn't have");
	LockMut(g_Mutex);
	m_bFocusChanged = true;
}

bool
ArchHooks::AppFocusChanged()
{
	LockMut(g_Mutex);
	bool bFocusChanged = m_bFocusChanged;

	m_bFocusChanged = false;
	return bFocusChanged;
}

bool
ArchHooks::GoToURL(const RString& sUrl)
{
	return false;
}

ArchHooks*
ArchHooks::Create()
{
	return new ARCH_HOOKS;
}

RString
ArchHooks::GetClipboard()
{
	LOG->Warn("ArchHooks: GetClipboard() NOT IMPLEMENTED");
	return "";
}

/* XXX: Most singletons register with lua in their constructor.  ArchHooks is
 * instantiated before Lua, so we encounter a dependency problem when
 * trying to register HOOKS. Work around it by registering HOOKS function
 * which sm_main will call after instantiating Lua. */
void
ArchHooks::RegisterWithLua()
{
	Lua* L = LUA->Get();
	lua_pushstring(L, "HOOKS");
	HOOKS->PushSelf(L);
	lua_settable(L, LUA_GLOBALSINDEX);
	LUA->Release(L);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Models/Lua/LuaReference.h"

class LunaArchHooks : public Luna<ArchHooks>
{
  public:
	DEFINE_METHOD(AppHasFocus, AppHasFocus());
	DEFINE_METHOD(GetArchName, GetArchName());
	DEFINE_METHOD(GetClipboard, GetClipboard());
	static int ShowCursor(T* p, lua_State* L)
	{
		p->sShowCursor(BArg(1));
		return 0;
	}

	LunaArchHooks()
	{
		ADD_METHOD(AppHasFocus);
		ADD_METHOD(GetArchName);
		ADD_METHOD(GetClipboard);
		ADD_METHOD(ShowCursor);
	}
};
LUA_REGISTER_CLASS(ArchHooks);
