#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "BGAnimation.h"
#include "BGAnimationLayer.h"
#include "Etterna/FileTypes/IniFile.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"

#include <algorithm>

REGISTER_ACTOR_CLASS(BGAnimation);

BGAnimation::BGAnimation() = default;

BGAnimation::~BGAnimation()
{
	DeleteAllChildren();
}

static bool
CompareLayerNames(const std::string& s1, const std::string& s2)
{
	int i1, i2;
	int ret;

	ret = sscanf(s1.c_str(), "Layer%d", &i1);
	ASSERT(ret == 1);
	ret = sscanf(s2.c_str(), "Layer%d", &i2);
	ASSERT(ret == 1);
	return i1 < i2;
}

void
BGAnimation::AddLayersFromAniDir(const std::string& _sAniDir,
								 const XNode* pNode)
{
	const auto& sAniDir = _sAniDir;

	{
		std::vector<std::string> vsLayerNames;
		FOREACH_CONST_Child(pNode, pLayer)
		{
			if (strncmp(pLayer->GetName().c_str(), "Layer", 5) == 0)
				vsLayerNames.push_back(pLayer->GetName());
		}

		std::sort(vsLayerNames.begin(), vsLayerNames.end(), CompareLayerNames);

		for (auto& s : vsLayerNames) {
			const auto& sLayer = s;
			const auto* pKey = pNode->GetChild(sLayer);
			ASSERT(pKey != nullptr);

			std::string sImportDir;
			if (pKey->GetAttrValue("Import", sImportDir)) {
				bool bCond;
				if (pKey->GetAttrValue("Condition", bCond) && !bCond)
					continue;

				// import a whole BGAnimation
				sImportDir = sAniDir + sImportDir;
				CollapsePath(sImportDir);

				if (sImportDir.back() != '/')
					sImportDir += "/";

				ASSERT_M(IsADirectory(sImportDir),
						 sImportDir + " isn't a directory");

				auto sPathToIni = sImportDir + "BGAnimation.ini";

				IniFile ini2;
				ini2.ReadFile(sPathToIni);

				AddLayersFromAniDir(sImportDir, &ini2);
			} else {
				// import as a single layer
				auto* bgLayer = new BGAnimationLayer;
				bgLayer->LoadFromNode(pKey);
				this->AddChild(bgLayer);
			}
		}
	}
}

void
BGAnimation::LoadFromAniDir(const std::string& _sAniDir)
{
	DeleteAllChildren();

	if (_sAniDir.empty())
		return;

	auto sAniDir = _sAniDir;
	if (sAniDir.back() != '/')
		sAniDir += "/";

	ASSERT_M(IsADirectory(sAniDir), sAniDir + " isn't a directory");

	auto sPathToIni = sAniDir + "BGAnimation.ini";
	{
		// This is an 3.0 and before-style BGAnimation (not using .ini)

		// loading a directory of layers
		std::vector<std::string> asImagePaths;
		ASSERT(sAniDir != "");

		FILEMAN->GetDirListing(
		  sAniDir + "*.png", asImagePaths, ONLY_FILE, true);
		FILEMAN->GetDirListing(
		  sAniDir + "*.jpg", asImagePaths, ONLY_FILE, true);
		FILEMAN->GetDirListing(
		  sAniDir + "*.jpeg", asImagePaths, ONLY_FILE, true);
		FILEMAN->GetDirListing(
		  sAniDir + "*.gif", asImagePaths, ONLY_FILE, true);
		FILEMAN->GetDirListing(
		  sAniDir + "*.ogv", asImagePaths, ONLY_FILE, true);
		FILEMAN->GetDirListing(
		  sAniDir + "*.avi", asImagePaths, ONLY_FILE, true);
		FILEMAN->GetDirListing(
		  sAniDir + "*.mpg", asImagePaths, ONLY_FILE, true);
		FILEMAN->GetDirListing(
		  sAniDir + "*.mpeg", asImagePaths, ONLY_FILE, true);

		SortStringArray(asImagePaths);

		for (auto& sPath : asImagePaths) {
			if (Basename(sPath).front() == '_')
				continue; // don't directly load files starting with an
						  // underscore
			auto* pLayer = new BGAnimationLayer;
			pLayer->LoadFromAniLayerFile(sPath);
			AddChild(pLayer);
		}
	}
}

void
BGAnimation::LoadFromNode(const XNode* pNode)
{
	std::string sDir;
	if (pNode->GetAttrValue("AniDir", sDir))
		LoadFromAniDir(sDir);

	ActorFrame::LoadFromNode(pNode);

	/* Backwards-compatibility: if a "LengthSeconds" value is present, create a
	 * dummy actor that sleeps for the given length of time. This will extend
	 * GetTweenTimeLeft. */
	float fLengthSeconds = 0;
	if (pNode->GetAttrValue("LengthSeconds", fLengthSeconds)) {
		auto* pActor = new Actor;
		pActor->SetName("BGAnimation dummy");
		pActor->SetVisible(false);
		auto ap =
		  ActorUtil::ParseActorCommands(ssprintf("sleep,%f", fLengthSeconds));
		pActor->AddCommand("On", ap);
		AddChild(pActor);
	}
}
