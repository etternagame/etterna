#ifndef Profile_XML
#define Profile_XML

#include "global.h"
#include "GameConstantsAndTypes.h"
#include "HighScore.h"
#include "XmlFile.h"
#include "XmlFileUtil.h"


class XNode;

class XMLProfile {
public:
	static void MoveBackupToDir(const RString &sFromDir, const RString &sToDir);
	// For converting to etterna from stats.xml
	void LoadStatsXmlForConversion();

	// Etterna profile
	ProfileLoadResult LoadEttFromDir(RString dir);
	ProfileLoadResult LoadStatsFromDir(RString dir, bool require_signature);

	bool SaveStatsXmlToDir(RString sDir, bool bSignData, const Profile* profile);
	bool SaveEttXmlToDir(RString sDir, const Profile* profile) const;
	void SetLoadingProfile(Profile* p) { loadingProfile = p; }
private:
	Profile* loadingProfile;
	ProfileLoadResult LoadStatsXmlFromNode(const XNode* pNode, bool bIgnoreEditable = true);

	ProfileLoadResult LoadEttXmlFromNode(const XNode* pNode);

	void LoadEttGeneralDataFromNode(const XNode* pNode);
	void LoadEttScoresFromNode(const XNode* pNode);
	void LoadFavoritesFromNode(const XNode *pNode);
	void LoadPermaMirrorFromNode(const XNode *pNode);
	void LoadScoreGoalsFromNode(const XNode *pNode);
	void LoadPlaylistsFromNode(const XNode *pNode);
	void LoadGeneralDataFromNode(const XNode* pNode);
	void LoadSongScoresFromNode(const XNode* pNode);
	void LoadCategoryScoresFromNode(const XNode* pNode);
	void LoadScreenshotDataFromNode(const XNode* pNode);


	XNode* SaveEttGeneralDataCreateNode(const Profile* profile) const;
	XNode* SaveEttScoresCreateNode(const Profile* profile) const;
	XNode* SaveEttXmlCreateNode(const Profile* profile) const;


	XNode* SaveFavoritesCreateNode(const Profile* profile) const;
	XNode* SavePermaMirrorCreateNode(const Profile* profile) const;
	XNode* SaveScoreGoalsCreateNode(const Profile* profile) const;
	XNode* SavePlaylistsCreateNode(const Profile* profile) const;

	XNode* SaveStatsXmlCreateNode(const Profile* profile) const;

	XNode* SaveGeneralDataCreateNode(const Profile* profile) const;
	XNode* SaveSongScoresCreateNode(const Profile* profile) const;

	XNode* SaveCategoryScoresCreateNode(const Profile* profile) const;
	XNode* SaveScreenshotDataCreateNode(const Profile* profile) const;

	XNode* SaveCoinDataCreateNode(const Profile* profile) const;
	RString profiledir;
};


#endif