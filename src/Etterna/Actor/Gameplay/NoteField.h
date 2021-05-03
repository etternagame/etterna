#ifndef NOTE_FIELD_H
#define NOTE_FIELD_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Actor/Gameplay/GhostArrowRow.h"
#include "Etterna/Actor/Gameplay/NoteDisplay.h"
#include "Etterna/Actor/Base/Quad.h"
#include "Etterna/Actor/Gameplay/ReceptorArrowRow.h"
#include "Etterna/Actor/Base/Sprite.h"

class NoteData;
/** @brief An Actor that renders NoteData. */
class NoteField : public ActorFrame
{
  public:
	NoteField();
	~NoteField() override;
	void Update(float fDeltaTime) override;
	void DrawPrimitives() override;
	void CalcPixelsBeforeAndAfterTargets();
	void DrawBoardPrimitive();

	void Init(const PlayerState* pPlayerState,
			  float fYReverseOffsetPixels,
			  bool use_states_zoom = true);
	void Load(const NoteData* pNoteData,
			  int iDrawDistanceAfterTargetsPixels,
			  int iDrawDistanceBeforeTargetsPixels);
	void Unload();

	virtual void ensure_note_displays_have_skin();
	void InitColumnRenderers();

	void HandleMessage(const Message& msg) override;

	// This is done automatically by Init(), but can be re-called explicitly if
	// the note skin changes.
	void CacheAllUsedNoteSkins();
	void FadeToFail();

	void Step(int col, TapNoteScore score, bool from_lua = false) const;
	void SetPressed(int col, bool from_lua = false) const;
	void DidTapNote(int col,
					TapNoteScore score,
					bool bright,
					bool from_lua = false) const;
	void DidHoldNote(int col,
					 HoldNoteScore score,
					 bool bright,
					 bool from_lua = false) const;

	void PushSelf(lua_State* L) override;

	// Allows the theme to modify the parameters to Step, SetPressed,
	// DidTapNote, and DidHoldNote before they pass on to the ghost arrows or
	// receptors. -Kyz
	LuaReference m_StepCallback;
	LuaReference m_SetPressedCallback;
	LuaReference m_DidTapNoteCallback;
	LuaReference m_DidHoldNoteCallback;

	[[nodiscard]] auto GetPlayerState() const -> const PlayerState*
	{
		return m_pPlayerState;
	}

	int m_iBeginMarker, m_iEndMarker; // only used with MODE_EDIT

	// m_ColumnRenderers belongs in the protected section, but it's here in
	// public so that the Lua API can access it. -Kyz
	std::vector<NoteColumnRenderer> m_ColumnRenderers;

  protected:
	void CacheNoteSkin(const std::string& sNoteSkin);
	void UncacheNoteSkin(const std::string& sNoteSkin);

	void DrawBoard(int iDrawDistanceAfterTargetsPixels,
				   int iDrawDistanceBeforeTargetsPixels);

	enum BeatBarType
	{
		measure,
		beat,
		half_beat,
		quarter_beat
	};
	void DrawBeatBar(float fBeat, BeatBarType type, int iMeasureIndex);
	void DrawMarkerBar(int fBeat);
	void DrawAreaHighlight(int iStartBeat, int iEndBeat);
	void set_text_measure_number_for_draw(float beat,
										  float side_sign,
										  float x_offset,
										  float horiz_align,
										  const RageColor& color,
										  const RageColor& glow);
	void draw_timing_segment_text(const std::string& text,
								  float beat,
								  float side_sign,
								  float x_offset,
								  float horiz_align,
								  const RageColor& color,
								  const RageColor& glow);
	void DrawBGChangeText(float beat,
						  const std::string& new_bg_name,
						  const RageColor& glow);
	[[nodiscard]] auto GetWidth() const -> float;

	const NoteData* m_pNoteData;

	const PlayerState* m_pPlayerState;
	int m_iDrawDistanceAfterTargetsPixels;	// this should be a negative number
	int m_iDrawDistanceBeforeTargetsPixels; // this should be a positive number
	float m_fYReverseOffsetPixels;

	// This exists so that the board can be drawn underneath combo/judge. -Kyz
	bool m_drawing_board_primitive;

	// color arrows
	struct NoteDisplayCols
	{
		NoteDisplay* display;
		ReceptorArrowRow m_ReceptorArrowRow;
		GhostArrowRow m_GhostArrowRow;
		explicit NoteDisplayCols(int iNumCols)
		{
			display = new NoteDisplay[iNumCols];
		}

		~NoteDisplayCols() { delete[] display; }
	};

	NoteFieldRenderArgs m_FieldRenderArgs;

	/* All loaded note displays, mapped by their name. */
	std::map<std::string, NoteDisplayCols*> m_NoteDisplays;
	NoteDisplayCols* m_pCurDisplay;
	// leaving this here in case we want to vectorize this in the future
	// the purpose is to have a display for each player
	// this pointer does not get deleted
	// why: it points to a member of m_NoteDisplays which is managed
	// (same for m_pCurDisplay)
	NoteDisplayCols* m_pDisplays;

	// decorations, mostly used in MODE_EDIT
	AutoActor m_sprBoard;
	float m_fBoardOffsetPixels;
	float m_fCurrentBeatLastUpdate;		// -1 on first update
	float m_fYPosCurrentBeatLastUpdate; // -1 on first update

	Sprite m_sprBeatBars; // 4 frames: Measure, 4th, 8th, 16th
	BitmapText m_textMeasureNumber;
	Quad m_rectMarkerBar;
	Quad m_rectAreaHighlight;
};

#endif
