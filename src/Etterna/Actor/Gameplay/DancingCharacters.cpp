#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/Character.h"
#include "DancingCharacters.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Actor/Base/Model.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Misc/RageMath.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/StatsManager.h"

int
Neg1OrPos1();

#define DC_X(choice)                                                           \
	THEME->GetMetricF("DancingCharacters",                                     \
					  ssprintf("2DCharacterXP%d", (choice) + 1))
#define DC_Y(choice)                                                           \
	THEME->GetMetricF("DancingCharacters",                                     \
					  ssprintf("2DCharacterYP%d", (choice) + 1))

/*
 * TODO:
 * - Metrics/Lua for lighting and camera sweeping.
 * - Ability to load secondary elements i.e. stages.
 * - Remove support for 2D characters (Lua can do it).
 * - Cleanup!
 *
 * -- Colby
 */
const float CAMERA_REST_DISTANCE = 32.f;
const float CAMERA_REST_LOOK_AT_HEIGHT = -11.f;

const float CAMERA_SWEEP_DISTANCE = 28.f;
const float CAMERA_SWEEP_DISTANCE_VARIANCE = 4.f;
const float CAMERA_SWEEP_HEIGHT_VARIANCE = 7.f;
const float CAMERA_SWEEP_PAN_Y_RANGE_DEGREES = 45.f;
const float CAMERA_SWEEP_PAN_Y_VARIANCE_DEGREES = 60.f;
const float CAMERA_SWEEP_LOOK_AT_HEIGHT = -11.f;

const float CAMERA_STILL_DISTANCE = 26.f;
const float CAMERA_STILL_DISTANCE_VARIANCE = 3.f;
const float CAMERA_STILL_PAN_Y_RANGE_DEGREES = 120.f;
const float CAMERA_STILL_HEIGHT_VARIANCE = 5.f;
const float CAMERA_STILL_LOOK_AT_HEIGHT = -10.f;

const float MODEL_X_ONE_PLAYER = 0;

