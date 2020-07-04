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
	std::string GetLocalizedString() const;
};

class GameManager
{
  public:
	GameManager();
	~GameManager();

	void GetStylesForGame(const Game* pGame,
						  vector<const Style*>& aStylesAddTo,
						  bool editor = false);
	const Game* GetGameForStyle(const Style* pStyle);
	void GetStepsTypesForGame(const Game* pGame,
							  vector<StepsType>& aStepsTypeAddTo);
	const Style* GetEditorStyleForStepsType(StepsType st);
	void GetDemonstrationStylesForGame(const Game* pGame,
									   vector<const Style*>& vpStylesOut);
	const Style* GetHowToPlayStyleForGame(const Game* pGame);
	void GetCompatibleStyles(const Game* pGame,
							 int iNumPlayers,
							 vector<const Style*>& vpStylesOut);
	const Style* GetFirstCompatibleStyle(const Game* pGame,
										 int iNumPlayers,
										 StepsType st);

	void GetEnabledGames(vector<const Game*>& aGamesOut);
	const Game* GetDefaultGame();
	bool IsGameEnabled(const Game* pGame);
	int GetIndexFromGame(const Game* pGame);
	const Game* GetGameFromIndex(int index);

	const StepsTypeInfo& GetStepsTypeInfo(StepsType st);
	StepsType StringToStepsType(std::string sStepsType);
	const Game* StringToGame(const std::string& sGame);
	const Style* GameAndStringToStyle(const Game* pGame,
									  const std::string& sStyle);
	std::string StyleToLocalizedString(const Style* s);

	bool m_bResetModifiers;
	bool m_bResetTurns;
	float m_fPreviousRate;
	RString m_sModsToReset;
	vector<std::string> m_vTurnsToReset;

	// Lua
	void PushSelf(lua_State* L);
};

extern GameManager*
  GAMEMAN; // global and accessible from anywhere in our program

#endif
