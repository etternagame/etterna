#ifndef Profile_XML
#define Profile_XML

#include "Etterna/Globals/global.h"
#include "GameConstantsAndTypes.h"
#include "Etterna/FileTypes/XmlFile.h"

class XNode;

class XMLProfile
{
  public:
	static void MoveBackupToDir(std::string sFromDir, std::string sToDir);

	// Etterna profile
	auto LoadEttFromDir(std::string dir) -> ProfileLoadResult;
	auto SaveEttXmlToDir(std::string sDir, const Profile* profile) const
	  -> bool;
	void SetLoadingProfile(Profile* p) { loadingProfile = p; }

  private:
	Profile* loadingProfile{};

	auto LoadEttXmlFromNode(const XNode* pNode) -> ProfileLoadResult;

	void LoadEttGeneralDataFromNode(const XNode* pNode);
	void LoadEttScoresFromNode(const XNode* pNode);
	void LoadFavoritesFromNode(const XNode* pNode);
	void LoadPermaMirrorFromNode(const XNode* pNode);
	void LoadScoreGoalsFromNode(const XNode* pNode);
	void LoadPlaylistsFromNode(const XNode* pNode);

	void LoadScreenshotDataFromNode(const XNode* pNode);

	auto SaveEttGeneralDataCreateNode(const Profile* profile) const -> XNode*;
	auto SaveEttScoresCreateNode(const Profile* profile) const -> XNode*;
	auto SaveEttXmlCreateNode(const Profile* profile) const -> XNode*;

	auto SaveFavoritesCreateNode(const Profile* profile) const -> XNode*;
	auto SavePermaMirrorCreateNode(const Profile* profile) const -> XNode*;
	auto SaveScoreGoalsCreateNode(const Profile* profile) const -> XNode*;
	auto SavePlaylistsCreateNode(const Profile* profile) const -> XNode*;
	auto SaveScreenshotDataCreateNode(const Profile* profile) const -> XNode*;

	std::string profiledir;
};

#endif
