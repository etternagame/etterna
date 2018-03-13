#ifndef NetworkSyncManager_H
#define NetworkSyncManager_H

#include "PlayerNumber.h"
#include "Difficulty.h"
#include "ScreenNetRoom.h"
#include "RoomWheel.h"
#include "ScreenNetSelectMusic.h"
#include "ScreenSMOnlineLogin.h"
#include "PlayerState.h"
#include "Song.h"
#include "global.h"
#include <queue>
#include "uWS.h"
#include "JsonUtil.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

class LoadingWindow;

const int NETPROTOCOLVERSION=4;
const int NETMAXBUFFERSIZE=1020; //1024 - 4 bytes for EzSockets
const int NETNUMTAPSCORES=8;

// [SMLClientCommands name]
enum NSCommand
{
	NSCPing = 0,
	NSCPingR,		//  1 [SMLC_PingR]
	NSCHello,		//  2 [SMLC_Hello]
	NSCGSR,			//  3 [SMLC_GameStart]
	NSCGON,			//  4 [SMLC_GameOver]
	NSCGSU,			//  5 [SMLC_GameStatusUpdate]
	NSCSU,			//  6 [SMLC_StyleUpdate]
	NSCCM,			//  7 [SMLC_Chat]
	NSCRSG,			//  8 [SMLC_RequestStart]
	NSCUUL,			//  9 [SMLC_Reserved1]
	NSCSMS,			// 10 [SMLC_MusicSelect]
	NSCUPOpts,		// 11 [SMLC_PlayerOpts]
	NSCSMOnline,	// 12 [SMLC_SMO]
	NSCFormatted,	// 13 [SMLC_RESERVED1]
	NSCAttack,		// 14 [SMLC_RESERVED2]
	XML,		// 15 [SMLC_RESERVED3]
	FLU,		// 16 [SMLC_FriendListUpdate]
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

const NSCommand NSServerOffset = (NSCommand)128;

// TODO: Provide a Lua binding that gives access to this data. -aj
struct EndOfGame_PlayerData
{
	int name;
	int score;
	int grade;
	Difficulty difficulty;
	int tapScores[NETNUMTAPSCORES];	//This will be a const soon enough
	RString playerOptions;
};

enum NSScoreBoardColumn
{
	NSSB_NAMES=0,
	NSSB_COMBO,
	NSSB_GRADE,
	NUM_NSScoreBoardColumn,
	NSScoreBoardColumn_Invalid
};

enum ETTServerMessageTypes {
	ettps_hello=0,
	ettps_ping,
	ettps_recievechat,
	ettps_loginresponse,
	ettps_roomlist,
	ettps_recievescore,
	ettps_gameplayleaderboard,
	ettps_createroomresponse,
	ettps_enterroomresponse,
	ettps_selectchart,
	ettps_startchart,
	ettps_deleteroom,
	ettps_newroom,
	ettps_updateroom,
	ettps_end
};
enum ETTClientMessageTypes {
	ettpc_login=0,
	ettpc_ping,
	ettpc_sendchat,
	ettpc_sendscore,
	ettpc_gameplayupdate,
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
	ettpc_end
};
/** @brief A special foreach loop going through each NSScoreBoardColumn. */
#define FOREACH_NSScoreBoardColumn( sc ) FOREACH_ENUM( NSScoreBoardColumn, sc )

struct NetServerInfo
{
	RString Name;
	RString Address;
};

class EzSockets;
class StepManiaLanServer;

class PacketFunctions
{
public:
	unsigned char Data[NETMAXBUFFERSIZE];	//Data
	int Position;				//Other info (Used for following functions)
	int size;					//When sending these pacs, Position should
								//be used; NOT size.

	//Commands used to operate on NetPackets
	uint8_t Read1();
	uint16_t Read2();
	uint32_t Read4();
	RString ReadNT();

	void Write1( uint8_t Data );
	void Write2( uint16_t Data );
	void Write4( uint32_t Data );
	void WriteNT( const RString& Data );

