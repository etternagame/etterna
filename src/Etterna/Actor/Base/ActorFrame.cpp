#include "Etterna/Globals/global.h"
#include "ActorFrame.h"
#include "ActorUtil.h"
#include "RageUtil/Graphics/Display/RageDisplay.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "Etterna/FileTypes/XmlFile.h"

#include <algorithm>

/* Tricky: We need ActorFrames created in Lua to auto delete their children.
 * We don't want classes that derive from ActorFrame to auto delete their
 * children.  The name "ActorFrame" is widely used in Lua, so we'll have
 * that string instead create an ActorFrameAutoDeleteChildren object.
 */
// REGISTER_ACTOR_CLASS( ActorFrame );
REGISTER_ACTOR_CLASS_WITH_NAME(ActorFrameAutoDeleteChildren, ActorFrame);
ActorFrame*
ActorFrame::Copy() const
{
	return new ActorFrame(*this);
}

ActorFrame::ActorFrame()
{
	m_bPropagateCommands = false;
	m_bDeleteChildren = false;
	m_bDrawByZPosition = false;
	m_DrawFunction.SetFromNil();
	m_UpdateFunction.SetFromNil();
	m_fUpdateRate = 1;
	m_fFOV = -1;
	m_fVanishX = SCREEN_CENTER_X;
	m_fVanishY = SCREEN_CENTER_Y;
	m_bOverrideLighting = false;
	m_bLighting = false;
	m_bFirstUpdate = true;
	m_ambientColor = RageColor(1, 1, 1, 1);
	m_diffuseColor = RageColor(1, 1, 1, 1);
	m_specularColor = RageColor(1, 1, 1, 1);
	m_lightDirection = RageVector3(0, 0, 1);
}

ActorFrame::~ActorFrame()
{
	if (m_bDeleteChildren)
		DeleteAllChildren();
}

ActorFrame::ActorFrame(const ActorFrame& cpy)
  : Actor(cpy)
{
#define CPY(x) this->x = cpy.x;
	CPY(m_bPropagateCommands);
	CPY(m_bDeleteChildren);
	CPY(m_bDrawByZPosition);
	CPY(m_DrawFunction);
	CPY(m_UpdateFunction);
	CPY(m_fUpdateRate);
	CPY(m_fFOV);
	CPY(m_fVanishX);
	CPY(m_fVanishY);
	CPY(m_bOverrideLighting);
	CPY(m_bLighting);
	CPY(m_ambientColor);
	CPY(m_diffuseColor);
	CPY(m_specularColor);
	CPY(m_lightDirection);
	CPY(m_bFirstUpdate);
#undef CPY

	/* If m_bDeleteChildren, we own our children and it's up to us to copy
	 * them.  If not, the derived class owns the children.  This must preserve
	 * the current order of m_SubActors. */
	if (m_bDeleteChildren) {
		for (auto* m_SubActor : cpy.m_SubActors) {
			auto* pActor = m_SubActor->Copy();
			this->ActorFrame::AddChild(pActor);
		}
	}
}

void
ActorFrame::InitState()
{
	for (auto& a : m_SubActors) {
		a->InitState();
	}

	Actor::InitState();
}

void
ActorFrame::LoadFromNode(const XNode* pNode)
{
	if (AutoLoadChildren())
		LoadChildrenFromNode(pNode);

	Actor::LoadFromNode(pNode);

	pNode->GetAttrValue("UpdateRate", m_fUpdateRate);
	pNode->GetAttrValue("FOV", m_fFOV);
	pNode->GetAttrValue("VanishX", m_fVanishX);
	pNode->GetAttrValue("VanishY", m_fVanishY);
	m_bOverrideLighting = pNode->GetAttrValue("Lighting", m_bLighting);
	// new lighting values (only ambient color seems to work?) -aj
	std::string sTemp1, sTemp2, sTemp3;
	pNode->GetAttrValue("AmbientColor", sTemp1);
	m_ambientColor.FromString(sTemp1);
	pNode->GetAttrValue("DiffuseColor", sTemp2);
	m_diffuseColor.FromString(sTemp2);
	pNode->GetAttrValue("SpecularColor", sTemp3);
	m_specularColor.FromString(sTemp3);
	// Values need to be converted into a RageVector3, so more work needs to be
	// done...
	// pNode->GetAttrValue( "LightDirection", m_lightDirection );
}

