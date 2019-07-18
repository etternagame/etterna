#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/BackgroundUtil.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Foreground.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/GameState.h"

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
		std::string sBGName = change.m_def.m_sFile1,
				sLuaFile = pSong->GetSongDir() + sBGName + "/default.lua",
				sXmlFile = pSong->GetSongDir() + sBGName + "/default.xml";

		LoadedBGA bga;
		if (DoesFileExist(sLuaFile)) {
			LOG->Warn("Mod map detected, invalidating sequential assumption.");
			TimingData* td =
			  GAMESTATE->m_pCurSteps
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
