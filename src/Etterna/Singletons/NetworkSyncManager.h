#ifndef NetworkSyncManager_H
#define NetworkSyncManager_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/HighScore/HighScore.h"

#include "Etterna/Models/Network/NetworkConstants.h"
#include "Etterna/Models/Network/NetProtocol.h"
#include "Etterna/Models/Network/ETTProtocol.h"

#include "rapidjson/fwd.h"
#include <curl/curl.h>

#include <queue>
#include <thread>
#include <mutex>
#include <functional>

class LoadingWindow;
class RoomData;
class ScreenNetSelectMusic;
class ScreenNetRoom;
class ScreenSMOnlineLogin;
class Song;
class PlayerStageStats;

const int NETPROTOCOLVERSION = 4;
const int ETTPCVERSION = 3;
const int NETMAXBUFFERSIZE = 1020; // 1024 - 4 bytes for EzSockets
const int NETNUMTAPSCORES = 8;

class EndOfGame_PlayerData
{
  public:
	int name{0};
	std::string nameStr;
	int grade;
	int score;
	Difficulty difficulty;
	int tapScores[NETNUMTAPSCORES]; // This will be a const soon enough
	HighScore hs;
	std::string playerOptions;
};

/** @brief A special foreach loop going through each NSScoreBoardColumn. */
#define FOREACH_NSScoreBoardColumn(sc) FOREACH_ENUM(NSScoreBoardColumn, sc)

class ChartRequest
{
  public:
	ChartRequest(const char* ck, const char* requester, int rate)
	  : chartkey(ck)
	  , user(requester)
	  , rate(rate)
	{
	}
	const std::string chartkey;
	const std::string user; // User that requested this chart
	const int rate;			// rate * 1000
	void PushSelf(lua_State* L);
};

class GameplayScore
{
  public:
	float wife;
	std::string jdgstr;
};

// Regular pair map except [anyString, 0] is the same key
class Chat
{
  public:
	std::map<std::pair<std::string, int>, std::vector<std::string>> rawMap;

	auto operator[](const std::pair<std::string, int>& p)
	  -> std::vector<std::string>&
	{
		if (p.second == 0) {
			return rawMap.operator[](std::make_pair(std::string(""), 0));
		}
		{
			return rawMap.operator[](p);
		}
	}
};

class NetworkSyncManager
{
  public:
	NetworkSyncManager(LoadingWindow* ld = nullptr);
	~NetworkSyncManager();
	ETTProtocol ETTP;
	NetProtocol* curProtocol{ nullptr };

	void ReportHighScore(HighScore* hs, PlayerStageStats& pss);
	void ReportSongOver();

	// triggered by button presses in gameplay
	void ReportReplayInput(bool isPress,
						   int col,
						   int row,
						   float fMusicSeconds,
						   float fNoteOffset,
						   int tapNoteType,
						   int tapNoteSubType);

	// triggered by a miss in gameplay
	void ReportReplayMiss(int col,
						  int row,
						  int tapNoteType,
						  int tapNoteSubType);

	// triggered by completing or dropping a hold in gameplay
	void ReportReplayHold(int col, int row, int subType);

	// triggered by hitting a mine in gameplay
	void ReportReplayMine(int row, int col);

	void OnMusicSelect();
	void OffMusicSelect();

	void OnOptions();
	void OffOptions();

	void OnEval();
	void OffEval();

	// Request a start; Block until granted.
	void StartRequest(short position);

	auto GetServerName() -> std::string;

	void CreateNewRoom(std::string name,
					   std::string desc = "",
					   std::string password = "");
	void EnterRoom(std::string name, std::string password = "");
	void LeaveRoom();

	void PostStartUp(const std::string& ServerIP);

	auto IsETTP() -> bool;

	void CloseConnection();

	// Notify user if connect attempt was
	// successful or not.
	void DisplayStartupStatus();

	void Update(float fDeltaTime);

	bool useSMserver;
	bool isSMOnline;
	bool loggedIn;
	std::string loggedInUsername;
	std::string loginResponse; // Failure reason

	Chat chat; //[{Tabname, int}] = std::vector<line>

	std::vector<int> m_PlayerStatus;
	int m_ActivePlayers;
	std::vector<int> m_ActivePlayer;
	std::vector<std::string> m_PlayerNames;
	std::vector<bool> m_PlayerReady;
	std::vector<std::string> commonpacks;

	// Used for ScreenNetEvaluation
	std::vector<EndOfGame_PlayerData> m_EvalPlayerData;

	std::set<std::string> lobbyuserlist;

	void SendMPLeaderboardUpdate(float wife, std::string& jdgstr);

	// Used for chatting
	// 0=lobby (ettp only)
	void SendChat(const std::string& message,
				  std::string tab = "",
				  int type = 0);

	/////////////////////////
	// Used for song checking/changing
	std::string m_sMainTitle;
	std::string m_sArtist;
	std::string m_sSubTitle;
	std::string m_sFileHash;
	std::string chartkey;
	Song* song{ nullptr };
	Steps* steps{ nullptr };
	std::map<std::string, GameplayScore> mpleaderboard;
	void PushMPLeaderboard(lua_State* L);
	Difficulty difficulty;
	int meter;
	int rate;
	/////////////////////////

	int m_iSelectMode;
	void SelectUserSong();

	std::string m_sChatText;

	int m_startupStatus; // Used to see if attempt was successful or not.

	void Login(std::string user, std::string pass);
	void Logout();
	auto GetServerVersion() -> int;

	std::vector<RoomData> m_Rooms;
	std::vector<ChartRequest*> requests;
	std::vector<ChartRequest*> staleRequests;

	bool m_scoreboardchange[NUM_NSScoreBoardColumn];

	// Lua
	void PushSelf(lua_State* L);

  private:
	void StartUp();
};

extern NetworkSyncManager* NSMAN;

#endif