void
ActorFrame::LoadChildrenFromNode(const XNode* pNode)
{
	// Shouldn't be calling this unless we're going to delete our children.
	ASSERT(m_bDeleteChildren);

	// Load children
	const auto* pChildren = pNode->GetChild("children");
	auto bArrayOnly = false;
	if (pChildren == nullptr) {
		bArrayOnly = true;
		pChildren = pNode;
	}

	FOREACH_CONST_Child(pChildren, pChild)
	{
		if (bArrayOnly && !IsAnInt(pChild->GetName()))
			continue;

		auto* pChildActor = ActorUtil::LoadFromNode(pChild, this);
		if (pChildActor)
			AddChild(pChildActor);
	}
	SortByDrawOrder();
}

void
ActorFrame::AddChild(Actor* pActor)
{
#ifdef DEBUG
	// check that this Actor isn't already added.
	vector<Actor*>::iterator iter =
	  find(m_SubActors.begin(), m_SubActors.end(), pActor);
	if (iter != m_SubActors.end())
		Dialog::OK(ssprintf("Actor \"%s\" adds child \"%s\" more than once",
							GetLineage().c_str(),
							pActor->GetName().c_str()));
#endif

	ASSERT(pActor != nullptr);
	m_SubActors.push_back(pActor);

	pActor->SetParent(this);
}

void
ActorFrame::RemoveChild(Actor* pActor)
{
	const auto iter = find(m_SubActors.begin(), m_SubActors.end(), pActor);
	if (iter != m_SubActors.end())
		m_SubActors.erase(iter);
}

void
ActorFrame::TransferChildren(ActorFrame* pTo)
{
	for (auto* a : m_SubActors) {
		pTo->AddChild(a);
	}

	RemoveAllChildren();
}

Actor*
ActorFrame::GetChild(const std::string& sName)
{
	for (auto* a : m_SubActors) {
		if (a->GetName() == sName)
			return a;
	}

	return nullptr;
}

void
ActorFrame::RemoveAllChildren()
{
	m_SubActors.clear();
}

void
ActorFrame::MoveToTail(Actor* pActor)
{
	const auto iter = find(m_SubActors.begin(), m_SubActors.end(), pActor);
	if (iter == m_SubActors.end()) // didn't find
		FAIL_M("Nonexistent actor");

	m_SubActors.erase(iter);
	m_SubActors.push_back(pActor);
}

void
ActorFrame::MoveToHead(Actor* pActor)
{
	const auto iter = find(m_SubActors.begin(), m_SubActors.end(), pActor);
	if (iter == m_SubActors.end()) // didn't find
		FAIL_M("Nonexistent actor");

	m_SubActors.erase(iter);
	m_SubActors.insert(m_SubActors.begin(), pActor);
}

void
ActorFrame::BeginDraw()
{
	Actor::BeginDraw();
	if (m_fFOV != -1) {
		DISPLAY->CameraPushMatrix();
		DISPLAY->LoadMenuPerspective(
		  m_fFOV, SCREEN_WIDTH, SCREEN_HEIGHT, m_fVanishX, m_fVanishY);
	}

	if (m_bOverrideLighting) {
		DISPLAY->SetLighting(m_bLighting);
		if (m_bLighting)
			DISPLAY->SetLightDirectional(0,
										 m_ambientColor,
										 m_diffuseColor,
										 m_specularColor,
										 m_lightDirection);
	}
}

