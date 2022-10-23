#ifndef NetworkSyncManager_H
#define NetworkSyncManager_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/HighScore/HighScore.h"
#include <queue>
#include "rapidjson/fwd.h"
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_INTERNAL_
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
typedef websocketpp::config::asio_tls_client::message_type::ptr wss_message_ptr;
using wss_client = ::websocketpp::client<websocketpp::config::asio_tls_client>;
#include <websocketpp/config/asio_no_tls_client.hpp>
using ws_message_ptr = ::websocketpp::config::asio_client::message_type::ptr;
using ws_client = ::websocketpp::client<websocketpp::config::asio_client>;

class LoadingWindow;

class RoomData;
class ScreenNetSelectMusic;
class ScreenSMOnlineLogin;
class Song;
const int NETPROTOCOLVERSION = 4;
const int ETTPCVERSION = 3;
const int NETMAXBUFFERSIZE = 1020; // 1024 - 4 bytes for EzSockets
const int NETNUMTAPSCORES = 8;

class PlayerStageStats;
// [SMLClientCommands name]
enum NSCommand
{
	NSCPing = 0,
	NSCPingR,	  //  1 [SMLC_PingR]
	NSCHello,	  //  2 [SMLC_Hello]
	NSCGSR,		  //  3 [SMLC_GameStart]
	NSCGON,		  //  4 [SMLC_GameOver]
	NSCGSU,		  //  5 [SMLC_GameStatusUpdate]
	NSCSU,		  //  6 [SMLC_StyleUpdate]
	NSCCM,		  //  7 [SMLC_Chat]
	NSCRSG,		  //  8 [SMLC_RequestStart]
	NSCUUL,		  //  9 [SMLC_Reserved1]
	NSCSMS,		  // 10 [SMLC_MusicSelect]
	NSCUPOpts,	  // 11 [SMLC_PlayerOpts]
	NSCSMOnline,  // 12 [SMLC_SMO]
	NSCFormatted, // 13 [SMLC_RESERVED1]
	NSCAttack,	  // 14 [SMLC_RESERVED2]
	XML,		  // 15 [SMLC_RESERVED3]
	FLU,		  // 16 [SMLC_FriendListUpdate]
	NUM_NS_COMMANDS
};

enum SMOStepType
{
	SMOST_UNUSED = 0,
	SMOST_HITMINE,
	SMOST_AVOIDMINE,
	SMOST_MISS,
	SMOST_W5,
	SMOST_W4,
	SMOST_W3,
	SMOST_W2,
	SMOST_W1,
	SMOST_LETGO,
	SMOST_HELD
	/*
	,SMOST_CHECKPOINTMISS,
	SMOST_CHECKPOINTHIT
	 */
};

const NSCommand NSServerOffset = static_cast<NSCommand>(128);

// TODO(Sam): Provide a Lua binding that gives access to this data. -aj
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

enum ETTServerMessageTypes
{
	ettps_hello = 0,
	ettps_ping,
	ettps_recievechat,
	ettps_loginresponse,
	ettps_roomlist,
	ettps_lobbyuserlist,
	ettps_lobbyuserlistupdate,
	ettps_recievescore,
	ettps_mpleaderboardupdate,
	ettps_createroomresponse,
	ettps_enterroomresponse,
	ettps_selectchart,
	ettps_startchart,
	ettps_deleteroom,
	ettps_newroom,
	ettps_updateroom,
	ettps_roomuserlist,
	ettps_chartrequest,
	ettps_roompacklist,
	ettps_end
};
enum ETTClientMessageTypes
{
	ettpc_login = 0,
	ettpc_ping,
	ettpc_sendchat,
	ettpc_sendscore,
	ettpc_mpleaderboardupdate,
	ettpc_createroom,
	ettpc_enterroom,
	ettpc_leaveroom,
	ettpc_selectchart,
	ettpc_startchart,
	ettpc_gameover,
	ettpc_haschart,
	ettpc_missingchart,
	ettpc_startingchart,
	ettpc_notstartingchart,
	ettpc_openoptions,
	ettpc_closeoptions,
	ettpc_openeval,
	ettpc_closeeval,
	ettpc_logout,
	ettpc_hello,
	ettpc_end
};
/** @brief A special foreach loop going through each NSScoreBoardColumn. */
#define FOREACH_NSScoreBoardColumn(sc) FOREACH_ENUM(NSScoreBoardColumn, sc)

