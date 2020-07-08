#ifndef NOTE_DISPLAY_H
#define NOTE_DISPLAY_H

#include "Etterna/Actor/Base/Actor.h"
#include "Etterna/Models/Misc/CubicSpline.h"
#include "Etterna/Models/Misc/GameInput.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/Misc/PlayerNumber.h"

class Sprite;
class Model;
class PlayerState;
class GhostArrowRow;
class ReceptorArrowRow;
struct TapNote;
struct HoldNoteResult;
struct NoteMetricCache_t;
/** @brief the various parts of a Note. */
enum NotePart
{
	NotePart_Tap,			/**< The part representing a traditional TapNote. */
	NotePart_Mine,			/**< The part representing a mine. */
	NotePart_Lift,			/**< The part representing a lift note. */
	NotePart_Fake,			/**< The part representing a fake note. */
	NotePart_HoldHead,		/**< The part representing a hold head. */
	NotePart_HoldTail,		/**< The part representing a hold tail. */
	NotePart_HoldTopCap,	/**< The part representing a hold's top cap. */
	NotePart_HoldBody,		/**< The part representing a hold's body. */
	NotePart_HoldBottomCap, /**< The part representing a hold's bottom cap. */
	NUM_NotePart,
	NotePart_Invalid
};

/** @brief the color type of a Note. */
enum NoteColorType
{
	NoteColorType_Denominator, /**< Color by note type. */
	NoteColorType_Progress,	   /**< Color by progress. */
	NUM_NoteColorType,
	NoteColorType_Invalid
};
auto
NoteColorTypeToString(NoteColorType nct) -> const std::string&;
auto
StringToNoteColorType(const std::string& s) -> NoteColorType;

struct NoteResource;

struct NoteColorActor
{
	NoteColorActor();
	~NoteColorActor();
	void Load(const std::string& sButton,
			  const std::string& sElement,
			  PlayerNumber,
			  GameController,
			  const std::string&);
	auto Get(const std::string&) -> Actor*;

  private:
	std::map<std::string, NoteResource*> g_p;
};

struct NoteColorSprite
{
	NoteColorSprite();
	~NoteColorSprite();
	void Load(const std::string& sButton,
			  const std::string& sElement,
			  PlayerNumber,
			  GameController,
			  const std::string&);
	auto Get(const std::string&) -> Sprite*;

  private:
	std::map<std::string, NoteResource*> g_p;
};
/** @brief What types of holds are there? */
enum HoldType
{
	hold, /**< Merely keep your foot held on the body for it to count. */
	roll, /**< Keep hitting the hold body for it to stay alive. */
	// minefield,
	NUM_HoldType,
	HoldType_Invalid
};
/** @brief Loop through each HoldType. */
#define FOREACH_HoldType(i) FOREACH_ENUM(HoldType, i)
auto
HoldTypeToString(HoldType ht) -> const std::string&;

enum ActiveType
{
	active,
	inactive,
	NUM_ActiveType,
	ActiveType_Invalid
};
/** @brief Loop through each ActiveType. */
#define FOREACH_ActiveType(i) FOREACH_ENUM(ActiveType, i)
auto
ActiveTypeToString(ActiveType at) -> const std::string&;

enum NoteColumnSplineMode
{
	NCSM_Disabled,
	NCSM_Offset,
	NCSM_Position,
	NUM_NoteColumnSplineMode,
	NoteColumnSplineMode_Invalid
};

auto
NoteColumnSplineModeToString(NoteColumnSplineMode ncsm) -> const std::string&;
LuaDeclareType(NoteColumnSplineMode);

// A little pod struct to carry the data the NoteField needs to pass to the
// NoteDisplay during rendering.
struct NoteFieldRenderArgs
{
	const PlayerState* player_state; // to look up PlayerOptions
	float reverse_offset_pixels;
	ReceptorArrowRow* receptor_row;
	GhostArrowRow* ghost_row;
	const NoteData* note_data;
	float first_beat;
	float last_beat;
	int first_row;
	int last_row;
	float draw_pixels_before_targets;
	float draw_pixels_after_targets;
	int* selection_begin_marker;
	int* selection_end_marker;
	float selection_glow;
	float fail_fade;
	float fade_before_targets;
};

// NCSplineHandler exists to allow NoteColumnRenderer to have separate
// splines for position, rotation, and zoom, while concisely presenting the
// same interface for all three.
struct NCSplineHandler
{
	NCSplineHandler()
	{
		m_spline.redimension(3);
		m_spline.m_owned_by_actor = true;
		m_spline_mode = NCSM_Disabled;
		m_receptor_t = 0.0F;
		m_beats_per_t = 1.0F;
		m_subtract_song_beat_from_curr = true;
	}

	[[nodiscard]] auto BeatToTValue(float song_beat, float note_beat) const
	  -> float;
	void EvalForBeat(float song_beat, float note_beat, RageVector3& ret) const;
	void EvalDerivForBeat(float song_beat,
						  float note_beat,
						  RageVector3& ret) const;
	void EvalForReceptor(float song_beat, RageVector3& ret) const;
	static void MakeWeightedAverage(NCSplineHandler& out,
									const NCSplineHandler& from,
									const NCSplineHandler& to,
									float between);

