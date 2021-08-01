#ifndef SONG_CACHE_INDEX_H
#define SONG_CACHE_INDEX_H

#include "Etterna/FileTypes/IniFile.h"
#include "Etterna/Models/Songs/Song.h"
#include "arch/LoadingWindow/LoadingWindow.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

class SongDBCacheItem
{
};
class SongCacheIndex
{
	IniFile CacheIndex;
	static std::string MangleName(const std::string& Name);

	bool OpenDB();
	void ResetDB();
	void DeleteDB();
	void CreateDBTables() const;
	bool DBEmpty{ true };
	SQLite::Transaction* curTransaction{ nullptr };

  public:
	SQLite::Database* db{ nullptr };
	SongCacheIndex();
	~SongCacheIndex();
	inline std::pair<std::string, int> SongFromStatement(
	  Song* song,
	  SQLite::Statement& query) const;
	void LoadHyperCache(LoadingWindow* ld,
						std::map<std::string, Song*>& hyperCache);
	void LoadCache(
	  LoadingWindow* ld,
	  std::vector<std::pair<std::pair<std::string, unsigned int>, Song*>*>&
		cache) const;
	void DeleteSongFromDBByCondition(const std::string& condition) const;
	void DeleteSongFromDB(Song* songPtr) const;
	void DeleteSongFromDBByDir(const std::string& dir) const;
	void DeleteSongFromDBByDirHash(unsigned int hash) const;
	static std::string GetCacheFilePath(const std::string& sGroup,
										const std::string& sPath);
	unsigned GetCacheHash(const std::string& path) const;
	bool delay_save_cache;

	int64_t InsertStepsTimingData(const TimingData& timing) const;
	int64_t InsertSteps(Steps* pSteps, int64_t songID) const;
	bool LoadSongFromCache(Song* song, const std::string& dir);
	bool CacheSong(Song& song, const std::string& dir) const;
	void StartTransaction();
	void FinishTransaction();
};

extern SongCacheIndex*
  SONGINDEX; // global and accessible from anywhere in our program

#endif
