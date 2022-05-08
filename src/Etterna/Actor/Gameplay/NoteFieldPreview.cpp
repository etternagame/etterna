#include "Etterna/Globals/global.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "NoteFieldPreview.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Screen/Others/Screen.h"
#include "Player.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Singletons/NoteSkinManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Singletons/ScreenManager.h"

#include <cmath>
#include <limits>
#include <RageUtil/Graphics/RageDisplay.h>

#include "Etterna/Models/NoteData/NoteDataUtil.h"

REGISTER_ACTOR_CLASS(NoteFieldPreview);

const Style*
attemptToEnsureStyle(const PlayerState* ps)
{
	// without PlayerState, 
	if (ps == nullptr)
		return nullptr;
	
	const auto* out = GAMESTATE->GetCurrentStyle(ps->m_PlayerNumber);
	if (out == nullptr) {
		GAMESTATE->SetCompatibleStylesForPlayers();
		const auto* out2 = GAMESTATE->GetCurrentStyle(ps->m_PlayerNumber);
		// this may be null or not
		return out2;
	}
	// if this is returned, it isn't null
	return out;
}

void
NoteFieldPreview::LoadFromNode(const XNode* pNode)
{
	int iDrawBefore, iDrawAfter;
	const auto b4success = pNode->GetAttrValue("DrawDistanceBeforeTargetsPixels",
						iDrawBefore);
	const auto afsuccess = pNode->GetAttrValue("DrawDistanceAfterTargetsPixels",
						iDrawAfter);

	// fall back to gameplay metrics for draw distance
	if (!b4success)
		m_iDrawDistanceBeforeTargetsPixels =
		  THEME->GetMetricI("Player", "DrawDistanceBeforeTargetsPixels");
	else
		m_iDrawDistanceBeforeTargetsPixels =
		  std::clamp(iDrawBefore, 0, std::numeric_limits<int>::max());
	if (!afsuccess)
		m_iDrawDistanceAfterTargetsPixels =
		  THEME->GetMetricI("Player", "DrawDistanceAfterTargetsPixels");
	else
		m_iDrawDistanceAfterTargetsPixels =
		  std::clamp(iDrawAfter, std::numeric_limits<int>::min(), 0);

	ReceptorArrowsYReverse =
	  THEME->GetMetricF("Player", "ReceptorArrowsYReverse");
	ReceptorArrowsYStandard =
	  THEME->GetMetricF("Player", "ReceptorArrowsYStandard");
	
	float reversePixels, noteFieldHeight;
	const auto reverseSuccess = pNode->GetAttrValue("YReverseOffsetPixels", reversePixels);
	if (reverseSuccess)
		noteFieldHeight =
		  std::clamp(reversePixels, 0.F, std::numeric_limits<float>::max());
	else {
		// for NoteField height
		// 100 is a kind of typical number
		noteFieldHeight = ReceptorArrowsYReverse - ReceptorArrowsYStandard;
	}

	if (m_pPlayerState == nullptr) {
		LuaHelpers::ReportScriptError(
		  "PlayerState was somehow null when creating NoteFieldPreview. Report "
		  "to developers.");
		return;
	}

	const auto* style = attemptToEnsureStyle(m_pPlayerState);
	if (style == nullptr) {
		LuaHelpers::ReportScriptError("GetCurrentStyle was null when creating "
									  "NoteFieldPreview. Report to developers.");
		return;
	}

	ActorFrame::LoadFromNode(pNode);

	// fall back to this basic name if no name is set on init
	if (m_sName.empty())
		SetName("NoteFieldPreview");

	// This causes crashing if mismatched upon NoteField render
	// (only happens when not loading into a 4k compatible Game)
	if (p_dummyNoteData->GetNumTracks() != style->m_iColsPerPlayer)
		p_dummyNoteData->SetNumTracks(style->m_iColsPerPlayer);
	
	Init(m_pPlayerState, noteFieldHeight, false);

	// force update current player options
	// (bad interim solution until we hand preview its own player options)
	m_pPlayerState->m_PlayerOptions.GetCurrent() =
	  m_pPlayerState->m_PlayerOptions.GetPreferred();
	m_pPlayerState->m_PlayerOptions.GetStage() =
	  m_pPlayerState->m_PlayerOptions.GetPreferred();
	m_pPlayerState->m_PlayerOptions.GetSong() =
	  m_pPlayerState->m_PlayerOptions.GetPreferred();

	// If NoteData was loaded in InitCommand, this isn't necessary
	// It would be null if not loaded in InitCommand
	if (m_pNoteData == nullptr) {
		Load(p_dummyNoteData,
			 m_iDrawDistanceAfterTargetsPixels,
			 m_iDrawDistanceBeforeTargetsPixels);
		loadedNoteDataAtLeastOnce = true;
	}
}