void
ActorFrame::DrawPrimitives()
{
	if (m_bClearZBuffer) {
		LuaHelpers::ReportScriptErrorFmt(
		  "ClearZBuffer not supported on ActorFrames");
		m_bClearZBuffer = false;
	}

	// Don't set Actor-defined render states because we won't be drawing
	// any geometry that belongs to this object.
	// Actor::DrawPrimitives();

	if (unlikely(!m_DrawFunction.IsNil())) {
		auto* L = LUA->Get();
		m_DrawFunction.PushSelf(L);
		if (lua_isnil(L, -1)) {
			LUA->Release(L);
			LuaHelpers::ReportScriptErrorFmt("Error compiling DrawFunction");
			return;
		}
		this->PushSelf(L);
		std::string Error = "Error running DrawFunction: ";
		LuaHelpers::RunScriptOnStack(L, Error, 1, 0, true); // 1 arg, 0 results
		LUA->Release(L);
		return;
	}

	const auto diffuse = m_pTempState->diffuse[0];
	const auto glow = m_pTempState->glow;

	// Word of warning:  Actor::Draw duplicates the structure of how an
	// Actor is drawn inside of an ActorFrame for its wrapping feature.  So
	// if you're adding something new to ActorFrames that affects how Actors
	// are drawn, make sure to also apply it in Actor::Draw's handling of
	// the wrappers. -Kyz

	// draw all sub-ActorFrames while we're in the ActorFrame's local
	// coordinate space
	if (m_bDrawByZPosition) {
		auto subs = m_SubActors;
		ActorUtil::SortByZPosition(subs);
		for (auto& sub : subs) {
			sub->SetInternalDiffuse(diffuse);
			sub->SetInternalGlow(glow);
			sub->Draw();
		}
	} else {
		for (auto& m_SubActor : m_SubActors) {
			m_SubActor->SetInternalDiffuse(diffuse);
			m_SubActor->SetInternalGlow(glow);
			m_SubActor->Draw();
		}
	}
}

void
ActorFrame::EndDraw()
{
	if (m_bOverrideLighting) {
		// TODO: pop state instead of turning lighting off
		DISPLAY->SetLightOff(0);
		DISPLAY->SetLighting(false);
	}

	if (m_fFOV != -1) {
		DISPLAY->CameraPopMatrix();
	}
	Actor::EndDraw();
}

// This exists solely as a helper for children table.
// This applies the desired function to te first child in the table.
static int
IdenticalChildrenSingleApplier(lua_State* L)
{
	// First arg is the table of items, get the last object.
	// It is the one that would have been in the children table in the old
	// version. The other args are meant for the function. The upvalue for
	// this function is the function the theme tried to call.
	lua_rawgeti(L, 1, lua_objlen(L, 1));   // stack: table, args, obj
	lua_insert(L, 2);					   // stack: table, obj, args
	lua_pushvalue(L, lua_upvalueindex(1)); // stack: table, obj, args, func
	lua_insert(L, 2);					   // stack: table, func, obj, args
	const auto args_count = lua_gettop(L) - 2;
	// Not using RunScriptOnStack because we're inside a lua call already
	// and we want an error to propagate up.
	lua_call(L, args_count, LUA_MULTRET); // stack: table, return_values
	return lua_gettop(L) - 1;
}

// This exists solely as a helper for children table.
// This is the __index function for the table of all children with the same
// name.
static int
IdenticalChildrenIndexLayer(lua_State* L)
{
	if (lua_isnumber(L, 2) != 0) {
		lua_rawget(L, 1);
	} else {
		lua_pushvalue(L, 1);
		lua_pushvalue(L, 2);
		lua_pop(L, 2);
		// Get the last object in the table.
		// Its meta table contains the function the theme wanted to run.
		// The function is then pushed as an upvalue for ICSA as a closure.
		// The closure is then returned so that when the function call is
		// performed, ICSA is actually called.
		lua_pushnumber(L, lua_objlen(L, 1)); // stack: 1
		lua_gettable(L, 1);					 // stack: object
		lua_getmetatable(L, -1);			 // stack: object, obj_meta
		lua_getfield(L, -1, "__index"); // stack: object, obj_meta, obj_index
		lua_pushvalue(L,
					  2);	 // stack: object, obj_meta, obj_index, func_name
		lua_gettable(L, -2); // stack: object, obj_meta, obj_index, obj_function
		lua_pushcclosure(L,
						 IdenticalChildrenSingleApplier,
						 1); // stack: object, obj_meta, obj_index, closure
		lua_insert(L, -4);	 // stack: closure, object, obj_meta, obj_index
		lua_pop(L, 3);		 // stack: closure
	}
	return 1;
}

