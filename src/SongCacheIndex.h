#ifndef SONG_CACHE_INDEX_H
#define SONG_CACHE_INDEX_H

#include "IniFile.h"
#include "TimingData.h"
#include "Song.h"
#include "Steps.h"

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

class SongCacheIndex
{
	IniFile CacheIndex;
	static RString MangleName( const RString &Name );

	bool OpenDB();
	void ResetDB();
	void DeleteDB();
	void CreateDBTables();
	bool DBEmpty{true};
	SQLite::Database *db;
	SQLite::Transaction *curTransaction;
public:
	SongCacheIndex();
	~SongCacheIndex();
	void ReadFromDisk();
	static RString GetCacheFilePath( const RString &sGroup, const RString &sPath );

	void ReadCacheIndex();
	void SaveCacheIndex();
	void AddCacheIndex( const RString &path, unsigned hash );
	unsigned GetCacheHash( const RString &path ) const;
	bool delay_save_cache;
	
	int InsertStepsTimingData(TimingData timing);
	int InsertSteps(const Steps* pSteps, int songID);
	bool LoadSongFromCache(Song* song, string dir);
	bool CacheSong(Song& song, string dir);
	void StartTransaction();
	void FinishTransaction();
};

extern SongCacheIndex *SONGINDEX;	// global and accessible from anywhere in our program

#endif

/*
 * (c) 2002-2003 Glenn Maynard
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
