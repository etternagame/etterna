#ifndef Profile_DB
#define Profile_DB

#include "global.h"
#include "GameConstantsAndTypes.h"
#include "HighScore.h"
#include "Profile.h"
#include "SQLiteCpp/SQLiteCpp.h"



class DBProfile {
public:

	ProfileLoadResult LoadDBFromDir(RString dir);
	void LoadFavourites(SQLite::Database db);
	void LoadPlayLists(SQLite::Database db);
	void LoadPlayerScores(SQLite::Database db);
	void LoadGeneralData(SQLite::Database db);
	void LoadPermaMirrors(SQLite::Database db);
	void LoadScoreGoals(SQLite::Database db);

	bool SaveDBToDir(RString sDir);
	void SaveFavourites(SQLite::Database db);
	void SavePlayLists(SQLite::Database db);
	void SavePlayerScores(SQLite::Database db);
	void SaveGeneralData(SQLite::Database db);
	void SavePermaMirrors(SQLite::Database db);
	void SaveScoreGoals(SQLite::Database db);

	static void MoveBackupToDir(const RString &sFromDir, const RString &sToDir);

	Profile* profile;
	RString profiledir;
};


#endif