static void
CreateChildTable(lua_State* L, Actor* a)
{
	// Old PushChildrenTable assumed that all children had unique names, so
	// only the last one of a name ended up in the table that was returned.
	// Create a table that will hold all the children that have this name
	// and act as a pass through layer for function calls. stack: old_entry
	lua_createtable(L, 0, 0); // stack: old_entry, table_entry
	lua_insert(L, -2);		  // stack: table_entry, old_entry
	lua_rawseti(L, -2, 1);	  // stack: table_entry
	a->PushSelf(L);			  // stack: table_entry, new_entry
	lua_rawseti(L, -2, 2);	  // stack: table_entry
	lua_createtable(L, 0, 1); // stack: table_entry, table_meta
	lua_pushcfunction(
	  L,
	  IdenticalChildrenIndexLayer); // stack: table_entry, table_meta, ICIL
	lua_setfield(L, -2, "__index"); // stack: table_entry, table_meta
	lua_setmetatable(L, -2);		// stack: table_entry
}

static void
AddToChildTable(lua_State* L, Actor* a)
{
	// stack: table_entry
	const int next_index = lua_objlen(L, -1) + 1;
	a->PushSelf(L);					// stack: table_entry, actor
	lua_rawseti(L, -2, next_index); // stack: table_entry
}

void
ActorFrame::PushChildrenTable(lua_State* L)
{
	lua_newtable(L); // stack: all_actors
	for (auto& a : m_SubActors) {
		LuaHelpers::Push(L, a->GetName()); // stack: all_actors, name
		lua_gettable(L, -2);			   // stack: all_actors, entry
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);					   // stack: all_actors
			LuaHelpers::Push(L, a->GetName()); // stack: all_actors, name
			a->PushSelf(L);					   // stack: all_actors, name, actor
			lua_rawset(L, -3);				   // stack: all_actors
		} else {
			// Fun fact:  PushSelf pushes a table.
			if (lua_objlen(L, -1) > 0) {
				// stack: all_actors, table_entry
				AddToChildTable(L, a); // stack: all_actors, table_entry
				lua_pop(L, 1);		   // stack: all_actors
			} else {
				// stack: all_actors, old_entry
				CreateChildTable(L, a); // stack: all_actors, table_entry
				LuaHelpers::Push(
				  L,
				  a->GetName());   // stack: all_actors, table_entry, name
				lua_insert(L, -2); // stack: all_actors, name, table_entry
				lua_rawset(L, -3); // stack: all_actors
			}
		}
	}
}

void
ActorFrame::PushChildTable(lua_State* L, const std::string& sName)
{
	auto found = 0;
	for (auto& a : m_SubActors) {
		if (a->GetName() == sName) {
			switch (found) {
				case 0:
					a->PushSelf(L);
					break;
				case 1:
					CreateChildTable(L, a);
					break;
				default:
					AddToChildTable(L, a);
					break;
			}
			++found;
		}
	}
	if (found == 0) {
		lua_pushnil(L);
	}
}

void
ActorFrame::PlayCommandOnChildren(const std::string& sCommandName,
								  const LuaReference* pParamTable)
{
	const auto* pCmd = GetCommand(sCommandName);
	if (pCmd != nullptr)
		RunCommandsOnChildren(*pCmd, pParamTable);
}

void
ActorFrame::PlayCommandOnLeaves(const std::string& sCommandName,
								const LuaReference* pParamTable)
{
	const auto* pCmd = GetCommand(sCommandName);
	if (pCmd != nullptr)
		RunCommandsOnLeaves(**pCmd, pParamTable);
}

void
ActorFrame::RunCommandsRecursively(const LuaReference& cmds,
								   const LuaReference* pParamTable)
{
	for (auto& m_SubActor : m_SubActors)
		m_SubActor->RunCommandsRecursively(cmds, pParamTable);
	Actor::RunCommandsRecursively(cmds, pParamTable);
}

