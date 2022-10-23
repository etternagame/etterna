/*
 * Styles define a set of columns for each player, and information about those
 * columns, like what Instruments are used play those columns and what track
 * to use to populate the column's notes.
 * A "track" is the term used to descibe a particular vertical sting of note
 * in NoteData.
 * A "column" is the term used to describe the vertical string of notes that
 * a player sees on the screen while they're playing.  Column notes are
 * picked from a track, but columns and tracks don't have a 1-to-1
 * correspondance.  For example, dance-versus has 8 columns but only 4 tracks
 * because two players place from the same set of 4 tracks.
 */

#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/InputMapper.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Style.h"
#include "Etterna/Globals/global.h"

#include <algorithm>
#include <cfloat>

bool
Style::GetUsesCenteredArrows() const
{
	switch (m_StyleType) {
		case StyleType_OnePlayerTwoSides:
		default:
			return false;
	}
}

void
Style::GetTransformedNoteDataForStyle(PlayerNumber pn,
									  const NoteData& original,
									  NoteData& noteDataOut) const
{
	ASSERT(pn >= 0 && pn <= NUM_PLAYERS);

	int iNewToOriginalTrack[MAX_COLS_PER_PLAYER];
	for (auto col = 0; col < m_iColsPerPlayer; col++) {
		auto colInfo = m_ColumnInfo[col];
		iNewToOriginalTrack[col] = colInfo.track;
	}

	noteDataOut.LoadTransformed(
	  original, m_iColsPerPlayer, iNewToOriginalTrack);
}

void
Style::StyleInputToGameInput(int iCol,
							 std::vector<GameInput>& ret) const
{
	ASSERT_M(iCol < MAX_COLS_PER_PLAYER,
			 ssprintf("C%i", iCol));
	// auto bUsingOneSide = true;

	FOREACH_ENUM(GameController, gc)
	{
		// if (bUsingOneSide && gc != (int)pn)
		//	continue;

		// this treats player 2's game buttons as an extension of player 1's
		// when determining if there are appropriately mapped buttons for the
		// style, however since player 2 was removed internally this will
		// cause anything mapped to gc2 ("p2") to be ignored and cause a crash.
		// i dont really see how force crashing the game because buttons aren't
		// mapped is a good idea in the first place but uhhh whatever -mina

		auto iButtonsPerController =
		  INPUTMAPPER->GetInputScheme()->m_iButtonsPerController;
		for (auto gb = GAME_BUTTON_NEXT; gb < iButtonsPerController;
			 gb = (GameButton)(gb + 1)) {
			auto iThisInputCol = m_iInputColumn[gc][gb - GAME_BUTTON_NEXT];
			if (iThisInputCol == END_MAPPING)
				break;

			// A style can have multiple game inputs mapped to a single column,
			// so we have to return all the game inputs that are valid.  If only
			// the first is returned, then holds will drop on other inputs that
			// should be valid. -Kyz
			if (iThisInputCol == iCol) {
				ret.push_back(GameInput(gc, gb));
			}
		}
	}
	if (unlikely(ret.empty())) {
		FAIL_M(
		  ssprintf("Invalid column number %i in the style %s",
				   iCol,
				   m_szName));
	}
};

int
Style::GameInputToColumn(const GameInput& GameI) const
{
	if (GameI.button < GAME_BUTTON_NEXT)
		return Column_Invalid;
	auto iColumnIndex = GameI.button - GAME_BUTTON_NEXT;
	if (m_iInputColumn[GameI.controller][iColumnIndex] == NO_MAPPING)
		return Column_Invalid;

	for (auto i = 0; i <= iColumnIndex; ++i) {
		if (m_iInputColumn[GameI.controller][i] == END_MAPPING) {
			return Column_Invalid;
		}
	}

	return m_iInputColumn[GameI.controller][iColumnIndex];
}

