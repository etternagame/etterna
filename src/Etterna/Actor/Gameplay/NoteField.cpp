#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Gameplay/ArrowEffects.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "NoteField.h"
#include "Etterna/Singletons/NoteSkinManager.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Misc/RageMath.h"
#include "RageUtil/Misc/RageTimer.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Singletons/ThemeManager.h"

#include <algorithm>

#include "Etterna/Models/Misc/CommonMetrics.h"

void
FindDisplayedBeats(const PlayerState* pPlayerState,
				   float& firstBeat,
				   float& lastBeat,
				   int iDrawDistanceAfterTargetsPixels,
				   int iDrawDistanceBeforeTargetsPixels);

static ThemeMetric<bool> SHOW_BOARD("NoteField", "ShowBoard");
static ThemeMetric<bool> SHOW_BEAT_BARS("NoteField", "ShowBeatBars");
static ThemeMetric<float> FADE_BEFORE_TARGETS_PERCENT(
  "NoteField",
  "FadeBeforeTargetsPercent");
static ThemeMetric<float> BAR_MEASURE_ALPHA("NoteField", "BarMeasureAlpha");
static ThemeMetric<float> BAR_4TH_ALPHA("NoteField", "Bar4thAlpha");
static ThemeMetric<float> BAR_8TH_ALPHA("NoteField", "Bar8thAlpha");
static ThemeMetric<float> BAR_16TH_ALPHA("NoteField", "Bar16thAlpha");
static ThemeMetric<float> FADE_FAIL_TIME("NoteField", "FadeFailTime");
static Preference<float> ihatethisprogram("NotefieldXRatio",
										  1.f); // i dont really know the best
												// place to put this offhand so
												// this can be temporary - mina

NoteField::NoteField()
{
	m_pNoteData = nullptr;
	m_pCurDisplay = nullptr;
	m_drawing_board_primitive = false;

	m_textMeasureNumber.LoadFromFont(
	  THEME->GetPathF("NoteField", "MeasureNumber"));
	m_textMeasureNumber.SetZoom(1.0f);
	m_textMeasureNumber.SetShadowLength(2);
	m_textMeasureNumber.SetWrapWidthPixels(300);

	m_rectMarkerBar.SetEffectDiffuseShift(
	  2, RageColor(1, 1, 1, 0.5f), RageColor(0.5f, 0.5f, 0.5f, 0.5f));

	m_sprBoard.Load(THEME->GetPathG("NoteField", "board"));
	m_sprBoard->SetName("Board");
	m_sprBoard->PlayCommand("On");
	this->AddChild(m_sprBoard);

	m_fBoardOffsetPixels = 0;
	m_fCurrentBeatLastUpdate = -1;
	m_fYPosCurrentBeatLastUpdate = -1;
	this->SubscribeToMessage(Message_CurrentSongChanged);

	m_sprBeatBars.Load(THEME->GetPathG("NoteField", "bars"));
	m_sprBeatBars.StopAnimating();

	// I decided to do it this way because I don't want to dig through
	// ScreenEdit to change all the places it touches the markers. -Kyz
	m_FieldRenderArgs.selection_begin_marker = &m_iBeginMarker;
	m_FieldRenderArgs.selection_end_marker = &m_iEndMarker;
	m_iBeginMarker = m_iEndMarker = -1;

	m_FieldRenderArgs.fail_fade = -1;

	m_StepCallback.SetFromNil();
	m_SetPressedCallback.SetFromNil();
	m_DidTapNoteCallback.SetFromNil();
	m_DidHoldNoteCallback.SetFromNil();
}

NoteField::~NoteField()
{
	Unload();
}

void
NoteField::Unload()
{
	for (auto& m_NoteDisplay : m_NoteDisplays)
		delete m_NoteDisplay.second;
	m_NoteDisplays.clear();
	m_pCurDisplay = nullptr;
	m_pDisplays = nullptr;
}

void
NoteField::CacheNoteSkin(const std::string& sNoteSkin_)
{
	if (m_NoteDisplays.find(sNoteSkin_) != m_NoteDisplays.end())
		return;

	LockNoteSkin l(sNoteSkin_);

	if (PREFSMAN->m_verbose_log > 1)
		Locator::getLogger()->trace("NoteField::CacheNoteSkin: cache {}", sNoteSkin_.c_str());
	auto* nd = new NoteDisplayCols(
	  GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber)
		->m_iColsPerPlayer);

	for (auto c = 0;
		 c < GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber)
			   ->m_iColsPerPlayer;
		 c++)
		nd->display[c].Load(c, m_pPlayerState, m_fYReverseOffsetPixels);
	nd->m_ReceptorArrowRow.Load(m_pPlayerState, m_fYReverseOffsetPixels);
	nd->m_GhostArrowRow.Load(m_pPlayerState, m_fYReverseOffsetPixels);

	m_NoteDisplays[sNoteSkin_] = nd;
}

void
NoteField::UncacheNoteSkin(const std::string& sNoteSkin_)
{
	Locator::getLogger()->trace("NoteField::CacheNoteSkin: release {}", sNoteSkin_.c_str());
	ASSERT_M(m_NoteDisplays.find(sNoteSkin_) != m_NoteDisplays.end(),
			 sNoteSkin_);
	delete m_NoteDisplays[sNoteSkin_];
	m_NoteDisplays.erase(sNoteSkin_);
}

