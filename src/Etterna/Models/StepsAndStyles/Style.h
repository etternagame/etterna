/** @brief Style - A data structure that holds the definition for one of a
 * Game's styles. */

#ifndef STYLE_H
#define STYLE_H

#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/GameInput.h"
#include "Etterna/Models/Misc/NoteTypes.h"
#include "Etterna/Models/Misc/PlayerNumber.h"

/** @brief Each style can have a maximum amount of columns to work with. */
const int MAX_COLS_PER_PLAYER = MAX_NOTE_TRACKS;
/** @brief Provide a default value for an invalid column. */
static const int Column_Invalid = -1;

class NoteData;
struct Game;
struct lua_State;

class Style
{
  public:
	/** @brief Can this style be used for gameplay purposes? */
	bool m_bUsedForGameplay;
	/** @brief Can this style be used for making edits? */
	bool m_bUsedForEdit;
	/** @brief Can this style be used in a demonstration? */
	bool m_bUsedForDemonstration;
	/** @brief Can this style be used to explain how to play the game? */
	bool m_bUsedForHowToPlay;

	/**
	 * @brief The name of the style.
	 *
	 * Used by GameManager::GameAndStringToStyle to determine whether this is
	 * the style that matches the string. */
	const char* m_szName;

	/**
	 * @brief Steps format used for each player.
	 *
	 * For example, "dance versus" reads the Steps with the tag "dance-single".
	 */
	StepsType m_StepsType;

	/** @brief Style format used for each player. */
	StyleType m_StyleType;

	/**
	 * @brief The number of total tracks/columns this style expects.
	 *
	 * As an example, 4 is expected for ITG style versus, but 8 for ITG style
	 * double. */
	int m_iColsPerPlayer;
	/** @brief Some general column infromation */
	struct ColumnInfo
	{
		int track;		/**< Take note data from this track. */
		float fXOffset; /**< This is the x position of the column relative to
						   the player's center. */
		const char* pzName; /**< The name of the column, or NULL to use the
							   button name mapped to it. */
	};

	/** @brief Map each players' colun to a track in the NoteData. */
	ColumnInfo m_ColumnInfo[MAX_COLS_PER_PLAYER];

	/* This maps from game inputs to columns. More than one button may map to a
	 * single column. */
	enum
	{
		NO_MAPPING = -1,
		END_MAPPING = -2
	};
	/** @brief Map each input to a column, or GameButton_Invalid. */
	int m_iInputColumn[NUM_GameController][NUM_GameButton];
	int m_iColumnDrawOrder[MAX_COLS_PER_PLAYER];

	void StyleInputToGameInput(int iCol,
							   std::vector<GameInput>& ret) const;
	/**
	 * @brief Retrieve the column based on the game input.
	 * @param GameI the game input.
	 * @return the Column number of the style, or Column_Invalid if it's an
	 * invalid column. Examples of this include getting the upper left hand
	 * corner in a traditional four panel mode. */
	[[nodiscard]] auto GameInputToColumn(const GameInput& GameI) const -> int;
	[[nodiscard]] auto ColToButtonName(int iCol) const -> std::string;

	[[nodiscard]] auto GetUsesCenteredArrows() const -> bool;
	void GetTransformedNoteDataForStyle(PlayerNumber pn,
										const NoteData& original,
										NoteData& noteDataOut) const;
	void GetMinAndMaxColX(PlayerNumber pn,
						  float& fMixXOut,
						  float& fMaxXOut) const;
	[[nodiscard]] auto GetWidth(PlayerNumber pn) const -> float;

	// Lua
	void PushSelf(lua_State* L);
};

#endif
