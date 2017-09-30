#ifndef Profile_DB
#define Profile_DB

#include "global.h"
#include "GameConstantsAndTypes.h"
#include "HighScore.h"
#include "Profile.h"
#include "sqlite3.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>



class DBProfile {
public:

	ProfileLoadResult LoadDBFromDir(RString dir);
	ProfileLoadResult LoadDBFromDir(RString dir, Profile* profile);

	ProfileLoadResult SaveDBToDir(RString sDir, const Profile* profile) const;

	static void MoveBackupToDir(const RString &sFromDir, const RString &sToDir);

	void SetLoadingProfile(Profile* p) { loadingProfile = p; }
private:
	Profile* loadingProfile;
	int GetChartKeyID(SQLite::Database* db, RString key) const;
	RString GetChartKeyByID(SQLite::Database* db, int id) const;
	int FindOrCreateChartKey(SQLite::Database* db, RString key) const;
	int FindOrCreateSong(SQLite::Database* db, string pack, string song, Difficulty diff) const;

	void LoadFavourites(SQLite::Database* db);
	void LoadPlayLists(SQLite::Database* db);
	void LoadPlayerScores(SQLite::Database* db);
	bool LoadGeneralData(SQLite::Database* db);
	void LoadPermaMirrors(SQLite::Database* db);
	void LoadScoreGoals(SQLite::Database* db);

	void SaveFavourites(SQLite::Database* db, const Profile* profile) const;
	void SavePlayLists(SQLite::Database* db, const Profile* profile) const;
	void SavePlayerScores(SQLite::Database* db, const Profile* profile) const;
	void SaveGeneralData(SQLite::Database* db, const Profile* profile) const;
	void SavePermaMirrors(SQLite::Database* db, const Profile* profile) const;
	void SaveScoreGoals(SQLite::Database* db, const Profile* profile) const;

};


#endif