void
Style::GetMinAndMaxColX(PlayerNumber pn, float& fMixXOut, float& fMaxXOut) const
{
	ASSERT(pn != PLAYER_INVALID);

	fMixXOut = FLT_MAX;
	fMaxXOut = FLT_MIN;
	for (auto i = 0; i < m_iColsPerPlayer; i++) {
		fMixXOut = std::min(fMixXOut, m_ColumnInfo[i].fXOffset);
		fMaxXOut = std::max(fMaxXOut, m_ColumnInfo[i].fXOffset);
	}
}

float
Style::GetWidth(PlayerNumber pn) const
{
	float left, right;
	GetMinAndMaxColX(pn, left, right);
	// left and right are the center positions of the columns.  The full width
	// needs to be from the edges.
	auto width = right - left;
	return width + (width / static_cast<float>(m_iColsPerPlayer - 1));
}

std::string
Style::ColToButtonName(int iCol) const
{
	auto pzColumnName = m_ColumnInfo[iCol].pzName;
	if (pzColumnName != nullptr)
		return pzColumnName;

	std::vector<GameInput> GI;
	StyleInputToGameInput(iCol, GI);
	return INPUTMAPPER->GetInputScheme()->GetGameButtonName(GI[0].button);
}

// Lua bindings
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the Style. */
class LunaStyle : public Luna<Style>
{
  public:
	static int GetName(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, (std::string)p->m_szName);
		return 1;
	}
	DEFINE_METHOD(GetStyleType, m_StyleType)
	DEFINE_METHOD(GetStepsType, m_StepsType)
	DEFINE_METHOD(ColumnsPerPlayer, m_iColsPerPlayer)
	static int NeedsZoomOutWith2Players(T* p, lua_State* L)
	{
		// m_bNeedsZoomOutWith2Players was removed in favor of having
		// ScreenGameplay use the style's width and margin values to calculate
		// the zoom.  So this always returns false. -Kyz
		lua_pushboolean(L, 0);
		return 1;
	}
	static int GetWidth(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetWidth(PLAYER_1));
		return 1;
	}
	static int GetColumnInfo(T* p, lua_State* L)
	{
		auto iCol = IArg(2) - 1;
		if (iCol < 0 || iCol >= p->m_iColsPerPlayer) {
			LuaHelpers::ReportScriptErrorFmt(
			  "Style:GetColumnDrawOrder(): column %i out of range( 1 to %i )",
			  iCol + 1,
			  p->m_iColsPerPlayer);
			return 0;
		}

		LuaTable ret;
		lua_pushnumber(L, p->m_ColumnInfo[iCol].track + 1);
		ret.Set(L, "Track");
		lua_pushnumber(L, p->m_ColumnInfo[iCol].fXOffset);
		ret.Set(L, "XOffset");
		lua_pushstring(L, p->ColToButtonName(iCol).c_str());
		ret.Set(L, "Name");

		ret.PushSelf(L);
		return 1;
	}

	static int GetColumnDrawOrder(T* p, lua_State* L)
	{
		auto iCol = IArg(1) - 1;
		if (iCol < 0 || iCol >= p->m_iColsPerPlayer * NUM_PLAYERS) {
			LuaHelpers::ReportScriptErrorFmt(
			  "Style:GetColumnDrawOrder(): column %i out of range( 1 to %i )",
			  iCol + 1,
			  p->m_iColsPerPlayer * NUM_PLAYERS);
			return 0;
		}
		lua_pushnumber(L, p->m_iColumnDrawOrder[iCol] + 1);
		return 1;
	}

	LunaStyle()
	{
		ADD_METHOD(GetName);
		ADD_METHOD(GetStyleType);
		ADD_METHOD(GetStepsType);
		ADD_METHOD(GetColumnInfo);
		ADD_METHOD(GetColumnDrawOrder);
		ADD_METHOD(ColumnsPerPlayer);
		ADD_METHOD(NeedsZoomOutWith2Players);
		ADD_METHOD(GetWidth);
	}
};

LUA_REGISTER_CLASS(Style)
