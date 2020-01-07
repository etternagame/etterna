#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "BGAnimation.h"
#include "BGAnimationLayer.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/FileTypes/IniFile.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Utils/RageUtil.h"

REGISTER_ACTOR_CLASS(BGAnimation);

BGAnimation::BGAnimation() = default;

BGAnimation::~BGAnimation()
{
	DeleteAllChildren();
}

static bool
CompareLayerNames(const RString& s1, const RString& s2)
{
	int i1, i2;
	int ret;

	ret = sscanf(s1, "Layer%d", &i1);
	ASSERT(ret == 1);
	ret = sscanf(s2, "Layer%d", &i2);
	ASSERT(ret == 1);
	return i1 < i2;
}

void
BGAnimation::AddLayersFromAniDir(const RString& _sAniDir, const XNode* pNode)
{
	const RString& sAniDir = _sAniDir;

	{
		std::vector<RString> vsLayerNames;
		FOREACH_CONST_Child(pNode, pLayer)
		{
			if (strncmp(pLayer->GetName().c_str(), "Layer", 5) == 0)
				vsLayerNames.push_back(pLayer->GetName());
		}

		std::sort(vsLayerNames.begin(), vsLayerNames.end(), CompareLayerNames);

		FOREACH_CONST(RString, vsLayerNames, s)
		{
			const RString& sLayer = *s;
			const XNode* pKey = pNode->GetChild(sLayer);
			ASSERT(pKey != NULL);

			RString sImportDir;
			if (pKey->GetAttrValue("Import", sImportDir)) {
				bool bCond;
				if (pKey->GetAttrValue("Condition", bCond) && !bCond)
					continue;

				// import a whole BGAnimation
				sImportDir = sAniDir + sImportDir;
				CollapsePath(sImportDir);

				if (sImportDir.Right(1) != "/")
					sImportDir += "/";

				ASSERT_M(IsADirectory(sImportDir),
						 sImportDir + " isn't a directory");

				RString sPathToIni = sImportDir + "BGAnimation.ini";

				IniFile ini2;
				ini2.ReadFile(sPathToIni);

				AddLayersFromAniDir(sImportDir, &ini2);
			} else {
				// import as a single layer
				BGAnimationLayer* bgLayer = new BGAnimationLayer;
				bgLayer->LoadFromNode(pKey);
				this->AddChild(bgLayer);
			}
		}
	}
}

void
BGAnimation::LoadFromAniDir(const RString& _sAniDir)
{
	DeleteAllChildren();

	if (_sAniDir.empty())
		return;

	RString sAniDir = _sAniDir;
	if (sAniDir.Right(1) != "/")
		sAniDir += "/";

	ASSERT_M(IsADirectory(sAniDir), sAniDir + " isn't a directory");

	RString sPathToIni = sAniDir + "BGAnimation.ini";
	{
		// This is an 3.0 and before-style BGAnimation (not using .ini)

		// loading a directory of layers
		std::vector<RString> asImagePaths;
		ASSERT(sAniDir != "");

		GetDirListing(sAniDir + "*.png", asImagePaths, false, true);
		GetDirListing(sAniDir + "*.jpg", asImagePaths, false, true);
		GetDirListing(sAniDir + "*.jpeg", asImagePaths, false, true);
		GetDirListing(sAniDir + "*.gif", asImagePaths, false, true);
		GetDirListing(sAniDir + "*.ogv", asImagePaths, false, true);
		GetDirListing(sAniDir + "*.avi", asImagePaths, false, true);
		GetDirListing(sAniDir + "*.mpg", asImagePaths, false, true);
		GetDirListing(sAniDir + "*.mpeg", asImagePaths, false, true);

		SortRStringArray(asImagePaths);

		for (unsigned i = 0; i < asImagePaths.size(); i++) {
			const RString sPath = asImagePaths[i];
			if (Basename(sPath).Left(1) == "_")
				continue; // don't directly load files starting with an
						  // underscore
			BGAnimationLayer* pLayer = new BGAnimationLayer;
			pLayer->LoadFromAniLayerFile(asImagePaths[i]);
			AddChild(pLayer);
		}
	}
}

void
BGAnimation::LoadFromNode(const XNode* pNode)
{
	RString sDir;
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
		apActorCommands ap =
		  ActorUtil::ParseActorCommands(ssprintf("sleep,%f", fLengthSeconds));
		pActor->AddCommand("On", ap);
		AddChild(pActor);
	}
}