struct NetServerInfo
{
	std::string Name;
	std::string Address;
};

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

class EzSockets;
class StepManiaLanServer;
class GameplayScore
{
  public:
	float wife;
	std::string jdgstr;
};

class PacketFunctions
{
  public:
	unsigned char Data[NETMAXBUFFERSIZE]; // Data
	int Position; // Other info (Used for following functions)
	int size;	  // When sending these pacs, Position should
			  // be used; NOT size.

	// Commands used to operate on NetPackets
	auto Read1() -> uint8_t;
	auto Read2() -> uint16_t;
	auto Read4() -> uint32_t;
	auto ReadNT() -> std::string;

	void Write1(uint8_t Data);
	void Write2(uint16_t Data);
	void Write4(uint32_t Data);
	void WriteNT(const std::string& Data);

	void ClearPacket();
};

class NetworkSyncManager;
class ScreenNetRoom;
class NetProtocol
{
  public:
	std::string serverName;
	int serverVersion{ 0 }; // ServerVersion
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
	virtual void RequestRoomInfo(std::string name) {}
	virtual void SendChat(const std::string& message, std::string tab, int type)
	{
	}
	virtual void ReportNSSOnOff(int i) {}
	virtual void ReportScore(NetworkSyncManager* n,
							 int playerID,
							 int step,
							 int score,
							 int combo,
							 float offset,
							 int numNotes)
	{
	}
	virtual void ReportScore(NetworkSyncManager* n,
							 int playerID,
							 int step,
							 int score,
							 int combo,
							 float offset)
	{
	}
	virtual void ReportHighScore(HighScore* hs, PlayerStageStats& pss) {}
	virtual void ReportSongOver(NetworkSyncManager* n) {}
	virtual void ReportStyle(NetworkSyncManager* n) {}
	virtual void StartRequest(NetworkSyncManager* n, short position) {}
	virtual void Login(std::string user, std::string pass) {}
	virtual void Logout() {}
	virtual void OnMusicSelect(){};
	virtual void OffMusicSelect(){};
	virtual void OnRoomSelect(){};
	virtual void OffRoomSelect(){};
	virtual void OnOptions(){};
	virtual void OffOptions(){};
	virtual void OnEval(){};
	virtual void OffEval(){};
	virtual void SendMPLeaderboardUpdate(float wife, std::string& jdgstr){};
};

class ETTProtocol : public NetProtocol
{ // Websockets using websocketpp sending json
	std::unique_ptr<std::thread> thread;
	std::mutex messageBufferMutex;
	std::vector<std::unique_ptr<rapidjson::Document>> newMessages;
	unsigned int msgId{ 0 };
	bool error{ false };
	std::string errorMsg;
	std::shared_ptr<ws_client> client{ nullptr };
	std::shared_ptr<wss_client> secure_client{ nullptr };
	std::shared_ptr<websocketpp::connection_hdl> hdl{ nullptr };
	void FindJsonChart(NetworkSyncManager* n, rapidjson::Value& ch);
	int state = 0; // 0 = ready, 1 = playing, 2 = evalScreen, 3 = options, 4 =
				   // notReady(unkown reason)
  public:
	~ETTProtocol();
	bool waitingForTimeout{ false };
	bool creatingRoom{ false };
	clock_t timeoutStart = 0;
	double timeout = 0;
	std::function<void(void)> onTimeout;
	std::string roomName;
	std::string roomDesc;
	bool inRoom{ false };
	auto Connect(NetworkSyncManager* n,
				 unsigned short port,
				 std::string address) -> bool override; // Connect and say hello
	void close() override;
	void Update(NetworkSyncManager* n, float fDeltaTime) override;
	void Login(std::string user, std::string pass) override;
	void Logout() override;
	void SendChat(const std::string& message,
				  std::string tab,
				  int type) override;
	void CreateNewRoom(std::string name,
					   std::string desc,
					   std::string password) override;
	void EnterRoom(std::string name, std::string password) override;
	void LeaveRoom(NetworkSyncManager* n) override;
	void ReportSongOver(NetworkSyncManager* n) override;
	void SelectUserSong(NetworkSyncManager* n, Song* song) override;
	void OnMusicSelect() override;
	void OnOptions() override;
	void OffOptions() override;
	void OnEval() override;
	void OffEval() override;
	void SendMPLeaderboardUpdate(float wife, std::string& jdgstr) override;
	void ReportHighScore(HighScore* hs, PlayerStageStats& pss) override;
	void Send(const char* msg);
	/*
	void ReportScore(NetworkSyncManager* n, int playerID, int step, int score,
	int combo, float offset, int numNotes) override; void
	ReportScore(NetworkSyncManager* n, int playerID, int step, int score, int
	combo, float offset) override; void ReportStyle(NetworkSyncManager* n)
	override;
	*/
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
/** @brief Uses ezsockets for primitive song syncing and score reporting. */
class NetworkSyncManager
{
  public:
	NetworkSyncManager(LoadingWindow* ld = nullptr);
	~NetworkSyncManager();
	ETTProtocol ETTP;
	NetProtocol* curProtocol{ nullptr };
	// If "useSMserver" then send score to server
	void ReportScore(int playerID,
					 int step,
					 int score,
					 int combo,
					 float offset);
	void ReportScore(int playerID,
					 int step,
					 int score,
					 int combo,
					 float offset,
					 int numNotes);
	void ReportHighScore(HighScore* hs, PlayerStageStats& pss);
	void ReportSongOver();
	void ReportStyle();			// Report style, players, and names
	void ReportNSSOnOff(int i); // Report song selection screen on/off