DancingCharacters::DancingCharacters()
{
	PlayerNumber p = PLAYER_1;
	m_pCharacter = new Model;
	m_i2DAnimState = AS2D_IDLE; // start on idle state
	if (!GAMESTATE->IsPlayerEnabled(p))
		return;

	Character* pChar = GAMESTATE->m_pCurCharacters;
	if (pChar == nullptr)
		return;

	// load in any potential 2D stuff
	std::string sCharacterDirectory = pChar->m_sCharDir;
	std::string sCurrentAnim;
	sCurrentAnim = sCharacterDirectory + "2DIdle";
	if (DoesFileExist(sCurrentAnim +
					  "/BGAnimation.ini")) // check 2D Idle BGAnim exists
	{
		m_bgIdle.Load(sCurrentAnim);
		m_bgIdle->SetXY(DC_X(p), DC_Y(p));
	}

	sCurrentAnim = sCharacterDirectory + "2DMiss";
	if (DoesFileExist(sCurrentAnim +
					  "/BGAnimation.ini")) // check 2D Idle BGAnim exists
	{
		m_bgMiss.Load(sCurrentAnim);
		m_bgMiss->SetXY(DC_X(p), DC_Y(p));
	}

	sCurrentAnim = sCharacterDirectory + "2DGood";
	if (DoesFileExist(sCurrentAnim +
					  "/BGAnimation.ini")) // check 2D Idle BGAnim exists
	{
		m_bgGood.Load(sCurrentAnim);
		m_bgGood->SetXY(DC_X(p), DC_Y(p));
	}

	sCurrentAnim = sCharacterDirectory + "2DGreat";
	if (DoesFileExist(sCurrentAnim +
					  "/BGAnimation.ini")) // check 2D Idle BGAnim exists
	{
		m_bgGreat.Load(sCurrentAnim);
		m_bgGreat->SetXY(DC_X(p), DC_Y(p));
	}

	sCurrentAnim = sCharacterDirectory + "2DFever";
	if (DoesFileExist(sCurrentAnim +
					  "/BGAnimation.ini")) // check 2D Idle BGAnim exists
	{
		m_bgFever.Load(sCurrentAnim);
		m_bgFever->SetXY(DC_X(p), DC_Y(p));
	}

	sCurrentAnim = sCharacterDirectory + "2DFail";
	if (DoesFileExist(sCurrentAnim +
					  "/BGAnimation.ini")) // check 2D Idle BGAnim exists
	{
		m_bgFail.Load(sCurrentAnim);
		m_bgFail->SetXY(DC_X(p), DC_Y(p));
	}

	sCurrentAnim = sCharacterDirectory + "2DWin";
	if (DoesFileExist(sCurrentAnim +
					  "/BGAnimation.ini")) // check 2D Idle BGAnim exists
	{
		m_bgWin.Load(sCurrentAnim);
		m_bgWin->SetXY(DC_X(p), DC_Y(p));
	}

	sCurrentAnim = sCharacterDirectory + "2DWinFever";
	if (DoesFileExist(sCurrentAnim +
					  "/BGAnimation.ini")) // check 2D Idle BGAnim exists
	{
		m_bgWinFever.Load(sCurrentAnim);
		m_bgWinFever->SetXY(DC_X(p), DC_Y(p));
	}

	if (pChar->GetModelPath().empty())
		return;

	m_pCharacter->SetX(MODEL_X_ONE_PLAYER);

	m_pCharacter->LoadMilkshapeAscii(pChar->GetModelPath());
	m_pCharacter->LoadMilkshapeAsciiBones("rest",
										  pChar->GetRestAnimationPath());
	m_pCharacter->LoadMilkshapeAsciiBones("warmup",
										  pChar->GetWarmUpAnimationPath());
	m_pCharacter->LoadMilkshapeAsciiBones("dance",
										  pChar->GetDanceAnimationPath());
	m_pCharacter->SetCullMode(CULL_NONE); // many of the models floating
										  // around have the vertex order
										  // flipped

	m_pCharacter->RunCommands(pChar->m_cmdInit);
}

DancingCharacters::~DancingCharacters()
{
	delete m_pCharacter;
}

void
DancingCharacters::LoadNextSong()
{
	// initial camera sweep is still
	m_CameraDistance = CAMERA_REST_DISTANCE;
	m_CameraPanYStart = 0;
	m_CameraPanYEnd = 0;
	m_fCameraHeightStart = CAMERA_REST_LOOK_AT_HEIGHT;
	m_fCameraHeightEnd = CAMERA_REST_LOOK_AT_HEIGHT;
	m_fLookAtHeight = CAMERA_REST_LOOK_AT_HEIGHT;
	m_fThisCameraStartBeat = 0;
	m_fThisCameraEndBeat = 0;

	ASSERT(GAMESTATE->m_pCurSong != NULL);
	m_fThisCameraEndBeat = GAMESTATE->m_pCurSong->GetFirstBeat();

	if (GAMESTATE->IsPlayerEnabled(PLAYER_1))
		m_pCharacter->PlayAnimation("rest");
}

int
Neg1OrPos1()
{
	return RandomInt(2) != 0 ? -1 : +1;
}

