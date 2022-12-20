#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "BPMDisplay.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Globals/rngthing.h"

#include <climits>
#include <algorithm>

REGISTER_ACTOR_CLASS(BPMDisplay);

BPMDisplay::BPMDisplay()
{
	m_fBPMFrom = m_fBPMTo = 0;
	m_iCurrentBPM = 0;
	m_BPMS.push_back(0);
	m_fPercentInState = 0;
	m_fCycleTime = 1.0f;
}

void
BPMDisplay::Load()
{
	SET_NO_BPM_COMMAND.Load(m_sName, "SetNoBpmCommand");
	SET_NORMAL_COMMAND.Load(m_sName, "SetNormalCommand");
	SET_CHANGING_COMMAND.Load(m_sName, "SetChangeCommand");
	SET_RANDOM_COMMAND.Load(m_sName, "SetRandomCommand");
	SET_EXTRA_COMMAND.Load(m_sName, "SetExtraCommand");
	CYCLE.Load(m_sName, "Cycle");
	RANDOM_CYCLE_SPEED.Load(m_sName, "RandomCycleSpeed");
	SEPARATOR.Load(m_sName, "Separator");
	SHOW_QMARKS.Load(m_sName, "ShowQMarksInRandomCycle");
	NO_BPM_TEXT.Load(m_sName, "NoBpmText");
	QUESTIONMARKS_TEXT.Load(m_sName, "QuestionMarksText");
	RANDOM_TEXT.Load(m_sName, "RandomText");
	VARIOUS_TEXT.Load(m_sName, "VariousText");
	BPM_FORMAT_STRING.Load(m_sName, "FormatString");

	RunCommands(SET_NORMAL_COMMAND);
}

void
BPMDisplay::LoadFromNode(const XNode* pNode)
{
	BitmapText::LoadFromNode(pNode);
	Load();
}

float
BPMDisplay::GetActiveBPM() const
{
	return m_fBPMTo + (m_fBPMFrom - m_fBPMTo) * m_fPercentInState;
}

void
BPMDisplay::Update(float fDeltaTime)
{
	BitmapText::Update(fDeltaTime);

	if (!static_cast<bool>(CYCLE))
		return;
	if (m_BPMS.empty())
		return; // no bpm

	m_fPercentInState -= fDeltaTime / m_fCycleTime;
	if (m_fPercentInState < 0) {
		// go to next state
		m_fPercentInState = 1; // reset timer

		m_iCurrentBPM = (m_iCurrentBPM + 1) % m_BPMS.size();
		m_fBPMFrom = m_fBPMTo;
		m_fBPMTo = m_BPMS[m_iCurrentBPM];

		if (m_fBPMTo == -1) {
			m_fBPMFrom = -1;
			if (static_cast<bool>(SHOW_QMARKS))
				SetText(
				  (RandomFloat(0, 1) > 0.90f)
					? static_cast<std::string>(QUESTIONMARKS_TEXT)
					: ssprintf(static_cast<std::string>(BPM_FORMAT_STRING),
							   RandomFloat(0, 999)));
			else
				SetText(ssprintf(static_cast<std::string>(BPM_FORMAT_STRING),
								 RandomFloat(0, 999)));
		} else if (m_fBPMFrom == -1) {
			m_fBPMFrom = m_fBPMTo;
		}
	}

	if (m_fBPMTo != -1) {
		const float fActualBPM = GetActiveBPM();
		SetText(
		  ssprintf(static_cast<std::string>(BPM_FORMAT_STRING), fActualBPM));
	}
}

void
BPMDisplay::SetBPMRange(const DisplayBpms& bpms)
{
	ASSERT(!bpms.vfBpms.empty());

	m_BPMS.clear();

	const std::vector<float>& BPMS = bpms.vfBpms;

	bool AllIdentical = true;
	for (unsigned i = 0; i < BPMS.size(); ++i) {
		if (i > 0 && BPMS[i] != BPMS[i - 1])
			AllIdentical = false;
	}

	if (!static_cast<bool>(CYCLE)) {
		int MinBPM = INT_MAX;
		int MaxBPM = INT_MIN;
		for (float i : BPMS) {
			MinBPM = std::min(MinBPM, static_cast<int>(lround(i)));
			MaxBPM = std::max(MaxBPM, static_cast<int>(lround(i)));
		}
		if (MinBPM == MaxBPM) {
			if (MinBPM == -1)
				SetText(RANDOM_TEXT); // random (was "...") -aj
			else
				SetText(ssprintf("%i", MinBPM));
		} else {
			SetText(
			  ssprintf("%i%s%i", MinBPM, SEPARATOR.GetValue().c_str(), MaxBPM));
		}
	} else {
		for (float i : BPMS) {
			m_BPMS.push_back(i);
			if (i != -1)
				m_BPMS.push_back(i); // hold
		}

		m_iCurrentBPM = std::min(
		  1, static_cast<int>(m_BPMS.size())); // start on the first hold
		m_fBPMFrom = BPMS[0];
		m_fBPMTo = BPMS[0];
		m_fPercentInState = 1;
	}

	if (!AllIdentical)
		RunCommands(SET_CHANGING_COMMAND);
	else
		RunCommands(SET_NORMAL_COMMAND);
}

