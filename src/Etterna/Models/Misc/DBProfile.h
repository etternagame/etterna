#ifndef Profile_DB
#define Profile_DB

#include "GameConstantsAndTypes.h"
#include "HighScore.h"
#include <SQLiteCpp/SQLiteCpp.h>

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
	enum ProfileLoadResult LoadDBFromDir(std::string dir);
	ProfileLoadResult LoadDBFromDir(std::string dir, Profile* profile);

	ProfileLoadResult SaveDBToDir(std::string sDir,
								  const Profile* profile,
								  DBProfileMode mode) const;

	void MoveBackupToDir(const std::string& sFromDir,
						 const std::string& sToDir,
						 DBProfileMode mode);

	void SetLoadingProfile(Profile* p) { loadingProfile = p; }

	static auto WriteReplayData(const HighScore* hs) -> bool;

  private:
	Profile* loadingProfile{ nullptr };
	static auto GetChartKeyID(SQLite::Database* db, std::string key) -> int;
	static auto GetChartKeyByID(SQLite::Database* db, int id) -> std::string;
	static auto FindOrCreateChartKey(SQLite::Database* db, std::string key)
	  -> int;
	static auto FindOrCreateSong(SQLite::Database* db,
								 std::string pack,
								 std::string song) -> int;
	static auto FindOrCreateChart(SQLite::Database* db,
								  std::string chartkey,
								  std::string pack,
								  std::string song,
								  Difficulty diff) -> int;
	static auto GetScoreKeyID(SQLite::Database* db, std::string key) -> int;
	static auto FindOrCreateScoreKey(SQLite::Database* db, std::string key)
	  -> int;

	void LoadFavourites(SQLite::Database* db);
	void LoadPlayLists(SQLite::Database* db);
	void LoadPlayerScores(SQLite::Database* db);
	auto LoadGeneralData(SQLite::Database* db) -> bool;
	void LoadPermaMirrors(SQLite::Database* db);
	void LoadScoreGoals(SQLite::Database* db);

	void SaveFavourites(SQLite::Database* db, const Profile* profile) const;
	void SavePlayLists(SQLite::Database* db, const Profile* profile) const;
	void SavePlayerScores(SQLite::Database* db,
						  const Profile* profile,
						  DBProfileMode mode) const;
	void SaveGeneralData(SQLite::Database* db, const Profile* profile) const;
	void SavePermaMirrors(SQLite::Database* db, const Profile* profile) const;
	void SaveScoreGoals(SQLite::Database* db, const Profile* profile) const;
};

#endif
