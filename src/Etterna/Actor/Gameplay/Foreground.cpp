#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/BackgroundUtil.h"
#include "Foreground.h"
#include "Etterna/Singletons/GameState.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Core/Services/Locator.hpp"

#include <algorithm>

Foreground::~Foreground()
{
	Unload();
}

void
Foreground::Unload()
{
	for (auto& m_BGAnimation : m_BGAnimations)
		delete m_BGAnimation.m_bga;
	m_BGAnimations.clear();
	m_SubActors.clear();
	m_fLastMusicSeconds = -9999;
	m_pSong = nullptr;
}

void
Foreground::LoadFromSong(const Song* pSong)
{
	// Song graphics can get very big; never keep them in memory.
	const auto OldPolicy = TEXTUREMAN->GetDefaultTexturePolicy();
	TEXTUREMAN->SetDefaultTexturePolicy(RageTextureID::TEX_VOLATILE);

	m_pSong = pSong;
	for (const auto& bgc : pSong->GetForegroundChanges()) {
		const auto& change = bgc;
		auto sBGName = change.m_def.m_sFile1,
			 sLuaFile = pSong->GetSongDir() + sBGName + "/default.lua",
			 sXmlFile = pSong->GetSongDir() + sBGName + "/default.xml";

		LoadedBGA bga;
		if (DoesFileExist(sLuaFile)) {
			Locator::getLogger()->warn("Mod map detected, invalidating sequential assumption.");
			auto* td = GAMESTATE->m_pCurSteps->GetTimingData();
			td->InvalidateSequentialAssmption();

			bga.m_bga = ActorUtil::MakeActor(sLuaFile, this);
		} else {
			bga.m_bga =
			  ActorUtil::MakeActor(pSong->GetSongDir() + sBGName, this);
		}
		if (bga.m_bga == nullptr)
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
	const auto fRate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

	for (auto& bga : m_BGAnimations) {
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

			const auto fStartSecond =
			  m_pSong->m_SongTiming.WhereUAtBro(bga.m_fStartBeat);
			const auto fStopSecond =
			  fStartSecond + bga.m_bga->GetTweenTimeLeft();
			bga.m_fStopBeat =
			  m_pSong->m_SongTiming.GetBeatFromElapsedTime(fStopSecond);

			lDeltaTime = GAMESTATE->m_Position.m_fMusicSeconds - fStartSecond;
		} else {
			lDeltaTime =
			  GAMESTATE->m_Position.m_fMusicSeconds - m_fLastMusicSeconds;
		}

		// This shouldn't go down, but be safe:
		lDeltaTime = std::max(lDeltaTime, 0.F);

		bga.m_bga->Update(lDeltaTime / fRate);

		if (GAMESTATE->m_Position.m_fSongBeat > bga.m_fStopBeat) {
			// Finished.
			bga.m_bga->SetVisible(false);
			bga.m_bFinished = true;
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
