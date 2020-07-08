#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Singletons/GameState.h"
#include "LyricDisplay.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/Models/Songs/SongOptions.h"

#include <algorithm>

static ThemeMetric<float> IN_LENGTH("LyricDisplay", "InLength");
static ThemeMetric<float> OUT_LENGTH("LyricDisplay", "OutLength");

LyricDisplay::LyricDisplay()
{
	m_textLyrics[0].SetName("LyricBack");
	ActorUtil::LoadAllCommands(m_textLyrics[0], "LyricDisplay");
	m_textLyrics[0].LoadFromFont(THEME->GetPathF("LyricDisplay", "text"));
	this->AddChild(&m_textLyrics[0]);

	m_textLyrics[1].SetName("LyricFront");
	ActorUtil::LoadAllCommands(m_textLyrics[1], "LyricDisplay");
	m_textLyrics[1].LoadFromFont(THEME->GetPathF("LyricDisplay", "text"));
	this->AddChild(&m_textLyrics[1]);

	Init();
}

void
LyricDisplay::Init()
{
	for (auto& m_textLyric : m_textLyrics)
		m_textLyric.SetText("");
	m_iCurLyricNumber = 0;

	m_fLastSecond = -500;
	m_bStopped = false;
}

void
LyricDisplay::Stop()
{
	m_bStopped = true;
}

void
LyricDisplay::Update(float fDeltaTime)
{
	if (m_bStopped)
		return;

	ActorFrame::Update(fDeltaTime);

	if (GAMESTATE->m_pCurSong == nullptr)
		return;

	// If the song has changed (in a course), reset.
	if (GAMESTATE->m_Position.m_fMusicSeconds < m_fLastSecond)
		Init();
	m_fLastSecond = GAMESTATE->m_Position.m_fMusicSeconds;

	if (m_iCurLyricNumber >= GAMESTATE->m_pCurSong->m_LyricSegments.size())
		return;

	const Song* pSong = GAMESTATE->m_pCurSong;
	const auto fStartTime =
	  (pSong->m_LyricSegments[m_iCurLyricNumber].m_fStartTime) -
	  IN_LENGTH.GetValue();

	if (GAMESTATE->m_Position.m_fMusicSeconds < fStartTime)
		return;

	// Clamp this lyric to the beginning of the next or the end of the music.
	float fEndTime;
	if (m_iCurLyricNumber + 1 < GAMESTATE->m_pCurSong->m_LyricSegments.size())
		fEndTime = pSong->m_LyricSegments[m_iCurLyricNumber + 1].m_fStartTime;
	else
		fEndTime = pSong->GetLastSecond();

	const auto fDistance =
	  fEndTime - pSong->m_LyricSegments[m_iCurLyricNumber].m_fStartTime;
	const auto fTweenBufferTime = IN_LENGTH.GetValue() + OUT_LENGTH.GetValue();

	/* If it's negative, two lyrics are so close together that there's no time
	 * to tween properly. Lyrics should never be this brief, anyway, so just
	 * skip it. */
	auto fShowLength = std::max(fDistance - fTweenBufferTime, 0.0f);

	// Make lyrics show faster for faster song rates.
	fShowLength /= GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

	const auto& seg = GAMESTATE->m_pCurSong->m_LyricSegments[m_iCurLyricNumber];

	LuaThreadVariable var1("LyricText", seg.m_sLyric);
	LuaThreadVariable var2("LyricDuration", LuaReference::Create(fShowLength));
	LuaThreadVariable var3("LyricColor", LuaReference::Create(seg.m_Color));

	PlayCommand("Changed");

	m_iCurLyricNumber++;
}