void
ActorFrame::RunCommandsOnChildren(const LuaReference& cmds,
								  const LuaReference* pParamTable)
{
	for (auto& m_SubActor : m_SubActors)
		m_SubActor->RunCommands(cmds, pParamTable);
}

void
ActorFrame::RunCommandsOnLeaves(const LuaReference& cmds,
								const LuaReference* pParamTable)
{
	for (auto& m_SubActor : m_SubActors)
		m_SubActor->RunCommandsOnLeaves(cmds, pParamTable);
}

bool
ActorFrame::IsFirstUpdate() const
{
	return m_bFirstUpdate;
}

void
ActorFrame::UpdateInternal(float fDeltaTime)
{
	//	LOG->Trace( "ActorFrame::Update( %f )", fDeltaTime );
	if (m_bFirstUpdate)
		m_bFirstUpdate = false;

	fDeltaTime *= m_fUpdateRate;

	Actor::UpdateInternal(fDeltaTime);

	// update all sub-Actors
	for (auto* a : m_SubActors)
		a->Update(fDeltaTime);

	if (unlikely(!m_UpdateFunction.IsNil())) {
		secsSinceLuaUpdateFWasRun += fDeltaTime;
		if (secsSinceLuaUpdateFWasRun >= m_fUpdateFInterval) {
			secsSinceLuaUpdateFWasRun = 0;
			auto* L = LUA->Get();
			m_UpdateFunction.PushSelf(L);
			if (lua_isnil(L, -1)) {
				LUA->Release(L);
				LuaHelpers::ReportScriptErrorFmt(
				  "Error compiling UpdateFunction");
				return;
			}
			this->PushSelf(L);
			lua_pushnumber(L, fDeltaTime);
			std::string Error = "Error running UpdateFunction: ";
			LuaHelpers::RunScriptOnStack(
			  L, Error, 2, 0, true); // 1 args, 0 results
			LUA->Release(L);
		}
	}
}

#define PropagateActorFrameCommand(cmd)                                        \
	void ActorFrame::cmd()                                                     \
	{                                                                          \
		Actor::cmd();                                                          \
                                                                               \
		/* set all sub-Actors */                                               \
		for (unsigned i = 0; i < m_SubActors.size(); i++)                      \
			m_SubActors[i]->cmd();                                             \
	}

#define PropagateActorFrameCommand1Param(cmd, type)                            \
	void ActorFrame::cmd(type f)                                               \
	{                                                                          \
		Actor::cmd(f);                                                         \
                                                                               \
		/* set all sub-Actors */                                               \
		for (unsigned i = 0; i < m_SubActors.size(); i++)                      \
			m_SubActors[i]->cmd(f);                                            \
	}

PropagateActorFrameCommand(FinishTweening)
  PropagateActorFrameCommand1Param(SetZTestMode, ZTestMode)
	PropagateActorFrameCommand1Param(SetZWrite, bool)
	  PropagateActorFrameCommand1Param(HurryTweening, float)

		float ActorFrame::GetTweenTimeLeft() const
{
	auto m = Actor::GetTweenTimeLeft();

	for (auto* a : m_SubActors) {
		m = std::max(m, a->GetTweenTimeLeft());
	}

	return m;
}

static bool
CompareActorsByDrawOrder(const Actor* p1, const Actor* p2)
{
	return p1->GetDrawOrder() < p2->GetDrawOrder();
}

void
ActorFrame::SortByDrawOrder()
{
	// Preserve ordering of Actors with equal DrawOrders.
	stable_sort(
	  m_SubActors.begin(), m_SubActors.end(), CompareActorsByDrawOrder);
}

void
ActorFrame::DeleteAllChildren()
{
	for (auto& m_SubActor : m_SubActors)
		delete m_SubActor;
	m_SubActors.clear();
}

void
ActorFrame::RunCommands(const LuaReference& cmds,
						const LuaReference* pParamTable)
{
	if (m_bPropagateCommands)
		RunCommandsOnChildren(cmds, pParamTable);
	else
		Actor::RunCommands(cmds, pParamTable);
}

void
ActorFrame::SetPropagateCommands(bool b)
{
	m_bPropagateCommands = b;
}

