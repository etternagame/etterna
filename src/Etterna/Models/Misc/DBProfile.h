#ifndef Profile_DB
#define Profile_DB

#include "GameConstantsAndTypes.h"
#include "Etterna/Models/HighScore/HighScore.h"
#include <SQLiteCpp/SQLiteCpp.h>

class Profile;

enum DBProfileMode
{
	WriteOnlyWebExport,
	LocalWithReplayData,
	LocalWithoutReplayData,
	NUM_DBProfileMode,
	DBProfileMode_Invalid,
};

class DBProfile
{
  public:
	enum ProfileLoadResult LoadDBFromDir(const std::string& dir);
	ProfileLoadResult LoadDBFromDir(const std::string& dir, Profile* profile);

	ProfileLoadResult SaveDBToDir(const std::string& sDir,
								  const Profile* profile,
								  DBProfileMode mode) const;

	static void MoveBackupToDir(const std::string& sFromDir,
						 const std::string& sToDir,
						 DBProfileMode mode);

	void SetLoadingProfile(Profile* p) { loadingProfile = p; }

	static auto WriteReplayData(const HighScore* hs) -> bool;

  private:
	Profile* loadingProfile{ nullptr };
	static auto GetChartKeyID(SQLite::Database* db, const std::string& key) -> int;
	static auto GetChartKeyByID(SQLite::Database* db, int id) -> std::string;
	static auto FindOrCreateChartKey(SQLite::Database* db, const std::string& key)
	  -> int;
	static auto FindOrCreateSong(SQLite::Database* db,
								 const std::string& pack,
								 const std::string& song) -> int;
	static auto FindOrCreateChart(SQLite::Database* db,
								  const std::string& chartkey,
								  const std::string& pack,
								  const std::string& song,
								  Difficulty diff) -> int;
	static auto GetScoreKeyID(SQLite::Database* db, const std::string& key) -> int;
	static auto FindOrCreateScoreKey(SQLite::Database* db, const std::string& key)
	  -> int;

	void LoadFavourites(SQLite::Database* db);
	void LoadPlayLists(SQLite::Database* db);
	static void LoadPlayerScores(SQLite::Database* db);
	auto LoadGeneralData(SQLite::Database* db) -> bool;
	void LoadPermaMirrors(SQLite::Database* db);
	void LoadScoreGoals(SQLite::Database* db);

	static void SaveFavourites(SQLite::Database* db, const Profile* profile);
	static void SavePlayLists(SQLite::Database* db, const Profile* profile);
	static void SavePlayerScores(SQLite::Database* db,
						  const Profile* profile,
						  DBProfileMode mode);
	static void SaveGeneralData(SQLite::Database* db, const Profile* profile);
	static void SavePermaMirrors(SQLite::Database* db, const Profile* profile);
	static void SaveScoreGoals(SQLite::Database* db, const Profile* profile);
};

#endif
