#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/LuaManager.h"
#include "WheelItemBase.h"

static const char* WheelItemDataTypeNames[] = {
	"Generic", "Section", "Song", "Roulette", "Random",
	"Portal",  "Course",  "Sort", "Custom",
};
XToString(WheelItemDataType);
StringToX(WheelItemDataType);
LuaXType(WheelItemDataType);

WheelItemBaseData::WheelItemBaseData(WheelItemDataType type,
									 const std::string& sText,
									 const RageColor& color)
{
	m_Type = type;
	m_sText = sText;
	m_color = color;
}

// begin WheelItemBase
WheelItemBase::WheelItemBase(const WheelItemBase& cpy)
  : ActorFrame(cpy)
  , m_pData(cpy.m_pData)
  , m_bExpanded(cpy.m_bExpanded)
{
	// what
	m_pGrayBar = nullptr;

	// FIXME
	// if( cpy.m_pGrayBar == cpy.m_sprBar )
	//	m_pGrayBar = m_sprBar;
}

WheelItemBase::WheelItemBase(const std::string& sType)
{
	SetName(sType);
	m_pData = nullptr;
	m_bExpanded = false;
	m_pGrayBar = nullptr;
	Load();
}

void
WheelItemBase::Load()
{
	m_colorLocked = RageColor(0, 0, 0, 0.25f);
}

void
WheelItemBase::LoadFromWheelItemData(const WheelItemBaseData* pWID,
									 int iIndex,
									 bool bHasFocus,
									 int iDrawIndex)
{
	ASSERT(pWID != NULL);
	m_pData = pWID;
}

void
WheelItemBase::DrawGrayBar(Actor& bar)
{
	if (m_colorLocked.a == 0)
		return;

	RageColor glow = bar.GetGlow();
	RageColor diffuse = bar.GetDiffuse();

	bar.SetGlow(m_colorLocked);
	bar.SetDiffuse(RageColor(0, 0, 0, 0));

	bar.Draw();

	bar.SetGlow(glow);
	bar.SetDiffuse(diffuse);
}

void
WheelItemBase::DrawPrimitives()
{
	ActorFrame::DrawPrimitives();

	if (m_pGrayBar != nullptr)
		DrawGrayBar(*m_pGrayBar);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the WheelItemBase. */
class LunaWheelItemBase : public Luna<WheelItemBase>
{
  public:
#define IS_LOADED_CHECK                                                        \
	if (!p->IsLoaded()) {                                                      \
		luaL_error(L,                                                          \
				   "Wheel item is not loaded yet.  Use WheelItem:IsLoaded() "  \
				   "to check.");                                               \
	}

	static int GetColor(T* p, lua_State* L)
	{
		IS_LOADED_CHECK;
		LuaHelpers::Push(L, p->GetColor());
		return 1;
	}

	static int GetText(T* p, lua_State* L)
	{
		IS_LOADED_CHECK;
		LuaHelpers::Push(L, p->GetText());
		return 1;
	}

	static int GetType(T* p, lua_State* L)
	{
		IS_LOADED_CHECK;
		lua_pushnumber(L, p->GetType());
		return 1;
	}

	static int IsLoaded(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->IsLoaded());
		return 1;
	}

	LunaWheelItemBase()
	{
		ADD_METHOD(GetColor);
		ADD_METHOD(GetText);
		ADD_METHOD(GetType);
		ADD_METHOD(IsLoaded);
	}
};
LUA_REGISTER_DERIVED_CLASS(WheelItemBase, ActorFrame)
// lua end