	void ClearPacket();
};

class NetworkSyncManager;
class ScreenNetRoom;
class NetProtocol {
public:
	RString serverName;
	int serverVersion{0}; // ServerVersion
	virtual bool Connect(NetworkSyncManager* n, unsigned short port, RString address) {return false;}
	virtual void close() {}
	virtual void Update(NetworkSyncManager* n, float fDeltaTime) {}
	virtual void CreateNewRoom(RString name, RString desc, RString password) {}
	virtual void SelectUserSong(NetworkSyncManager* n, Song* song) {}
	virtual void EnterRoom(RString name, RString password) {}
	virtual void LeaveRoom(NetworkSyncManager* n) {}
	virtual void RequestRoomInfo(RString name) {}
	virtual void SendChat(const RString& message, string tab, int type) {}
	virtual void ReportNSSOnOff(int i) {}
	virtual void ReportScore(NetworkSyncManager* n, int playerID, int step, int score, int combo, float offset, int numNotes) {}
	virtual void ReportScore(NetworkSyncManager* n, int playerID, int step, int score, int combo, float offset) {}
	virtual void ReportSongOver(NetworkSyncManager* n) {}
	virtual void ReportStyle(NetworkSyncManager* n) {}
	virtual void StartRequest(NetworkSyncManager* n, short position) {}
	virtual	void Login(RString user, RString pass) {}
	virtual	void Logout() {}
	virtual void OnMusicSelect() {};
	virtual void OffMusicSelect() {};
	virtual void OnRoomSelect() {};
	virtual void OffRoomSelect() {};
	virtual void OnOptions() {};
	virtual void OffOptions() {};
	virtual void OnEval() {};
	virtual void OffEval() {};
};
class SMOProtocol : public NetProtocol { // Built on raw tcp
	EzSockets *NetPlayerClient;
	EzSockets *BroadcastReception;
	PacketFunctions m_packet;
	int m_iSalt;
	bool TryConnect(unsigned short port, RString address);
	void SendSMOnline();
public:
	PacketFunctions	SMOnlinePacket;
	SMOProtocol();
	~SMOProtocol();
	bool Connect(NetworkSyncManager* n, unsigned short port, RString address) override; // Connect and say hello
	void close() override;
	void Update(NetworkSyncManager* n, float fDeltaTime) override;
	void SelectUserSong(NetworkSyncManager* n, Song* song) override;
	void CreateNewRoom(RString name, RString desc, RString password) override;
	void EnterRoom(RString name, RString password) override;
	void RequestRoomInfo(RString name) override;
	void ReportPlayerOptions();
	void SendChat(const RString& message, string tab, int type) override;
	void ReportNSSOnOff(int i) override;
	void ReportScore(NetworkSyncManager* n, int playerID, int step, int score, int combo, float offset, int numNotes) override;
	void ReportScore(NetworkSyncManager* n, int playerID, int step, int score, int combo, float offset) override;
	void ReportSongOver(NetworkSyncManager* n) override;
	void ReportStyle(NetworkSyncManager* n) override;
	void StartRequest(NetworkSyncManager* n, short position) override; 
	void ProcessInput(NetworkSyncManager* n);
	void Login(RString user, RString pass) override;
	void OnMusicSelect() override;
	void OffMusicSelect() override;
	void OnRoomSelect() override;
	void OffRoomSelect() override;
	void OnOptions() override;
	void OffOptions() override;
	void OnEval() override;
	void OffEval() override;

	static void DealWithSMOnlinePack(PacketFunctions &SMOnlinePacket, ScreenNetSelectMusic* s);
	static void DealWithSMOnlinePack(PacketFunctions &SMOnlinePacket, ScreenNetRoom* s);
	static int DealWithSMOnlinePack(PacketFunctions &SMOnlinePacket, ScreenSMOnlineLogin* s, RString& response);
};
class ETTProtocol : public NetProtocol { // Websockets using uwebsockets sending json
	uWS::Hub uWSh;
	vector<json> newMessages;
	unsigned int msgId{0};
	bool error{ false };
	uWS::WebSocket<uWS::CLIENT>* ws;
	void FindJsonChart(NetworkSyncManager* n, json& ch);
public:
	string roomName;
	string roomDesc;
	bool Connect(NetworkSyncManager* n, unsigned short port, RString address) override; // Connect and say hello
	void close() override;
	void Update(NetworkSyncManager* n, float fDeltaTime) override;
	void Login(RString user, RString pass) override;
	void Logout() override;
	void SendChat(const RString& message, string tab, int type) override;
	void CreateNewRoom(RString name, RString desc, RString password) override;
	void EnterRoom(RString name, RString password) override;
	void LeaveRoom(NetworkSyncManager* n) override;
	void ReportSongOver(NetworkSyncManager* n) override;
	void SelectUserSong(NetworkSyncManager* n, Song* song) override;
	void StartRequest(NetworkSyncManager* n, short position) override;
	void OnOptions() override;
	void OffOptions() override;
	void OnEval() override;
	void OffEval() override;
	/*
	void ReportScore(NetworkSyncManager* n, int playerID, int step, int score, int combo, float offset, int numNotes) override;
	void ReportScore(NetworkSyncManager* n, int playerID, int step, int score, int combo, float offset) override;
	void ReportStyle(NetworkSyncManager* n) override;
	*/
};
//Regular pair map except [anyString, 0] is the same key
class Chat  {
public:
	map<pair<string, int>, vector<string>> rawMap;

