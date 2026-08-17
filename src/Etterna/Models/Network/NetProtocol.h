#pragma once

#include "Etterna/Models/Misc/PlayerStageStats.h"
#include <string>

class NetworkSyncManager;
struct HighScore;
class Song;

class NetProtocol
{
  public:
	std::string serverName;
	int serverVersion{ 0 };

	virtual auto Connect(NetworkSyncManager* /*n*/,
						 unsigned short /*port*/,
						 std::string /*address*/) -> bool
	{
		return false;
	}
	virtual void close() {}
	virtual void Update(NetworkSyncManager* n, float fDeltaTime) {}
	virtual void CreateNewRoom(std::string name,
							   std::string desc,
							   std::string password)
	{
	}
	virtual void SelectUserSong(NetworkSyncManager* n, Song* song) {}
	virtual void EnterRoom(std::string name, std::string password) {}
	virtual void LeaveRoom(NetworkSyncManager* n) {}

	virtual void SendChat(const std::string& message, std::string tab, int type) {}

	virtual void ReportHighScore(HighScore* hs, PlayerStageStats& pss) {}
	virtual void ReportSongOver(NetworkSyncManager* n) {}

	virtual void StartRequest(NetworkSyncManager* n, short position) {}

	virtual void Login(std::string user, std::string pass) {}
	virtual void Logout() {}

	virtual void OnMusicSelect() {};
	virtual void OffMusicSelect() {};

	virtual void OnOptions() {};
	virtual void OffOptions() {};

	virtual void OnEval() {};
	virtual void OffEval() {};

	virtual void SendMPLeaderboardUpdate(float wife, std::string& jdgstr) {};

	// triggered by button presses in gameplay
	virtual void ReportReplayInput(NetworkSyncManager* n,
								   bool isPress,
								   int col,
								   int row,
								   float fMusicSeconds,
								   float fNoteOffset,
								   int tapNoteType,
								   int tapNoteSubType) {};

	// triggered by a miss in gameplay
	virtual void ReportReplayMiss(NetworkSyncManager* n,
								  int col,
								  int row,
								  int tapNoteType,
								  int tapNoteSubType) {};

	// triggered by completing or dropping a hold in gameplay
	virtual void ReportReplayHold(NetworkSyncManager* n,
								  int col,
								  int row,
								  int subType) {};

	// triggered by hitting a mine in gameplay
	virtual void ReportReplayMine(NetworkSyncManager* n, int row, int col) {};
};
