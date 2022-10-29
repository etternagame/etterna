/** @brief GameManager - Manages Games and Styles. */

#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

class Style;
struct Game;
struct lua_State;

#include "Etterna/Models/Misc/GameConstantsAndTypes.h"

/** @brief The collective information about a Steps' Type. */
struct StepsTypeInfo
{
	const char* szName;
	/** @brief The number of tracks, or columns, of this type. */
	int iNumTracks;
	/** @brief A flag to determine if we allow this type to be autogen'ed to
	 * other types. */
	bool bAllowAutogen;
	/** @brief The most basic StyleType that this StpesTypeInfo is used with. */
	StepsTypeCategory m_StepsTypeCategory;
	[[nodiscard]] auto GetLocalizedString() const -> std::string;
};

class GameManager
{
  public:
	GameManager();
	~GameManager();

	void GetStylesForGame(const Game* pGame,
						  std::vector<const Style*>& aStylesAddTo,
						  bool editor = false);
	auto GetGameForStyle(const Style* pStyle) -> const Game*;
	void GetStepsTypesForGame(const Game* pGame,
							  std::vector<StepsType>& aStepsTypeAddTo);
	auto GetEditorStyleForStepsType(StepsType st) -> const Style*;
	auto GetStyleForStepsType(StepsType st) -> const Style*;
	void GetDemonstrationStylesForGame(const Game* pGame,
									   std::vector<const Style*>& vpStylesOut);
	auto GetHowToPlayStyleForGame(const Game* pGame) -> const Style*;
	void GetCompatibleStyles(const Game* pGame,
							 int iNumPlayers,
							 std::vector<const Style*>& vpStylesOut);
	auto GetFirstCompatibleStyle(const Game* pGame,
								 int iNumPlayers,
								 StepsType st) -> const Style*;

	void GetEnabledGames(std::vector<const Game*>& aGamesOut);
	auto GetDefaultGame() -> const Game*;
	auto IsGameEnabled(const Game* pGame) -> bool;
	auto GetIndexFromGame(const Game* pGame) -> int;
	auto GetGameFromIndex(int index) -> const Game*;

	auto GetStepsTypeInfo(StepsType st) -> const StepsTypeInfo&;
	auto StringToStepsType(std::string sStepsType) -> StepsType;
	auto StringToGame(const std::string& sGame) -> const Game*;
	auto GameAndStringToStyle(const Game* pGame, const std::string& sStyle)
	  -> const Style*;
	auto StyleToLocalizedString(const Style* s) -> std::string;

	bool m_bResetModifiers;
	bool m_bResetTurns;
	float m_fPreviousRate;
	std::string m_sModsToReset;
	std::vector<std::string> m_vTurnsToReset;

	// Lua
	void PushSelf(lua_State* L);
};

extern GameManager*
  GAMEMAN; // global and accessible from anywhere in our program

#endif
