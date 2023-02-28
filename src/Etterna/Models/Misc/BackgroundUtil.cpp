#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Actor/Gameplay/Background.h"
#include "BackgroundUtil.h"
#include "Etterna/FileTypes/IniFile.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"

#include <algorithm>

bool
BackgroundDef::operator<(const BackgroundDef& other) const
{
#define COMPARE(x)                                                             \
	if ((x) < other.x)                                                         \
		return true;                                                           \
	else if ((x) > other.x)                                                    \
		return false;
	COMPARE(m_sEffect);
	COMPARE(m_sFile1);
	COMPARE(m_sFile2);
	COMPARE(m_sColor1);
	COMPARE(m_sColor2);
#undef COMPARE
	return false;
}

bool
BackgroundDef::operator==(const BackgroundDef& other) const
{
	return m_sEffect == other.m_sEffect && m_sFile1 == other.m_sFile1 &&
		   m_sFile2 == other.m_sFile2 && m_sColor1 == other.m_sColor1 &&
		   m_sColor2 == other.m_sColor2;
}

XNode*
BackgroundDef::CreateNode() const
{
	auto pNode = new XNode("BackgroundDef");

	if (!m_sEffect.empty())
		pNode->AppendAttr("Effect", m_sEffect);
	if (!m_sFile1.empty())
		pNode->AppendAttr("File1", m_sFile1);
	if (!m_sFile2.empty())
		pNode->AppendAttr("File2", m_sFile2);
	if (!m_sColor1.empty())
		pNode->AppendAttr("Color1", m_sColor1);
	if (!m_sColor2.empty())
		pNode->AppendAttr("Color2", m_sColor2);

	return pNode;
}

std::string
BackgroundChange::GetTextDescription() const
{
	std::vector<std::string> vsParts;
	if (!m_def.m_sFile1.empty())
		vsParts.push_back(m_def.m_sFile1);
	if (!m_def.m_sFile2.empty())
		vsParts.push_back(m_def.m_sFile2);
	if (m_fRate != 1.0f)
		vsParts.push_back(ssprintf("%.2f%%", m_fRate * 100));
	if (!m_sTransition.empty())
		vsParts.push_back(m_sTransition);
	if (!m_def.m_sEffect.empty())
		vsParts.push_back(m_def.m_sEffect);
	if (!m_def.m_sColor1.empty())
		vsParts.push_back(m_def.m_sColor1);
	if (!m_def.m_sColor2.empty())
		vsParts.push_back(m_def.m_sColor2);

	if (vsParts.empty())
		vsParts.push_back("(empty)");

	auto s = join("\n", vsParts);
	return s;
}

std::string
BackgroundChange::ToString() const
{
	/* TODO:  Technically we need to double-escape the filename
	 * (because it might contain '=') and then unescape the value
	 * returned by the MsdFile. */
	return ssprintf(
	  "%.3f=%s=%.3f=%d=%d=%d=%s=%s=%s=%s=%s",
	  this->m_fStartBeat,
	  SmEscape(this->m_def.m_sFile1).c_str(),
	  this->m_fRate,
	  this->m_sTransition == SBT_CrossFade,		  // backward compat
	  this->m_def.m_sEffect == SBE_StretchRewind, // backward compat
	  this->m_def.m_sEffect != SBE_StretchNoLoop, // backward compat
	  this->m_def.m_sEffect.c_str(),
	  this->m_def.m_sFile2.c_str(),
	  this->m_sTransition.c_str(),
	  SmEscape(RageColor::NormalizeColorString(this->m_def.m_sColor1)).c_str(),
	  SmEscape(RageColor::NormalizeColorString(this->m_def.m_sColor2)).c_str());
}

const std::string BACKGROUND_EFFECTS_DIR = "BackgroundEffects/";
const std::string BACKGROUND_TRANSITIONS_DIR = "BackgroundTransitions/";
const std::string BG_ANIMS_DIR = "BGAnimations/";

const std::string RANDOM_BACKGROUND_FILE = "-random-";
const std::string NO_SONG_BG_FILE = "-nosongbg-";
const std::string SONG_BACKGROUND_FILE = "songbackground";

const std::string SBE_UpperLeft = "UpperLeft";
const std::string SBE_Centered = "Centered";
const std::string SBE_StretchNormal = "StretchNormal";
const std::string SBE_StretchNoLoop = "StretchNoLoop";
const std::string SBE_StretchRewind = "StretchRewind";
const std::string SBT_CrossFade = "CrossFade";

int
CompareBackgroundChanges(const BackgroundChange& seg1,
						 const BackgroundChange& seg2)
{
	return seg1.m_fStartBeat < seg2.m_fStartBeat;
}

void
BackgroundUtil::SortBackgroundChangesArray(
  std::vector<BackgroundChange>& vBackgroundChanges)
{
	sort(vBackgroundChanges.begin(),
		 vBackgroundChanges.end(),
		 CompareBackgroundChanges);
}