	void OnMusicSelect();
	void OffMusicSelect();
	void OnRoomSelect();
	void OffRoomSelect();
	void OnOptions();
	void OffOptions();
	void OnEval();
	void OffEval();

	void StartRequest(short position); // Request a start; Block until granted.
	auto GetServerName() -> std::string;

	void CreateNewRoom(std::string name,
					   std::string desc = "",
					   std::string password = "");
	void EnterRoom(std::string name, std::string password = "");
	void LeaveRoom();
	void RequestRoomInfo(std::string name);

	void PostStartUp(const std::string& ServerIP);

	auto IsETTP() -> bool;

	void CloseConnection();

	void DisplayStartupStatus(); // Notify user if connect attempt was
								 // successful or not.

	int m_playerLife; // Life (used for sending to server)

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

	// friendlist
	std::vector<std::string> fl_PlayerNames;
	std::vector<int> fl_PlayerStates;

	// Used for ScreenNetEvaluation
	std::vector<EndOfGame_PlayerData> m_EvalPlayerData;

	// Used together:
	auto ChangedScoreboard(int Column)
	  -> bool; // Returns true if scoreboard changed
			   // since function was last called.
	std::string m_Scoreboard[NUM_NSScoreBoardColumn];

	std::set<std::string> lobbyuserlist;

	void SendMPLeaderboardUpdate(float wife, std::string& jdgstr);

	// Used for chatting
	void SendChat(const std::string& message,
				  std::string tab = "",
				  int type = 0); // 0=lobby (ettp only)
	std::string m_WaitingChat;

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

	int m_iSelectMode;
	void SelectUserSong();

	auto GetServerVersion() -> int;

	std::string m_sChatText;

	StepManiaLanServer* LANserver;

	void GetListOfLANServers(std::vector<NetServerInfo>& AllServers);

	// Aldo: Please move this method to a new class, I didn't want to create new
	// files because I don't know how to properly update the files for each
	// platform. I preferred to misplace code rather than cause unneeded
	// headaches to non-windows users, although it would be nice to have in the
	// wiki which files to update when adding new files.
	static auto GetCurrentSMBuild(LoadingWindow* ld) -> unsigned long;

	int m_startupStatus; // Used to see if attempt was successful or not.

	void Login(std::string user, std::string pass);
	void Logout();
	std::vector<RoomData> m_Rooms;
	std::vector<ChartRequest*> requests;
	std::vector<ChartRequest*> staleRequests;

	auto TranslateStepType(int score) -> SMOStepType;
	std::vector<NetServerInfo> m_vAllLANServers;
	bool m_scoreboardchange[NUM_NSScoreBoardColumn];

	// Lua
	void PushSelf(lua_State* L);

  private:
	void ProcessInput();
	void StartUp();

	int m_playerID; // Currently unused, but need to stay
	int m_step;
	int m_score;
	int m_combo;
};

extern NetworkSyncManager* NSMAN;

#endif