void
NoteField::CacheAllUsedNoteSkins()
{
	/* Cache all note skins that we might need for the whole song, course or
	 * battle
	 * play, so we don't have to load them later (such as between course songs).
	 */
	vector<std::string> asSkinsLower;
	GAMESTATE->GetAllUsedNoteSkins(asSkinsLower);
	asSkinsLower.push_back(
	  m_pPlayerState->m_PlayerOptions.GetStage().m_sNoteSkin);
	for (auto& s : asSkinsLower) {
		NOTESKIN->ValidateNoteSkinName(s);
		s = make_lower(s);
	}

	for (auto& i : asSkinsLower) {
		CacheNoteSkin(i);
	}

	/* If we're changing note skins in the editor, we can have old note skins
	 * lying around.  Remove them so they don't accumulate. */
	std::set<std::string> setNoteSkinsToUnload;
	for (auto& d : m_NoteDisplays) {
		const auto unused =
		  find(asSkinsLower.begin(), asSkinsLower.end(), d.first) ==
		  asSkinsLower.end();
		if (unused)
			setNoteSkinsToUnload.insert(d.first);
	}
	for (const auto& s : setNoteSkinsToUnload) {
		UncacheNoteSkin(s);
	}

	auto sCurrentNoteSkinLower =
	  m_pPlayerState->m_PlayerOptions.GetCurrent().m_sNoteSkin;
	NOTESKIN->ValidateNoteSkinName(sCurrentNoteSkinLower);
	sCurrentNoteSkinLower = make_lower(sCurrentNoteSkinLower);

	auto it = m_NoteDisplays.find(sCurrentNoteSkinLower);
	ASSERT_M(it != m_NoteDisplays.end(), sCurrentNoteSkinLower);
	m_pCurDisplay = it->second;
	
	m_pDisplays = it->second;

	// I don't think this is needed?
	// It's done in Load -- Nick12
	// InitColumnRenderers();
}

void
NoteField::Init(const PlayerState* pPlayerState,
				float fYReverseOffsetPixels,
				bool use_states_zoom)
{
	m_pPlayerState = pPlayerState;
	m_fYReverseOffsetPixels = fYReverseOffsetPixels;
	CacheAllUsedNoteSkins();
	// Design change:  Instead of having a flag in the style that toggles a
	// fixed zoom that is only applied to the columns, ScreenGameplay now
	// calculates a zoom factor to apply to the notefield and puts it in the
	// PlayerState. -Kyz
	// use_states_zoom flag exists because edit mode has to set its own special
	// zoom factor. -Kyz
	if (use_states_zoom) {
		SetZoom(pPlayerState->m_NotefieldZoom);
	}
	// Pass the player state info down to children so that they can set
	// per-player things.  For example, if a screen filter is in the notefield
	// board, this tells it what player it's for. -Kyz
	Message msg("PlayerStateSet");
	msg.SetParam("PlayerNumber", pPlayerState->m_PlayerNumber);
	HandleMessage(msg);

	SetBaseZoomX(ihatethisprogram);
}

void
NoteField::Load(const NoteData* pNoteData,
				int iDrawDistanceAfterTargetsPixels,
				int iDrawDistanceBeforeTargetsPixels)
{
	ASSERT(pNoteData != nullptr);
	m_pNoteData = pNoteData;
	m_iDrawDistanceAfterTargetsPixels = iDrawDistanceAfterTargetsPixels;
	m_iDrawDistanceBeforeTargetsPixels = iDrawDistanceBeforeTargetsPixels;
	ASSERT(m_iDrawDistanceBeforeTargetsPixels >=
		   m_iDrawDistanceAfterTargetsPixels);

	m_FieldRenderArgs.fail_fade = -1;

	// int i1 = m_pNoteData->GetNumTracks();
	// int i2 =
	// GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber)->m_iColsPerPlayer;
	ASSERT_M(m_pNoteData->GetNumTracks() ==
			   GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber)
				 ->m_iColsPerPlayer,
			 ssprintf("NumTracks %d = ColsPerPlayer %d",
					  m_pNoteData->GetNumTracks(),
					  GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber)
						->m_iColsPerPlayer));

	ensure_note_displays_have_skin();
	InitColumnRenderers();
}

void
NoteField::ensure_note_displays_have_skin()
{
	// The NoteSkin may have changed at the beginning of a new course song.
	auto sNoteSkinLower =
	  m_pPlayerState->m_PlayerOptions.GetCurrent().m_sNoteSkin;

	// Guarantee a display is loaded if the selected Noteskin seems (doubly) empty
	// if this does get entered, the visible Noteskin changes if not already changing
	if (sNoteSkinLower.empty()) {
		sNoteSkinLower = make_lower(
		  m_pPlayerState->m_PlayerOptions.GetPreferred().m_sNoteSkin);

		if (sNoteSkinLower.empty()) {
			sNoteSkinLower = make_lower(CommonMetrics::DEFAULT_NOTESKIN_NAME);
		}
		
		CacheNoteSkin(sNoteSkinLower);
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
	m_pDisplays = it->second;
}

void
NoteField::InitColumnRenderers()
{
	m_FieldRenderArgs.player_state = m_pPlayerState;
	m_FieldRenderArgs.reverse_offset_pixels = m_fYReverseOffsetPixels;
	m_FieldRenderArgs.receptor_row = &(m_pCurDisplay->m_ReceptorArrowRow);
	m_FieldRenderArgs.ghost_row = &(m_pCurDisplay->m_GhostArrowRow);
	m_FieldRenderArgs.note_data = m_pNoteData;
	m_ColumnRenderers.resize(
	  GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber)
		->m_iColsPerPlayer);
	for (size_t ncr = 0; ncr < m_ColumnRenderers.size(); ++ncr) {
		m_ColumnRenderers[ncr].m_displays[PLAYER_1] =
		  &(m_pDisplays->display[ncr]);
		m_ColumnRenderers[ncr].m_displays[PLAYER_INVALID] =
		  &(m_pCurDisplay->display[ncr]);
		m_ColumnRenderers[ncr].m_column = ncr;
		m_ColumnRenderers[ncr].m_column_render_args.column = ncr;
		m_ColumnRenderers[ncr].m_field_render_args = &m_FieldRenderArgs;
	}
	m_pCurDisplay->m_ReceptorArrowRow.SetColumnRenderers(m_ColumnRenderers);
	m_pCurDisplay->m_GhostArrowRow.SetColumnRenderers(m_ColumnRenderers);
}