void
BackgroundUtil::AddBackgroundChange(
  std::vector<BackgroundChange>& vBackgroundChanges,
  const BackgroundChange& seg)
{
	std::vector<BackgroundChange>::iterator it;
	it = upper_bound(vBackgroundChanges.begin(),
					 vBackgroundChanges.end(),
					 seg,
					 CompareBackgroundChanges);
	vBackgroundChanges.insert(it, seg);
}

void
BackgroundUtil::GetBackgroundEffects(const std::string& _sName,
									 std::vector<std::string>& vsPathsOut,
									 std::vector<std::string>& vsNamesOut)
{
	auto sName = _sName;
	if (sName.empty())
		sName = "*";

	vsPathsOut.clear();
	FILEMAN->GetDirListing(
	  BACKGROUND_EFFECTS_DIR + sName + ".lua", vsPathsOut, ONLY_FILE, true);

	vsNamesOut.clear();
	for (auto& s : vsPathsOut)
		vsNamesOut.push_back(GetFileNameWithoutExtension(s));
}

void
BackgroundUtil::GetBackgroundTransitions(const std::string& _sName,
										 std::vector<std::string>& vsPathsOut,
										 std::vector<std::string>& vsNamesOut)
{
	auto sName = _sName;
	if (sName.empty())
		sName = "*";

	vsPathsOut.clear();
	FILEMAN->GetDirListing(
	  BACKGROUND_TRANSITIONS_DIR + sName + ".xml", vsPathsOut, ONLY_FILE, true);
	FILEMAN->GetDirListing(
	  BACKGROUND_TRANSITIONS_DIR + sName + ".lua", vsPathsOut, ONLY_FILE, true);

	vsNamesOut.clear();
	for (auto& s : vsPathsOut)
		vsNamesOut.push_back(GetFileNameWithoutExtension(s));
}

void
BackgroundUtil::GetSongBGAnimations(const Song* pSong,
									const std::string& sMatch,
									std::vector<std::string>& vsPathsOut,
									std::vector<std::string>& vsNamesOut)
{
	vsPathsOut.clear();
	if (sMatch.empty()) {
		FILEMAN->GetDirListing(pSong->GetSongDir() + "*", vsPathsOut, true, true);
	} else {
		FILEMAN->GetDirListing(pSong->GetSongDir() + sMatch, vsPathsOut, true, true);
	}

	vsNamesOut.clear();
	for (auto& s : vsPathsOut)
		vsNamesOut.push_back(GetFileNameWithoutExtension(s));
}

void
BackgroundUtil::GetSongMovies(const Song* pSong,
							  const std::string& sMatch,
							  std::vector<std::string>& vsPathsOut,
							  std::vector<std::string>& vsNamesOut)
{
	vsPathsOut.clear();
	if (sMatch.empty()) {
		FILEMAN->GetDirListingWithMultipleExtensions(
		  pSong->GetSongDir() + sMatch,
		  ActorUtil::GetTypeExtensionList(FT_Movie),
		  vsPathsOut,
		  ONLY_FILE,
		  true);
	} else {
		FILEMAN->GetDirListing(
		  pSong->GetSongDir() + sMatch, vsPathsOut, ONLY_FILE, true);
	}

	vsNamesOut.clear();
	for (auto& s : vsPathsOut)
		vsNamesOut.push_back(GetFileNameWithoutExtension(s));
}

void
BackgroundUtil::GetSongBitmaps(const Song* pSong,
							   const std::string& sMatch,
							   std::vector<std::string>& vsPathsOut,
							   std::vector<std::string>& vsNamesOut)
{
	vsPathsOut.clear();
	if (sMatch.empty()) {
		FILEMAN->GetDirListingWithMultipleExtensions(
		  pSong->GetSongDir() + sMatch,
		  ActorUtil::GetTypeExtensionList(FT_Bitmap),
		  vsPathsOut,
		  ONLY_FILE,
		  true);
	} else {
		FILEMAN->GetDirListing(
		  pSong->GetSongDir() + sMatch, vsPathsOut, ONLY_FILE, true);
	}

	vsNamesOut.clear();
	for (auto& s : vsPathsOut)
		vsNamesOut.push_back(GetFileNameWithoutExtension(s));
}

void
BackgroundUtil::GetGlobalBGAnimations(const Song* pSong,
									  const std::string& sMatch,
									  std::vector<std::string>& vsPathsOut,
									  std::vector<std::string>& vsNamesOut)
{
	vsPathsOut.clear();
	FILEMAN->GetDirListing(BG_ANIMS_DIR + sMatch + "*", vsPathsOut, true, true);
	FILEMAN->GetDirListing(BG_ANIMS_DIR + sMatch + "*.xml", vsPathsOut, ONLY_FILE, true);

	vsNamesOut.clear();
	for (auto& s : vsPathsOut)
		vsNamesOut.push_back(GetFileNameWithoutExtension(s));
}

void
BackgroundUtil::BakeAllBackgroundChanges(Song* pSong)
{
	Background bg;
	bg.LoadFromSong(pSong);
	std::vector<BackgroundChange>* vBGChanges[NUM_BackgroundLayer];
	FOREACH_BackgroundLayer(i) vBGChanges[i] = &pSong->GetBackgroundChanges(i);
	bg.GetLoadedBackgroundChanges(vBGChanges);
}
