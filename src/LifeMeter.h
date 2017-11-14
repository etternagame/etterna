#ifndef LIFE_METER_H
#define LIFE_METER_H

#include "PlayerNumber.h"
#include "GameConstantsAndTypes.h"
#include "ActorFrame.h"
#include "PlayerOptions.h"

class PlayerState;
class PlayerStageStats;
/** @brief The player's life. */
class LifeMeter : public ActorFrame
{
public:
	LifeMeter() = default;
	~LifeMeter() override = default;
	
	virtual void Load( const PlayerState *pPlayerState, PlayerStageStats *pPlayerStageStats )
	{
		m_pPlayerState = pPlayerState;
		m_pPlayerStageStats = pPlayerStageStats;
	}
	virtual void OnLoadSong() {};
	virtual void OnSongEnded() {};
	/**
	 * @brief Change life after receiving a tap note grade.
	 *
	 * This *is* called for the head of hold notes. 
	 * @param score the tap note grade in question. */
	virtual void ChangeLife( TapNoteScore score ) = 0;
	/**
	 * @brief Change life after receiving a hold note grade.
	 *
	 * @param hns the hold note grade in question.
	 * @param tns the score received for the initial tap note. */
	virtual void ChangeLife( HoldNoteScore hns, TapNoteScore tns ) = 0;
	virtual void ChangeLife(float delta) = 0;
	virtual void SetLife(float value) = 0;
	virtual void HandleTapScoreNone() = 0;
	virtual bool IsInDanger() const = 0;
	virtual bool IsHot() const = 0;
	virtual bool IsFailing() const = 0;
	virtual float GetLife() const { return 0; } // for cosmetic use only
	virtual void UpdateNonstopLifebar() { }

	static LifeMeter *MakeLifeMeter( LifeType t );

	//
	// Lua
	//
	void PushSelf( lua_State *L ) override;

protected:
	const PlayerState *m_pPlayerState;
	PlayerStageStats *m_pPlayerStageStats;
};


#endif

/**
 * @file
 * @author Chris Danford (c) 2001-2003
 * @section LICENSE
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
