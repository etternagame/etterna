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
