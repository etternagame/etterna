#ifndef DancingCharacters_H
#define DancingCharacters_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "Etterna/Models/Misc/PlayerNumber.h"
class Model;

/** @brief The different animation states for the dancer. */
enum ANIM_STATES_2D
{
	AS2D_IDLE = 0, /**< The dancer is idle. */
	AS2D_MISS,	 /**< The dancer just missed a note. */
	AS2D_GOOD,	 /**< The dancer is doing a good job. */
	AS2D_GREAT,	/**< The dancer is doing a great job. */
	AS2D_FEVER,	/**< The dancer is on fire! */
	AS2D_FAIL,	 /**< The dancer has failed. */
	AS2D_WIN,	  /**< The dancer has won. */
	AS2D_WINFEVER, /**< The dancer has won while on fire. */
	AS2D_IGNORE,   /**< This is a special case -- so that we can timer to idle
					  again. */
	AS2D_MAXSTATES /**< A count of the maximum number of states. */
};

/** @brief Characters that react to how the players are doing. */
class DancingCharacters : public ActorFrame
{
  public:
	DancingCharacters();
	~DancingCharacters() override;

	void LoadNextSong();

	void Update(float fDelta) override;
	void DrawPrimitives() override;
	bool m_bDrawDangerLight{ false };
	void Change2DAnimState(PlayerNumber pn, int iState);

  protected:
	Model* m_pCharacter;

	/** @brief How far away is the camera from the dancer? */
	float m_CameraDistance{ 0 };
	float m_CameraPanYStart{ 0 };
	float m_CameraPanYEnd{ 0 };
	float m_fLookAtHeight{ 0 };
	float m_fCameraHeightStart{ 0 };
	float m_fCameraHeightEnd{ 0 };
	float m_fThisCameraStartBeat{ 0 };
	float m_fThisCameraEndBeat{ 0 };

	bool m_bHas2DElements = false;

	AutoActor m_bgIdle;
	AutoActor m_bgMiss;
	AutoActor m_bgGood;
	AutoActor m_bgGreat;
	AutoActor m_bgFever;
	AutoActor m_bgFail;
	AutoActor m_bgWin;
	AutoActor m_bgWinFever;

	int m_i2DAnimState;
};

#endif

/**
 * @file
 * @author Chris Danford (c) 2003-2004
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