	CubicSplineN m_spline;
	NoteColumnSplineMode m_spline_mode;
	float m_receptor_t;
	float m_beats_per_t;
	bool m_subtract_song_beat_from_curr;

	void PushSelf(lua_State* L);
};

struct NoteColumnRenderArgs
{
	void spae_pos_for_beat(const PlayerState* player_state,
						   float beat,
						   float y_offset,
						   float y_reverse_offset,
						   RageVector3& sp_pos,
						   RageVector3& ae_pos) const;
	void spae_zoom_for_beat(const PlayerState* state,
							float beat,
							RageVector3& sp_zoom,
							RageVector3& ae_zoom) const;
	static void SetPRZForActor(Actor* actor,
							   const RageVector3& sp_pos,
							   const RageVector3& ae_pos,
							   const RageVector3& sp_rot,
							   const RageVector3& ae_rot,
							   const RageVector3& sp_zoom,
							   const RageVector3& ae_zoom);
	const NCSplineHandler* pos_handler;
	const NCSplineHandler* rot_handler;
	const NCSplineHandler* zoom_handler;
	RageColor diffuse;
	RageColor glow;
	float song_beat;
	int column;
};

/** @brief Draws TapNotes and HoldNotes. */
class NoteDisplay
{
  public:
	NoteDisplay();
	~NoteDisplay();

	void Load(int iColNum,
			  const PlayerState* pPlayerState,
			  float fYReverseOffsetPixels);

	static void Update(float fDeltaTime);

	auto DrawHoldsInRange(
	  const NoteFieldRenderArgs& field_args,
	  const NoteColumnRenderArgs& column_args,
	  const vector<NoteData::TrackMap::const_iterator>& tap_set) -> bool;
	auto DrawTapsInRange(
	  const NoteFieldRenderArgs& field_args,
	  const NoteColumnRenderArgs& column_args,
	  const vector<NoteData::TrackMap::const_iterator>& tap_set) -> bool;
	/**
	 * @brief Draw the TapNote onto the NoteField.
	 * @param tn the TapNote in question.
	 * @param iCol the column.
	 * @param float fBeat the beat to draw them on.
	 * @param bOnSameRowAsHoldStart a flag to see if a hold is on the same beat.
	 * @param bOnSameRowAsRollStart a flag to see if a roll is on the same beat.
	 * @param bIsAddition a flag to see if this note was added via mods.
	 * @param fPercentFadeToFail at what point do the notes fade on failure?
	 * @param fReverseOffsetPixels How are the notes adjusted on Reverse?
	 * @param fDrawDistanceAfterTargetsPixels how much to draw after the
	 * receptors.
	 * @param fDrawDistanceBeforeTargetsPixels how much ot draw before the
	 * receptors.
	 * @param fFadeInPercentOfDrawFar when to start fading in. */
	void DrawTap(const TapNote& tn,
				 const NoteFieldRenderArgs& field_args,
				 const NoteColumnRenderArgs& column_args,
				 float fBeat,
				 bool bOnSameRowAsHoldStart,
				 bool bOnSameRowAsRollBeat,
				 bool bIsAddition,
				 float fPercentFadeToFail);
	void DrawHold(const TapNote& tn,
				  const NoteFieldRenderArgs& field_args,
				  const NoteColumnRenderArgs& column_args,
				  int iRow,
				  bool bIsBeingHeld,
				  const HoldNoteResult& Result,
				  bool bIsAddition,
				  float fPercentFadeToFail);

	[[nodiscard]] auto DrawHoldHeadForTapsOnSameRow() const -> bool;
	[[nodiscard]] auto DrawRollHeadForTapsOnSameRow() const -> bool;

  private:
	void SetActiveFrame(float fNoteBeat,
						Actor& actorToSet,
						float fAnimationLength,
						bool bVivid) const;
	auto GetTapActor(NoteColorActor& nca, NotePart part, float fNoteBeat) const
	  -> Actor*;
	auto GetHoldActor(NoteColorActor nca[NUM_HoldType][NUM_ActiveType],
					  NotePart part,
					  float fNoteBeat,
					  bool bIsRoll,
					  bool bIsBeingHeld) const -> Actor*;
	auto GetHoldSprite(NoteColorSprite ncs[NUM_HoldType][NUM_ActiveType],
					   NotePart part,
					   float fNoteBeat,
					   bool bIsRoll,
					   bool bIsBeingHeld) const -> Sprite*;

	struct draw_hold_part_args
	{
		int y_step;
		float percent_fade_to_fail;
		float color_scale;
		float overlapped_time;
		float y_top;
		float y_bottom;
		float y_length;
		float y_start_pos;
		float y_end_pos;
		float top_beat;
		float bottom_beat;
		bool wrapping;
		bool anchor_to_top;
		bool flip_texture_vertically;
	};