void
BPMDisplay::CycleRandomly()
{
	DisplayBpms bpms;
	bpms.Add(-1);
	SetBPMRange(bpms);

	RunCommands(SET_RANDOM_COMMAND);

	m_fCycleTime = static_cast<float>(RANDOM_CYCLE_SPEED);

	// Go to default value in event of a negative value in the metrics
	if (m_fCycleTime < 0)
		m_fCycleTime = 0.2f;
}

void
BPMDisplay::NoBPM()
{
	m_BPMS.clear();
	SetText(NO_BPM_TEXT);
	RunCommands(SET_NO_BPM_COMMAND);
}

void
BPMDisplay::SetBpmFromSong(const Song* pSong, bool bIgnoreCurrentRate)
{
	ASSERT(pSong != nullptr);
	switch (pSong->m_DisplayBPMType) {
		case DISPLAY_BPM_ACTUAL:
		case DISPLAY_BPM_SPECIFIED: {
			DisplayBpms bpms;
			pSong->GetDisplayBpms(bpms, bIgnoreCurrentRate);
			SetBPMRange(bpms);
			m_fCycleTime = 1.0f;
		} break;
		case DISPLAY_BPM_RANDOM:
			CycleRandomly();
			break;
		default:
			FAIL_M(ssprintf("Invalid display BPM type: %i",
							pSong->m_DisplayBPMType));
	}
}

void
BPMDisplay::SetBpmFromSteps(const Steps* pSteps, bool bIgnoreCurrentRate)
{
	ASSERT(pSteps != nullptr);
	DisplayBpms bpms;
	pSteps->GetDisplayBpms(bpms, bIgnoreCurrentRate);
	SetBPMRange(bpms);
	m_fCycleTime = 1.0f;
}

void
BPMDisplay::SetConstantBpm(float fBPM)
{
	DisplayBpms bpms;
	bpms.Add(fBPM);
	SetBPMRange(bpms);
}

void
BPMDisplay::SetVarious()
{
	m_BPMS.clear();
	m_BPMS.push_back(-1);
	SetText(VARIOUS_TEXT);
}

void
BPMDisplay::SetFromGameState()
{
	if (GAMESTATE->m_pCurSong.Get()) {
		SetBpmFromSong(GAMESTATE->m_pCurSong);
		return;
	}
	NoBPM();
}

// SongBPMDisplay (in-game BPM display)
class SongBPMDisplay : public BPMDisplay
{
  public:
	SongBPMDisplay();
	SongBPMDisplay* Copy() const override;
	void Update(float fDeltaTime) override;

  private:
	float m_fLastGameStateBPM;
};

SongBPMDisplay::SongBPMDisplay()
{
	m_fLastGameStateBPM = 0;
}

void
SongBPMDisplay::Update(float fDeltaTime)
{
	const float fGameStateBPM = GAMESTATE->m_Position.m_fCurBPS * 60.0f;
	if (m_fLastGameStateBPM != fGameStateBPM) {
		m_fLastGameStateBPM = fGameStateBPM;
		SetConstantBpm(fGameStateBPM);
	}

	BPMDisplay::Update(fDeltaTime);
}

REGISTER_ACTOR_CLASS(SongBPMDisplay);

#include "Etterna/Models/Lua/LuaBinding.h"
/** @brief Allow Lua to have access to the BPMDisplay. */
class LunaBPMDisplay : public Luna<BPMDisplay>
{
  public:
	static int SetFromGameState(T* p, lua_State* L)
	{
		p->SetFromGameState();
		COMMON_RETURN_SELF;
	}
	static int SetFromSong(T* p, lua_State* L)
	{
		if (lua_isnoneornil(L, 1)) {
			p->NoBPM();
		} else {
			const Song* pSong = Luna<Song>::check(L, 1, true);
			bool bIgnoreCurrentRate = false;
			if (!lua_isnoneornil(L, 2)) {
				bIgnoreCurrentRate = BArg(2);
			}
			p->SetBpmFromSong(pSong, bIgnoreCurrentRate);
		}
		COMMON_RETURN_SELF;
	}
	static int SetFromSteps(T* p, lua_State* L)
	{
		if (lua_isnoneornil(L, 1)) {
			p->NoBPM();
		} else {
			const Steps* pSteps = Luna<Steps>::check(L, 1, true);
			bool bIgnoreCurrentRate = false;
			if (!lua_isnoneornil(L, 2)) {
				bIgnoreCurrentRate = BArg(2);
			}
			p->SetBpmFromSteps(pSteps, bIgnoreCurrentRate);
		}
		COMMON_RETURN_SELF;
	}
	static int GetText(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetText().c_str());
		return 1;
	}

	LunaBPMDisplay()
	{
		ADD_METHOD(SetFromGameState);
		ADD_METHOD(SetFromSong);
		ADD_METHOD(SetFromSteps);
		ADD_METHOD(GetText);
	}
};

LUA_REGISTER_DERIVED_CLASS(BPMDisplay, BitmapText)
