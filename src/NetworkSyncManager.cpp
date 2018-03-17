#include "global.h"
#include "NetworkSyncManager.h"
#include "uWS.h"
#include "LuaManager.h"
#include "ScreenNetRoom.h"
#include "PlayerStageStats.h"
#include "ScreenNetSelectMusic.h"
#include "LocalizedString.h"
#include "JsonUtil.h"
#include "GameState.h"
#include "Style.h"
#include <cerrno>
#include <chrono>
#include <nlohmann/json.hpp>
using json = nlohmann::json;


NetworkSyncManager *NSMAN;

// Aldo: version_num used by GetCurrentSMVersion()
// XXX: That's probably not what you want... --root

#include "ver.h"

// Maps to associate the strings with the enum values
std::map<ETTClientMessageTypes, std::string> ettClientMessageMap = {
	{ ettpc_login, "login" },
	{ ettpc_ping, "ping" },
	{ ettpc_sendchat, "chat" },
	{ ettpc_sendscore, "score" },
	{ ettpc_gameplayupdate, "gameplayupdate" },
	{ ettpc_createroom, "createroom" },
	{ ettpc_enterroom, "enterroom" },
	{ ettpc_selectchart, "selectchart" },
	{ ettpc_startchart, "startchart" },
	{ ettpc_leaveroom, "leaveroom" },
	{ ettpc_gameover, "gameover" },
	{ ettpc_haschart, "haschart" },
	{ ettpc_missingchart, "missingchart" },
	{ ettpc_startingchart, "startingchart" },
	{ ettpc_notstartingchart, "notstartingchart" },
	{ ettpc_openoptions, "openoptions" },
	{ ettpc_closeoptions, "closeoptions" },
	{ ettpc_openeval, "openeval" },
	{ ettpc_closeeval, "closeeval" },
	{ ettpc_logout, "logout" },
	{ ettpc_hello, "hello" },
};
std::map<std::string, ETTServerMessageTypes> ettServerMessageMap = {
	{ "hello", ettps_hello },
	{ "roomlist", ettps_roomlist },
	{ "ping", ettps_ping },
	{ "chat", ettps_recievechat },
	{ "login", ettps_loginresponse },
	{ "score", ettps_recievescore },
	{ "leaderboard", ettps_gameplayleaderboard },
	{ "createroom", ettps_createroomresponse },
	{ "enterroom", ettps_enterroomresponse },
	{ "selectchart", ettps_selectchart },
	{ "startchart", ettps_startchart },
	{ "deleteroom", ettps_deleteroom },
	{ "newroom", ettps_newroom },
	{ "updateroom", ettps_updateroom },
	{ "userlist", ettps_roomuserlist },
};


#if defined(WITHOUT_NETWORKING)
NetworkSyncManager::NetworkSyncManager( LoadingWindow *ld ) { useSMserver=false; isSMOnline = false; }
NetworkSyncManager::~NetworkSyncManager () { }
void NetworkSyncManager::CloseConnection() { }
void NetworkSyncManager::PostStartUp( const RString& ServerIP ) { }
bool NetworkSyncManager::Connect( const RString& addy, unsigned short port ) { return false; }
RString NetworkSyncManager::GetServerName() { return RString(); }
void NetworkSyncManager::ReportNSSOnOff( int i ) { }
void NetworkSyncManager::OnMusicSelect()
{
}
void NetworkSyncManager::OffMusicSelect() { }
void NetworkSyncManager::OnRoomSelect() { }
void NetworkSyncManager::OffRoomSelect() { }
void NetworkSyncManager::OnOptions() { }
void NetworkSyncManager::OffOptions() { }
void NetworkSyncManager::OnEval() { }
void NetworkSyncManager::OffEval() { }
void NetworkSyncManager::ReportScore( int playerID, int step, int score, int combo, float offset ) { }
void NetworkSyncManager::ReportSongOver() { }
void NetworkSyncManager::ReportStyle() {}
void NetworkSyncManager::StartRequest( short position ) { }
void NetworkSyncManager::DisplayStartupStatus() { }
void NetworkSyncManager::Update( float fDeltaTime ) { }
bool NetworkSyncManager::ChangedScoreboard( int Column ) { return false; }
void NetworkSyncManager::SendChat( const RString& message ) { }
void NetworkSyncManager::SelectUserSong() { }
RString NetworkSyncManager::MD5Hex( const RString &sInput ) { return RString(); }
int NetworkSyncManager::GetSMOnlineSalt() { return 0; }
void NetworkSyncManager::GetListOfLANServers( vector<NetServerInfo>& AllServers ) { }
unsigned long NetworkSyncManager::GetCurrentSMBuild( LoadingWindow* ld ) { return 0; }
#else
#include "ezsockets.h"
#include "ProfileManager.h"
#include "RageLog.h"
#include "ScreenManager.h"
#include "Song.h"
#include "GameState.h"
#include "StatsManager.h"
#include "Steps.h"
#include "ProductInfo.h"
#include "ScreenMessage.h"
#include "GameManager.h"
#include "MessageManager.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "PlayerState.h"
#include "CryptManager.h"
#include "HighScore.h"

AutoScreenMessage(SM_AddToChat);
AutoScreenMessage(SM_ChangeSong);
AutoScreenMessage(SM_GotEval);
AutoScreenMessage(SM_UsersUpdate);
AutoScreenMessage(SM_FriendsUpdate);
AutoScreenMessage(SM_SMOnlinePack);

AutoScreenMessage(ETTP_Disconnect);
AutoScreenMessage(ETTP_LoginResponse);
AutoScreenMessage(ETTP_IncomingChat);
AutoScreenMessage(ETTP_RoomsChange);
AutoScreenMessage(ETTP_SelectChart);
AutoScreenMessage(ETTP_StartChart);
AutoScreenMessage(ETTP_NewScore);

extern Preference<RString> g_sLastServer;
Preference<unsigned int> autoConnectMultiplayer("AutoConnectMultiplayer", 1);

