#ifndef PLAYERINFO_H
#define PLAYERINFO_H

#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/PlayerNumber.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/Misc/PlayerStageStats.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/SoundEffectControl.h"
#include "Etterna/Models/NoteData/NoteData.h"

class Player;

class LifeMeter;
class StepsDisplay;
class ScoreKeeper;

class PlayerInfo
{
  public:
	PlayerInfo();
	~PlayerInfo();

	void Load(PlayerNumber pn,
			  MultiPlayer mp,
			  bool bShowNoteField,
			  int iAddToDifficulty,
			  GameplayMode mode = GameplayMode_Normal);

	/**
	 * @brief Retrieve the player's state and stage stats index.
	 * @return the player's state and stage stats index.
	 */
	MultiPlayer GetPlayerStateAndStageStatsIndex()
	{
		return m_pn == PLAYER_INVALID ? m_mp : static_cast<MultiPlayer>(m_pn);
	}
	PlayerState* GetPlayerState();
	PlayerStageStats* GetPlayerStageStats();
	PlayerNumber GetStepsAndTrailIndex()
	{
		return m_pn == PLAYER_INVALID ? PLAYER_1 : m_pn;
	}
	/**
	 * @brief Determine if the player information is enabled.
	 * @return its success or failure. */
	bool IsEnabled();
	/**
	 * @brief Determine if we're in MultiPlayer.
	 * @return true if it is MultiPlayer, false otherwise. */
	bool IsMultiPlayer() const { return m_mp != MultiPlayer_Invalid; }
	/**
	 * @brief Retrieve the name of the Player based on the mode.
	 * @return the name of the Player. */
	std::string GetName() const
	{
		if (m_bIsDummy)
			return ssprintf("PlayerInfoDummy");
		if (IsMultiPlayer())
			return MultiPlayerToString(m_mp);
		else
			return PlayerNumberToString(m_pn);
	}

	// Lua
	void PushSelf(lua_State* L);

	/** @brief The present Player's number. */
	PlayerNumber m_pn;
	/** @brief The present Player's multiplayer number. */
	MultiPlayer m_mp{ MultiPlayer_Invalid };
	bool m_bIsDummy{ false };
	int m_iAddToDifficulty{ 0 };	// if > 0, use the Nth harder Steps
	bool m_bPlayerEnabled{ false }; // IsEnabled cache for iterators
	SoundEffectControl m_SoundEffectControl;

	/**
	 * @brief The list of Steps a player has to go through in this set.
	 *
	 * The size may be greater than 1 if playing a course. */
	vector<Steps*> m_vpStepsQueue;

	/** @brief The LifeMeter showing a Player's health. */
	LifeMeter* m_pLifeMeter;
	/** @brief The description of the current Steps. */
	BitmapText* m_ptextStepsDescription;
	/** @brief The primary ScoreKeeper for keeping track of the score. */
	ScoreKeeper* m_pPrimaryScoreKeeper;
	/** @brief The current PlayerOptions that are activated. */
	BitmapText* m_ptextPlayerOptions;

	/** @brief The NoteData the Player has to get through. */
	NoteData m_NoteData;
	/** @brief The specific Player that is going to play. */
	Player* m_pPlayer;

	StepsDisplay* m_pStepsDisplay;
};

#endif
