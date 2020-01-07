#ifndef SONG_CACHE_INDEX_H
#define SONG_CACHE_INDEX_H

#include "Etterna/FileTypes/IniFile.h"
#include "Etterna/Models/Misc/TimingData.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "arch/LoadingWindow/LoadingWindow.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

class SongDBCacheItem
{
};
class SongCacheIndex
{
	IniFile CacheIndex;
	static RString MangleName(const RString& Name);

	bool OpenDB();
	void ResetDB();
	void DeleteDB();
	void CreateDBTables();
	bool DBEmpty{ true };
	SQLite::Transaction* curTransaction{ nullptr };

  public:
	SQLite::Database* db{ nullptr };
	SongCacheIndex();
	~SongCacheIndex();
	inline std::pair<RString, int> SongFromStatement(Song* song,
												SQLite::Statement& query);
	void LoadHyperCache(LoadingWindow* ld, std::map<RString, Song*>& hyperCache);
	void LoadCache(LoadingWindow* ld,
				   std::vector<std::pair<std::pair<RString, unsigned int>, Song*>*>& cache);
	void DeleteSongFromDBByCondition(std::string& condition);
	void DeleteSongFromDB(Song* songPtr);
	void DeleteSongFromDBByDir(std::string dir);
	void DeleteSongFromDBByDirHash(unsigned int hash);
	static RString GetCacheFilePath(const RString& sGroup,
									const RString& sPath);
	unsigned GetCacheHash(const RString& path) const;
	bool delay_save_cache;

	int64_t InsertStepsTimingData(const TimingData& timing);
	int64_t InsertSteps(const Steps* pSteps, int64_t songID);
	bool LoadSongFromCache(Song* song, std::string dir);
	bool CacheSong(Song& song, std::string dir);
	void StartTransaction();
	void FinishTransaction();
};

extern SongCacheIndex*
  SONGINDEX; // global and accessible from anywhere in our program

#endif