void
NoteField::Update(float fDeltaTime)
{
	if (!this->GetVisible() || !GAMESTATE->m_pCurSong)
		return;

	if (m_bFirstUpdate) {
		m_pCurDisplay->m_ReceptorArrowRow.PlayCommand("On");
	}

	ActorFrame::Update(fDeltaTime);
	ArrowEffects::SetCurrentOptions(
	  &m_pPlayerState->m_PlayerOptions.GetCurrent());

	for (auto& m_ColumnRenderer : m_ColumnRenderers) {
		m_ColumnRenderer.Update(fDeltaTime);
	}

	// update m_fBoardOffsetPixels, m_fCurrentBeatLastUpdate,
	// m_fYPosCurrentBeatLastUpdate
	const auto fCurrentBeat = GAMESTATE->m_Position.m_fSongBeat;
	const auto bTweeningOn =
	  m_sprBoard->GetCurrentDiffuseAlpha() >= 0.98 &&
	  m_sprBoard->GetCurrentDiffuseAlpha() < 1.00; // HACK
	if (!bTweeningOn && m_fCurrentBeatLastUpdate != -1) {
		const auto fYOffsetLast =
		  ArrowEffects::GetYOffset(m_pPlayerState, 0, m_fCurrentBeatLastUpdate);
		const auto fYPosLast =
		  ArrowEffects::GetYPos(0, fYOffsetLast, m_fYReverseOffsetPixels);
		const auto fPixelDifference = fYPosLast - m_fYPosCurrentBeatLastUpdate;

		// LOG->Trace( "speed = %f, %f, %f, %f, %f, %f", fSpeed,
		// fYOffsetAtCurrent, fYOffsetAtNext, fSecondsAtCurrent, fSecondsAtNext,
		// fPixelDifference, fSecondsDifference );

		m_fBoardOffsetPixels += fPixelDifference;
		wrap(m_fBoardOffsetPixels, m_sprBoard->GetUnzoomedHeight());
	}
	m_fCurrentBeatLastUpdate = fCurrentBeat;
	const auto fYOffsetCurrent =
	  ArrowEffects::GetYOffset(m_pPlayerState, 0, m_fCurrentBeatLastUpdate);
	m_fYPosCurrentBeatLastUpdate =
	  ArrowEffects::GetYPos(0, fYOffsetCurrent, m_fYReverseOffsetPixels);

	m_rectMarkerBar.Update(fDeltaTime);

	auto* cur = m_pCurDisplay;

	cur->m_ReceptorArrowRow.Update(fDeltaTime);
	cur->m_GhostArrowRow.Update(fDeltaTime);

	if (m_FieldRenderArgs.fail_fade >= 0)
		m_FieldRenderArgs.fail_fade = std::min(
		  m_FieldRenderArgs.fail_fade + fDeltaTime / FADE_FAIL_TIME, 1.F);

	// Update fade to failed
	m_pCurDisplay->m_ReceptorArrowRow.SetFadeToFailPercent(
	  m_FieldRenderArgs.fail_fade);

	/* No idea what this is supposed to be doing but it seems to be doing an
	awful lot of absolutely nothing at an awfully quick rate. Noteskins are
	fine... 3d noteskins are fine... mod maps are fine. Welp - Mina */

	// NoteDisplay::Update(fDeltaTime);
	/* Update all NoteDisplays. Hack: We need to call this once per frame, not
	 * once per player. */
	// TODO: Remove use of PlayerNumber.

	const auto pn = m_pPlayerState->m_PlayerNumber;
	if (pn == GAMESTATE->GetMasterPlayerNumber())
		NoteDisplay::Update(fDeltaTime);
}

float
NoteField::GetWidth() const
{
	const auto* const pStyle =
	  GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber);
	float fMinX, fMaxX;
	// TODO: Remove use of PlayerNumber.
	pStyle->GetMinAndMaxColX(m_pPlayerState->m_PlayerNumber, fMinX, fMaxX);

	const auto fYZoom = ArrowEffects::GetZoom(m_pPlayerState);
	return (fMaxX - fMinX + ARROW_SIZE) * fYZoom;
}