void
NoteFieldPreview::UpdateYReversePixels(float YReverseOffsetPixels)
{
	// requires a style to be set
	if (attemptToEnsureStyle(m_pPlayerState) == nullptr)
		return;
	
	m_fYReverseOffsetPixels = YReverseOffsetPixels;
	m_FieldRenderArgs.reverse_offset_pixels = YReverseOffsetPixels;
	for (auto& ndisplay : m_NoteDisplays) {
		if (ndisplay.second == nullptr)
			continue;
		
		for (auto c = 0;
			 c < GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber)
				   ->m_iColsPerPlayer;
			 c++)
				ndisplay.second->display[c].m_fYReverseOffsetPixels =
				  YReverseOffsetPixels;
		
		ndisplay.second->m_ReceptorArrowRow.m_fYReverseOffsetPixels =
		  YReverseOffsetPixels;
		ndisplay.second->m_GhostArrowRow.m_fYReverseOffsetPixels =
		  YReverseOffsetPixels;
	}
}

void
NoteFieldPreview::LoadNoteData(NoteData* pNoteData)
{
	// if the current and incoming NoteData pointers are the same, why load?
	if (m_pNoteData == pNoteData)
		return;

	const auto* style = attemptToEnsureStyle(m_pPlayerState);
	if (pNoteData->GetNumTracks() != style->m_iColsPerPlayer) {
		// this typically cannot happen
		Locator::getLogger()->warn(
		  "NoteFieldPreview issue: NoteData Tracks {} - Style Columns {}",
		  pNoteData->GetNumTracks(),
		  style->m_iColsPerPlayer);
	}

	// Running NoteSkin Recache will solve issues related to changing style as an end-all
	// This can't be run on init: there will be no loaded ReceptorArrowRow displays (null)
	if (loadedNoteDataAtLeastOnce &&
		m_pCurDisplay->m_ReceptorArrowRow.GetRendererCount() !=
		  pNoteData->GetNumTracks()) {
		for (auto& d : m_NoteDisplays) {
			UncacheNoteSkin(d.first);
		}
		CacheAllUsedNoteSkins();
	} else
		loadedNoteDataAtLeastOnce = true;

	// Generate some cache data structure.
	if (pNoteData != p_dummyNoteData && pNoteData != nullptr && !pNoteData->IsEmpty())
		m_pPlayerState->ResetCacheInfo(/**pNoteData*/);
	
	Load(pNoteData,
		 m_iDrawDistanceAfterTargetsPixels,
		 m_iDrawDistanceBeforeTargetsPixels);
	
	// Let everything know the NoteField loaded new NoteData
	// also pass the name of this Actor if we happen to have multiple (why would you)
	Message m("LoadedNewPreviewNoteData");
	m.SetParam("NoteField", m_sName);
	MESSAGEMAN->Broadcast(m);
}

void
NoteFieldPreview::LoadNoteData(Steps* pSteps, bool bTransform)
{
	auto* nd = new NoteData;
	*nd = pSteps->GetNoteData();

	// If the style must change to adapt to this new NoteData, do so
	const auto* style = attemptToEnsureStyle(m_pPlayerState);
	if (pSteps == GAMESTATE->m_pCurSteps && style != nullptr &&
		nd->GetNumTracks() != style->m_iColsPerPlayer) {
		GAMESTATE->SetCompatibleStylesForPlayers();
		style = GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber);
	}

	// This says it transforms the NoteData but really all it does:
	// Map NoteData to the Style it is for (usually for beat or something?)
	if (nd != nullptr && style != nullptr) {
		NoteData ndo;
		style->GetTransformedNoteDataForStyle(PLAYER_1, *nd, ndo);
		*nd = ndo;
	}

	// Transform NoteData incoming (this being only here is because only lua uses it)
	// (for now)
	if (nd != nullptr && bTransform) {
		auto* td = pSteps->GetTimingData();
		NoteDataUtil::TransformNoteData(
		  *nd,
		  *td,
		  GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent(),
		  pSteps->m_StepsType);
	}
	
	if (nd != p_NoteDataFromSteps && p_NoteDataFromSteps != nullptr)
		delete p_NoteDataFromSteps;
	p_NoteDataFromSteps = nd;

	if (nd == nullptr) {
		LoadDummyNoteData();
	} else {
		LoadNoteData(nd);
	}
}

