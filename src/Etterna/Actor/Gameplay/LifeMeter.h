#ifndef LIFE_METER_H
#define LIFE_METER_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/PlayerOptions.h"

class PlayerState;
class PlayerStageStats;

/** @brief The player's life. */
class LifeMeter : public ActorFrame
{
  public:
	LifeMeter() = default;
	~LifeMeter() override = default;

	virtual void Load(const PlayerState* pPlayerState,
					  PlayerStageStats* pPlayerStageStats)
	{
		m_pPlayerState = pPlayerState;
		m_pPlayerStageStats = pPlayerStageStats;
	}
	virtual void OnLoadSong(){};
	virtual void OnSongEnded(){};
	/**
	 * @brief Change life after receiving a tap note grade.
	 *
	 * This *is* called for the head of hold notes.
	 * @param score the tap note grade in question. */
	virtual void ChangeLife(TapNoteScore score) = 0;
	/**
	 * @brief Change life after receiving a hold note grade.
	 *
	 * @param hns the hold note grade in question.
	 * @param tns the score received for the initial tap note. */
	virtual void ChangeLife(HoldNoteScore hns, TapNoteScore tns) = 0;
	virtual void ChangeLife(float delta) = 0;
	virtual void SetLife(float value) = 0;
	virtual void HandleTapScoreNone() = 0;
	[[nodiscard]] virtual auto IsInDanger() const -> bool = 0;
	[[nodiscard]] virtual auto IsHot() const -> bool = 0;
	[[nodiscard]] virtual auto IsFailing() const -> bool = 0;
	[[nodiscard]] virtual auto GetLife() const -> float
	{
		return 0;
	} // for cosmetic use only

	static auto MakeLifeMeter(LifeType t) -> LifeMeter*;

	//
	// Lua
	//
	void PushSelf(lua_State* L) override;

  protected:
	const PlayerState* m_pPlayerState{};
	PlayerStageStats* m_pPlayerStageStats{};
};

#endif