void
NoteField::DrawBeatBar(const float fBeat, BeatBarType type, int iMeasureIndex)
{
	const auto bIsMeasure = type == measure;

	const auto fYOffset = ArrowEffects::GetYOffset(m_pPlayerState, 0, fBeat);
	const auto fYPos =
	  ArrowEffects::GetYPos(0, fYOffset, m_fYReverseOffsetPixels);

	float fAlpha;
	int iState;

	if (bIsMeasure) {
		fAlpha = BAR_MEASURE_ALPHA;
		iState = 0;
	} else {
		auto const& curr_ops = m_pPlayerState->m_PlayerOptions.GetCurrent();
		auto fScrollSpeed = curr_ops.m_fScrollSpeed;
		if (curr_ops.m_fTimeSpacing > 0) {
			fScrollSpeed = 4;
		} else if (curr_ops.m_fMaxScrollBPM != 0) {
			fScrollSpeed =
			  curr_ops.m_fMaxScrollBPM / m_pPlayerState->m_fReadBPM;
		}
		switch (type) {
			DEFAULT_FAIL(type);
			case measure: // handled above
			case beat:	  // fall through
				fAlpha = BAR_4TH_ALPHA;
				iState = 1;
				break;
			case half_beat:
				fAlpha = SCALE(fScrollSpeed, 1.0f, 2.0f, 0.0f, BAR_8TH_ALPHA);
				iState = 2;
				break;
			case quarter_beat:
				fAlpha = SCALE(fScrollSpeed, 2.0f, 4.0f, 0.0f, BAR_16TH_ALPHA);
				iState = 3;
				break;
		}
		CLAMP(fAlpha, 0, 1);
	}

	const auto fWidth = GetWidth();
	const auto fFrameWidth = m_sprBeatBars.GetUnzoomedWidth();

	m_sprBeatBars.SetX(0);
	m_sprBeatBars.SetY(fYPos);
	m_sprBeatBars.SetDiffuse(RageColor(1, 1, 1, fAlpha));
	m_sprBeatBars.SetState(iState);
	m_sprBeatBars.SetCustomTextureRect(
	  RectF(0,
			SCALE(iState, 0.f, 4.f, 0.f, 1.f),
			fWidth / fFrameWidth,
			SCALE(iState + 1, 0.f, 4.f, 0.f, 1.f)));
	m_sprBeatBars.SetZoomX(fWidth / m_sprBeatBars.GetUnzoomedWidth());
	m_sprBeatBars.Draw();
}

void
NoteField::DrawBoard(int iDrawDistanceAfterTargetsPixels,
					 int iDrawDistanceBeforeTargetsPixels)
{
	// todo: make this an AutoActor instead? -aj
	auto* pSprite = dynamic_cast<Sprite*>(static_cast<Actor*>(m_sprBoard));
	if (pSprite == nullptr) {
		m_sprBoard->Draw();
	} else {
		// Draw the board centered on fYPosAt0 so that the board doesn't slide
		// as the draw distance changes with modifiers.
		const auto fYPosAt0 =
		  ArrowEffects::GetYPos(0, 0, m_fYReverseOffsetPixels);

		auto rect = *pSprite->GetCurrentTextureCoordRect();
		const auto fBoardGraphicHeightPixels = pSprite->GetUnzoomedHeight();
		const auto fTexCoordOffset =
		  m_fBoardOffsetPixels / fBoardGraphicHeightPixels;

		// top half
		const auto fHeight =
		  iDrawDistanceBeforeTargetsPixels - iDrawDistanceAfterTargetsPixels;
		const auto fY = fYPosAt0 - ((iDrawDistanceBeforeTargetsPixels +
									 iDrawDistanceAfterTargetsPixels) /
									2.0f);

		pSprite->ZoomToHeight(static_cast<float>(fHeight));
		pSprite->SetY(fY);

		// handle tex coord offset and fade
		rect.top = -fTexCoordOffset - (iDrawDistanceBeforeTargetsPixels /
									   fBoardGraphicHeightPixels);
		rect.bottom = -fTexCoordOffset + (-iDrawDistanceAfterTargetsPixels /
										  fBoardGraphicHeightPixels);
		pSprite->SetCustomTextureRect(rect);
		const auto fFadeTop =
		  FADE_BEFORE_TARGETS_PERCENT * iDrawDistanceBeforeTargetsPixels /
		  (iDrawDistanceBeforeTargetsPixels - iDrawDistanceAfterTargetsPixels);
		pSprite->SetFadeTop(fFadeTop);
		pSprite->Draw();
	}
}

void
NoteField::DrawMarkerBar(int iBeat)
{
	const auto fBeat = NoteRowToBeat(iBeat);
	const auto fYOffset = ArrowEffects::GetYOffset(m_pPlayerState, 0, fBeat);
	const auto fYPos =
	  ArrowEffects::GetYPos(0, fYOffset, m_fYReverseOffsetPixels);

	m_rectMarkerBar.StretchTo(RectF(-GetWidth() / 2,
									fYPos - ARROW_SIZE / 2,
									GetWidth() / 2,
									fYPos + ARROW_SIZE / 2));
	m_rectMarkerBar.Draw();
}

static ThemeMetric<RageColor> AREA_HIGHLIGHT_COLOR("NoteField",
												   "AreaHighlightColor");
void
NoteField::DrawAreaHighlight(int iStartBeat, int iEndBeat)
{
	const auto fStartBeat = NoteRowToBeat(iStartBeat);
	const auto fEndBeat = NoteRowToBeat(iEndBeat);
	const auto fDrawDistanceAfterTargetsPixels =
	  ArrowEffects::GetYOffset(m_pPlayerState, 0, fStartBeat);
	const auto fYStartPos = ArrowEffects::GetYPos(
	  0, fDrawDistanceAfterTargetsPixels, m_fYReverseOffsetPixels);
	const auto fDrawDistanceBeforeTargetsPixels =
	  ArrowEffects::GetYOffset(m_pPlayerState, 0, fEndBeat);
	const auto fYEndPos = ArrowEffects::GetYPos(
	  0, fDrawDistanceBeforeTargetsPixels, m_fYReverseOffsetPixels);

	// The caller should have clamped these to reasonable values
	ASSERT(fYStartPos > -1000);
	ASSERT(fYEndPos < +5000);

	m_rectAreaHighlight.StretchTo(
	  RectF(-GetWidth() / 2, fYStartPos, GetWidth() / 2, fYEndPos));
	m_rectAreaHighlight.SetDiffuse(AREA_HIGHLIGHT_COLOR);
	m_rectAreaHighlight.Draw();
}