void
NoteFieldPreview::LoadDummyNoteData()
{
	const auto* style = attemptToEnsureStyle(m_pPlayerState);
	if (style != nullptr &&
		p_dummyNoteData->GetNumTracks() != style->m_iColsPerPlayer)
		p_dummyNoteData->SetNumTracks(style->m_iColsPerPlayer);
	LoadNoteData(p_dummyNoteData);
}

void
NoteFieldPreview::UpdateDrawDistance(int aftertargetspixels, int beforetargetspixels)
{
	// These numbers must remain outside of certain bounds
	// negative only
	if (aftertargetspixels > 0)
		aftertargetspixels = 0;
	// positive only
	if (beforetargetspixels < 0)
		beforetargetspixels = 0;
	
	m_iDrawDistanceBeforeTargetsPixels = beforetargetspixels;
	m_iDrawDistanceAfterTargetsPixels = aftertargetspixels;
}

void
NoteFieldPreview::ensure_note_displays_have_skin()
{
	// slight adjustments to prevent accidentally changing the NoteDisplay

	bool skipDisplayChange = false;
	// Guarantee a display is loaded if the selected Noteskin seems (doubly)
	// empty if this does get entered, the visible Noteskin changes if not
	// already changing
	auto sNoteSkinLower =
	  m_pPlayerState->m_PlayerOptions.GetCurrent().m_sNoteSkin;
	if (sNoteSkinLower.empty()) {
		sNoteSkinLower = make_lower(
		  m_pPlayerState->m_PlayerOptions.GetPreferred().m_sNoteSkin);

		if (sNoteSkinLower.empty()) {
			sNoteSkinLower = make_lower(CommonMetrics::DEFAULT_NOTESKIN_NAME);
		}

		// force this to work whether you like it or not
		if (!NOTESKIN->DoesNoteSkinExist(sNoteSkinLower))
			sNoteSkinLower = make_lower(NOTESKIN->GetFirstWorkingNoteSkin());

		CacheNoteSkin(sNoteSkinLower);
		
		// if we already have a display loaded with the "correct" number of
		// columns, don't change needlessly
		if (m_pDisplays != nullptr &&
			m_NoteDisplays[sNoteSkinLower]
				->m_ReceptorArrowRow.GetReceptorCount() ==
			  m_pDisplays->m_ReceptorArrowRow.GetReceptorCount())
			skipDisplayChange = true;
	}

	sNoteSkinLower = make_lower(sNoteSkinLower);
	auto it = m_NoteDisplays.find(sNoteSkinLower);
	if (it == m_NoteDisplays.end()) {
		CacheAllUsedNoteSkins();
		it = m_NoteDisplays.find(sNoteSkinLower);
		ASSERT_M(
		  it != m_NoteDisplays.end(),
		  ssprintf("iterator != m_NoteDisplays.end() [sNoteSkinLower = %s]",
				   sNoteSkinLower.c_str()));
	}
	
	if (!skipDisplayChange)
		m_pDisplays = it->second;
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
	p_NoteDataFromSteps = nullptr;

	// This is not guaranteed to be non-null!
	m_pPlayerState = GAMESTATE->m_pPlayerState;

	// There's a few rare conditions that are necessary to use this
	// Primarily for ScreenSelectMusic
	auto* scrn = SCREENMAN->GetTopScreen();
	if (scrn != nullptr)
		scrn->b_PreviewNoteFieldIsActive = true;
}

NoteFieldPreview::~NoteFieldPreview()
{
	delete p_dummyNoteData;
	delete p_NoteDataFromSteps;
}

void
NoteFieldPreview::Update(float fDeltaTime)
{
	if (!this->GetVisible() || !GAMESTATE->m_pCurSong ||
		m_pCurDisplay == nullptr)
		return;
	NoteField::Update(fDeltaTime);
}

