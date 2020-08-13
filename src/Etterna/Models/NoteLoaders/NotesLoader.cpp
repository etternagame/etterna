#include "Etterna/Globals/global.h"
#include "NotesLoader.h"
#include "NotesLoaderBMS.h"
#include "NotesLoaderDWI.h"
#include "NotesLoaderKSF.h"
#include "NotesLoaderSM.h"
#include "NotesLoaderSMA.h"
#include "NotesLoaderSSC.h"
#include "RageUtil/Utils/RageUtil.h"
#include "NotesLoaderOSU.h"

void
NotesLoader::GetMainAndSubTitlesFromFullTitle(const std::string& sFullTitle,
											  std::string& sMainTitleOut,
											  std::string& sSubTitleOut)
{
	const std::string sLeftSeps[] = { "\t", " -", " ~", " (", " [" };

	for (const auto& sLeftSep : sLeftSeps) {
		size_t iBeginIndex = sFullTitle.find(sLeftSep);
		if (iBeginIndex == std::string::npos)
			continue;
		sMainTitleOut = sFullTitle.substr(0, static_cast<int>(iBeginIndex));
		sSubTitleOut = sFullTitle.substr(iBeginIndex + 1,
										 sFullTitle.size() - iBeginIndex + 1);
		return;
	}
	sMainTitleOut = sFullTitle;
	sSubTitleOut = "";
};

bool
NotesLoader::LoadFromDir(const std::string& sPath,
						 Song& out,
						 std::set<std::string>& BlacklistedImages)
{
	vector<std::string> list;

	BlacklistedImages.clear();
	SSCLoader loaderSSC;
	loaderSSC.GetApplicableFiles(sPath, list);
	if (!list.empty()) {
		if (!loaderSSC.LoadFromDir(sPath, out)) {
			return false;
		}
		return true;
	}
	SMALoader loaderSMA;
	loaderSMA.GetApplicableFiles(sPath, list);
	if (!list.empty())
		return loaderSMA.LoadFromDir(sPath, out);
	SMLoader loaderSM;
	loaderSM.GetApplicableFiles(sPath, list);
	if (!list.empty())
		return loaderSM.LoadFromDir(sPath, out);
	DWILoader::GetApplicableFiles(sPath, list);
	if (!list.empty())
		return DWILoader::LoadFromDir(sPath, out, BlacklistedImages);
	BMSLoader::GetApplicableFiles(sPath, list);
	if (!list.empty())
		return BMSLoader::LoadFromDir(sPath, out);
	/*
	PMSLoader::GetApplicableFiles( sPath, list );
	if( !list.empty() )
		return PMSLoader::LoadFromDir( sPath, out );
	*/
	KSFLoader::GetApplicableFiles(sPath, list);
	if (!list.empty())
		return KSFLoader::LoadFromDir(sPath, out);
	OsuLoader::GetApplicableFiles(sPath, list);
	if (!list.empty())
		return OsuLoader::LoadFromDir(sPath, out);
	return false;
}