void
NoteField::set_text_measure_number_for_draw(const float beat,
											const float side_sign,
											float x_offset,
											const float horiz_align,
											const RageColor& color,
											const RageColor& glow)
{
	const auto y_offset = ArrowEffects::GetYOffset(m_pPlayerState, 0, beat);
	const auto y_pos =
	  ArrowEffects::GetYPos(0, y_offset, m_fYReverseOffsetPixels);
	const auto zoom = ArrowEffects::GetZoom(m_pPlayerState);
	const auto x_base = GetWidth() * .5f;
	x_offset *= zoom;

	m_textMeasureNumber.SetZoom(zoom);
	m_textMeasureNumber.SetHorizAlign(horiz_align);
	m_textMeasureNumber.SetDiffuse(color);
	m_textMeasureNumber.SetGlow(glow);
	m_textMeasureNumber.SetXY((x_offset + x_base) * side_sign, y_pos);
}

void
NoteField::draw_timing_segment_text(const std::string& text,
									const float beat,
									const float side_sign,
									float x_offset,
									const float horiz_align,
									const RageColor& color,
									const RageColor& glow)
{
	set_text_measure_number_for_draw(
	  beat, side_sign, x_offset, horiz_align, color, glow);
	m_textMeasureNumber.SetText(text);
	m_textMeasureNumber.Draw();
}

void
NoteField::DrawBGChangeText(const float beat,
							const std::string& new_bg_name,
							const RageColor& glow)
{
	set_text_measure_number_for_draw(
	  beat, 1, 0, align_left, RageColor(0, 1, 0, 1), glow);
	m_textMeasureNumber.SetText(new_bg_name);
	m_textMeasureNumber.Draw();
}

void
FindDisplayedBeats(const PlayerState* pPlayerState,
				   float& firstBeat,
				   float& lastBeat,
				   int iDrawDistanceAfterTargetsPixels,
				   int iDrawDistanceBeforeTargetsPixels)
{
	auto fFirstBeatToDraw = GAMESTATE->m_Position.m_fSongBeatVisible;
	auto fLastBeatToDraw = fFirstBeatToDraw;
	const auto fSpeedMultiplier =
	  pPlayerState->GetDisplayedTiming().GetDisplayedSpeedPercent(
		GAMESTATE->m_Position.m_fSongBeatVisible,
		GAMESTATE->m_Position.m_fMusicSecondsVisible);

	bool bBoomerang;
	{
		const auto* const fAccels =
		  pPlayerState->m_PlayerOptions.GetCurrent().m_fAccels;
		bBoomerang = (fAccels[PlayerOptions::ACCEL_BOOMERANG] != 0);
	}

	// Search for the draw distance pixels
	float fSearchDistance = 10;
	const auto NUM_ITERATIONS = 20;

	// the imaginary line to start drawing "until" the receptor
	for (auto i = 0; i < NUM_ITERATIONS; i++) {
		bool bIsPastPeakYOffset;
		float fPeakYOffset;
		const auto fYOffset = ArrowEffects::GetYOffset(pPlayerState,
													   0,
													   fLastBeatToDraw,
													   fPeakYOffset,
													   bIsPastPeakYOffset,
													   true);

		if (bBoomerang && !bIsPastPeakYOffset)
			fLastBeatToDraw += fSearchDistance;
		else if (fYOffset > iDrawDistanceBeforeTargetsPixels) // off screen
			fLastBeatToDraw -= fSearchDistance;
		else // on screen
			fLastBeatToDraw += fSearchDistance;

		fSearchDistance /= 2;
	}

	fSearchDistance = 10;
	// the imaginary line to start drawing "after" the receptor
	for (auto i = 0; i < NUM_ITERATIONS; i++) {
		bool bIsPastPeakYOffset;
		float fPeakYOffset;
		const auto fYOffset = ArrowEffects::GetYOffset(pPlayerState,
													   0,
													   fFirstBeatToDraw,
													   fPeakYOffset,
													   bIsPastPeakYOffset,
													   true);

		if (bBoomerang && !bIsPastPeakYOffset)
			fFirstBeatToDraw -= fSearchDistance;
		else if (fYOffset < iDrawDistanceAfterTargetsPixels) // off screen
			fFirstBeatToDraw += fSearchDistance;
		else // on screen
			fFirstBeatToDraw -= fSearchDistance;

		fSearchDistance /= 2;
	}

	if (fSpeedMultiplier < 0.75f) {
		fFirstBeatToDraw =
		  std::min(fFirstBeatToDraw, GAMESTATE->m_Position.m_fSongBeat + 16);
		fLastBeatToDraw =
		  std::min(fLastBeatToDraw, GAMESTATE->m_Position.m_fSongBeat + 16);
	}

	firstBeat = fFirstBeatToDraw;
	lastBeat = fLastBeatToDraw;
}