void
ActorFrame::HandleMessage(const Message& msg)
{
	Actor::HandleMessage(msg);

	/* Don't propagate broadcasts.  They'll receive it directly if they're
	 * subscribed. */
	if (msg.IsBroadcast())
		return;

	for (auto* pActor : m_SubActors) {
		pActor->HandleMessage(msg);
	}
}

void
ActorFrame::SetDrawByZPosition(bool b)
{
	m_bDrawByZPosition = b;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ActorFrame. */
class LunaActorFrame : public Luna<ActorFrame>
{
  public:
	static int playcommandonchildren(T* p, lua_State* L)
	{
		p->PlayCommandOnChildren(SArg(1));
		COMMON_RETURN_SELF;
	}
	static int playcommandonleaves(T* p, lua_State* L)
	{
		p->PlayCommandOnLeaves(SArg(1));
		COMMON_RETURN_SELF;
	}
	static int runcommandsonleaves(T* p, lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TFUNCTION);
		LuaReference cmds;
		cmds.SetFromStack(L);

		p->RunCommandsOnLeaves(cmds);
		COMMON_RETURN_SELF;
	}
	static int RunCommandsOnChildren(T* p, lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TFUNCTION);
		lua_pushvalue(L, 2);
		LuaReference ParamTable;
		ParamTable.SetFromStack(L);

		lua_pushvalue(L, 1);
		LuaReference cmds;
		cmds.SetFromStack(L);

		p->RunCommandsOnChildren(cmds, &ParamTable);
		COMMON_RETURN_SELF;
	}
	static int propagate(T* p, lua_State* L)
	{
		p->SetPropagateCommands(BIArg(1));
		COMMON_RETURN_SELF;
	}
	static int fov(T* p, lua_State* L)
	{
		p->SetFOV(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int SetUpdateFunctionInterval(T* p, lua_State* L)
	{
		const auto seconds = FArg(1);
		if (seconds <= 0) {
			luaL_error(L,
					   "ActorFrame:SetUpdateRate(%f) Update interval must be "
					   "greater than 0.",
					   seconds);
		}
		p->SetUpdateFunctionInterval(seconds);
		COMMON_RETURN_SELF;
	}
	static int SetUpdateRate(T* p, lua_State* L)
	{
		const auto rate = FArg(1);
		if (rate <= 0) {
			luaL_error(L,
					   "ActorFrame:SetUpdateRate(%f) Update rate must be "
					   "greater than 0.",
					   rate);
		}
		p->SetUpdateRate(rate);
		COMMON_RETURN_SELF;
	}
	DEFINE_METHOD(GetUpdateRate, GetUpdateRate());
	static int SetFOV(T* p, lua_State* L)
	{
		p->SetFOV(FArg(1));
		COMMON_RETURN_SELF;
	}
	static int vanishpoint(T* p, lua_State* L)
	{
		p->SetVanishPoint(FArg(1), FArg(2));
		COMMON_RETURN_SELF;
	}
	static int GetChild(T* p, lua_State* L)
	{
		p->PushChildTable(L, SArg(1));
		return 1;
	}
	static int GetChildren(T* p, lua_State* L)
	{
		p->PushChildrenTable(L);
		return 1;
	}
	static int GetNumChildren(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetNumChildren());
		return 1;
	}
	static int SetDrawByZPosition(T* p, lua_State* L)
	{
		p->SetDrawByZPosition(BArg(1));
		COMMON_RETURN_SELF;
	}
	static int SetDrawFunction(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) {
			LuaReference ref;
			lua_pushnil(L);
			ref.SetFromStack(L);
			p->SetDrawFunction(ref);
			COMMON_RETURN_SELF;
		}

		luaL_checktype(L, 1, LUA_TFUNCTION);

		LuaReference ref;
		lua_pushvalue(L, 1);
		ref.SetFromStack(L);
		p->SetDrawFunction(ref);
		COMMON_RETURN_SELF;
	}
	static int GetDrawFunction(T* p, lua_State* L)
	{
		p->GetDrawFunction().PushSelf(L);
		return 1;
	}
	static int SetUpdateFunction(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) {
			LuaReference ref;
			lua_pushnil(L);
			ref.SetFromStack(L);
			p->SetUpdateFunction(ref);
			COMMON_RETURN_SELF;
		}

		luaL_checktype(L, 1, LUA_TFUNCTION);

		LuaReference ref;
		lua_pushvalue(L, 1);
		ref.SetFromStack(L);
		p->SetUpdateFunction(ref);
		COMMON_RETURN_SELF;
	}
	static int SortByDrawOrder(T* p, lua_State* L)
	{
		p->SortByDrawOrder();
		COMMON_RETURN_SELF;
	}

	// static int CustomLighting( T* p, lua_State *L )			{
	// p->SetCustomLighting(BArg(1)); COMMON_RETURN_SELF; }
	static int SetAmbientLightColor(T* p, lua_State* L)
	{
		RageColor c;
		c.FromStackCompat(L, 1);
		p->SetAmbientLightColor(c);
		COMMON_RETURN_SELF;
	}
	static int SetDiffuseLightColor(T* p, lua_State* L)
	{
		RageColor c;
		c.FromStackCompat(L, 1);
		p->SetDiffuseLightColor(c);
		COMMON_RETURN_SELF;
	}
	static int SetSpecularLightColor(T* p, lua_State* L)
	{
		RageColor c;
		c.FromStackCompat(L, 1);
		p->SetSpecularLightColor(c);
		COMMON_RETURN_SELF;
	}
	static int SetLightDirection(T* p, lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TTABLE);
		lua_pushvalue(L, 1);
		vector<float> coords;
		LuaHelpers::ReadArrayFromTable(coords, L);
		lua_pop(L, 1);
		if (coords.size() != 3) {
			// error
		}
		const auto vTmp = RageVector3(coords[0], coords[1], coords[2]);
		p->SetLightDirection(vTmp);
		COMMON_RETURN_SELF;
	}
	static int AddChildFromPath(T* p, lua_State* L)
	{
		// this one is tricky, we need to get an Actor from Lua.
		auto* pActor = ActorUtil::MakeActor(SArg(1));
		if (pActor == nullptr) {
			lua_pushboolean(L, 0);
			return 1;
		}
		p->AddChild(pActor);
		lua_pushboolean(L, 1);
		return 1;
	}

	static int RemoveChild(T* p, lua_State* L)
	{
		auto* child = p->GetChild(SArg(1));
		if (child != nullptr) {
			p->RemoveChild(child);
			SAFE_DELETE(child);
		}
		COMMON_RETURN_SELF;
	}
	static int RemoveAllChildren(T* p, lua_State* L)
	{
		p->DeleteAllChildren();
		COMMON_RETURN_SELF;
	}

	LunaActorFrame()
	{
		ADD_METHOD(playcommandonchildren);
		ADD_METHOD(playcommandonleaves);
		ADD_METHOD(runcommandsonleaves);
		ADD_METHOD(RunCommandsOnChildren);
		ADD_METHOD(propagate); // deprecated
		ADD_METHOD(fov);
		ADD_METHOD(SetUpdateRate);
		ADD_METHOD(GetUpdateRate);
		ADD_METHOD(SetFOV);
		ADD_METHOD(vanishpoint);
		ADD_METHOD(GetChild);
		ADD_METHOD(GetChildren);
		ADD_METHOD(GetNumChildren);
		ADD_METHOD(SetDrawByZPosition);
		ADD_METHOD(SetDrawFunction);
		ADD_METHOD(GetDrawFunction);
		ADD_METHOD(SetUpdateFunction);
		ADD_METHOD(SetUpdateFunctionInterval);
		ADD_METHOD(SortByDrawOrder);
		// ADD_METHOD( CustomLighting );
		ADD_METHOD(SetAmbientLightColor);
		ADD_METHOD(SetDiffuseLightColor);
		ADD_METHOD(SetSpecularLightColor);
		ADD_METHOD(SetLightDirection);
		ADD_METHOD(AddChildFromPath);
		ADD_METHOD(RemoveChild);
		ADD_METHOD(RemoveAllChildren);
	}
};

LUA_REGISTER_DERIVED_CLASS(ActorFrame, Actor)
// lua end
