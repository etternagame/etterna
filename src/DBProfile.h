#ifndef Profile_DB
#define Profile_DB

#include "global.h"
#include "GameConstantsAndTypes.h"
#include "HighScore.h"
#include "Profile.h"
#include "sqlite3.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>



enum DBProfileMode {
	WriteOnlyWebExport,
	LocalWithReplayData ,
	LocalWithoutReplayData,
	NUM_DBProfileMode,
	DBProfileMode_Invalid,
};

class DBProfile {
public:
	enum 
	ProfileLoadResult LoadDBFromDir(RString dir);
	ProfileLoadResult LoadDBFromDir(RString dir, Profile* profile);

	ProfileLoadResult SaveDBToDir(RString sDir, const Profile* profile, DBProfileMode mode) const;

	void MoveBackupToDir(const RString &sFromDir, const RString &sToDir, DBProfileMode mode);

	void SetLoadingProfile(Profile* p) { loadingProfile = p; }

	static bool WriteReplayData(const HighScore* hs);

private:
	Profile* loadingProfile;
	static int GetChartKeyID(SQLite::Database* db, string key);
	static string GetChartKeyByID(SQLite::Database* db, int id);
	static int FindOrCreateChartKey(SQLite::Database* db, string key);
	static int FindOrCreateSong(SQLite::Database* db, string pack, string song);
	static int FindOrCreateChart(SQLite::Database* db, string chartkey, string pack, string song, Difficulty diff);
	static int GetScoreKeyID(SQLite::Database* db, string key);
	static int FindOrCreateScoreKey(SQLite::Database* db, string key);

	void LoadFavourites(SQLite::Database* db);
	void LoadPlayLists(SQLite::Database* db);
	void LoadPlayerScores(SQLite::Database* db);
	bool LoadGeneralData(SQLite::Database* db);
	void LoadPermaMirrors(SQLite::Database* db);
	void LoadScoreGoals(SQLite::Database* db);

	void SaveFavourites(SQLite::Database* db, const Profile* profile) const;
	void SavePlayLists(SQLite::Database* db, const Profile* profile) const;
	void SavePlayerScores(SQLite::Database* db, const Profile* profile, DBProfileMode mode) const;
	void SaveGeneralData(SQLite::Database* db, const Profile* profile) const;
	void SavePermaMirrors(SQLite::Database* db, const Profile* profile) const;
	void SaveScoreGoals(SQLite::Database* db, const Profile* profile) const;

};


#endif