void
NoteField::CalcPixelsBeforeAndAfterTargets()
{
	const auto& curr_options = m_pPlayerState->m_PlayerOptions.GetCurrent();
	// Adjust draw range depending on some effects
	m_FieldRenderArgs.draw_pixels_after_targets =
	  static_cast<float>(m_iDrawDistanceAfterTargetsPixels);
	// HACK: If boomerang and centered are on, then we want to draw much
	// earlier so that the notes don't pop on screen.
	const auto centered_times_boomerang =
	  curr_options.m_fScrolls[PlayerOptions::SCROLL_CENTERED] *
	  curr_options.m_fAccels[PlayerOptions::ACCEL_BOOMERANG];
	m_FieldRenderArgs.draw_pixels_after_targets += static_cast<int>(
	  SCALE(centered_times_boomerang, 0.f, 1.f, 0.f, -SCREEN_HEIGHT / 2));
	m_FieldRenderArgs.draw_pixels_before_targets =
	  static_cast<float>(m_iDrawDistanceBeforeTargetsPixels);

	float draw_scale = 1;
	draw_scale *= 1 + 0.5f * fabsf(curr_options.m_fPerspectiveTilt);
	draw_scale *=
	  1 + fabsf(curr_options.m_fEffects[PlayerOptions::EFFECT_MINI]);

	m_FieldRenderArgs.draw_pixels_after_targets *= draw_scale;
	m_FieldRenderArgs.draw_pixels_before_targets *= draw_scale;
}

void
NoteField::DrawPrimitives()
{
	if (!this->GetVisible() || !GAMESTATE->m_pCurSong)
		return;

	// This should be filled in on the first update.
	ASSERT(m_pCurDisplay != NULL);

	// ArrowEffects::Update call moved because having it happen once per
	// NoteField (which means twice in two player) seemed wasteful. -Kyz

	if (m_drawing_board_primitive) {
		CalcPixelsBeforeAndAfterTargets();
		DrawBoard(
		  static_cast<int>(m_FieldRenderArgs.draw_pixels_after_targets),
		  static_cast<int>(m_FieldRenderArgs.draw_pixels_before_targets));
		return;
	}

	// Clear the z buffer so 3D notes aren't hidden by anything in the underlay
	// using masking. -Kyz
	DISPLAY->ClearZBuffer();

	// Some might prefer an else block, instead of returning from the if, but I
	// don't want to bump the indent on the entire remaining section. -Kyz
	ArrowEffects::SetCurrentOptions(
	  &m_pPlayerState->m_PlayerOptions.GetCurrent());

	CalcPixelsBeforeAndAfterTargets();
	auto* cur = m_pCurDisplay;

	FindDisplayedBeats(
	  m_pPlayerState,
	  m_FieldRenderArgs.first_beat,
	  m_FieldRenderArgs.last_beat,
	  static_cast<int>(m_FieldRenderArgs.draw_pixels_after_targets),
	  static_cast<int>(m_FieldRenderArgs.draw_pixels_before_targets));

	m_FieldRenderArgs.first_row = BeatToNoteRow(m_FieldRenderArgs.first_beat);
	m_FieldRenderArgs.last_row = BeatToNoteRow(m_FieldRenderArgs.last_beat);

	m_pPlayerState->m_fLastDrawnBeat = m_FieldRenderArgs.last_beat;
	// LOG->Trace( "start = %f.1, end = %f.1", first_beat_to_draw-fSongBeat,
	// last_beat_to_draw-fSongBeat );  LOG->Trace( "Drawing elements %d through
	// %d", m_FieldRenderArgs.first_row, m_FieldRenderArgs.last_row );

#define IS_ON_SCREEN(fBeat)                                                    \
	(m_FieldRenderArgs.first_beat <= (fBeat) &&                                \
	 (fBeat) <= m_FieldRenderArgs.last_beat)

	// Draw Receptors
	{
		cur->m_ReceptorArrowRow.Draw();
	}

	const auto* const pTiming = &m_pPlayerState->GetDisplayedTiming();
	const vector<TimingSegment*>* segs[NUM_TimingSegmentType];

	FOREACH_TimingSegmentType(tst) segs[tst] =
	  &(pTiming->GetTimingSegments(tst));

	unsigned i = 0;
	// Draw beat bars
	if (SHOW_BEAT_BARS && pTiming != nullptr) {
		const auto& tSigs = *segs[SEGMENT_TIME_SIG];
		auto iMeasureIndex = 0;
		for (i = 0; i < tSigs.size(); i++) {
			const TimeSignatureSegment* ts = ToTimeSignature(tSigs[i]);
			const auto iSegmentEndRow = (i + 1 == tSigs.size())
										  ? m_FieldRenderArgs.last_row
										  : tSigs[i + 1]->GetRow();

			// beat bars every 16th note
			const auto iDrawBeatBarsEveryRows =
			  BeatToNoteRow((static_cast<float>(ts->GetDen())) / 4) / 4;

			// In 4/4, every 16th beat bar is a measure
			const auto iMeasureBarFrequency = ts->GetNum() * 4;
			auto iBeatBarsDrawn = 0;

			for (auto j = ts->GetRow(); j < iSegmentEndRow;
				 j += iDrawBeatBarsEveryRows) {
				const auto bMeasureBar =
				  iBeatBarsDrawn % iMeasureBarFrequency == 0;
				auto type = quarter_beat;
				if (bMeasureBar)
					type = measure;
				else if (iBeatBarsDrawn % 4 == 0)
					type = beat;
				else if (iBeatBarsDrawn % 2 == 0)
					type = half_beat;
				const auto fBeat = NoteRowToBeat(j);

				if (IS_ON_SCREEN(fBeat)) {
					DrawBeatBar(fBeat, type, iMeasureIndex);
				}

				iBeatBarsDrawn++;
				if (bMeasureBar)
					iMeasureIndex++;
			}
		}
	}

	// Optimization is very important here because there are so many arrows to
	// draw. Draw the arrows in order of column. This minimizes texture switches
	// and lets us draw in big batches.

	const auto* pStyle =
	  GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber);
	ASSERT_M(m_pNoteData->GetNumTracks() ==
			   GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber)
				 ->m_iColsPerPlayer,
			 ssprintf("NumTracks %d != ColsPerPlayer %d",
					  m_pNoteData->GetNumTracks(),
					  GAMESTATE->GetCurrentStyle(m_pPlayerState->m_PlayerNumber)
						->m_iColsPerPlayer));

	if (*m_FieldRenderArgs.selection_begin_marker != -1 &&
		*m_FieldRenderArgs.selection_end_marker != -1) {
		m_FieldRenderArgs.selection_glow = SCALE(
		  RageFastCos(RageTimer::GetTimeSinceStart() * 2), -1, 1, 0.1f, 0.3f);
	}
	m_FieldRenderArgs.fade_before_targets = FADE_BEFORE_TARGETS_PERCENT;

	for (auto j = 0; j < m_pNoteData->GetNumTracks();
		 j++) // for each arrow column
	{
		const auto c = pStyle->m_iColumnDrawOrder[j];
		m_ColumnRenderers[c].Draw();
	}

	cur->m_GhostArrowRow.Draw();
	cur->m_ReceptorArrowRow.DrawOverlay();
}