void
DancingCharacters::Update(float fDelta)
{
	if (GAMESTATE->m_Position.m_bFreeze || GAMESTATE->m_Position.m_bDelay) {
		// spin the camera Matrix-style
		m_CameraPanYStart += fDelta * 40;
		m_CameraPanYEnd += fDelta * 40;
	} else {
		// make the characters move
		float fBPM = GAMESTATE->m_Position.m_fCurBPS * 60;
		float fUpdateScale = SCALE(fBPM, 60.f, 300.f, 0.75f, 1.5f);
		CLAMP(fUpdateScale, 0.75f, 1.5f);

		/* It's OK for the animation to go slower than natural when we're
		 * at a very low music rate. */
		fUpdateScale *= GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

		if (GAMESTATE->IsPlayerEnabled(PLAYER_1))
			m_pCharacter->Update(fDelta * fUpdateScale);
	}

	static bool bWasGameplayStarting = false;
	bool bGameplayStarting = GAMESTATE->m_bGameplayLeadIn;
	if (!bWasGameplayStarting && bGameplayStarting) {
		if (GAMESTATE->IsPlayerEnabled(PLAYER_1))
			m_pCharacter->PlayAnimation("warmup");
	}
	bWasGameplayStarting = bGameplayStarting;

	static float fLastBeat = GAMESTATE->m_Position.m_fSongBeat;
	float firstBeat = GAMESTATE->m_pCurSong->GetFirstBeat();
	float fThisBeat = GAMESTATE->m_Position.m_fSongBeat;
	if (fLastBeat < firstBeat && fThisBeat >= firstBeat) {
		m_pCharacter->PlayAnimation("dance");
	}
	fLastBeat = fThisBeat;

	// time for a new sweep?
	if (GAMESTATE->m_Position.m_fSongBeat > m_fThisCameraEndBeat) {
		if (RandomInt(6) >= 4) {
			// sweeping camera
			m_CameraDistance =
			  CAMERA_SWEEP_DISTANCE +
			  RandomInt(-1, 1) * CAMERA_SWEEP_DISTANCE_VARIANCE;
			m_CameraPanYStart = m_CameraPanYEnd =
			  RandomInt(-1, 1) * CAMERA_SWEEP_PAN_Y_RANGE_DEGREES;
			m_fCameraHeightStart = m_fCameraHeightEnd =
			  CAMERA_STILL_LOOK_AT_HEIGHT;

			m_CameraPanYEnd +=
			  RandomInt(-1, 1) * CAMERA_SWEEP_PAN_Y_VARIANCE_DEGREES;
			m_fCameraHeightStart = m_fCameraHeightEnd =
			  m_fCameraHeightStart +
			  RandomInt(-1, 1) * CAMERA_SWEEP_HEIGHT_VARIANCE;

			float fCameraHeightVariance =
			  RandomInt(-1, 1) * CAMERA_SWEEP_HEIGHT_VARIANCE;
			m_fCameraHeightStart -= fCameraHeightVariance;
			m_fCameraHeightEnd += fCameraHeightVariance;

			m_fLookAtHeight = CAMERA_SWEEP_LOOK_AT_HEIGHT;
		} else {
			// still camera
			m_CameraDistance =
			  CAMERA_STILL_DISTANCE +
			  RandomInt(-1, 1) * CAMERA_STILL_DISTANCE_VARIANCE;
			m_CameraPanYStart = m_CameraPanYEnd =
			  Neg1OrPos1() * CAMERA_STILL_PAN_Y_RANGE_DEGREES;
			m_fCameraHeightStart = m_fCameraHeightEnd =
			  CAMERA_SWEEP_LOOK_AT_HEIGHT +
			  Neg1OrPos1() * CAMERA_STILL_HEIGHT_VARIANCE;

			m_fLookAtHeight = CAMERA_STILL_LOOK_AT_HEIGHT;
		}

		auto iCurBeat = static_cast<int>(GAMESTATE->m_Position.m_fSongBeat);
		iCurBeat -= iCurBeat % 8;

		m_fThisCameraStartBeat = static_cast<float>(iCurBeat);
		m_fThisCameraEndBeat = float(iCurBeat + 8);
	}
}

void
DancingCharacters::Change2DAnimState(PlayerNumber pn, int iState)
{
	ASSERT(pn < NUM_PLAYERS);
	ASSERT(iState < AS2D_MAXSTATES);

	m_i2DAnimState = iState;
}