SMOProtocol::SMOProtocol() {
	NetPlayerClient = new EzSockets;
	NetPlayerClient->blocking = false;
	BroadcastReception = new EzSockets;
	BroadcastReception->create(IPPROTO_UDP);
	BroadcastReception->bind(8765);
	BroadcastReception->blocking = false;
}
static LocalizedString INITIALIZING_CLIENT_NETWORK	( "NetworkSyncManager", "Initializing Client Network..." );
NetworkSyncManager::NetworkSyncManager( LoadingWindow *ld )
{
	LANserver = NULL;	//So we know if it has been created yet
	useSMserver = false;
	isSMOnline = false;
	loggedIn = false;
	m_startupStatus = 0;	//By default, connection not tried.
	m_ActivePlayers = 0;
	StartUp();
	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring(L, "NSMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

SMOProtocol::~SMOProtocol() 
{
	if (NetPlayerClient) {
		NetPlayerClient->close();
		SAFE_DELETE(NetPlayerClient);
	}
	if (BroadcastReception)
	{
		BroadcastReception->close();
		SAFE_DELETE(BroadcastReception);
	}
}
NetworkSyncManager::~NetworkSyncManager ()
{

}


void NetworkSyncManager::OnMusicSelect()
{
	if (curProtocol != nullptr)
		curProtocol->OnMusicSelect();
}
void NetworkSyncManager::OffMusicSelect()
{
	if (curProtocol != nullptr)
		curProtocol->OffMusicSelect();
}
void NetworkSyncManager::OnRoomSelect()
{
	if (curProtocol != nullptr)
		curProtocol->OnRoomSelect();
}
void NetworkSyncManager::OffRoomSelect()
{
	if (curProtocol != nullptr)
		curProtocol->OffRoomSelect();
}
void NetworkSyncManager::OnOptions()
{
	if (curProtocol != nullptr)
		curProtocol->OnOptions();
}
void NetworkSyncManager::OffOptions()
{
	if (curProtocol != nullptr)
		curProtocol->OffOptions();
}
void NetworkSyncManager::OnEval()
{
	if (curProtocol != nullptr)
		curProtocol->OnEval();
}
void NetworkSyncManager::OffEval()
{
	if (curProtocol != nullptr)
		curProtocol->OffEval();
}

void SMOProtocol::OffEval()
{
	ReportNSSOnOff(4);
}
void SMOProtocol::OnEval()
{
	ReportNSSOnOff(5);
}
void SMOProtocol::OnMusicSelect()
{
	ReportNSSOnOff(1);
	ReportPlayerOptions();
}
void SMOProtocol::OffMusicSelect()
{
	ReportNSSOnOff(0);
}
void SMOProtocol::OffRoomSelect()
{
	ReportNSSOnOff(6);
}
void SMOProtocol::OnRoomSelect()
{
	ReportNSSOnOff(7);
}
void SMOProtocol::OnOptions()
{
	ReportNSSOnOff(3);
}
void SMOProtocol::OffOptions()
{
	ReportNSSOnOff(1);
	ReportPlayerOptions();
}


void ETTProtocol::OffEval()
{
	if (ws == nullptr)
		return;
	json j;
	j["type"] = ettClientMessageMap[ettpc_closeeval];
	j["id"] = msgId++;
	Send(j.dump().c_str());
	state = 0;
}
void ETTProtocol::OnEval()
{
	if (ws == nullptr)
		return;
	json j;
	j["type"] = ettClientMessageMap[ettpc_openeval];
	j["id"] = msgId++;
	Send(j.dump().c_str());
	state = 2;
}
void ETTProtocol::OnOptions()
{
	if (ws == nullptr)
		return;
	json j;
	j["type"] = ettClientMessageMap[ettpc_openoptions];
	j["id"] = msgId++;
	Send(j.dump().c_str());
	state = 3;
}
void ETTProtocol::OffOptions()
{
	if (ws == nullptr)
		return;
	json j;
	j["type"] = ettClientMessageMap[ettpc_closeoptions];
	j["id"] = msgId++;
	Send(j.dump().c_str());
	state = 0;
}

void SMOProtocol::close()
{
	serverVersion = 0;
	serverName = "";
	NetPlayerClient->close();
}

void ETTProtocol::close()
{
	serverVersion = 0;
	msgId = 0;
	serverName = "";
	roomName = "";
	roomDesc = "";
	uWSh.getDefaultGroup<uWS::SERVER>().close();
	uWSh.getDefaultGroup<uWS::CLIENT>().close();
	uWSh.poll();
}

void NetworkSyncManager::CloseConnection()
{
	if( !useSMserver )
		return; 
	m_sChatText = "";
   	useSMserver = false;
	isSMOnline = false;
	loggedIn = false;
	loginResponse = "";
	m_startupStatus = 0;
	song = nullptr;
	steps = nullptr;
	rate = 0;
	chartkey = "";
	m_sFileHash = "";
	m_sMainTitle = "";
	m_sSubTitle = "";
	m_sArtist = "";
	difficulty = Difficulty_Invalid;
	meter = -1;
	SMOP.close();
	ETTP.close();
	curProtocol = nullptr;
}
bool startsWith(const string& haystack, const string& needle) {
	return needle.length() <= haystack.length()
		&& equal(needle.begin(), needle.end(), haystack.begin());
}
void NetworkSyncManager::PostStartUp(const RString& ServerIP)
{
	RString sAddress;
	unsigned short iPort;
	m_startupStatus = 2;

	size_t cLoc = ServerIP.find(':');
	if (ServerIP.find(':') != RString::npos)
	{
		sAddress = ServerIP.substr(0, cLoc);
		char* cEnd;
		errno = 0;
		iPort = (unsigned short)strtol(ServerIP.substr(cLoc + 1).c_str(), &cEnd, 10);
		if (*cEnd != 0 || errno != 0)
		{
			LOG->Warn("Invalid port");
			return;
		}
	}
	else
	{
		iPort = 8765;
		sAddress = ServerIP;
	}

	chat.rawMap.clear();
	LOG->Info("Attempting to connect to: %s, Port: %i", sAddress.c_str(), iPort);
	curProtocol = nullptr;
	CloseConnection();
	if (ETTP.Connect(this, iPort, sAddress))
		curProtocol = &ETTP;
	else
		if (SMOP.Connect(this, iPort, sAddress))
			curProtocol = &SMOP;
	if (curProtocol == nullptr)
		return;
	g_sLastServer.Set(ServerIP);
	loggedIn = false;
	useSMserver = true;
	m_startupStatus = 1;	// Connection attempt successful
	song = nullptr;
	steps = nullptr;
	rate = 0;
	chartkey = "";
	m_sFileHash = "";
	m_sMainTitle = "";
	m_sSubTitle = "";
	m_sArtist = "";
	difficulty = Difficulty_Invalid;
	meter = -1;
	LOG->Info("Server Version: %d %s", curProtocol->serverVersion, curProtocol->serverName.c_str());
}

bool ETTProtocol::Connect(NetworkSyncManager * n, unsigned short port, RString address)
{
	n->isSMOnline = false;
	msgId = 0;
	error = false;
	uWSh.onDisconnection([this, n](uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, size_t length) {
		LOG->Trace("Dissconnected from ett server %s", serverName.c_str());
		n->isSMOnline = false;
		this->ws = nullptr;
		n->CloseConnection();
		SCREENMAN->SendMessageToTopScreen(ETTP_Disconnect);
	});
	uWSh.onConnection([n, this, address](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) {
		n->isSMOnline = true;
		this->ws = ws;
		LOG->Trace("Connected to ett server: %s", address.c_str());
	});
	uWSh.onError([this](void* ptr) {
		this->error = true;
	});
	uWSh.onMessage([this](uWS::WebSocket<uWS::CLIENT> *ws, char *message, size_t length, uWS::OpCode opCode) {
		string msg(message, length);
		try {
			json json = json::parse(msg);
			this->newMessages.emplace_back(json);
		}
		catch (exception e) {
			LOG->Trace("Error while processing ettprotocol json: %s", e.what());
		}
	});
	bool ws = true;
	bool wss = true;
	bool prepend = true;
	if (startsWith(address, "ws://")) {
		wss = false;
		prepend = false;
	}
	else if (startsWith(address, "wss://")) {
		ws = false;
		prepend = false;
	}
	time_t start;
	if (wss) {
		uWSh.connect(((prepend ? "wss://" + address : address)+ ":" + to_string(port)).c_str(), nullptr);
		uWSh.poll();
		start = time(0);
		while (!n->isSMOnline && !error) {
			uWSh.poll();
			if (difftime(time(0), start) > 2)
				break;
		}
	}
	if (ws && !n->isSMOnline) {
		error = false;
		uWSh.connect(((prepend ? "ws://" + address : address) + ":" + to_string(port)).c_str(), nullptr);
		uWSh.poll();
		start = time(0);
		while (!n->isSMOnline && !error) {
			uWSh.poll();
			if (difftime(time(0), start) > 2)
				break;
		}
	}
	return n->isSMOnline;
}
RoomData jsonToRoom(json& room)
{
	RoomData tmp;
	string s = room["name"].get<string>();
	tmp.SetName(s);
	s = room.value("desc", "");
	tmp.SetDescription(s);
	unsigned int state = room.value("state", 0);
	tmp.SetState(state);
	for (auto&& player : room.at("players")) 
		tmp.players.emplace_back(player.get<string>());
	return tmp;
}
void ETTProtocol::FindJsonChart(NetworkSyncManager* n, json& ch)
{
	n->song = nullptr;
	n->steps = nullptr;
	n->rate = ch.value("rate", 0);
	n->chartkey = ch.value("chartkey", "");
	n->m_sFileHash = ch.value("filehash", "");
	n->m_sMainTitle = ch.value("title", "");
	n->m_sSubTitle = ch.value("subtitle", "");
	n->m_sArtist = ch.value("artist", "");
	n->difficulty = StringToDifficulty(ch.value("difficulty", "Invalid"));
	n->meter = ch.value("meter", -1);
	StringToStyleType(ch.value("stepstype", "Invalid"));

	if (!n->chartkey.empty()) {
		auto song = SONGMAN->GetSongByChartkey(n->chartkey);
		if (song == nullptr)
			return;
		if ((n->m_sArtist.empty() || n->m_sArtist == song->GetTranslitArtist()) &&
			(n->m_sMainTitle.empty() || n->m_sMainTitle == song->GetTranslitMainTitle()) &&
			(n->m_sSubTitle.empty() || n->m_sSubTitle == song->GetTranslitSubTitle()) &&
			(n->m_sFileHash.empty() || n->m_sFileHash == song->GetFileHash()))
		{
			for (auto& steps : song->GetAllSteps()) {
				if ((n->meter == -1 || n->meter == steps->GetMeter()) &&
					(n->difficulty == Difficulty_Invalid || n->difficulty == steps->GetDifficulty()) &&
					(n->chartkey == steps->GetChartKey())) {
					n->song = song;
					n->steps = steps;
					break;
				}
				if (n->song != nullptr)
					break;
			}
		}
	}
	else {
		vector <Song *> AllSongs = SONGMAN->GetAllSongs();
		for (int i = 0; i < AllSongs.size(); i++)
		{
			auto& m_cSong = AllSongs[i];
			if ((n->m_sArtist.empty() || n->m_sArtist == m_cSong->GetTranslitArtist()) &&
				(n->m_sMainTitle.empty() || n->m_sMainTitle == m_cSong->GetTranslitMainTitle()) &&
				(n->m_sSubTitle.empty() || n->m_sSubTitle == m_cSong->GetTranslitSubTitle()) &&
				(n->m_sFileHash.empty() || n->m_sFileHash == m_cSong->GetFileHash()))
			{
				for (auto& steps : m_cSong->GetAllSteps()) {
					if ((n->meter == -1 || n->meter == steps->GetMeter()) &&
						(n->difficulty == Difficulty_Invalid || n->difficulty == steps->GetDifficulty())) {
						n->song = m_cSong;
						n->steps = steps;
						break;
					}
				}
				if (n->song != nullptr)
					break;
				n->song = m_cSong;
				break;
			}
		}
	}
}
void ETTProtocol::Update(NetworkSyncManager* n, float fDeltaTime)
{
	uWSh.poll();
	for (auto iterator = newMessages.begin(); iterator !=newMessages.end(); iterator++) {
		try {
			auto jType = (*iterator).find("type");
			auto payload = (*iterator).find("payload");
			auto error = (*iterator).find("error");
			if (jType == iterator->end())
			break;
			if (error != iterator->end()) {
				LOG->Trace(("Error on ETTP message " + jType->get<string>() + ":" + error->get<string>()).c_str());
				break;
			}
			switch (ettServerMessageMap[jType->get<string>()]) {
			case ettps_loginresponse:
				if (!(n->loggedIn = (*payload)["logged"]))
					n->loginResponse = (*payload)["msg"].get<string>();
				else {
					n->loginResponse = "";
					n->loggedIn = true;
				}
				SCREENMAN->SendMessageToTopScreen(ETTP_LoginResponse);
				break;
			case ettps_hello:
				serverName = (*payload).value("name", "");
				serverVersion = (*payload).value("version", 1);
				LOG->Trace("Ettp server identified: %s (Version:%d)", serverName.c_str(), serverVersion);
				n->DisplayStartupStatus();
				if (ws != nullptr) {
					json hello;
					hello["type"] = ettClientMessageMap[ettpc_hello];
					hello["payload"]["version"] = ETTPCVERSION;
					Send(hello.dump().c_str());
				}
				break;
			case ettps_recievescore:
			{
				json& score = (*payload)["score"];
				HighScore hs;
				EndOfGame_PlayerData result;
				hs.SetScoreKey(score.value("scorekey", ""));
				hs.SetSSRNormPercent(score.value("ssr_norm", 0));
				hs.SetEtternaValid(score.value("valid", 0));
				hs.SetModifiers(score.value("mods", ""));

				result.tapScores[0] = score.value("marv", 0);
				hs.SetTapNoteScore(TNS_W1, score.value("marv", 0));
				result.tapScores[1] = score.value("perfect", 0);
				hs.SetTapNoteScore(TNS_W2, score.value("perfect", 0));
				result.tapScores[2] = score.value("great", 0);
				hs.SetTapNoteScore(TNS_W3, score.value("great", 0));
				result.tapScores[3] = score.value("good", 0);
				hs.SetTapNoteScore(TNS_W4, score.value("good", 0));
				result.tapScores[4] = score.value("bad", 0);
				hs.SetTapNoteScore(TNS_W5, score.value("bad", 0));
				result.tapScores[5] = score.value("miss", 0);
				hs.SetTapNoteScore(TNS_Miss, score.value("miss", 0));
				result.tapScores[6] = 0;
				result.tapScores[7] = score.value("max_combo", 0);
				hs.SetMaxCombo(score.value("max_combo", 0));
				hs.SetGrade(PlayerStageStats::GetGrade(hs.GetSSRNormPercent()));
				hs.SetDateTime(DateTime());
				hs.SetTapNoteScore(TNS_HitMine, score.value("hitmine", 0));
				hs.SetHoldNoteScore(HNS_Held, score.value("held", 0));
				hs.SetChartKey(score.value("chartkey", ""));
				hs.SetHoldNoteScore(HNS_LetGo, score.value("letgo", 0));
				hs.SetHoldNoteScore(HNS_Missed, score.value("ng", 0));
				try {
					hs.SetChordCohesion(!score["nocc"].get<bool>());
				}
				catch (exception e) {
					hs.SetChordCohesion(true);
				}
				hs.SetMusicRate(score.value("rate", 0.1));
				try {
					json& replay = score["replay"];
					json& jOffsets = replay["offsets"];
					json& jNoterows = replay["noterows"];
					vector<float> offsets;
					vector<int> noterows;
					for (json::iterator offsetIt = jOffsets.begin(); offsetIt != jOffsets.end(); ++offsetIt)
						offsets.emplace_back(static_cast<float>(*offsetIt));
					for (json::iterator noterowIt = jNoterows.begin(); noterowIt != jNoterows.end(); ++noterowIt)
						noterows.emplace_back(noterowIt->get<int>());
					hs.SetOffsetVector(offsets);
					hs.SetNoteRowVector(noterows);
				}
				catch(exception e) { } //No replay data for this score, its still valid
				result.nameStr = (*payload)["name"].get<string>();
				result.hs = hs;
				result.playerOptions = payload->value("options", "");
				n->m_EvalPlayerData.emplace_back(result);
				n->m_ActivePlayers = n->m_EvalPlayerData.size();
				SCREENMAN->SendMessageToTopScreen(ETTP_NewScore);
				break;
			}
			case ettps_ping:
				if (ws != nullptr) {
					json ping;
					ping["type"] = ettClientMessageMap[ettpc_ping];
					ping["id"] = msgId++;
					Send(ping.dump().c_str());
				}
				break;
			case ettps_selectchart:
				{
					auto ch = (*payload).at("chart"); 
					FindJsonChart(n, ch);
					json j;
					if (n->song != nullptr) {
						SCREENMAN->SendMessageToTopScreen(ETTP_SelectChart);
						j["type"] = ettClientMessageMap[ettpc_haschart];
					}
					else {
						j["type"] = ettClientMessageMap[ettpc_missingchart];
					}
					j["id"] = msgId++;
					Send(j.dump().c_str());
				}
			break;
			case ettps_startchart:
				{
					n->m_EvalPlayerData.clear();
					auto ch = (*payload).at("chart");
					FindJsonChart(n, ch);
					json j;
					if (n->song != nullptr && state == 0) {
						SCREENMAN->SendMessageToTopScreen(ETTP_StartChart);
						j["type"] = ettClientMessageMap[ettpc_startingchart];
					}
					else
						j["type"] = ettClientMessageMap[ettpc_notstartingchart];
					j["id"] = msgId++;
					Send(j.dump().c_str());
				}
			break;
			case ettps_recievechat:
				{
					//chat[tabname, tabtype] = msg
					int type = (*payload)["msgtype"].get<int>();
					string tab = (*payload)["tab"].get<string>();
					n->chat[{tab, type}].emplace_back((*payload)["msg"].get<string>());
					SCREENMAN->SendMessageToTopScreen(ETTP_IncomingChat);
					Message msg("Chat");
					msg.SetParam("tab", RString(tab.c_str()));
					msg.SetParam("msg", RString((*payload)["msg"].get<string>().c_str()));
					msg.SetParam("type", type);
					MESSAGEMAN->Broadcast(msg);
					//Should end here
					n->m_sChatText = (n->m_sChatText + "\n" + (*payload)["msg"].get<string>()).c_str();
					SCREENMAN->SendMessageToTopScreen(SM_AddToChat);
				}
			break;
			case ettps_gameplayleaderboard:

			break;
			case ettps_createroomresponse:
				{
					bool created = (*payload)["created"];
					if (created) {
						Message msg(MessageIDToString(Message_UpdateScreenHeader));
						msg.SetParam("Header", roomName);
						msg.SetParam("Subheader", roomDesc);
						MESSAGEMAN->Broadcast(msg);
						RString SMOnlineSelectScreen = THEME->GetMetric("ScreenNetRoom", "MusicSelectScreen");
						SCREENMAN->SetNewScreen(SMOnlineSelectScreen);
					}
				}
			break;
			case ettps_enterroomresponse:
				{
					bool entered = (*payload)["entered"];
					if (entered) {
						try {
							Message msg(MessageIDToString(Message_UpdateScreenHeader));
							msg.SetParam("Header", roomName);
							msg.SetParam("Subheader", roomDesc);
							MESSAGEMAN->Broadcast(msg);
							RString SMOnlineSelectScreen = THEME->GetMetric("ScreenNetRoom", "MusicSelectScreen");
							SCREENMAN->SetNewScreen(SMOnlineSelectScreen);
						}
						catch (exception e) {
							LOG->Trace("Error while parsing ettp json enter room response: %s", e.what());
						}
					}
					else {
						roomDesc = "";
						roomName = "";
					}
				}
			break;
			case ettps_newroom:
				try {
					RoomData tmp = jsonToRoom((*payload)["room"]);
					n->m_Rooms.emplace_back(tmp);
					SCREENMAN->SendMessageToTopScreen(ETTP_RoomsChange);
				}
				catch (exception e) {
					LOG->Trace("Error while parsing ettp json newroom room: %s", e.what());
				}
			break;
			case ettps_deleteroom:
				try {
					string name = (*payload)["room"]["name"];
					n->m_Rooms.erase(
						std::remove_if(n->m_Rooms.begin(), n->m_Rooms.end(), [&](RoomData const & room) {
							return room.Name() == name;
						}),
					n->m_Rooms.end());
					SCREENMAN->SendMessageToTopScreen(ETTP_RoomsChange);
				}
				catch (exception e) {
					LOG->Trace("Error while parsing ettp json deleteroom room: %s", e.what());
				}
			break;
			case ettps_updateroom:
				try {
					auto updated = jsonToRoom((*payload)["room"]);
					auto roomIt = find_if(n->m_Rooms.begin(), n->m_Rooms.end(), 
						[&](RoomData const & room) {
								return room.Name() == updated.Name();
						}
					);
					if(roomIt != n->m_Rooms.end()) {
						roomIt->SetDescription(updated.Description());
						roomIt->SetState(updated.State());
						roomIt->players = updated.players;
						SCREENMAN->SendMessageToTopScreen(ETTP_RoomsChange);
					}
				}
				catch (exception e) {
					LOG->Trace("Error while parsing ettp json roomlist room: %s", e.what());
				}
			break;
			case ettps_roomlist:
				{
					RoomData tmp;
					n->m_Rooms.clear();
					auto j1 = payload->at("rooms");
					if (j1.is_array())
						for (auto&& room : j1) {
							try {
								n->m_Rooms.emplace_back(jsonToRoom(room));
							}
							catch (exception e) {
								LOG->Trace("Error while parsing ettp json roomlist room: %s", e.what());
							}
						}
					SCREENMAN->SendMessageToTopScreen(ETTP_RoomsChange);
				}
			break;
			case ettps_roomuserlist:
			{
				n->m_ActivePlayer.clear();
				n->m_PlayerNames.clear();
				n->m_PlayerStatus.clear();
				auto j1 = payload->at("players");
				if (j1.is_array())
					for (auto&& player : j1) {
						int stored = 0;
						try {
							n->m_PlayerNames.emplace_back(player["name"]);
							stored++;
							n->m_PlayerStatus.emplace_back(player["status"]);
							stored++;
							n->m_ActivePlayer.emplace_back(0);
						}
						catch (exception e) {
							if (stored > 0)
								n->m_PlayerNames.pop_back();
							if (stored > 1)
								n->m_PlayerStatus.pop_back();
							LOG->Trace("Error while parsing ettp json room player list: %s", e.what());
						}
					}
				SCREENMAN->SendMessageToTopScreen(SM_UsersUpdate);
			}
			break;
			}
		}
		catch (exception e) {
			LOG->Trace("Error while parsing ettp json message: %s", e.what());
		}
	}
	newMessages.clear();
}

bool SMOProtocol::TryConnect(unsigned short port, RString address)
{
	NetPlayerClient->create(); // Initialize Socket
	return NetPlayerClient->connect(address, port);
}

bool SMOProtocol::Connect(NetworkSyncManager * n, unsigned short port, RString address)
{
	if(!TryConnect(port, address))
	{
		n->m_startupStatus = 2;
		LOG->Warn( "SMOProtocol failed to connect" );
		return false;
	}

	// If network play is desired and the connection works,
	// halt until we know what server version we're dealing with

	m_packet.ClearPacket();
	m_packet.Write1( NSCHello );	//Hello Packet
	m_packet.Write1( NETPROTOCOLVERSION );
	m_packet.WriteNT( RString( string(PRODUCT_FAMILY) + product_version ) );

	/* Block until response is received.
	 * Move mode to blocking in order to give CPU back to the system,
	 * and not wait.
	 */
	bool dontExit = true;
	NetPlayerClient->blocking = true;

	// The following packet must get through, so we block for it.
	// If we are serving, we do not block for this.
	NetPlayerClient->SendPack( (char*)m_packet.Data, m_packet.Position );
	m_packet.ClearPacket();
	while(dontExit)
	{
		m_packet.ClearPacket();
		if( NetPlayerClient->ReadPack((char *)&m_packet, NETMAXBUFFERSIZE)<1 )
			dontExit=false; // Also allow exit if there is a problem on the socket
		if( m_packet.Read1() == NSServerOffset + NSCHello )
			dontExit=false;
		// Only allow passing on handshake; otherwise scoreboard updates and
		// such will confuse us.
	}

	NetPlayerClient->blocking = false;
	serverVersion = m_packet.Read1();
	if( serverVersion >= 128 )
		n->isSMOnline = true;
	serverName = m_packet.ReadNT();
	m_iSalt = m_packet.Read4();
	NSMAN->DisplayStartupStatus();
	return true;
}

void NetworkSyncManager::StartUp()
{
	RString ServerIP;

	if( GetCommandlineArgument( "netip", &ServerIP ) )
		PostStartUp( ServerIP );
	else if (autoConnectMultiplayer)
		PostStartUp(RString(g_sLastServer));
}

void NetworkSyncManager::ReportNSSOnOff(int i)
{
	if (curProtocol != nullptr)
		curProtocol->ReportNSSOnOff(i);
}
void SMOProtocol::ReportNSSOnOff(int i)
{
	m_packet.ClearPacket();
	m_packet.Write1( NSCSMS );
	m_packet.Write1( (uint8_t) i );
	NetPlayerClient->SendPack( (char*)m_packet.Data, m_packet.Position );
}

RString NetworkSyncManager::GetServerName() 
{ 
	return curProtocol != nullptr ? curProtocol->serverName : "";
}

void SMOProtocol::DealWithSMOnlinePack(PacketFunctions &SMOnlinePacket, ScreenNetSelectMusic* s)
{
	if (SMOnlinePacket.Read1() == 1)
	{
		switch (SMOnlinePacket.Read1())
		{
		case 0: // Room title Change
		{
			RString titleSub;
			titleSub = SMOnlinePacket.ReadNT() + "\n";
			titleSub += SMOnlinePacket.ReadNT();
			if (SMOnlinePacket.Read1() != 1)
			{
				RString SMOnlineSelectScreen = THEME->GetMetric("ScreenNetSelectMusic", "RoomSelectScreen");
				SCREENMAN->SetNewScreen(SMOnlineSelectScreen);
			}
		}
		}
	}
}

int SMOProtocol::DealWithSMOnlinePack(PacketFunctions &SMOnlinePacket, ScreenSMOnlineLogin* s, RString& response)
{
	int ResponseCode = SMOnlinePacket.Read1();
	if (ResponseCode == 0)
	{
		int Status = SMOnlinePacket.Read1();
		if (Status == 0)
		{
			NSMAN->loggedIn = true;
			SCREENMAN->SetNewScreen(THEME->GetMetric("ScreenSMOnlineLogin", "NextScreen"));
			return 2;
		}
		else
		{
			response = SMOnlinePacket.ReadNT();
			return 3;
		}
	}
	return 4;
}

void SMOProtocol::DealWithSMOnlinePack(PacketFunctions &SMOnlinePacket, ScreenNetRoom* s)
{
	switch (SMOnlinePacket.Read1())
	{
	case 1:
		switch (SMOnlinePacket.Read1())
		{
		case 0: //Room title Change
		{
			RString title, subtitle;
			title = SMOnlinePacket.ReadNT();
			subtitle = SMOnlinePacket.ReadNT();

			Message msg(MessageIDToString(Message_UpdateScreenHeader));
			msg.SetParam("Header", title);
			msg.SetParam("Subheader", subtitle);
			MESSAGEMAN->Broadcast(msg);

			if (SMOnlinePacket.Read1() != 0)
			{
				RString SMOnlineSelectScreen = THEME->GetMetric("ScreenNetRoom", "MusicSelectScreen");
				SCREENMAN->SetNewScreen(SMOnlineSelectScreen);
			}
		}
		case 1: //Rooms list change
		{
			int numRooms = SMOnlinePacket.Read1();
			(NSMAN->m_Rooms).clear();
			for (int i = 0; i<numRooms; ++i)
			{
				RoomData tmpRoomData;
				tmpRoomData.SetName(SMOnlinePacket.ReadNT());
				tmpRoomData.SetDescription(SMOnlinePacket.ReadNT());
				(NSMAN->m_Rooms).push_back(tmpRoomData);
			}
			//Abide by protocol and read room status
			for (int i = 0; i<numRooms; ++i)
				(NSMAN->m_Rooms)[i].SetState(SMOnlinePacket.Read1());

			for (int i = 0; i<numRooms; ++i)
				(NSMAN->m_Rooms)[i].SetFlags(SMOnlinePacket.Read1());

			s->UpdateRoomsList();
		}
		}
		break;
	case 3:
		RoomInfo info;
		info.songTitle = SMOnlinePacket.ReadNT();
		info.songSubTitle = SMOnlinePacket.ReadNT();
		info.songArtist = SMOnlinePacket.ReadNT();
		info.numPlayers = SMOnlinePacket.Read1();
		info.maxPlayers = SMOnlinePacket.Read1();
		info.players.resize(info.numPlayers);
		for (int i = 0; i < info.numPlayers; ++i)
			info.players[i] = SMOnlinePacket.ReadNT();

		s->m_roomInfo.SetRoomInfo(info);
		break;
	}
}

void NetworkSyncManager::Logout()
{
	if (curProtocol != nullptr)
		curProtocol->Logout();
}
void ETTProtocol::Logout()
{
	if (ws == nullptr)
		return;
	json logout;
	logout["type"] = ettClientMessageMap[ettpc_logout];
	Send(logout.dump().c_str());
}
void NetworkSyncManager::Login(RString user, RString pass)
{
	if (curProtocol != nullptr)
		curProtocol->Login(user, pass);
}
void ETTProtocol::SendChat(const RString& message, string tab, int type)
{
	if (ws == nullptr)
		return;
	json chatMsg;
	chatMsg["type"] = ettClientMessageMap[ettpc_sendchat];
	auto& payload = chatMsg["payload"];
	payload["msg"] = message.c_str();
	payload["tab"] = tab.c_str();
	payload["msgtype"] = type;
	chatMsg["id"] = msgId++;
	Send(chatMsg.dump().c_str());
}
void ETTProtocol::CreateNewRoom(RString name, RString desc, RString password) 
{
	if (ws == nullptr)
		return;
	roomName = name.c_str();
	roomDesc = desc.c_str();
	json createRoom;
	createRoom["type"] = ettClientMessageMap[ettpc_createroom];
	auto& payload = createRoom["payload"];
	payload["name"] = name.c_str();
	payload["pass"] = password.c_str();
	payload["desc"] = desc.c_str();
	createRoom["id"] = msgId++;
	Send(createRoom.dump().c_str());
}
void ETTProtocol::LeaveRoom(NetworkSyncManager* n)
{
	if (ws == nullptr)
		return;
	n->song = nullptr;
	n->steps = nullptr;
	n->rate = 0;
	n->chartkey = "";
	n->m_sFileHash = "";
	n->m_sMainTitle = "";
	n->m_sSubTitle = "";
	n->m_sArtist = "";
	n->difficulty = Difficulty_Invalid;
	n->meter = -1;
	json leaveRoom;
	leaveRoom["type"] = ettClientMessageMap[ettpc_leaveroom];
	leaveRoom["id"] = msgId++;
	Send(leaveRoom.dump().c_str());
}
void ETTProtocol::EnterRoom(RString name, RString password) 
{
	if (ws == nullptr)
		return;
	auto& it = find_if(NSMAN->m_Rooms.begin(), NSMAN->m_Rooms.end(), [&name](const RoomData& r) { return r.Name() == name; });
	if (it == NSMAN->m_Rooms.end())
		return; //Unknown room
	roomName = name.c_str();
	roomDesc = it->Description().c_str();
	json enterRoom;
	enterRoom["type"] = ettClientMessageMap[ettpc_enterroom];
	auto& payload = enterRoom["payload"];
	payload["name"] = name.c_str();
	payload["pass"] = password.c_str();
	enterRoom["id"] = msgId++;
	Send(enterRoom.dump().c_str());
}
void ETTProtocol::Login(RString user, RString pass)
{
	if (ws == nullptr)
		return;
	json login;
	login["type"] = ettClientMessageMap[ettpc_login];
	auto& payload = login["payload"];
	payload["user"] = user.c_str();
	payload["pass"] = pass.c_str();
	login["id"] = msgId++;
	Send(login.dump().c_str());
}
void SMOProtocol::Login(RString user, RString pass)
{
	RString HashedName = NSMAN->MD5Hex(pass);
	int authMethod = 0;
	if (m_iSalt != 0)
	{
		authMethod = 1;
		HashedName = NSMAN->MD5Hex(HashedName + ssprintf("%d", m_iSalt));
	}
	SMOnlinePacket.ClearPacket();
	SMOnlinePacket.Write1((uint8_t)0);		//Login command
	SMOnlinePacket.Write1((uint8_t)0);	//Player
	SMOnlinePacket.Write1((uint8_t)authMethod);	//MD5 hash style
	SMOnlinePacket.WriteNT(user);
	SMOnlinePacket.WriteNT(HashedName);
	SendSMOnline();
}

void NetworkSyncManager::ReportScore(int playerID, int step, int score, int combo, float offset, int numNotes)
{
	if (curProtocol != nullptr)
		curProtocol->ReportScore(this, playerID, step, score, combo, offset, numNotes);
}
void SMOProtocol::ReportScore(NetworkSyncManager* n, int playerID, int step, int score, int combo, float offset, int numNotes)
{
	if (!n->useSMserver) //Make sure that we are using the network
		return;

	LOG->Trace("Player ID %i combo = %i", playerID, combo);
	m_packet.ClearPacket();

	m_packet.Write1(NSCGSU);
	step = n->TranslateStepType(step);
	uint8_t ctr = (uint8_t)(playerID * 16 + step - (SMOST_HITMINE - 1));
	m_packet.Write1(ctr);

	ctr = uint8_t(STATSMAN->m_CurStageStats.m_player[playerID].GetGrade() * 16 + numNotes);

	if (STATSMAN->m_CurStageStats.m_player[playerID].m_bFailed)
		ctr = uint8_t(112);	//Code for failed (failed constant seems not to work)

	m_packet.Write1(ctr);
	m_packet.Write4(score);
	m_packet.Write2((uint16_t)combo);
	m_packet.Write2((uint16_t)n->m_playerLife);

	// Offset Info
	// Note: if a 0 is sent, then disregard data.

	// ASSUMED: No step will be more than 16 seconds off center.
	// If this assumption is false, read 16 seconds in either direction.
	auto iOffset = int((offset + 16.384)*2000.0f);
	if (iOffset>65535)
		iOffset = 65535;
	if (iOffset<1)
		iOffset = 1;

	// Report 0 if hold, or miss (don't forget mines should report)
	if (step == SMOST_HITMINE || step > SMOST_W1)
		iOffset = 0;
	m_packet.Write2((uint16_t)iOffset);
	NetPlayerClient->SendPack((char*)m_packet.Data, m_packet.Position);
}
void NetworkSyncManager::ReportHighScore(HighScore* hs, PlayerStageStats& pss)
{
	if (curProtocol != nullptr)
		curProtocol->ReportHighScore(hs, pss);
}

void ETTProtocol::Send(const char* msg)
{
	if (ws != nullptr)
		ws->send(msg);
}
void ETTProtocol::ReportHighScore(HighScore* hs, PlayerStageStats& pss)
{
	if (ws == nullptr)
		return;
	json j;
	j["type"] = ettClientMessageMap[ettpc_sendscore];
	auto& payload = j["payload"];
	payload["scorekey"] = hs->GetScoreKey();
	payload["ssr_norm"] = hs->GetSSRNormPercent();
	payload["max_combo"] = hs->GetMaxCombo();
	payload["valid"] = static_cast<int>(hs->GetEtternaValid());
	payload["mods"] = hs->GetModifiers();
	payload["miss"] = hs->GetTapNoteScore(TNS_Miss);
	payload["bad"] = hs->GetTapNoteScore(TNS_W5);
	payload["good"] = hs->GetTapNoteScore(TNS_W4);
	payload["great"] = hs->GetTapNoteScore(TNS_W3);
	payload["perfect"] = hs->GetTapNoteScore(TNS_W2);
	payload["marv"] = hs->GetTapNoteScore(TNS_W1);
	payload["datetime"] = string(hs->GetDateTime().GetString().c_str());
	payload["hitmine"] = hs->GetTapNoteScore(TNS_HitMine);
	payload["held"] = hs->GetHoldNoteScore(HNS_Held);
	payload["letgo"] = hs->GetHoldNoteScore(HNS_LetGo);
	payload["ng"] = hs->GetHoldNoteScore(HNS_Missed);
	payload["chartkey"] = hs->GetChartKey();
	payload["rate"] = hs->GetMusicRate();
	if(GAMESTATE->m_pPlayerState[PLAYER_1]!=nullptr)
		payload["options"] = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.GetCurrent().GetString();
	auto chart = SONGMAN->GetStepsByChartkey(hs->GetChartKey());
	payload["negsolo"] = chart->GetTimingData()->HasWarps() || chart->m_StepsType != StepsType_dance_single;
	payload["nocc"] = static_cast<int>(!hs->GetChordCohesion());
	payload["calc_version"] = hs->GetSSRCalcVersion();
	payload["topscore"] = hs->GetTopScore();
	payload["uuid"] = hs->GetMachineGuid();
	payload["hash"] = hs->GetValidationKey(ValidationKey_Brittle);
	auto& offsets = pss.GetOffsetVector();
	auto& noterows = pss.GetNoteRowVector();
	if (offsets.size() > 0) {
		payload["replay"] = json::object();
		payload["replay"]["noterows"] = json::array();
		payload["replay"]["offsets"] = json::array();
		for (int i = 0; i < noterows.size(); i++)
			payload["replay"]["noterows"].push_back(noterows[i]);
		for (int i = 0; i < offsets.size(); i++) 
			payload["replay"]["offsets"].push_back(offsets[i]);
	}
	Send(j.dump().c_str());
}
void NetworkSyncManager::ReportScore(int playerID, int step, int score, int combo, float offset)
{
	if (curProtocol != nullptr)
		curProtocol->ReportScore(this, playerID, step, score, combo, offset);
}
void SMOProtocol::ReportScore(NetworkSyncManager* n, int playerID, int step, int score, int combo, float offset)
{
	if( !n->useSMserver ) //Make sure that we are using the network
		return;

	LOG->Trace("Player ID %i combo = %i", playerID, combo);
	m_packet.ClearPacket();

	m_packet.Write1( NSCGSU );
	step = n->TranslateStepType(step);
	uint8_t ctr = (uint8_t) (playerID * 16 + step - (SMOST_HITMINE - 1));
	m_packet.Write1(ctr);

	ctr = uint8_t(STATSMAN->m_CurStageStats.m_player[playerID].GetGrade()*16);

	if ( STATSMAN->m_CurStageStats.m_player[playerID].m_bFailed )
		ctr = uint8_t(112);	//Code for failed (failed constant seems not to work)

	m_packet.Write1(ctr);
	m_packet.Write4(score);
	m_packet.Write2((uint16_t)combo);
	m_packet.Write2((uint16_t)n->m_playerLife);

	// Offset Info
	// Note: if a 0 is sent, then disregard data.

	// ASSUMED: No step will be more than 16 seconds off center.
	// If this assumption is false, read 16 seconds in either direction.
	auto iOffset = int((offset+16.384)*2000.0f);

	if( iOffset>65535 )
		iOffset=65535;
	if( iOffset<1 )
		iOffset=1;

	// Report 0 if hold, or miss (don't forget mines should report)
	if( step == SMOST_HITMINE || step > SMOST_W1 )
		iOffset = 0;

	m_packet.Write2((uint16_t)iOffset);

	NetPlayerClient->SendPack((char*)m_packet.Data, m_packet.Position); 

}

void NetworkSyncManager::ReportSongOver()
{
	if(curProtocol != nullptr)
		curProtocol->ReportSongOver(this);
}
void ETTProtocol::ReportSongOver(NetworkSyncManager* n)
{
	if (ws == nullptr)
		return;
	json gameOver;
	gameOver["type"] = ettClientMessageMap[ettpc_gameover];
	gameOver["id"] = msgId++;
	Send(gameOver.dump().c_str());
}
void SMOProtocol::ReportSongOver(NetworkSyncManager* n)
{
	if ( !n->useSMserver )	//Make sure that we are using the network
		return;
	m_packet.ClearPacket();
	m_packet.Write1(NSCGON);
	NetPlayerClient->SendPack((char*)&m_packet.Data, m_packet.Position); 
	return;
}

void NetworkSyncManager::ReportStyle()
{
	if (curProtocol != nullptr)
		curProtocol->ReportStyle(this);
}
void SMOProtocol::ReportStyle(NetworkSyncManager* n)
{
	LOG->Trace( R"(Sending "Style" to server)");

	if( !n->useSMserver )
		return;
	m_packet.ClearPacket();
	m_packet.Write1(NSCSU);
	m_packet.Write1((int8_t)GAMESTATE->GetNumPlayersEnabled());
	m_packet.Write1((uint8_t)PLAYER_1);
	m_packet.WriteNT(GAMESTATE->GetPlayerDisplayName(PLAYER_1));
	NetPlayerClient->SendPack((char*)&m_packet.Data, m_packet.Position);
}

void NetworkSyncManager::StartRequest(short position)
{
	//This needs to be reset before ScreenEvaluation could possibly be called
	m_EvalPlayerData.clear();
	if (curProtocol != nullptr)
		curProtocol->StartRequest(this, position);
}
void SMOProtocol::StartRequest(NetworkSyncManager* n, short position)
{
	if( !n->useSMserver )
		return;
	if( GAMESTATE->m_bDemonstrationOrJukebox )
		return;
	LOG->Trace("Requesting Start from Server.");

	m_packet.ClearPacket();
	m_packet.Write1(NSCGSR);
	unsigned char ctr=0;

	Steps * tSteps;
	tSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	if(tSteps!=NULL && GAMESTATE->IsPlayerEnabled(PLAYER_1))
		ctr = uint8_t(ctr+tSteps->GetMeter()*16);

	tSteps = GAMESTATE->m_pCurSteps[PLAYER_2];
	if( tSteps!=NULL && GAMESTATE->IsPlayerEnabled(PLAYER_2) )
		ctr = uint8_t(ctr+tSteps->GetMeter());

	m_packet.Write1(ctr);
	ctr=0;

	tSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
	if( tSteps!=NULL && GAMESTATE->IsPlayerEnabled(PLAYER_1) )
		ctr = uint8_t( ctr + (int)tSteps->GetDifficulty()*16 );

	tSteps = GAMESTATE->m_pCurSteps[PLAYER_2];
	if( tSteps!=NULL && GAMESTATE->IsPlayerEnabled(PLAYER_2) )
		ctr = uint8_t(ctr + (int)tSteps->GetDifficulty());

	m_packet.Write1(ctr);
	//Notify server if this is for sync or not.
	ctr = char( position*16 );
	m_packet.Write1(ctr);

	if( GAMESTATE->m_pCurSong != NULL )
	{
		m_packet.WriteNT(GAMESTATE->m_pCurSong->m_sMainTitle);
		m_packet.WriteNT(GAMESTATE->m_pCurSong->m_sSubTitle);
		m_packet.WriteNT(GAMESTATE->m_pCurSong->m_sArtist);
	}
	else
	{
		m_packet.WriteNT("");
		m_packet.WriteNT("");
		m_packet.WriteNT("");
	}

	m_packet.WriteNT(RString());

	//Send Player (and song) Options
	m_packet.WriteNT(GAMESTATE->m_SongOptions.GetCurrent().GetString());

	int players=0;
	FOREACH_PlayerNumber(p)
	{
		++players;
		m_packet.WriteNT(GAMESTATE->m_pPlayerState[p]->m_PlayerOptions.GetCurrent().GetString());
	}
	for (int i=0; i<2-players; ++i)
		m_packet.WriteNT("");	//Write a NULL if no player

	//Send chartkey
	if (serverVersion >= 129) {
		tSteps = GAMESTATE->m_pCurSteps[PLAYER_1];
		if (tSteps != NULL && GAMESTATE->IsPlayerEnabled(PLAYER_1)) {
			m_packet.WriteNT(tSteps->GetChartKey());
		} else {
			m_packet.WriteNT("");
		}

		tSteps = GAMESTATE->m_pCurSteps[PLAYER_2];
		if (tSteps != NULL && GAMESTATE->IsPlayerEnabled(PLAYER_2)) {
			m_packet.WriteNT(tSteps->GetChartKey());
		} else {
			m_packet.WriteNT("");
		}

		int rate = (int)(GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate * 100);
		m_packet.Write1(rate);
		m_packet.WriteNT(GAMESTATE->m_pCurSong->GetFileHash());
	}
	

	//Block until go is recieved.
	//Switch to blocking mode (this is the only
	//way I know how to get precievably instantanious results

	bool dontExit=true;

	NetPlayerClient->blocking=true;

	//The following packet HAS to get through, so we turn blocking on for it as well
	//Don't block if we are serving
	NetPlayerClient->SendPack((char*)&m_packet.Data, m_packet.Position); 
	
	LOG->Trace("Waiting for RECV");

	m_packet.ClearPacket();

	while (dontExit)
	{
		m_packet.ClearPacket();
		if (NetPlayerClient->ReadPack((char *)&m_packet, NETMAXBUFFERSIZE)<1)
				dontExit=false; // Also allow exit if there is a problem on the socket
								// Only do if we are not the server, otherwise the sync
								// gets hosed up due to non blocking mode.

			if (m_packet.Read1() == (NSServerOffset + NSCGSR))
				dontExit=false;
		// Only allow passing on Start request; otherwise scoreboard updates
		// and such will confuse us.

	}
	NetPlayerClient->blocking=false;

}

static LocalizedString CONNECTION_SUCCESSFUL( "NetworkSyncManager", "Connection to '%s' successful." );
static LocalizedString CONNECTION_FAILED	( "NetworkSyncManager", "Connection failed." );
void NetworkSyncManager::DisplayStartupStatus()
{
	RString sMessage("");

	switch (m_startupStatus)
	{
	case 0:
		//Networking wasn't attempted
		return;
	case 1:
		if(curProtocol != nullptr)
			sMessage = ssprintf(CONNECTION_SUCCESSFUL.GetValue(), curProtocol->serverName.c_str());
		else
			sMessage = CONNECTION_FAILED.GetValue();
		break;
	case 2:
		sMessage = CONNECTION_FAILED.GetValue();
		break;
	}
	SCREENMAN->SystemMessage(sMessage);
}

void NetworkSyncManager::Update(float fDeltaTime)
{
	if(curProtocol != nullptr)
		curProtocol->Update(this, fDeltaTime);
}
void SMOProtocol::Update(NetworkSyncManager* n, float fDeltaTime)
{
	if (n->useSMserver)
		ProcessInput(n);
	else
		return;
	PacketFunctions BroadIn;
	if ( BroadcastReception->ReadPack((char*)&BroadIn.Data, 1020) )
	{
		NetServerInfo ThisServer;
		BroadIn.Position = 0;
		if ( BroadIn.Read1() == 141 ) // SMLS_Info?
		{
			ThisServer.Name = BroadIn.ReadNT();
			int port = BroadIn.Read2();
			BroadIn.Read2();	//Num players connected.
			uint32_t addy = EzSockets::LongFromAddrIn(BroadcastReception->fromAddr);
			ThisServer.Address = ssprintf( "%u.%u.%u.%u:%d",
				(addy<<0)>>24, (addy<<8)>>24, (addy<<16)>>24, (addy<<24)>>24, port);

			//It's fairly safe to assume that users will not be on networks with more than
			//30 or 40 servers.  Until this point, maps would be slower than vectors. 
			//So I am going to use a vector to store all of the servers.  
			//
			//In this situation, I will traverse the vector to find the element that 
			//contains the corresponding server.

			unsigned int i;
			for ( i = 0; i < n->m_vAllLANServers.size(); i++ )
			{
				if ( n->m_vAllLANServers[i].Address == ThisServer.Address )
				{
					n->m_vAllLANServers[i].Name = ThisServer.Name;
					break;
				}
			}
			if ( i >= n->m_vAllLANServers.size() )
				n->m_vAllLANServers.push_back(ThisServer);
		}
	}
}

static LocalizedString CONNECTION_DROPPED( "NetworkSyncManager", "Connection to server dropped." );
void SMOProtocol::ProcessInput(NetworkSyncManager* n)
{
	//If we're disconnected, just exit
	if ((NetPlayerClient->state!=NetPlayerClient->skCONNECTED) || 
			NetPlayerClient->IsError())
	{
		SCREENMAN->SystemMessageNoAnimate(CONNECTION_DROPPED);
		n->useSMserver=false;
		n->isSMOnline = false;
		n->loggedIn = false;
		NetPlayerClient->close();
		return;
	}

	// load new data into buffer
	NetPlayerClient->update();
	m_packet.ClearPacket();

	int packetSize;
	while ( (packetSize = NetPlayerClient->ReadPack((char *)&m_packet, NETMAXBUFFERSIZE) ) > 0 )
	{
		m_packet.size = packetSize;
		int command = m_packet.Read1();
		//Check to make sure command is valid from server
		if (command < NSServerOffset)
		{
			LOG->Trace("CMD (below 128) Invalid> %d",command);
 			break;
		}

		command = command - NSServerOffset;

		switch (command)
		{
		case NSCPing: // Ping packet responce
			m_packet.ClearPacket();
			m_packet.Write1(NSCPingR);
			NetPlayerClient->SendPack((char*)m_packet.Data,m_packet.Position);
			break;
		case NSCPingR: // These are in response to when/if we send packet 0's
		case NSCHello: // This is already taken care of by the blocking code earlier
		case NSCGSR:   // This is taken care of by the blocking start code
			break;
		case NSCGON: 
			{
				int PlayersInPack = m_packet.Read1();
				n->m_EvalPlayerData.resize(PlayersInPack);
				for (int i=0; i<PlayersInPack; ++i)
					n->m_EvalPlayerData[i].name = m_packet.Read1();
				for (int i=0; i<PlayersInPack; ++i)
					n->m_EvalPlayerData[i].score = m_packet.Read4();
				for (int i=0; i<PlayersInPack; ++i)
					n->m_EvalPlayerData[i].grade = m_packet.Read1();
				for (int i=0; i<PlayersInPack; ++i)
					n->m_EvalPlayerData[i].difficulty = (Difficulty) m_packet.Read1();
				for (int j=0; j<NETNUMTAPSCORES; ++j) 
					for (int i=0; i<PlayersInPack; ++i)
						n->m_EvalPlayerData[i].tapScores[j] = m_packet.Read2();
				for (int i=0; i<PlayersInPack; ++i)
					n->m_EvalPlayerData[i].playerOptions = m_packet.ReadNT();
				SCREENMAN->SendMessageToTopScreen(SM_GotEval);
			}
			break;
		case NSCGSU: // Scoreboard Update
			{	//Ease scope
				int ColumnNumber=m_packet.Read1();
				int NumberPlayers=m_packet.Read1();
				RString ColumnData;

				switch (ColumnNumber)
				{
				case NSSB_NAMES:
					ColumnData = "Names\n";
					for (int i=0; i<NumberPlayers; ++i)
						ColumnData += n->m_PlayerNames[m_packet.Read1()] + "\n";
					break;
				case NSSB_COMBO:
					ColumnData = "Combo\n";
					for (int i=0; i<NumberPlayers; ++i)
						ColumnData += ssprintf("%d\n",m_packet.Read2());
					break;
				case NSSB_GRADE:
					ColumnData = "Grade\n";
					for (int i=0;i<NumberPlayers;i++)
						ColumnData += GradeToLocalizedString(Grade(m_packet.Read1())) + "\n";
					break;
				}
				n->m_Scoreboard[ColumnNumber] = ColumnData;
				n->m_scoreboardchange[ColumnNumber]=true;
				/*
				Message msg("ScoreboardUpdate");
				msg.SetParam( "NumPlayers", NumberPlayers );
				MESSAGEMAN->Broadcast( msg );
				*/
			}
			break;
		case NSCSU: //System message from server
			{
				RString SysMSG = m_packet.ReadNT();
				SCREENMAN->SystemMessage(SysMSG);
			}
			break;
		case NSCCM: //Chat message from server
			{
				n->m_sChatText += m_packet.ReadNT() + " \n ";
				//10000 chars backlog should be more than enough
				n->m_sChatText = n->m_sChatText.Right(10000);
				SCREENMAN->SendMessageToTopScreen(SM_AddToChat);
			}
			break;
		case NSCRSG: //Select Song/Play song
			{
				n->m_EvalPlayerData.clear();
				n->m_iSelectMode = m_packet.Read1();
				n->m_sMainTitle = m_packet.ReadNT();
				n->m_sArtist = m_packet.ReadNT();
				n->m_sSubTitle = m_packet.ReadNT();
				//Read songhash
				if (serverVersion >= 129) {
					n->m_sFileHash = m_packet.ReadNT();
				}
				SCREENMAN->SendMessageToTopScreen(SM_ChangeSong);
				SCREENMAN->SendMessageToTopScreen(SM_ChangeSong);
			}
			break;
		case NSCUUL:
			{
				/*int ServerMaxPlayers=*/m_packet.Read1();
				int PlayersInThisPacket=m_packet.Read1();
				n->m_ActivePlayer.clear();
				n->m_PlayerStatus.clear();
				n->m_PlayerNames.clear();
				n->m_ActivePlayers = 0;
				for (int i=0; i<PlayersInThisPacket; ++i)
				{
					int PStatus = m_packet.Read1();
					if ( PStatus > 0 )
					{
						n->m_ActivePlayers++;
						n->m_ActivePlayer.push_back(i);
					}
					n->m_PlayerStatus.push_back(PStatus);
					n->m_PlayerNames.push_back(m_packet.ReadNT());
				}
				SCREENMAN->SendMessageToTopScreen(SM_UsersUpdate);
			}
			break;
		case NSCSMS:
			{
				RString StyleName, GameName;
				GameName = m_packet.ReadNT();
				StyleName = m_packet.ReadNT();

				GAMESTATE->SetCurGame(GAMEMAN->StringToGame(GameName));
				GAMESTATE->SetCurrentStyle(GAMEMAN->GameAndStringToStyle(GAMESTATE->m_pCurGame,StyleName), PLAYER_INVALID);

				SCREENMAN->SetNewScreen("ScreenNetSelectMusic"); //Should this be metric'd out?
			}
			break;
		case NSCSMOnline:
			{
				SMOnlinePacket.size = packetSize - 1;
				SMOnlinePacket.Position = 0;
				memcpy(SMOnlinePacket.Data, (m_packet.Data + 1), packetSize-1);
				LOG->Trace("Received SMOnline Command: %d, size:%d", command, packetSize - 1);
				SCREENMAN->SendMessageToTopScreen(SM_SMOnlinePack);
			}
			break;
		case NSCAttack:
			{
				PlayerNumber iPlayerNumber = (PlayerNumber)m_packet.Read1();

				if( GAMESTATE->IsPlayerEnabled( iPlayerNumber ) ) // There was an attack here, now there isnt
				{
				}
				m_packet.ClearPacket();
			}
			break;
		case FLU:
			{
				int PlayersInThisPacket = m_packet.Read1();
				n->fl_PlayerNames.clear();
				n->fl_PlayerStates.clear();
				for (int i = 0; i<PlayersInThisPacket; ++i)
				{
					int PStatus = m_packet.Read1();
					n->fl_PlayerStates.push_back(PStatus);
					n->fl_PlayerNames.push_back(m_packet.ReadNT());
				}
				SCREENMAN->SendMessageToTopScreen(SM_FriendsUpdate);
			}
			break;
		}
		m_packet.ClearPacket();
	}
}

bool NetworkSyncManager::ChangedScoreboard(int Column) 
{
	if (!m_scoreboardchange[Column])
		return false;
	m_scoreboardchange[Column]=false;
	return true;
}

void NetworkSyncManager::SendChat(const RString& message, string tab, int type)
{
	if (curProtocol != nullptr)
		curProtocol->SendChat(message, tab, type);
}
void SMOProtocol::SendChat(const RString& message, string tab, int type)
{
	m_packet.ClearPacket();
	m_packet.Write1(NSCCM);
	m_packet.WriteNT(message);
	NetPlayerClient->SendPack((char*)&m_packet.Data, m_packet.Position);
}

void SMOProtocol::ReportPlayerOptions()
{
	ModsGroup<PlayerOptions>& opts = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions;
	m_packet.ClearPacket();
	m_packet.Write1( NSCUPOpts );
	m_packet.WriteNT( opts.GetCurrent().GetString() );
	NetPlayerClient->SendPack((char*)&m_packet.Data, m_packet.Position); 
}

int NetworkSyncManager::GetServerVersion()
{
	return curProtocol != nullptr ? curProtocol->serverVersion : -1;
}
void NetworkSyncManager::SelectUserSong()
{
	m_EvalPlayerData.clear();
	if (curProtocol != nullptr)
		curProtocol->SelectUserSong(this, GAMESTATE->m_pCurSong);
}
void ETTProtocol::SelectUserSong(NetworkSyncManager* n, Song* song)
{
	if (ws == nullptr || song == nullptr)
		return;
	if (song == n->song) {
		if (GAMESTATE->m_pCurSong == nullptr ||
			GAMESTATE->m_pCurSteps[PLAYER_1] == nullptr || GAMESTATE->m_pPlayerState[PLAYER_1] == nullptr)
			return;
		json startChart;
		startChart["type"] = ettClientMessageMap[ettpc_startchart];
		auto& payload = startChart["payload"];
		payload["title"] = GAMESTATE->m_pCurSong->m_sMainTitle;
		payload["subtitle"] = GAMESTATE->m_pCurSong->m_sSubTitle;
		payload["artist"] = GAMESTATE->m_pCurSong->m_sArtist;
		payload["filehash"] = GAMESTATE->m_pCurSong->GetFileHash();
		payload["chartkey"] = GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey();
		payload["options"] = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.GetCurrent().GetString();
		payload["rate"] = static_cast<int>((GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate * 1000));
		startChart["id"] = msgId++;
		Send(startChart.dump().c_str());
	}
	else {
		if (GAMESTATE->m_pCurSteps[PLAYER_1] == nullptr)
			return;
		json selectChart;
		selectChart["type"] = ettClientMessageMap[ettpc_selectchart];
		auto& payload = selectChart["payload"];
		payload["title"] = n->m_sMainTitle.c_str();
		payload["subtitle"] = n->m_sSubTitle.c_str();
		payload["artist"] = n->m_sArtist.c_str();
		payload["filehash"] = song->GetFileHash().c_str();
		payload["chartkey"] = GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey().c_str();
		payload["difficulty"] = DifficultyToString(GAMESTATE->m_pCurSteps[PLAYER_1]->GetDifficulty());
		payload["meter"] = GAMESTATE->m_pCurSteps[PLAYER_1]->GetMeter();
		payload["options"] = GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.GetCurrent().GetString();
		payload["rate"] = static_cast<int>((GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate * 1000));
		selectChart["id"] = msgId++;
		Send(selectChart.dump().c_str());
	}
}
void SMOProtocol::SelectUserSong(NetworkSyncManager * n, Song* song)
{
	m_packet.ClearPacket();
	m_packet.Write1( NSCRSG );
	m_packet.Write1( (uint8_t)n->m_iSelectMode );
	m_packet.WriteNT( n->m_sMainTitle );
	m_packet.WriteNT(n->m_sArtist );
	m_packet.WriteNT(n->m_sSubTitle );
	//Send songhash
	if (serverVersion >= 129) {
		m_packet.WriteNT(GAMESTATE->m_pCurSong->GetFileHash());
	}
	NetPlayerClient->SendPack( (char*)&m_packet.Data, m_packet.Position );
}

void SMOProtocol::SendSMOnline( )
{
	m_packet.Position = SMOnlinePacket.Position + 1;
	memcpy( (m_packet.Data + 1), SMOnlinePacket.Data, SMOnlinePacket.Position );
	m_packet.Data[0] = NSCSMOnline;
	NetPlayerClient->SendPack( (char*)&m_packet.Data , m_packet.Position );
}
void SMOProtocol::CreateNewRoom(RString name, RString desc, RString password)
{
	SMOnlinePacket.ClearPacket();
	SMOnlinePacket.Write1((uint8_t)2); // Create room command
	SMOnlinePacket.Write1(1);  // Type game room
	SMOnlinePacket.WriteNT(name);
	SMOnlinePacket.WriteNT(desc);
	if (!password.empty())
		SMOnlinePacket.WriteNT(password);
	SendSMOnline();
}
void SMOProtocol::EnterRoom(RString name, RString password)
{
	SMOnlinePacket.ClearPacket();
	SMOnlinePacket.Write1(1);
	SMOnlinePacket.Write1(1); //Type (enter a room)
	SMOnlinePacket.WriteNT(name);
	if (!password.empty())
		SMOnlinePacket.WriteNT(password);
	SendSMOnline();
}
void SMOProtocol::RequestRoomInfo(RString name)
{
	SMOnlinePacket.ClearPacket();
	SMOnlinePacket.Write1((uint8_t)3); //Request Room Info
	SMOnlinePacket.WriteNT(name);
	SendSMOnline();
}

void NetworkSyncManager::EnterRoom(RString name, RString password)
{
	if (curProtocol != nullptr)
		curProtocol->EnterRoom(name, password);
}

void NetworkSyncManager::LeaveRoom()
{
	if (curProtocol != nullptr)
		curProtocol->LeaveRoom(this);
}

void NetworkSyncManager::CreateNewRoom(RString name, RString desc, RString password)
{
	if (curProtocol != nullptr)
		curProtocol->CreateNewRoom(name, desc, password);
}

void NetworkSyncManager::RequestRoomInfo(RString name)
{
	if (curProtocol != nullptr)
		curProtocol->RequestRoomInfo(name);
}

SMOStepType NetworkSyncManager::TranslateStepType(int score)
{
	/* Translate from Stepmania's constantly changing TapNoteScore
	 * to SMO's note scores */
	switch(score)
	{
	case TNS_HitMine:
		return SMOST_HITMINE;
	case TNS_AvoidMine:
		return SMOST_AVOIDMINE;
	case TNS_Miss:
		return SMOST_MISS;
	case TNS_W5:
		return SMOST_W5;
	case TNS_W4:
		return SMOST_W4;
	case TNS_W3:
		return SMOST_W3;
	case TNS_W2:
		return SMOST_W2;
	case TNS_W1:
		return SMOST_W1;
	case HNS_LetGo+TapNoteScore_Invalid:
		return SMOST_LETGO;
	case HNS_Held+TapNoteScore_Invalid:
		return SMOST_HELD;
	default:
		return SMOST_UNUSED;
	}
}

// Packet functions
uint8_t PacketFunctions::Read1()
{
	if (Position>=NETMAXBUFFERSIZE)
		return 0;

	return Data[Position++];
}

uint16_t PacketFunctions::Read2()
{
	if (Position>=NETMAXBUFFERSIZE-1)
		return 0;

	uint16_t Temp;
	memcpy( &Temp, Data + Position,2 );
	Position+=2;
	return ntohs(Temp);
}

uint32_t PacketFunctions::Read4()
{
	if (Position>=NETMAXBUFFERSIZE-3)
		return 0;

	uint32_t Temp;
	memcpy( &Temp, Data + Position,4 );
	Position+=4;
	return ntohl(Temp);
}

RString PacketFunctions::ReadNT()
{
	//int Orig=Packet.Position;
	RString TempStr;
	while ((Position<NETMAXBUFFERSIZE)&& (((char*)Data)[Position]!=0))
		TempStr= TempStr + (char)Data[Position++];

	++Position;
	return TempStr;
}


void PacketFunctions::Write1(uint8_t data)
{
	if (Position>=NETMAXBUFFERSIZE)
		return;
	if (data != 1)
		LOG->Trace("testing");
	memcpy( &Data[Position], &data, 1 );
	++Position;
}

void PacketFunctions::Write2(uint16_t data)
{
	if (Position>=NETMAXBUFFERSIZE-1)
		return;
	data = htons(data);
	memcpy( &Data[Position], &data, 2 );
	Position+=2;
}

void PacketFunctions::Write4(uint32_t data)
{
	if (Position>=NETMAXBUFFERSIZE-3)
		return ;

	data = htonl(data);
	memcpy( &Data[Position], &data, 4 );
	Position+=4;
}

void PacketFunctions::WriteNT(const RString& data)
{
	size_t index=0;
	while( Position<NETMAXBUFFERSIZE && index<data.size() )
		Data[Position++] = (unsigned char)(data.c_str()[index++]);
	Data[Position++] = 0;
}

void PacketFunctions::ClearPacket()
{
	memset((void*)(&Data),0, NETMAXBUFFERSIZE);
	Position = 0;
}

RString NetworkSyncManager::MD5Hex( const RString &sInput ) 
{
	return BinaryToHex( CryptManager::GetMD5ForString(sInput) ).MakeUpper();
}

void NetworkSyncManager::GetListOfLANServers( vector<NetServerInfo>& AllServers ) 
{
	AllServers = m_vAllLANServers;
}

// Aldo: Please move this method to a new class, I didn't want to create new files because I don't know how to properly update the files for each platform.
// I preferred to misplace code rather than cause unneeded headaches to non-windows users, although it would be nice to have in the wiki which files to
// update when adding new files and how (Xcode/stepmania_xcode4.3.xcodeproj has a really crazy structure :/).
#if defined(CMAKE_POWERED)
unsigned long NetworkSyncManager::GetCurrentSMBuild( LoadingWindow* ld ) { return 0; }
#else
unsigned long NetworkSyncManager::GetCurrentSMBuild( LoadingWindow* ld )
{
	// Aldo: Using my own host by now, upload update_check/check_sm5.php to an official URL and change the following constants accordingly:
	const RString sHost = "aldo.mx";
	const unsigned short uPort = 80;
	const RString sResource = "/stepmania/check_sm5.php";
	const RString sUserAgent = PRODUCT_ID;
	const RString sReferer = "http://aldo.mx/stepmania/";
	
	if( ld )
	{
		ld->SetIndeterminate( true );
		ld->SetText("Checking for updates...");
	}

	unsigned long uCurrentSMBuild = 0;
	bool bSuccess = false;
	EzSockets* socket = new EzSockets();
	socket->create();
	socket->blocking = true;

	if( socket->connect(sHost, uPort) )
	{
		RString sHTTPRequest = ssprintf(
			"GET %s HTTP/1.1"			"\r\n"
			"Host: %s"					"\r\n"
			"User-Agent: %s"			"\r\n"
			"Cache-Control: no-cache"	"\r\n"
			"Referer: %s"				"\r\n"
			"X-SM-Build: %lu"			"\r\n"
			"\r\n",
			sResource.c_str(), sHost.c_str(),
			sUserAgent.c_str(), sReferer.c_str(),
			0
		);

		socket->SendData(sHTTPRequest);
		
		// Aldo: EzSocket::pReadData() is a lower level function, I used it because I was having issues
		// with EzSocket::ReadData() in 3.9, feel free to refactor this function, the low lever character
		// manipulation might look scary to people not used to it.
		char* cBuffer = new char[NETMAXBUFFERSIZE];
		// Reading the first NETMAXBUFFERSIZE bytes (usually 1024), should be enough to get the HTTP Header only
		int iBytes = socket->pReadData(cBuffer);
		if( iBytes )
		{
			// \r\n\r\n = Separator from HTTP Header and Body
			char* cBodyStart = strstr(cBuffer, "\r\n\r\n");
			if( cBodyStart != NULL )
			{
				// Get the HTTP Header only
				int iHeaderLength = cBodyStart - cBuffer;
				char* cHeader = new char[iHeaderLength+1];
				strncpy( cHeader, cBuffer, iHeaderLength );
				cHeader[iHeaderLength] = '\0';	// needed to make it a valid C String

				RString sHTTPHeader( cHeader );
				SAFE_DELETE( cHeader );
				Trim( sHTTPHeader );
				//LOG->Trace( sHTTPHeader.c_str() );

				vector<RString> svResponse;
				split( sHTTPHeader, "\r\n", svResponse );
			
				// Check for 200 OK
				if( svResponse[0].find("200") != RString::npos )
				{
					// Iterate through every field until an X-SM-Build field is found
					for( unsigned h=1; h<svResponse.size(); h++ )
					{
						RString::size_type sFieldPos = svResponse[h].find(": ");
						if( sFieldPos != RString::npos )
						{
							RString sFieldName = svResponse[h].Left(sFieldPos),
								sFieldValue = svResponse[h].substr(sFieldPos+2);

							Trim( sFieldName );
							Trim( sFieldValue );

							if( 0 == strcasecmp(sFieldName,"X-SM-Build") )
							{
								bSuccess = true;
								uCurrentSMBuild = strtoul( sFieldValue, NULL, 10 );
								break;
							}
						}
					}
				} // if( svResponse[0].find("200") != RString::npos )
			} // if( cBodyStart != NULL )
		} // if( iBytes )
		SAFE_DELETE( cBuffer );
	} // if( socket->connect(sHost, uPort) )
	
	socket->close();
	SAFE_DELETE( socket );

	if( ld )
	{
		ld->SetIndeterminate(false);
		ld->SetTotalWork(100);

		if( bSuccess )
		{
			ld->SetProgress(100);
			ld->SetText("Checking for updates... OK");
		}
		else
		{
			ld->SetProgress(0);
			ld->SetText("Checking for updates... ERROR");
		}
	}

	return uCurrentSMBuild;
}
#endif

static bool ConnectToServer( const RString &t ) 
{ 
	NSMAN->PostStartUp( t ); 
	return true;
}


LuaFunction( ConnectToServer, 		ConnectToServer( ( RString(SArg(1)).length()==0 ) ? RString(g_sLastServer) : RString(SArg(1) ) ) )

#endif

static bool ReportStyle() { NSMAN->ReportStyle(); return true; }
static bool CloseNetworkConnection() { NSMAN->CloseConnection(); return true; }

LuaFunction( IsSMOnlineLoggedIn,	NSMAN->loggedIn )
LuaFunction( IsNetConnected,		NSMAN->useSMserver )
LuaFunction( IsNetSMOnline,			NSMAN->isSMOnline )
LuaFunction( ReportStyle,			ReportStyle() )
LuaFunction( GetServerName,			NSMAN->GetServerName() )
LuaFunction( CloseConnection,		CloseNetworkConnection() )

// lua start
#include "LuaBinding.h"

class LunaNetworkSyncManager : public Luna<NetworkSyncManager>
{
public:
	static int GetChatMsg(T* p, lua_State *L) {
		unsigned int l = IArg(1);
		int tabType = IArg(2);
		string tabName = SArg(3);
		lua_pushstring(L,p->chat[{tabName, tabType}][l].c_str());
		return 1;
	}
	static int SendChatMsg(T* p, lua_State *L) {
		string msg = SArg(1);
		int tabType = IArg(2);
		string tabName = SArg(3);
		p->SendChat(msg, tabName, tabType);
		return 1;
	}
	static int Logout(T* p, lua_State *L) {
		p->Logout();
		return 1;
	}
	static int Login(T* p, lua_State *L) {
		RString user = SArg(1);
		RString pass = SArg(2);
		p->Login(user, pass);
		return 1;
	}
	LunaNetworkSyncManager() {
		ADD_METHOD(GetChatMsg);
		ADD_METHOD(SendChatMsg);
		ADD_METHOD(Login);
		ADD_METHOD(Logout);
	}
};

LUA_REGISTER_CLASS(NetworkSyncManager)
// lua end
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