void
NoteField::DrawBoardPrimitive()
{
	if (!SHOW_BOARD) {
		return;
	}
	m_drawing_board_primitive = true;
	Draw();
	m_drawing_board_primitive = false;
}

void
NoteField::FadeToFail()
{
	m_FieldRenderArgs.fail_fade = std::max(
	  0.0f,
	  m_FieldRenderArgs
		.fail_fade); // this will slowly increase every Update()
					 // don't fade all over again if this is called twice
}

// A few functions and macros to take care of processing the callback
// return values, since the code would be identical in all of them. -Kyz

#define OPEN_CALLBACK_BLOCK(member_name)                                       \
	if (!from_lua && !(member_name).IsNil()) {                                 \
		Lua* L = LUA->Get();                                                   \
		(member_name).PushSelf(L);

#define OPEN_RUN_BLOCK(arg_count)                                              \
	std::string error = "Error running callback: ";                            \
	if (LuaHelpers::RunScriptOnStack(L, error, arg_count, arg_count, true)) {

#define CLOSE_RUN_AND_CALLBACK_BLOCKS                                          \
	}                                                                          \
	lua_settop(L, 0);                                                          \
	LUA->Release(L);                                                           \
	}
#define PUSH_COLUMN lua_pushnumber(L, col + 1)

static void
get_returned_column(Lua* L, PlayerNumber pn, int index, int& col)
{
	if (lua_isnumber(L, index) != 0) {
		// 1-indexed columns in lua
		const auto tmpcol = static_cast<int>(lua_tonumber(L, index)) - 1;
		if (tmpcol < 0 ||
			tmpcol >= GAMESTATE->GetCurrentStyle(pn)->m_iColsPerPlayer) {
			LuaHelpers::ReportScriptErrorFmt(
			  "Column returned by callback must be between 1 and %d "
			  "(GAMESTATE:GetCurrentStyle():ColumnsPerPlayer()).",
			  GAMESTATE->GetCurrentStyle(pn)->m_iColsPerPlayer);
		} else {
			col = tmpcol;
		}
	}
}

// Templated so it can be used for TNS and HNS. -Kyz
template<class T>
static void
get_returned_score(Lua* L, int index, T& score)
{
	T maybe_score = Enum::Check<T>(L, index, true, true);
	if (maybe_score != EnumTraits<T>::Invalid) {
		score = maybe_score;
	}
}

static void
get_returned_bright(Lua* L, int index, bool& bright)
{
	if (lua_isboolean(L, index)) {
		bright = (lua_toboolean(L, index) != 0);
	}
}

void
NoteField::Step(int col, TapNoteScore score, bool from_lua) const
{
	OPEN_CALLBACK_BLOCK(m_StepCallback);
	PUSH_COLUMN;
	Enum::Push(L, score);
	OPEN_RUN_BLOCK(2);
	get_returned_column(L, m_pPlayerState->m_PlayerNumber, 1, col);
	get_returned_score(L, 2, score);
	CLOSE_RUN_AND_CALLBACK_BLOCKS;
	m_pCurDisplay->m_ReceptorArrowRow.Step(col, score);
}
void
NoteField::SetPressed(int col, bool from_lua) const
{
	OPEN_CALLBACK_BLOCK(m_SetPressedCallback);
	PUSH_COLUMN;
	OPEN_RUN_BLOCK(1);
	get_returned_column(L, m_pPlayerState->m_PlayerNumber, 1, col);
	CLOSE_RUN_AND_CALLBACK_BLOCKS;
	m_pCurDisplay->m_ReceptorArrowRow.SetPressed(col);
}
void
NoteField::DidTapNote(int col,
					  TapNoteScore score,
					  bool bright,
					  bool from_lua) const
{
	OPEN_CALLBACK_BLOCK(m_DidTapNoteCallback);
	PUSH_COLUMN;
	Enum::Push(L, score);
	lua_pushboolean(L, static_cast<int>(bright));
	OPEN_RUN_BLOCK(3);
	get_returned_column(L, m_pPlayerState->m_PlayerNumber, 1, col);
	get_returned_score(L, 2, score);
	get_returned_bright(L, 3, bright);
	CLOSE_RUN_AND_CALLBACK_BLOCKS;
	m_pCurDisplay->m_GhostArrowRow.DidTapNote(col, score, bright);
}
void
NoteField::DidHoldNote(int col,
					   HoldNoteScore score,
					   bool bright,
					   bool from_lua) const
{
	OPEN_CALLBACK_BLOCK(m_DidHoldNoteCallback);
	PUSH_COLUMN;
	Enum::Push(L, score);
	lua_pushboolean(L, static_cast<int>(bright));
	OPEN_RUN_BLOCK(3);
	get_returned_column(L, m_pPlayerState->m_PlayerNumber, 1, col);
	get_returned_score(L, 2, score);
	get_returned_bright(L, 3, bright);
	CLOSE_RUN_AND_CALLBACK_BLOCKS;
	m_pCurDisplay->m_GhostArrowRow.DidHoldNote(col, score, bright);
}

