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
NotesLoader::GetMainAndSubTitlesFromFullTitle(const RString& sFullTitle,
											  RString& sMainTitleOut,
											  RString& sSubTitleOut)
{
	const RString sLeftSeps[] = { "\t", " -", " ~", " (", " [" };

	for (unsigned i = 0; i < ARRAYLEN(sLeftSeps); i++) {
		size_t iBeginIndex = sFullTitle.find(sLeftSeps[i]);
		if (iBeginIndex == string::npos)
			continue;
		sMainTitleOut = sFullTitle.Left(static_cast<int>(iBeginIndex));
		sSubTitleOut = sFullTitle.substr(iBeginIndex + 1,
										 sFullTitle.size() - iBeginIndex + 1);
		return;
	}
	sMainTitleOut = sFullTitle;
	sSubTitleOut = "";
};

bool
NotesLoader::LoadFromDir(const RString& sPath,
						 Song& out,
						 set<RString>& BlacklistedImages)
{
	vector<RString> list;

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
