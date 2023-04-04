#include "Etterna/Globals/global.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Sound/RageSound.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RandomSample.h"
#include "Etterna/Globals/rngthing.h"

#include <algorithm>

RandomSample::RandomSample()
{
	m_iIndexLastPlayed = -1;
}

RandomSample::~RandomSample()
{
	UnloadAll();
}

bool
RandomSample::Load(const std::string& sFilePath, int iMaxToLoad)
{
	if (GetExtension(sFilePath).empty())
		return LoadSoundDir(sFilePath, iMaxToLoad);
	return LoadSound(sFilePath);
}

void
RandomSample::UnloadAll()
{
	for (auto& m_pSample : m_pSamples)
		delete m_pSample;
	m_pSamples.clear();
}

bool
RandomSample::LoadSoundDir(std::string sDir, int iMaxToLoad)
{
	if (sDir.empty())
		return true;

	// make sure there's a slash at the end of this path
	ensure_slash_at_end(sDir);

	std::vector<std::string> arraySoundFiles;
	FILEMAN->GetDirListing(sDir + "*.mp3", arraySoundFiles, ONLY_FILE);
	FILEMAN->GetDirListing(sDir + "*.oga", arraySoundFiles, ONLY_FILE);
	FILEMAN->GetDirListing(sDir + "*.ogg", arraySoundFiles, ONLY_FILE);
	FILEMAN->GetDirListing(sDir + "*.wav", arraySoundFiles, ONLY_FILE);

	std::shuffle(
	  arraySoundFiles.begin(), arraySoundFiles.end(), g_RandomNumberGenerator);
	arraySoundFiles.resize(
	  std::min(arraySoundFiles.size(), static_cast<size_t>(iMaxToLoad)));

	for (auto& arraySoundFile : arraySoundFiles)
		LoadSound(sDir + arraySoundFile);

	return true;
}

bool
RandomSample::LoadSound(const std::string& sSoundFilePath)
{
	Locator::getLogger()->trace("RandomSample::LoadSound({})", sSoundFilePath.c_str());

	auto* pSS = new RageSound;
	if (!pSS->Load(sSoundFilePath)) {
		Locator::getLogger()->warn("RandomSample: Error loading \"{}\": {}",
				   sSoundFilePath.c_str(),
				   pSS->GetError().c_str());
		delete pSS;
		return false;
	}

	m_pSamples.push_back(pSS);

	return true;
}

int
RandomSample::GetNextToPlay()
{
	// play one of the samples
	if (m_pSamples.empty())
		return -1;

	auto iIndexToPlay = 0;
	for (auto i = 0; i < 5; i++) {
		iIndexToPlay = RandomInt(m_pSamples.size());
		if (iIndexToPlay != m_iIndexLastPlayed)
			break;
	}

	m_iIndexLastPlayed = iIndexToPlay;
	return iIndexToPlay;
}

void
RandomSample::PlayRandom()
{
	const auto iIndexToPlay = GetNextToPlay();
	if (iIndexToPlay == -1)
		return;
	m_pSamples[iIndexToPlay]->Play(true);
}

void
RandomSample::PlayCopyOfRandom()
{
	const auto iIndexToPlay = GetNextToPlay();
	if (iIndexToPlay == -1)
		return;
	m_pSamples[iIndexToPlay]->PlayCopy(true);
}

void
RandomSample::Stop()
{
	if (m_iIndexLastPlayed == -1) // nothing is currently playing
		return;

	m_pSamples[m_iIndexLastPlayed]->Stop();
}
