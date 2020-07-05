#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/LuaManager.h"
#include "MeterDisplay.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/FileTypes/XmlFile.h"

REGISTER_ACTOR_CLASS(MeterDisplay);
REGISTER_ACTOR_CLASS(SongMeterDisplay);

MeterDisplay::MeterDisplay() = default;

void
MeterDisplay::Load(const std::string& sStreamPath,
				   float fStreamWidth,
				   const std::string& sTipPath)
{
	m_sprStream.Load(sStreamPath);
	this->AddChild(m_sprStream);

	m_sprTip.Load(sTipPath);
	this->AddChild(m_sprTip);

	SetStreamWidth(fStreamWidth);
	SetPercent(0.5f);
}

void
MeterDisplay::LoadFromNode(const XNode* pNode)
{
	if (PREFSMAN->m_verbose_log > 1)
		LOG->Trace("MeterDisplay::LoadFromNode(%s)",
				   ActorUtil::GetWhere(pNode).c_str());

	const XNode* pStream = pNode->GetChild("Stream");
	if (pStream == NULL) {
		LuaHelpers::ReportScriptErrorFmt(
		  "%s: MeterDisplay: missing the \"Stream\" attribute",
		  ActorUtil::GetWhere(pNode).c_str());
		return;
	}
	m_sprStream.LoadActorFromNode(pStream, this);
	m_sprStream->SetName("Stream");
	// LOAD_ALL_COMMANDS( m_sprStream );
	this->AddChild(m_sprStream);

	const XNode* pChild = pNode->GetChild("Tip");
	if (pChild != NULL) {
		m_sprTip.LoadActorFromNode(pChild, this);
		m_sprTip->SetName("Tip");
		// LOAD_ALL_COMMANDS( m_sprTip );
		this->AddChild(m_sprTip);
	}

	float fStreamWidth = 0;
	pNode->GetAttrValue("StreamWidth", fStreamWidth);
	SetStreamWidth(fStreamWidth);

	SetPercent(0.5f);

	ActorFrame::LoadFromNode(pNode);
}

void
MeterDisplay::SetPercent(float fPercent)
{
	ASSERT(fPercent >= 0 && fPercent <= 1);

	m_sprStream->SetCropRight(1 - fPercent);

	if (m_sprTip.IsLoaded())
		m_sprTip->SetX(
		  SCALE(fPercent, 0.f, 1.f, -m_fStreamWidth / 2, m_fStreamWidth / 2));
}

void
MeterDisplay::SetStreamWidth(float fStreamWidth)
{
	m_fStreamWidth = fStreamWidth;
	m_sprStream->SetZoomX(m_fStreamWidth / m_sprStream->GetUnzoomedWidth());
}

void
SongMeterDisplay::Update(float fDeltaTime)
{
	if (GAMESTATE->m_pCurSong) {
		float fSongStartSeconds = GAMESTATE->m_pCurSong->GetFirstSecond();
		float fSongEndSeconds = GAMESTATE->m_pCurSong->GetLastSecond();
		float fPercentPositionSong =
		  SCALE(GAMESTATE->m_Position.m_fMusicSeconds,
				fSongStartSeconds,
				fSongEndSeconds,
				0.0f,
				1.0f);
		CLAMP(fPercentPositionSong, 0, 1);

		SetPercent(fPercentPositionSong);
	}

	MeterDisplay::Update(fDeltaTime);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

class LunaMeterDisplay : public Luna<MeterDisplay>
{
  public:
	static int SetStreamWidth(T* p, lua_State* L)
	{
		p->SetStreamWidth(FArg(1));
		COMMON_RETURN_SELF;
	}

	LunaMeterDisplay() { ADD_METHOD(SetStreamWidth); }
};

LUA_REGISTER_DERIVED_CLASS(MeterDisplay, ActorFrame)
// lua end