	void DrawActor(const TapNote& tn,
				   Actor* pActor,
				   NotePart part,
				   const NoteFieldRenderArgs& field_args,
				   const NoteColumnRenderArgs& column_args,
				   float fYOffset,
				   float fBeat,
				   bool bIsAddition,
				   float fPercentFadeToFail,
				   float fColorScale,
				   bool is_being_held) const;
	void DrawHoldPart(vector<Sprite*>& vpSpr,
					  const NoteFieldRenderArgs& field_args,
					  const NoteColumnRenderArgs& column_args,
					  const draw_hold_part_args& part_args,
					  bool glow,
					  int part_type) const;
	void DrawHoldBodyInternal(vector<Sprite*>& sprite_top,
							  vector<Sprite*>& sprite_body,
							  vector<Sprite*>& sprite_bottom,
							  const NoteFieldRenderArgs& field_args,
							  const NoteColumnRenderArgs& column_args,
							  draw_hold_part_args& part_args,
							  float head_minus_top,
							  float tail_plus_bottom,
							  float y_head,
							  float y_tail,
							  float y_length,
							  float top_beat,
							  float bottom_beat,
							  bool glow) const;
	void DrawHoldBody(const TapNote& tn,
					  const NoteFieldRenderArgs& field_args,
					  const NoteColumnRenderArgs& column_args,
					  float beat,
					  bool being_held,
					  float y_head,
					  float y_tail,
					  float y_end,
					  float percent_fade_to_fail,
					  float color_scale,
					  float top_beat,
					  float bottom_beat);

	const PlayerState* m_pPlayerState; // to look up PlayerOptions
	NoteMetricCache_t* cache;

	NoteColorActor m_TapNote;
	NoteColorActor m_TapMine;
	NoteColorActor m_TapLift;
	NoteColorActor m_TapFake;
	NoteColorActor m_HoldHead[NUM_HoldType][NUM_ActiveType];
	NoteColorSprite m_HoldTopCap[NUM_HoldType][NUM_ActiveType];
	NoteColorSprite m_HoldBody[NUM_HoldType][NUM_ActiveType];
	NoteColorSprite m_HoldBottomCap[NUM_HoldType][NUM_ActiveType];
	NoteColorActor m_HoldTail[NUM_HoldType][NUM_ActiveType];
	float m_fYReverseOffsetPixels;
};

// So, this is a bit screwy, and it's partly because routine forces rendering
// notes from different noteskins in the same column.
// NoteColumnRenderer exists to hold all the data needed for rendering a
// column and apply any transforms from that column's actor to the
// NoteDisplays that render the notes.
// NoteColumnRenderer is also used as a fake parent for the receptor and ghost
// actors so they can move with the rest of the column.  I didn't use
// ActorProxy because the receptor/ghost actors need to pull in the parent
// state of their rows and the parent state of the column. -Kyz

struct NoteColumnRenderer : public Actor
{
	NoteDisplay* m_displays[PLAYER_INVALID + 1];
	NoteFieldRenderArgs* m_field_render_args;
	NoteColumnRenderArgs m_column_render_args;
	int m_column;

	// UpdateReceptorGhostStuff takes care of the logic for making the ghost
	// and receptor positions follow the splines.  It's called by their row
	// update functions. -Kyz
	void UpdateReceptorGhostStuff(Actor* receptor) const;
	void DrawPrimitives() override;
	void PushSelf(lua_State* L) override;

	struct NCR_TweenState
	{
		NCR_TweenState();
		NCSplineHandler m_pos_handler;
		NCSplineHandler m_rot_handler;
		NCSplineHandler m_zoom_handler;
		static void MakeWeightedAverage(NCR_TweenState& out,
										const NCR_TweenState& from,
										const NCR_TweenState& to,
										float between);
		auto operator==(const NCR_TweenState& other) const -> bool;
		auto operator!=(const NCR_TweenState& other) const -> bool
		{
			return !operator==(other);
		}
	};

	auto NCR_DestTweenState() -> NCR_TweenState&
	{
		if (NCR_Tweens.empty()) {
			return NCR_current;
		}
		return NCR_Tweens.back();
	}

	[[nodiscard]] auto NCR_DestTweenState() const -> const NCR_TweenState&
	{
		return const_cast<NoteColumnRenderer*>(this)->NCR_DestTweenState();
	}

	void SetCurrentTweenStart() override;
	void EraseHeadTween() override;
	void UpdatePercentThroughTween(float between) override;
	void BeginTweening(float time, ITween* interp) override;
	void StopTweening() override;
	void FinishTweening() override;

	auto GetPosHandler() -> NCSplineHandler*
	{
		return &NCR_DestTweenState().m_pos_handler;
	}
	auto GetRotHandler() -> NCSplineHandler*
	{
		return &NCR_DestTweenState().m_rot_handler;
	}
	auto GetZoomHandler() -> NCSplineHandler*
	{
		return &NCR_DestTweenState().m_zoom_handler;
	}

  private:
	vector<NCR_TweenState> NCR_Tweens;
	NCR_TweenState NCR_current;
	NCR_TweenState NCR_start;
};

#endif
