#ifndef Profile_XML
#define Profile_XML

#include "Etterna/Globals/global.h"
#include "GameConstantsAndTypes.h"
#include "HighScore.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/FileTypes/XmlFileUtil.h"

class XNode;

class XMLProfile
{
  public:
	static void MoveBackupToDir(std::string sFromDir, std::string sToDir);

	// Etterna profile
	ProfileLoadResult LoadEttFromDir(std::string dir);
	bool SaveEttXmlToDir(std::string sDir, const Profile* profile) const;
	void SetLoadingProfile(Profile* p) { loadingProfile = p; }

  private:
	Profile* loadingProfile;

	ProfileLoadResult LoadEttXmlFromNode(const XNode* pNode);

	void LoadEttGeneralDataFromNode(const XNode* pNode);
	void LoadEttScoresFromNode(const XNode* pNode);
	void LoadFavoritesFromNode(const XNode* pNode);
	void LoadPermaMirrorFromNode(const XNode* pNode);
	void LoadScoreGoalsFromNode(const XNode* pNode);
	void LoadPlaylistsFromNode(const XNode* pNode);

	void LoadScreenshotDataFromNode(const XNode* pNode);

	XNode* SaveEttGeneralDataCreateNode(const Profile* profile) const;
	XNode* SaveEttScoresCreateNode(const Profile* profile) const;
	XNode* SaveEttXmlCreateNode(const Profile* profile) const;

	XNode* SaveFavoritesCreateNode(const Profile* profile) const;
	XNode* SavePermaMirrorCreateNode(const Profile* profile) const;
	XNode* SaveScoreGoalsCreateNode(const Profile* profile) const;
	XNode* SavePlaylistsCreateNode(const Profile* profile) const;
	XNode* SaveScreenshotDataCreateNode(const Profile* profile) const;

	std::string profiledir;
};

#endif
