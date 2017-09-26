
#include "global.h"
#include "Profile.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "ProfileManager.h"
#include "NoteData.h"
#include "DBProfile.h"
#include "RageFile.h"
#include "RageFileDriverDeflate.h"
#include "GameState.h"
#include "GameManager.h"
#include "LuaManager.h"
#include "NoteData.h"
#include "RageFileManager.h"

#include "ScoreManager.h"
#include "CryptManager.h"
#include "Song.h"
#include "SongManager.h"
#include "Steps.h"
#include "SQLiteCpp/SQLiteCpp.h"

ProfileLoadResult DBProfile::LoadDBFromDir(RString dir)
{
	try {
		// Open a database file
		SQLite::Database    db(dir, SQLite::OPEN_READWRITE);
		try {
			// Use the db
			LoadGeneralData(db);
			LoadFavourites(db);
			LoadPermaMirror(db);
			LoadScoreGoals(db);
			LoadPlayLists(db);
			LoadPlayerScores(db);
		}
		catch (std::exception& e)
		{
			return ProfileLoadResult_FailedTampered;
		}
		return ProfileLoadResult_Success;
	}
	catch (std::exception& e)
	{
		return ProfileLoadResult_FailedNoProfile;
	}

}

void DBProfile::LoadFavourites(SQLite::Database db)
{
	SQLite::Statement   query(db, "SELECT * FROM favourites");
	while (query.executeStep())
	{
		const char* key = query.getColumn(1);
		profile->FavoritedCharts.emplace(SONGMAN->ReconcileBustedKeys(key));
	}
	SONGMAN->SetFavoritedStatus(profile->FavoritedCharts);
	SONGMAN->MakePlaylistFromFavorites(profile->FavoritedCharts);
}


ProfileLoadResult DBProfile::SaveDBFromDir(RString dir)
{
	try {
		// Open or create a database file
		SQLite::Database    db(dir, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
		try {
			// Use the db
			SaveGeneralData(db);
			SaveFavourites(db);
			SavePermaMirror(db);
			SaveScoreGoals(db);
			SavePlayLists(db);
			SavePlayerScores(db);
		}
		catch (std::exception& e)
		{
			return ProfileLoadResult_FailedTampered;
		}
		return ProfileLoadResult_Success;
	}
	catch (std::exception& e)
	{
		return ProfileLoadResult_FailedNoProfile;
	}

}

void DBProfile::SaveFavourites(SQLite::Database db)
{
	// Begin transaction
	SQLite::Transaction transaction(db);
	db.exec("DROP TABLE IF EXISTS favourites");
	db.exec("CREATE TABLE favourites (id INTEGER PRIMARY KEY, chartkey TEXT)");
	RString insert;
	FOREACHS_CONST(string, profile->FavoritedCharts, ck)
	{
		insert = "INSERT INTO favourites VALUES (NULL, \"";
		insert.append(ck);
		insert.append("\")");
		int nb = db.exec(insert.c_str());
	}

	// Commit transaction
	transaction.commit();
}
