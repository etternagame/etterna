#include "Etterna/Globals/global.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "NoteFieldPreview.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"

#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/ThemeManager.h"

REGISTER_ACTOR_CLASS(NoteFieldPreview);

void
NoteFieldPreview::LoadFromNode(const XNode* pNode)
{
	ActorFrame::LoadFromNode(pNode);

	// fall back to this basic name if no name is set on init
	if (m_sName.empty())
		SetName("NoteFieldPreview");

	auto b4success = pNode->GetAttrValue("DrawDistanceBeforeTargetsPixels",
						m_iDrawDistanceBeforeTargetsPixels);
	auto afsuccess = pNode->GetAttrValue("DrawDistanceAfterTargetsPixels",
						m_iDrawDistanceAfterTargetsPixels);

	// fall back to gameplay defaults for draw distance
	if (!b4success)
		m_iDrawDistanceBeforeTargetsPixels =
		  THEME->GetMetricI("Player", "DrawDistanceBeforeTargetsPixels");
	if (!afsuccess)
		m_iDrawDistanceAfterTargetsPixels =
		  THEME->GetMetricI("Player", "DrawDistanceAfterTargetsPixels");

	// for NoteField height
	// 100 is a kind of typical number
	float yReverse = THEME->GetMetricF("Player", "ReceptorArrowsYReverse");
	float yStandard = THEME->GetMetricF("Player", "ReceptorArrowsYStandard");
	float noteFieldHeight = yReverse - yStandard;

	m_pPlayerState = GAMESTATE->m_pPlayerState;
	if (m_pPlayerState == nullptr) {
		LuaHelpers::ReportScriptError(
		  "PlayerState was somehow null when creating NoteFieldPreview. Report "
		  "to developers.");
		return;
	}

	if (GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber) == nullptr) {
		LuaHelpers::ReportScriptError("GetCurrentStyle was null when creating "
									  "NoteFieldPreview. Report to developers.");
		return;
	}

	Init(m_pPlayerState, noteFieldHeight);
	Load(p_dummyNoteData,
		 m_iDrawDistanceAfterTargetsPixels,
		 m_iDrawDistanceBeforeTargetsPixels);
}

void
NoteFieldPreview::LoadNoteData(NoteData* pNoteData)
{
	Load(pNoteData,
		 m_iDrawDistanceAfterTargetsPixels,
		 m_iDrawDistanceBeforeTargetsPixels);
}

void
NoteFieldPreview::LoadNoteData(Steps* pSteps)
{
	auto* nd = new NoteData;
	*nd = pSteps->GetNoteData();
	LoadNoteData(nd);
}

void
NoteFieldPreview::UpdateDrawDistance(int aftertargetspixels, int beforetargetspixels)
{
	// These numbers must remain outside of certain bounds
	if (aftertargetspixels > 0)
		aftertargetspixels = -1;
	if (beforetargetspixels < 0)
		beforetargetspixels = 1;
	
	m_iDrawDistanceBeforeTargetsPixels = beforetargetspixels;
	m_iDrawDistanceAfterTargetsPixels = aftertargetspixels;
}

NoteFieldPreview::NoteFieldPreview()
{
	// Start off each NoteFieldPreview with an empty NoteData
	// 4 tracks is cool because it works most of the time
	// but make sure to update when changing style
	auto* nd = new NoteData;
	nd->Init();
	nd->SetNumTracks(4);
	
	p_dummyNoteData = nd;
}

NoteFieldPreview::~NoteFieldPreview()
{
	delete p_dummyNoteData;
}


class LunaNoteFieldPreview : public Luna<NoteFieldPreview>
{
  public:

	static int UpdateDrawDistance(T* p, lua_State* L)
	{
		auto after = IArg(1);
		auto before = IArg(2);

		p->UpdateDrawDistance(after, before);
		
		COMMON_RETURN_SELF;	
	}
	static int LoadNoteData(T* p, lua_State* L)
	{
		auto* s = Luna<Steps>::check(L, 1);
		p->LoadNoteData(s);
		COMMON_RETURN_SELF;
	}
	
	LunaNoteFieldPreview()
	{
		ADD_METHOD(UpdateDrawDistance);
		ADD_METHOD(LoadNoteData);
	}
};

LUA_REGISTER_DERIVED_CLASS(NoteFieldPreview, NoteField)