#undef OPEN_CALLBACK_BLOCK
#undef OPEN_RUN_BLOCK
#undef CLOSE_RUN_AND_CALLBACK_BLOCKS
#undef PUSH_COLUMN

void
NoteField::HandleMessage(const Message& msg)
{
	if (msg == Message_CurrentSongChanged) {
		m_fCurrentBeatLastUpdate = -1;
		m_fYPosCurrentBeatLastUpdate = -1;
	}

	ActorFrame::HandleMessage(msg);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the Notefield. */
class LunaNoteField : public Luna<NoteField>
{
  public:
#define SET_CALLBACK_GENERIC(callback_name, member_name)                       \
	static int callback_name(T* p, lua_State* L)                               \
	{                                                                          \
		if (lua_isnoneornil(L, 1)) {                                           \
			p->member_name.SetFromNil();                                       \
		} else if (lua_isfunction(L, 1)) {                                     \
			p->member_name.SetFromStack(L);                                    \
		} else {                                                               \
			luaL_error(L,                                                      \
					   #callback_name "Callback argument must be nil (to "     \
									  "clear the callback) or a function (to " \
									  "set the callback).");                   \
		}                                                                      \
		return 0;                                                              \
	}
	SET_CALLBACK_GENERIC(set_step_callback, m_StepCallback);
	SET_CALLBACK_GENERIC(set_set_pressed_callback, m_SetPressedCallback);
	SET_CALLBACK_GENERIC(set_did_tap_note_callback, m_DidTapNoteCallback);
	SET_CALLBACK_GENERIC(set_did_hold_note_callback, m_DidHoldNoteCallback);
#undef SET_CALLBACK_GENERIC

	static int check_column(lua_State* L, int index, PlayerNumber pn)
	{
		// 1-indexed columns in lua
		const auto col = IArg(1) - 1;
		if (col < 0 ||
			col >= GAMESTATE->GetCurrentStyle(pn)->m_iColsPerPlayer) {
			luaL_error(L,
					   "Column must be between 1 and %d "
					   "(GAMESTATE:GetCurrentStyle(pn):ColumnsPerPlayer()).",
					   GAMESTATE->GetCurrentStyle(pn)->m_iColsPerPlayer);
		}
		return col;
	}

	static int step(T* p, lua_State* L)
	{
		const auto col =
		  check_column(L, 1, p->GetPlayerState()->m_PlayerNumber);
		const auto tns = Enum::Check<TapNoteScore>(L, 2);
		p->Step(col, tns, true);
		return 0;
	}

	static int set_pressed(T* p, lua_State* L)
	{
		const auto col =
		  check_column(L, 1, p->GetPlayerState()->m_PlayerNumber);
		p->SetPressed(col, true);
		return 0;
	}

	static int did_tap_note(T* p, lua_State* L)
	{
		const auto col =
		  check_column(L, 1, p->GetPlayerState()->m_PlayerNumber);
		const auto tns = Enum::Check<TapNoteScore>(L, 2);
		const auto bright = BArg(3);
		p->DidTapNote(col, tns, bright, true);
		return 0;
	}

	static int did_hold_note(T* p, lua_State* L)
	{
		const auto col =
		  check_column(L, 1, p->GetPlayerState()->m_PlayerNumber);
		const auto hns = Enum::Check<HoldNoteScore>(L, 2);
		const auto bright = BArg(3);
		p->DidHoldNote(col, hns, bright, true);
		return 0;
	}

	static int get_column_actors(T* p, lua_State* L)
	{
		lua_createtable(L, p->m_ColumnRenderers.size(), 0);
		for (size_t i = 0; i < p->m_ColumnRenderers.size(); ++i) {
			p->m_ColumnRenderers[i].PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	LunaNoteField()
	{
		ADD_METHOD(set_step_callback);
		ADD_METHOD(set_set_pressed_callback);
		ADD_METHOD(set_did_tap_note_callback);
		ADD_METHOD(set_did_hold_note_callback);
		ADD_METHOD(step);
		ADD_METHOD(set_pressed);
		ADD_METHOD(did_tap_note);
		ADD_METHOD(did_hold_note);
		ADD_METHOD(get_column_actors);
	}
};

LUA_REGISTER_DERIVED_CLASS(NoteField, ActorFrame)
// lua end