void
DancingCharacters::DrawPrimitives()
{
	DISPLAY->CameraPushMatrix();

	float fPercentIntoSweep;
	if (m_fThisCameraStartBeat == m_fThisCameraEndBeat)
		fPercentIntoSweep = 0;
	else
		fPercentIntoSweep = SCALE(GAMESTATE->m_Position.m_fSongBeat,
								  m_fThisCameraStartBeat,
								  m_fThisCameraEndBeat,
								  0.f,
								  1.f);
	float fCameraPanY =
	  SCALE(fPercentIntoSweep, 0.f, 1.f, m_CameraPanYStart, m_CameraPanYEnd);
	float fCameraHeight = SCALE(
	  fPercentIntoSweep, 0.f, 1.f, m_fCameraHeightStart, m_fCameraHeightEnd);

	RageVector3 m_CameraPoint(0, fCameraHeight, -m_CameraDistance);
	RageMatrix CameraRot;
	RageMatrixRotationY(&CameraRot, fCameraPanY);
	RageVec3TransformCoord(&m_CameraPoint, &m_CameraPoint, &CameraRot);

	RageVector3 m_LookAt(0, m_fLookAtHeight, 0);

	DISPLAY->LoadLookAt(45, m_CameraPoint, m_LookAt, RageVector3(0, 1, 0));

	bool bFailed = STATSMAN->m_CurStageStats.m_player.m_bFailed;
	bool bDanger = m_bDrawDangerLight;

	DISPLAY->SetLighting(true);

	RageColor ambient = bFailed ? RageColor(0.2f, 0.1f, 0.1f, 1)
								: (bDanger ? RageColor(0.4f, 0.1f, 0.1f, 1)
										   : RageColor(0.4f, 0.4f, 0.4f, 1));
	RageColor diffuse = bFailed ? RageColor(0.4f, 0.1f, 0.1f, 1)
								: (bDanger ? RageColor(0.8f, 0.1f, 0.1f, 1)
										   : RageColor(1, 0.95f, 0.925f, 1));
	RageColor specular = RageColor(0.8f, 0.8f, 0.8f, 1);
	DISPLAY->SetLightDirectional(
	  0, ambient, diffuse, specular, RageVector3(-3, -7.5f, +9));

	if (PREFSMAN->m_bCelShadeModels) {
		m_pCharacter->DrawCelShaded();

		DISPLAY->SetLightOff(0);
		DISPLAY->SetLighting(false);
	} else {
		m_pCharacter->Draw();

		DISPLAY->SetLightOff(0);
		DISPLAY->SetLighting(false);
	}
	DISPLAY->CameraPopMatrix();

	/*
	// Ugly! -Colby
	// now draw any potential 2D stuff
	{
		if(m_bgIdle.IsLoaded() && m_i2DAnimState[p] == AS2D_IDLE)
			m_bgIdle->Draw();
		if(m_bgMiss[p].IsLoaded() && m_i2DAnimState[p] == AS2D_MISS)
			m_bgMiss[p]->Draw();
		if(m_bgGood[p].IsLoaded() && m_i2DAnimState[p] == AS2D_GOOD)
			m_bgGood[p]->Draw();
		if(m_bgGreat[p].IsLoaded() && m_i2DAnimState[p] == AS2D_GREAT)
			m_bgGreat[p]->Draw();
		if(m_bgFever[p].IsLoaded() && m_i2DAnimState[p] == AS2D_FEVER)
			m_bgFever[p]->Draw();
		if(m_bgWinFever[p].IsLoaded() && m_i2DAnimState[p] == AS2D_WINFEVER)
			m_bgWinFever[p]->Draw();
		if(m_bgWin[p].IsLoaded() && m_i2DAnimState[p] == AS2D_WIN)
			m_bgWin[p]->Draw();
		if(m_bgFail[p].IsLoaded() && m_i2DAnimState[p] == AS2D_FAIL)
			m_bgFail[p]->Draw();
	}
	 */
}
