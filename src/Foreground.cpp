#include "global.h"
#include "ActorUtil.h"
#include "BackgroundUtil.h"
#include "Foreach.h"
#include "Foreground.h"
#include "GameState.h"
#include "PrefsManager.h"
#include "RageTextureManager.h"
#include "RageUtil.h"
#include "Song.h"
#include "GameState.h"

Foreground::~Foreground()
{
	Unload();
}

void
Foreground::Unload()
{
	for (unsigned i = 0; i < m_BGAnimations.size(); ++i)
		delete m_BGAnimations[i].m_bga;
	m_BGAnimations.clear();
	m_SubActors.clear();
	m_fLastMusicSeconds = -9999;
	m_pSong = NULL;
}

void
Foreground::LoadFromSong(const Song* pSong)
{
	// Song graphics can get very big; never keep them in memory.
	RageTextureID::TexPolicy OldPolicy = TEXTUREMAN->GetDefaultTexturePolicy();
	TEXTUREMAN->SetDefaultTexturePolicy(RageTextureID::TEX_VOLATILE);

	m_pSong = pSong;
	FOREACH_CONST(BackgroundChange, pSong->GetForegroundChanges(), bgc)
	{
		const BackgroundChange& change = *bgc;
		RString sBGName = change.m_def.m_sFile1,
				sLuaFile = pSong->GetSongDir() + sBGName + "/default.lua",
				sXmlFile = pSong->GetSongDir() + sBGName + "/default.xml";

		LoadedBGA bga;
		if (DoesFileExist(sLuaFile)) {
			LOG->Warn("Mod map detected, invalidating sequential assumption.");
			TimingData* td =
			  GAMESTATE->m_pCurSteps[GAMESTATE->GetMasterPlayerNumber()]
				->GetTimingData();
			td->InvalidateSequentialAssmption();

			bga.m_bga = ActorUtil::MakeActor(sLuaFile, this);
		} else {
			bga.m_bga =
			  ActorUtil::MakeActor(pSong->GetSongDir() + sBGName, this);
		}
		if (bga.m_bga == NULL)
			continue;
		bga.m_bga->SetName(sBGName);
		// ActorUtil::MakeActor calls LoadFromNode to load the actor, and
		// LoadFromNode takes care of running the InitCommand, so do not run the
		// InitCommand here. -Kyz
		bga.m_fStartBeat = change.m_fStartBeat;
		bga.m_bFinished = false;

		bga.m_bga->SetVisible(false);

		this->AddChild(bga.m_bga);
		m_BGAnimations.push_back(bga);
	}

	TEXTUREMAN->SetDefaultTexturePolicy(OldPolicy);

	this->SortByDrawOrder();
}

void
Foreground::Update(float fDeltaTime)
{
	// Calls to Update() should *not* be scaled by music rate. Undo it.
	const float fRate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

	for (unsigned i = 0; i < m_BGAnimations.size(); ++i) {
		LoadedBGA& bga = m_BGAnimations[i];

		if (GAMESTATE->m_Position.m_fSongBeat < bga.m_fStartBeat) {
			// The animation hasn't started yet.
			continue;
		}

		if (bga.m_bFinished)
			continue;

		/* Update the actor even if we're about to hide it, so queued commands
		 * are always run. */
		float lDeltaTime;
		if (!bga.m_bga->GetVisible()) {
			bga.m_bga->SetVisible(true);
			bga.m_bga->PlayCommand("On");

			const float fStartSecond =
			  m_pSong->m_SongTiming.WhereUAtBro(bga.m_fStartBeat);
			const float fStopSecond =
			  fStartSecond + bga.m_bga->GetTweenTimeLeft();
			bga.m_fStopBeat =
			  m_pSong->m_SongTiming.GetBeatFromElapsedTime(fStopSecond);

			lDeltaTime = GAMESTATE->m_Position.m_fMusicSeconds - fStartSecond;
		} else {
			lDeltaTime =
			  GAMESTATE->m_Position.m_fMusicSeconds - m_fLastMusicSeconds;
		}

		// This shouldn't go down, but be safe:
		lDeltaTime = max(lDeltaTime, 0);

		bga.m_bga->Update(lDeltaTime / fRate);

		if (GAMESTATE->m_Position.m_fSongBeat > bga.m_fStopBeat) {
			// Finished.
			bga.m_bga->SetVisible(false);
			bga.m_bFinished = true;
			continue;
		}
	}

	m_fLastMusicSeconds = GAMESTATE->m_Position.m_fMusicSeconds;
}

void
Foreground::HandleMessage(const Message& msg)
{
	// We want foregrounds to behave as if their On command happens at the
	// starting beat, not when the Foreground object receives an On command.
	// So don't propagate that; we'll call it ourselves.
	if (msg.GetName() == "On")
		Actor::HandleMessage(msg);
	else
		ActorFrame::HandleMessage(msg);
}

/*
 * (c) 2004 Glenn Maynard
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