	vector<string>& operator[](const pair<string, int>& p) {
		if (p.second == 0)
			return rawMap.operator[](make_pair(string(""), 0));
		else
			return rawMap.operator[](p);
	}
};
/** @brief Uses ezsockets for primitive song syncing and score reporting. */
class NetworkSyncManager 
{
public:
	NetworkSyncManager( LoadingWindow *ld = NULL );
	~NetworkSyncManager();
	SMOProtocol SMOP;
	ETTProtocol ETTP;
	NetProtocol* curProtocol{ nullptr };
    // If "useSMserver" then send score to server
	void ReportScore( int playerID, int step, int score, int combo, float offset );	
	void ReportScore(int playerID, int step, int score, int combo, float offset, int numNotes);
	void ReportSongOver();
	void ReportStyle(); // Report style, players, and names
	void ReportNSSOnOff( int i );	// Report song selection screen on/off

	void OnMusicSelect();
	void OffMusicSelect();
	void OnRoomSelect();
	void OffRoomSelect();
	void OnOptions();
	void OffOptions();
	void OnEval();
	void OffEval();

	void StartRequest( short position );	// Request a start; Block until granted.
	RString GetServerName();

	void CreateNewRoom(RString name, RString desc="", RString password="");
	void EnterRoom(RString name, RString password="");
	void LeaveRoom();
	void RequestRoomInfo(RString name);


	void PostStartUp( const RString& ServerIP );

	void CloseConnection();

	void DisplayStartupStatus();	// Notify user if connect attempt was successful or not.

	int m_playerLife;	// Life (used for sending to server)

	void Update(float fDeltaTime);

	bool useSMserver;
	bool isSMOnline;
	bool loggedIn;
	string loginResponse; // Failure reason

	Chat chat; //[{Tabname, int}] = vector<line>

	vector<int> m_PlayerStatus;
	int m_ActivePlayers;
	vector<int> m_ActivePlayer;
	vector<RString> m_PlayerNames;

	//friendlist
	vector<std::string> fl_PlayerNames;
	vector<int> fl_PlayerStates;

	// Used for ScreenNetEvaluation
	vector<EndOfGame_PlayerData> m_EvalPlayerData;

	// Used together: 
	bool ChangedScoreboard(int Column);	// Returns true if scoreboard changed since function was last called.
	RString m_Scoreboard[NUM_NSScoreBoardColumn];

	// Used for chatting
	void SendChat(const RString& message, string tab = "", int type = 0); // 0=lobby (ettp only)
	RString m_WaitingChat;


	// Used for song checking/changing
	RString m_sMainTitle;
	RString m_sArtist;
	RString m_sSubTitle;
	RString m_sFileHash;
	string chartkey;
	Song* song{ nullptr };
	Steps* steps{ nullptr };
	Difficulty difficulty;
	int meter;
	int rate;

	int m_sHash;
	int m_iSelectMode;
	void SelectUserSong();

	int GetServerVersion();

	RString m_sChatText;


	StepManiaLanServer *LANserver;


	RString MD5Hex( const RString &sInput );

	void GetListOfLANServers( vector<NetServerInfo>& AllServers );

	// Aldo: Please move this method to a new class, I didn't want to create new files because I don't know how to properly update the files for each platform.
	// I preferred to misplace code rather than cause unneeded headaches to non-windows users, although it would be nice to have in the wiki which files to
	// update when adding new files.
	static unsigned long GetCurrentSMBuild( LoadingWindow* ld );


	int m_startupStatus;	// Used to see if attempt was successful or not.

	void Login(RString user, RString pass);
	void Logout();
	vector<RoomData> m_Rooms;

#if !defined(WITHOUT_NETWORKING)
	SMOStepType TranslateStepType(int score);
	vector<NetServerInfo> m_vAllLANServers;
	bool m_scoreboardchange[NUM_NSScoreBoardColumn];

	//Lua
	void PushSelf(lua_State *L);

private:

	void ProcessInput();
	void StartUp();

	int m_playerID;  // Currently unused, but need to stay
	int m_step;
	int m_score;
	int m_combo;
#endif
};

extern NetworkSyncManager *NSMAN;
 
#endif
 
/*
 * (c) 2003-2004 Charles Lohr, Joshua Allen
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
