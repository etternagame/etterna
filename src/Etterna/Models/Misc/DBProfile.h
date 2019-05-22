#ifndef Profile_DB
#define Profile_DB

#include "Etterna/Globals/global.h"
#include "GameConstantsAndTypes.h"
#include "HighScore.h"
#include "sqlite3.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

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

	static bool WriteReplayData(const HighScore* hs);

  private:
	Profile* loadingProfile{ nullptr };
	static int GetChartKeyID(SQLite::Database* db, std::string key);
	static std::string GetChartKeyByID(SQLite::Database* db, int id);
	static int FindOrCreateChartKey(SQLite::Database* db, std::string key);
	static int FindOrCreateSong(SQLite::Database* db, std::string pack, std::string song);
	static int FindOrCreateChart(SQLite::Database* db,
								 std::string chartkey,
								 std::string pack,
								 std::string song,
								 Difficulty diff);
	static int GetScoreKeyID(SQLite::Database* db, std::string key);
	static int FindOrCreateScoreKey(SQLite::Database* db, std::string key);

	void LoadFavourites(SQLite::Database* db);
	void LoadPlayLists(SQLite::Database* db);
	void LoadPlayerScores(SQLite::Database* db);
	bool LoadGeneralData(SQLite::Database* db);
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