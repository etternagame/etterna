#ifndef Profile_XML
#define Profile_XML

#include "global.h"
#include "GameConstantsAndTypes.h"
#include "HighScore.h"
#include "XmlFile.h"
#include "Profile.h"
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

	bool SaveStatsXmlToDir(RString sDir, bool bSignData);
	bool SaveEttXmlToDir(RString sDir) const;
	Profile* profile;
private:
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


	XNode* SaveEttGeneralDataCreateNode() const;
	XNode* SaveEttScoresCreateNode() const;
	XNode* SaveEttXmlCreateNode() const;


	XNode* SaveFavoritesCreateNode() const;
	XNode* SavePermaMirrorCreateNode() const;
	XNode* SaveScoreGoalsCreateNode() const;
	XNode* SavePlaylistsCreateNode() const;

	XNode* SaveStatsXmlCreateNode() const;

	XNode* SaveGeneralDataCreateNode() const;
	XNode* SaveSongScoresCreateNode() const;

	XNode* SaveCategoryScoresCreateNode() const;
	XNode* SaveScreenshotDataCreateNode() const;

	XNode* SaveCoinDataCreateNode() const;
	RString profiledir;
};


#endif