void
NoteFieldPreview::DrawPrimitives()
{
	if (m_pCurDisplay == nullptr)
		return;
	
	const auto original_y = GetY();
	// Use the same logic of the PlayerNoteFieldPositioner here
	// This will make the NoteField adapt to the PlayerState PlayerOptions
	// Specifically, the Perspective options.
	// If false, no more updates. Have to fix it yourself.
	if (poseNoteField) {
		if (m_pPlayerState != nullptr) {
			const auto& curr_options =
			  m_pPlayerState->m_PlayerOptions.GetCurrent();
			const auto reverse =
			  curr_options.GetReversePercentForColumn(0) > 0.5F;
			const auto tilt = curr_options.m_fPerspectiveTilt;
			const auto reverse_mult = (reverse ? -1 : 1);
			const auto tilt_degrees =
			  SCALE(tilt, -1.F, +1.F, +30, -30) * reverse_mult;

			// for mini (HACK HACK HACK)
			// zooming notefield freely is not possible if mini is taken from PO
			// allow setting a constant mini separate from all PO
			// (ideal solution: set a preview specific PlayerOptions)
			auto mini = curr_options.m_fEffects[PlayerOptions::EFFECT_MINI];
			if (usingConstantMini)
				mini = constantMini;

			const auto x = GetX();
			const auto skew = curr_options.m_fSkew;
			const auto center_y =
			  original_y +
			  (ReceptorArrowsYReverse + ReceptorArrowsYStandard) / 2;

			// emulate Player::PushPlayerMatrix
			Player::PushPlayerMatrix(x, skew, center_y);
			
			if (tilt > 0) {
				SetY(original_y + SCALE(tilt, 0.F, 1.F, 0.F, -45.F) * reverse_mult);
				SetZoom(SCALE(mini, 0.F, 1.F, 1.F, 0.5F) *
						SCALE(tilt, 0.F, 1.F, 1.F, 0.9F));
			}
			else {
				SetY(original_y + SCALE(tilt, 0.F, -1.F, 0.F, -20.F) * reverse_mult);
				SetZoom(SCALE(mini, 0.F, 1.F, 1.F, 0.5F) *
						SCALE(tilt, 0.F, -1.F, 1.F, 0.9F));
			}
			SetRotationX(tilt_degrees);
		}
	}
	
	NoteField::DrawPrimitives();

	// clean up the emulation of PlayerNoteFieldPositioner
	// this happens in its destructor
	if (poseNoteField) {
		if (m_pPlayerState != nullptr) {
			SetY(original_y);
			Player::PopPlayerMatrix();
		}
	}
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
	static int UpdateYReversePixels(T* p, lua_State* L)
	{
		auto i = FArg(1);

		p->UpdateYReversePixels(i);
		
		COMMON_RETURN_SELF;
	}
	static int LoadNoteData(T* p, lua_State* L)
	{
		auto* s = Luna<Steps>::check(L, 1);
		auto transform = false;
		if (!lua_isnoneornil(L, 2)) {
			transform = BArg(2);
		}
		p->LoadNoteData(s, transform);
		COMMON_RETURN_SELF;
	}
	static int LoadDummyNoteData(T* p, lua_State* L)
	{
		p->LoadDummyNoteData();
		COMMON_RETURN_SELF;
	}
	static int SetFollowPlayerOptions(T* p, lua_State* L)
	{
		auto b = BArg(1);
		p->SetPoseNoteField(b);
		COMMON_RETURN_SELF;
	}
	static int SetConstantMini(T* p, lua_State* L)
	{
		auto f = FArg(1);
		p->SetConstantMini(f);
		COMMON_RETURN_SELF;
	}
	static int ResetConstantMini(T* p, lua_State* L)
	{
		p->ResetConstantMini();
		COMMON_RETURN_SELF;
	}
	
	LunaNoteFieldPreview()
	{
		ADD_METHOD(UpdateDrawDistance);
		ADD_METHOD(UpdateYReversePixels);
		ADD_METHOD(LoadNoteData);
		ADD_METHOD(LoadDummyNoteData);
		ADD_METHOD(SetFollowPlayerOptions);
		ADD_METHOD(SetConstantMini);
		ADD_METHOD(ResetConstantMini);
	}
};

LUA_REGISTER_DERIVED_CLASS(NoteFieldPreview, NoteField)
