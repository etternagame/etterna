#include "global.h"
#include "NetworkSyncManager.h"
#include "Song.h"
#include "HighScore.h"
#include "uWS.h"
#include "LuaManager.h"
#include "LocalizedString.h"
#include "JsonUtil.h"
#include "Style.h"
#include <cerrno>
#include <chrono>
#include <nlohmann/json.hpp>
#include "ScreenNetSelectMusic.h"
#include "ScreenSMOnlineLogin.h"
#include "ScreenNetRoom.h"
#include "RoomInfoDisplay.h"
#include "Song.h"
#include "SongManager.h"
using json = nlohmann::json;

NetworkSyncManager* NSMAN;

// Aldo: version_num used by GetCurrentSMVersion()
// XXX: That's probably not what you want... --root

#include "ver.h"

// Maps to associate the strings with the enum values
std::map<ETTClientMessageTypes, std::string> ettClientMessageMap = {
	{ ettpc_login, "login" },
	{ ettpc_ping, "ping" },
	{ ettpc_sendchat, "chat" },
	{ ettpc_sendscore, "score" },
	{ ettpc_mpleaderboardupdate, "gameplayupdate" },
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
	{ "lobbyuserlist", ettps_lobbyuserlist },
	{ "lobbyuserlistupdate", ettps_lobbyuserlistupdate },
	{ "ping", ettps_ping },
	{ "chat", ettps_recievechat },
	{ "login", ettps_loginresponse },
	{ "score", ettps_recievescore },
	{ "leaderboard", ettps_mpleaderboardupdate },
	{ "createroom", ettps_createroomresponse },
	{ "enterroom", ettps_enterroomresponse },
	{ "selectchart", ettps_selectchart },
	{ "startchart", ettps_startchart },
	{ "deleteroom", ettps_deleteroom },
	{ "newroom", ettps_newroom },
	{ "updateroom", ettps_updateroom },
	{ "userlist", ettps_roomuserlist },
	{ "chartrequest", ettps_chartrequest }
};

#if defined(WITHOUT_NETWORKING)
NetworkSyncManager::NetworkSyncManager(LoadingWindow* ld)
{
	useSMserver = false;
	isSMOnline = false;
}
NetworkSyncManager::~NetworkSyncManager() {}
void
NetworkSyncManager::CloseConnection()
{
}
void
NetworkSyncManager::PostStartUp(const RString& ServerIP)
{
}
bool
NetworkSyncManager::Connect(const RString& addy, unsigned short port)
{
	return false;
}
RString
NetworkSyncManager::GetServerName()
{
	return RString();
}
void
NetworkSyncManager::ReportNSSOnOff(int i)
{
}
void
NetworkSyncManager::OnMusicSelect()
{
}
void
NetworkSyncManager::OffMusicSelect()
{
}
void
NetworkSyncManager::OnRoomSelect()
{
}
void
NetworkSyncManager::OffRoomSelect()
{
}
void
NetworkSyncManager::OnOptions()
{
}
void
NetworkSyncManager::OffOptions()
{
}
void
NetworkSyncManager::OnEval()
{
}
void
NetworkSyncManager::OffEval()
{
}
void
NetworkSyncManager::ReportScore(int playerID,
								int step,
								int score,
								int combo,
								float offset)
{
}
void
NetworkSyncManager::ReportSongOver()
{
}
void
NetworkSyncManager::ReportStyle()
{
}
void
NetworkSyncManager::StartRequest(short position)
{
}
void
NetworkSyncManager::DisplayStartupStatus()
{
}
void
NetworkSyncManager::Update(float fDeltaTime)
{
}
bool
NetworkSyncManager::ChangedScoreboard(int Column)
{
	return false;
}
void
NetworkSyncManager::SendChat(const RString& message)
{
}
void
NetworkSyncManager::SelectUserSong()
{
}
RString
NetworkSyncManager::MD5Hex(const RString& sInput)
{
	return RString();
}
int
NetworkSyncManager::GetSMOnlineSalt()
{
	return 0;
}
void
NetworkSyncManager::GetListOfLANServers(vector<NetServerInfo>& AllServers)
{
}
unsigned long
NetworkSyncManager::GetCurrentSMBuild(LoadingWindow* ld)
{
	return 0;
}
#else
#include "CryptManager.h"
#include "GameManager.h"
#include "GameState.h"
#include "MessageManager.h"
#include "PlayerState.h"
#include "ProductInfo.h"
#include "ProfileManager.h"
#include "RageLog.h"
#include "ScreenManager.h"
#include "ScreenMessage.h"
#include "Song.h"
#include "StatsManager.h"
#include "Steps.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "PlayerState.h"
#include "CryptManager.h"
#include "HighScore.h"
#include "ezsockets.h"

AutoScreenMessage(SM_AddToChat);
AutoScreenMessage(SM_GotEval);
AutoScreenMessage(SM_UsersUpdate);
AutoScreenMessage(SM_FriendsUpdate);

AutoScreenMessage(ETTP_Disconnect);
AutoScreenMessage(ETTP_LoginResponse);
AutoScreenMessage(ETTP_IncomingChat);
AutoScreenMessage(ETTP_RoomsChange);
AutoScreenMessage(ETTP_SelectChart);
AutoScreenMessage(ETTP_StartChart);
AutoScreenMessage(ETTP_NewScore);

extern Preference<RString> g_sLastServer;
Preference<unsigned int> autoConnectMultiplayer("AutoConnectMultiplayer", 1);
Preference<unsigned int> logPackets("LogMultiPackets", 0);
static LocalizedString CONNECTION_SUCCESSFUL("NetworkSyncManager",
											 "Connection to '%s' successful.");
static LocalizedString CONNECTION_FAILED("NetworkSyncManager",
										 "Connection failed.");

static LocalizedString INITIALIZING_CLIENT_NETWORK(
  "NetworkSyncManager",
  "Initializing Client Network...");
NetworkSyncManager::NetworkSyncManager(LoadingWindow* ld)
{
	LANserver = NULL; // So we know if it has been created yet
	useSMserver = false;
	isSMOnline = false;
	loggedIn = false;
	m_startupStatus = 0; // By default, connection not tried.
	m_ActivePlayers = 0;
	ld->SetIndeterminate(true);
	ld->SetText("\nConnecting to multiplayer server");
	StartUp();
	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "NSMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

NetworkSyncManager::~NetworkSyncManager() {}

void
NetworkSyncManager::OnMusicSelect()
{
	if (curProtocol != nullptr)
		curProtocol->OnMusicSelect();
}
void
NetworkSyncManager::OffMusicSelect()
{
	if (curProtocol != nullptr)
		curProtocol->OffMusicSelect();
}
void
NetworkSyncManager::OnRoomSelect()
{
	if (curProtocol != nullptr)
		curProtocol->OnRoomSelect();
}
void
NetworkSyncManager::OffRoomSelect()
{
	if (curProtocol != nullptr)
		curProtocol->OffRoomSelect();
}
void
NetworkSyncManager::OnOptions()
{
	if (curProtocol != nullptr)
		curProtocol->OnOptions();
}
void
NetworkSyncManager::OffOptions()
{
	if (curProtocol != nullptr)
		curProtocol->OffOptions();
}
void
NetworkSyncManager::OnEval()
{
	if (curProtocol != nullptr)
		curProtocol->OnEval();
}
void
NetworkSyncManager::OffEval()
{
	if (curProtocol != nullptr)
		curProtocol->OffEval();
}

void
ETTProtocol::OffEval()
{
	if (ws == nullptr)
		return;
	json j;
	j["type"] = ettClientMessageMap[ettpc_closeeval];
	j["id"] = msgId++;
	Send(j);
	state = 0;
}
void
ETTProtocol::OnEval()
{
	if (ws == nullptr)
		return;
	json j;
	j["type"] = ettClientMessageMap[ettpc_openeval];
	j["id"] = msgId++;
	Send(j);
	state = 2;
}
void
ETTProtocol::OnOptions()
{
	if (ws == nullptr)
		return;
	json j;
	j["type"] = ettClientMessageMap[ettpc_openoptions];
	j["id"] = msgId++;
	Send(j);
	state = 3;
}
void
ETTProtocol::OffOptions()
{
	if (ws == nullptr)
		return;
	json j;
	j["type"] = ettClientMessageMap[ettpc_closeoptions];
	j["id"] = msgId++;
	Send(j);
	state = 0;
}

void
ETTProtocol::close()
{
	serverVersion = 0;
	msgId = 0;
	serverName = "";
	roomName = "";
	roomDesc = "";
	inRoom = false;
	((uWS::Group<uWS::SERVER>*)uWSh)->close();
	((uWS::Group<uWS::CLIENT>*)uWSh)->close();
	((uWS::Group<uWS::SERVER>*)uWSh)->terminate();
	((uWS::Group<uWS::CLIENT>*)uWSh)->terminate();
	delete uWSh;
	uWSh = new uWS::Hub();
}

void
NetworkSyncManager::CloseConnection()
{
	if (!useSMserver)
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
	ETTP.close();
	curProtocol = nullptr;
	MESSAGEMAN->Broadcast("MultiplayerDisconnection");
}
bool
startsWith(const string& haystack, const string& needle)
{
	return needle.length() <= haystack.length() &&
		   equal(needle.begin(), needle.end(), haystack.begin());
}
void
NetworkSyncManager::PostStartUp(const RString& ServerIP)
{
	RString sAddress;
	unsigned short iPort;
	m_startupStatus = 2;

	size_t cLoc = ServerIP.find(':');
	if (ServerIP.find(':') != RString::npos) {
		sAddress = ServerIP.substr(0, cLoc);
		char* cEnd;
		errno = 0;
		iPort =
		  (unsigned short)strtol(ServerIP.substr(cLoc + 1).c_str(), &cEnd, 10);
		if (*cEnd != 0 || errno != 0) {
			LOG->Warn("Invalid port");
			return;
		}
	} else {
		iPort = 8765;
		sAddress = ServerIP;
	}

	chat.rawMap.clear();
	if (PREFSMAN->m_verbose_log > 0)
		LOG->Info(
		  "Attempting to connect to: %s, Port: %i", sAddress.c_str(), iPort);
	curProtocol = nullptr;
	CloseConnection();

	if (ETTP.Connect(this, iPort, sAddress))
		curProtocol = &ETTP;
	if (curProtocol == nullptr)
		return;
	g_sLastServer.Set(ServerIP);
	loggedIn = false;
	useSMserver = true;
	m_startupStatus = 1; // Connection attempt successful
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
	LOG->Info("Server Version: %d %s",
			  curProtocol->serverVersion,
			  curProtocol->serverName.c_str());
	MESSAGEMAN->Broadcast("MultiplayerConnection");
}

bool
ETTProtocol::Connect(NetworkSyncManager* n,
					 unsigned short port,
					 RString address)
{
	char host[128];
	if (sscanf(address.c_str(), "ws://%[^:/]", host) > 0)
		;
	else if (sscanf(address.c_str(), "wss://%[^:/]", host) > 0)
		;
	else
		strcpy(host, address.c_str());
	if (!EzSockets::CanConnect(host, port))
		return false;
	n->isSMOnline = false;
	msgId = 0;
	error = false;
	uWSh->onConnection([n, this, address](uWS::WebSocket<uWS::CLIENT>* ws,
										  uWS::HttpRequest req) {
		n->isSMOnline = true;
		this->ws = ws;
		LOG->Trace("Connected to ett server: %s", address.c_str());
	});
	uWSh->onError([this](void* ptr) {
		this->error = true;
		this->ws = nullptr;
	});
	uWSh->onHttpDisconnection([this](uWS::HttpSocket<true>* ptr) {
		this->error = true;
		this->ws = nullptr;
	});
	uWSh->onDisconnection(
	  [this](
		uWS::WebSocket<uWS::CLIENT>*, int code, char* message, size_t length) {
		  this->error = true;
		  this->errorMsg = string(message, length);
		  this->ws = nullptr;
	  });
	uWSh->onDisconnection(
	  [this](
		uWS::WebSocket<uWS::SERVER>*, int code, char* message, size_t length) {
		  this->error = true;
		  this->errorMsg = string(message, length);
		  this->ws = nullptr;
	  });
	uWSh->onMessage([this](uWS::WebSocket<uWS::CLIENT>* ws,
						   char* message,
						   size_t length,
						   uWS::OpCode opCode) {
		string msg(message, length);
		try {
			json json = json::parse(msg);
			this->newMessages.emplace_back(json);
			if (logPackets) {
				LOG->Trace("Incoming ETTP:");
				LOG->Trace(json.dump(4).c_str());
			}
		} catch (exception e) {
			LOG->Trace(
			  "Error while processing ettprotocol json: %s (message: %s)",
			  e.what(),
			  message);
		}
	});
	bool ws = true;
	bool wss = true;
	bool prepend = true;
	if (startsWith(address, "ws://")) {
		wss = false;
		prepend = false;
	} else if (startsWith(address, "wss://")) {
		ws = false;
		prepend = false;
	}
	time_t start;
	if (wss) {
		uWSh->connect(
		  ((prepend ? "wss://" + address : address) + ":" + to_string(port))
			.c_str(),
		  nullptr,
		  {},
		  2000,
		  nullptr);
		uWSh->poll();
		start = time(0);
		while (!n->isSMOnline && !error) {
			uWSh->poll();
			if (difftime(time(0), start) > 1.5)
				break;
		}
	}
	if (ws && !n->isSMOnline) {
		error = false;
		uWSh->connect(
		  ((prepend ? "ws://" + address : address) + ":" + to_string(port))
			.c_str(),
		  nullptr);
		uWSh->poll();
		start = time(0);
		while (!n->isSMOnline && !error) {
			uWSh->poll();
			if (difftime(time(0), start) > 1.5)
				break;
		}
	}
	return n->isSMOnline;
}
RoomData
jsonToRoom(json& room)
{
	RoomData tmp;
	string s = room["name"].get<string>();
	tmp.SetName(s);
	s = room.value("desc", "");
	tmp.SetDescription(s);
	unsigned int state = room.value("state", 0);
	tmp.SetState(state);
	tmp.SetHasPassword(room.value("pass", false));
	for (auto&& player : room.at("players"))
		tmp.players.emplace_back(player.get<string>());
	return tmp;
}
void
ETTProtocol::FindJsonChart(NetworkSyncManager* n, json& ch)
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

	if (!n->chartkey.empty()) {
		auto song = SONGMAN->GetSongByChartkey(n->chartkey);
		if (song == nullptr)
			return;
		if ((n->m_sArtist.empty() ||
			 n->m_sArtist == song->GetTranslitArtist()) &&
			(n->m_sMainTitle.empty() ||
			 n->m_sMainTitle == song->GetTranslitMainTitle()) &&
			(n->m_sSubTitle.empty() ||
			 n->m_sSubTitle == song->GetTranslitSubTitle()) &&
			(n->m_sFileHash.empty() || n->m_sFileHash == song->GetFileHash())) {
			for (auto& steps : song->GetAllSteps()) {
				if ((n->meter == -1 || n->meter == steps->GetMeter()) &&
					(n->difficulty == Difficulty_Invalid ||
					 n->difficulty == steps->GetDifficulty()) &&
					(n->chartkey == steps->GetChartKey())) {
					n->song = song;
					n->steps = steps;
					break;
				}
			}
		}
	} else {
		vector<Song*> AllSongs = SONGMAN->GetAllSongs();
		for (size_t i = 0; i < AllSongs.size(); i++) {
			auto& m_cSong = AllSongs[i];
			if ((n->m_sArtist.empty() ||
				 n->m_sArtist == m_cSong->GetTranslitArtist()) &&
				(n->m_sMainTitle.empty() ||
				 n->m_sMainTitle == m_cSong->GetTranslitMainTitle()) &&
				(n->m_sSubTitle.empty() ||
				 n->m_sSubTitle == m_cSong->GetTranslitSubTitle()) &&
				(n->m_sFileHash.empty() ||
				 n->m_sFileHash == m_cSong->GetFileHash())) {
				for (auto& steps : m_cSong->GetAllSteps()) {
					if ((n->meter == -1 || n->meter == steps->GetMeter()) &&
						(n->difficulty == Difficulty_Invalid ||
						 n->difficulty == steps->GetDifficulty())) {
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
bool
NetworkSyncManager::IsETTP()
{
	return curProtocol == &ETTP;
}
void
ETTProtocol::Update(NetworkSyncManager* n, float fDeltaTime)
{
	uWSh->poll();
	if (this->ws == nullptr) {
		LOG->Trace("Dissconnected from ett server %s", serverName.c_str());
		n->isSMOnline = false;
		n->CloseConnection();
		SCREENMAN->SendMessageToTopScreen(ETTP_Disconnect);
	}
	if (waitingForTimeout) {
		clock_t now = clock();
		double elapsed_secs = double(now - timeoutStart) / CLOCKS_PER_SEC;
		if (elapsed_secs > timeout) {
			onTimeout();
			waitingForTimeout = false;
		}
	}
	for (auto iterator = newMessages.begin(); iterator != newMessages.end();
		 iterator++) {
		try {
			auto jType = (*iterator).find("type");
			auto payload = (*iterator).find("payload");
			auto error = (*iterator).find("error");
			if (jType == iterator->end())
				break;
			if (error != iterator->end()) {
				LOG->Trace(("Error on ETTP message " + jType->get<string>() +
							":" + error->get<string>())
							 .c_str());
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
					LOG->Trace("Ettp server identified: %s (Version:%d)",
							   serverName.c_str(),
							   serverVersion);
					n->DisplayStartupStatus();
					if (ws != nullptr) {
						json hello;
						hello["type"] = ettClientMessageMap[ettpc_hello];
						hello["payload"]["version"] = ETTPCVERSION;
						Send(hello);
					}
					break;
				case ettps_recievescore: {
					json& score = (*payload)["score"];
					HighScore hs;
					EndOfGame_PlayerData result;
					hs.SetScoreKey(score.value("scorekey", ""));
					hs.SetSSRNormPercent(
					  static_cast<float>(score.value("ssr_norm", 0)));
					hs.SetEtternaValid(score.value("valid", 0) != 0);
					hs.SetModifiers(score.value("mods", ""));
					FOREACH_ENUM(Skillset, ss)
					hs.SetSkillsetSSR(ss,
									  static_cast<float>(score.value(
										SkillsetToString(ss).c_str(), 0)));
					hs.SetSSRNormPercent(score.value("score", 0.0f));
					hs.SetWifeScore(score.value("score", 0.0f));
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
					hs.SetGrade(
					  PlayerStageStats::GetGrade(hs.GetSSRNormPercent()));
					hs.SetDateTime(DateTime());
					hs.SetTapNoteScore(TNS_HitMine, score.value("hitmine", 0));
					hs.SetHoldNoteScore(HNS_Held, score.value("held", 0));
					hs.SetChartKey(score.value("chartkey", ""));
					hs.SetHoldNoteScore(HNS_LetGo, score.value("letgo", 0));
					hs.SetHoldNoteScore(HNS_Missed, score.value("ng", 0));
					try {
						hs.SetChordCohesion(!score["nocc"].get<bool>());
					} catch (exception e) {
						hs.SetChordCohesion(true);
					}
					hs.SetMusicRate(score.value("rate", 0.1f));
					try {
						json& replay = score["replay"];
						json& jOffsets = replay["offsets"];
						json& jNoterows = replay["noterows"];
						json& jTracks = replay["tracks"];
						vector<float> offsets;
						vector<int> noterows;
						vector<int> tracks;
						for (json::iterator offsetIt = jOffsets.begin();
							 offsetIt != jOffsets.end();
							 ++offsetIt)
							offsets.emplace_back(static_cast<float>(*offsetIt));
						for (json::iterator noterowIt = jNoterows.begin();
							 noterowIt != jNoterows.end();
							 ++noterowIt)
							noterows.emplace_back(noterowIt->get<int>());
						for (json::iterator trackIt = jTracks.begin();
							 trackIt != jTracks.end();
							 ++trackIt)
							noterows.emplace_back(trackIt->get<int>());
						hs.SetOffsetVector(offsets);
						hs.SetNoteRowVector(noterows);
						hs.SetTrackVector(tracks);
					} catch (exception e) {
					} // No replay data for this score, its still valid
					result.nameStr = (*payload)["name"].get<string>();
					result.hs = hs;
					result.playerOptions = payload->value("options", "");
					n->m_EvalPlayerData.emplace_back(result);
					n->m_ActivePlayers = n->m_EvalPlayerData.size();
					SCREENMAN->SendMessageToTopScreen(ETTP_NewScore);
					MESSAGEMAN->Broadcast("NewMultiScore");
					break;
				}
				case ettps_ping:
					if (ws != nullptr) {
						json ping;
						ping["type"] = ettClientMessageMap[ettpc_ping];
						ping["id"] = msgId++;
						Send(ping);
					}
					break;
				case ettps_selectchart: {
					n->mpleaderboard.clear();
					auto ch = (*payload).at("chart");
					FindJsonChart(n, ch);
					json j;
					if (n->song != nullptr) {
						SCREENMAN->SendMessageToTopScreen(ETTP_SelectChart);
						j["type"] = ettClientMessageMap[ettpc_haschart];
					} else {
						j["type"] = ettClientMessageMap[ettpc_missingchart];
					}
					j["id"] = msgId++;
					Send(j);
				} break;
				case ettps_startchart: {
					n->mpleaderboard.clear();
					n->m_EvalPlayerData.clear();
					auto ch = (*payload).at("chart");
					FindJsonChart(n, ch);
					json j;
					if (n->song != nullptr && state == 0) {
						SCREENMAN->SendMessageToTopScreen(ETTP_StartChart);
						j["type"] = ettClientMessageMap[ettpc_startingchart];
					} else
						j["type"] = ettClientMessageMap[ettpc_notstartingchart];
					j["id"] = msgId++;
					Send(j);
				} break;
				case ettps_recievechat: {
					// chat[tabname, tabtype] = msg
					int type = (*payload)["msgtype"].get<int>();
					string tab = (*payload)["tab"].get<string>();
					n->chat[{ tab, type }].emplace_back(
					  (*payload)["msg"].get<string>());
					SCREENMAN->SendMessageToTopScreen(ETTP_IncomingChat);
					Message msg("Chat");
					msg.SetParam("tab", RString(tab.c_str()));
					msg.SetParam(
					  "msg", RString((*payload)["msg"].get<string>().c_str()));
					msg.SetParam("type", type);
					MESSAGEMAN->Broadcast(msg);
				} break;
				case ettps_mpleaderboardupdate: {
					if (PREFSMAN->m_bEnableScoreboard) {
						auto& scores = (*payload)["scores"];
						for (json::iterator it = scores.begin();
							 it != scores.end();
							 ++it) {
							float wife = (*it)["wife"];
							RString jdgstr = (*it)["jdgstr"];
							string user = (*it)["user"].get<string>();
							n->mpleaderboard[user].wife = wife;
							n->mpleaderboard[user].jdgstr = jdgstr;
						}
						Message msg("MPLeaderboardUpdate");
						MESSAGEMAN->Broadcast(msg);
					}
				} break;
				case ettps_createroomresponse: {
					bool created = (*payload)["created"];
					inRoom = created;
					if (created) {
						Message msg(
						  MessageIDToString(Message_UpdateScreenHeader));
						msg.SetParam("Header", roomName);
						msg.SetParam("Subheader", roomDesc);
						MESSAGEMAN->Broadcast(msg);
						RString SMOnlineSelectScreen = THEME->GetMetric(
						  "ScreenNetRoom", "MusicSelectScreen");
						SCREENMAN->SetNewScreen(SMOnlineSelectScreen);
					}
				} break;
				case ettps_chartrequest: {
					n->requests.emplace_back(ChartRequest(*payload));
					Message msg("ChartRequest");
					MESSAGEMAN->Broadcast(msg);
				} break;
				case ettps_enterroomresponse: {
					bool entered = (*payload)["entered"];
					inRoom = false;
					if (entered) {
						try {
							Message msg(
							  MessageIDToString(Message_UpdateScreenHeader));
							msg.SetParam("Header", roomName);
							msg.SetParam("Subheader", roomDesc);
							MESSAGEMAN->Broadcast(msg);
							inRoom = true;
							RString SMOnlineSelectScreen = THEME->GetMetric(
							  "ScreenNetRoom", "MusicSelectScreen");
							SCREENMAN->SetNewScreen(SMOnlineSelectScreen);
						} catch (exception e) {
							LOG->Trace("Error while parsing ettp json enter "
									   "room response: %s",
									   e.what());
						}
					} else {
						roomDesc = "";
						roomName = "";
					}
				} break;
				case ettps_newroom:
					try {
						RoomData tmp = jsonToRoom((*payload)["room"]);
						n->m_Rooms.emplace_back(tmp);
						SCREENMAN->SendMessageToTopScreen(ETTP_RoomsChange);
					} catch (exception e) {
						LOG->Trace(
						  "Error while parsing ettp json newroom room: %s",
						  e.what());
					}
					break;
				case ettps_deleteroom:
					try {
						string name = (*payload)["room"]["name"];
						n->m_Rooms.erase(
						  std::remove_if(n->m_Rooms.begin(),
										 n->m_Rooms.end(),
										 [&](RoomData const& room) {
											 return room.Name() == name;
										 }),
						  n->m_Rooms.end());
						SCREENMAN->SendMessageToTopScreen(ETTP_RoomsChange);
					} catch (exception e) {
						LOG->Trace(
						  "Error while parsing ettp json deleteroom room: %s",
						  e.what());
					}
					break;
				case ettps_updateroom:
					try {
						auto updated = jsonToRoom((*payload)["room"]);
						auto roomIt =
						  find_if(n->m_Rooms.begin(),
								  n->m_Rooms.end(),
								  [&](RoomData const& room) {
									  return room.Name() == updated.Name();
								  });
						if (roomIt != n->m_Rooms.end()) {
							roomIt->SetDescription(updated.Description());
							roomIt->SetState(updated.State());
							roomIt->players = updated.players;
							SCREENMAN->SendMessageToTopScreen(ETTP_RoomsChange);
						}
					} catch (exception e) {
						LOG->Trace(
						  "Error while parsing ettp json roomlist room: %s",
						  e.what());
					}
					break;
				case ettps_lobbyuserlist: {
					NSMAN->lobbyuserlist.clear();
					auto users = payload->at("users");
					for (auto& user : users) {
						NSMAN->lobbyuserlist.insert(user.get<string>());
					}
				} break;
				case ettps_lobbyuserlistupdate: {
					auto& vec = NSMAN->lobbyuserlist;
					if (payload->find("on") != payload->end()) {
						auto newUsers = payload->at("on");
						for (auto& user : newUsers) {
							NSMAN->lobbyuserlist.insert(user.get<string>());
						}
					}
					if (payload->find("off") != payload->end()) {
						auto removedUsers = payload->at("off");
						for (auto& user : removedUsers) {
							NSMAN->lobbyuserlist.erase(user.get<string>());
						}
					}
					SCREENMAN->SendMessageToTopScreen(SM_UsersUpdate);
				} break;
				case ettps_roomlist: {
					RoomData tmp;
					n->m_Rooms.clear();
					auto j1 = payload->at("rooms");
					if (j1.is_array())
						for (auto&& room : j1) {
							try {
								n->m_Rooms.emplace_back(jsonToRoom(room));
							} catch (exception e) {
								LOG->Trace("Error while parsing ettp json "
										   "roomlist room: %s",
										   e.what());
							}
						}
					SCREENMAN->SendMessageToTopScreen(ETTP_RoomsChange);
				} break;
				case ettps_roomuserlist: {
					n->m_ActivePlayer.clear();
					n->m_PlayerNames.clear();
					n->m_PlayerStatus.clear();
					auto j1 = payload->at("players");
					if (j1.is_array()) {
						int i = 0;
						for (auto&& player : j1) {
							int stored = 0;
							try {
								n->m_PlayerNames.emplace_back(
								  player["name"].get<string>().c_str());
								stored++;
								n->m_PlayerStatus.emplace_back(
								  player["status"]);
								stored++;
								n->m_ActivePlayer.emplace_back(i++);
							} catch (exception e) {
								if (stored > 0)
									n->m_PlayerNames.pop_back();
								if (stored > 1)
									n->m_PlayerStatus.pop_back();
								LOG->Trace("Error while parsing ettp json room "
										   "player list: %s",
										   e.what());
							}
						}
					}
					SCREENMAN->SendMessageToTopScreen(SM_UsersUpdate);
				} break;
			}
		} catch (exception e) {
			LOG->Trace("Error while parsing ettp json message: %s", e.what());
		}
	}
	newMessages.clear();
}

void
NetworkSyncManager::StartUp()
{
	RString ServerIP;

	if (GetCommandlineArgument("netip", &ServerIP))
		PostStartUp(ServerIP);
	else if (autoConnectMultiplayer)
		PostStartUp(RString(g_sLastServer));
}

void
NetworkSyncManager::ReportNSSOnOff(int i)
{
	if (curProtocol != nullptr)
		curProtocol->ReportNSSOnOff(i);
}

RString
NetworkSyncManager::GetServerName()
{
	return curProtocol != nullptr ? curProtocol->serverName : "";
}

void
NetworkSyncManager::Logout()
{
	if (curProtocol != nullptr)
		curProtocol->Logout();
}
void
ETTProtocol::Logout()
{
	if (ws == nullptr)
		return;
	json logout;
	logout["type"] = ettClientMessageMap[ettpc_logout];
	Send(logout);
}
void
NetworkSyncManager::Login(RString user, RString pass)
{
	if (curProtocol != nullptr)
		curProtocol->Login(user, pass);
}
void
ETTProtocol::SendChat(const RString& message, string tab, int type)
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
	Send(chatMsg);
}
void
ETTProtocol::SendMPLeaderboardUpdate(float wife, RString& jdgstr)
{
	if (ws == nullptr)
		return;
	json j;
	j["type"] = ettClientMessageMap[ettpc_mpleaderboardupdate];
	auto& payload = j["payload"];
	payload["wife"] = wife;
	payload["jdgstr"] = jdgstr;
	j["id"] = msgId++;
	Send(j);
}
void
ETTProtocol::CreateNewRoom(RString name, RString desc, RString password)
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
	Send(createRoom);
}
void
ETTProtocol::LeaveRoom(NetworkSyncManager* n)
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
	Send(leaveRoom);
	roomName = "";
	roomDesc = "";
	inRoom = false;
}
void
ETTProtocol::EnterRoom(RString name, RString password)
{
	if (ws == nullptr)
		return;
	auto it = find_if(NSMAN->m_Rooms.begin(),
					  NSMAN->m_Rooms.end(),
					  [&name](const RoomData& r) { return r.Name() == name; });
	if (it == NSMAN->m_Rooms.end())
		return; // Unknown room
	roomName = name.c_str();
	roomDesc = it->Description().c_str();
	json enterRoom;
	enterRoom["type"] = ettClientMessageMap[ettpc_enterroom];
	auto& payload = enterRoom["payload"];
	payload["name"] = name.c_str();
	payload["pass"] = password.c_str();
	enterRoom["id"] = msgId++;
	Send(enterRoom);
}
void
ETTProtocol::Login(RString user, RString pass)
{
	if (ws == nullptr)
		return;
	json login;
	login["type"] = ettClientMessageMap[ettpc_login];
	auto& payload = login["payload"];
	payload["user"] = user.c_str();
	payload["pass"] = pass.c_str();
	login["id"] = msgId++;
	Send(login);
	timeoutStart = clock();
	waitingForTimeout = true;
	timeout = 5.0;
	onTimeout = [](void) {
		NSMAN->loginResponse = "Login timed out";
		SCREENMAN->SendMessageToTopScreen(ETTP_LoginResponse);
	};
}

void
NetworkSyncManager::ReportScore(int playerID,
								int step,
								int score,
								int combo,
								float offset,
								int numNotes)
{
	if (curProtocol != nullptr)
		curProtocol->ReportScore(
		  this, playerID, step, score, combo, offset, numNotes);
}
void
NetworkSyncManager::ReportHighScore(HighScore* hs, PlayerStageStats& pss)
{
	if (curProtocol != nullptr)
		curProtocol->ReportHighScore(hs, pss);
}

void
ETTProtocol::Send(json msg)
{
	try {
		Send(msg.dump().c_str());
	} catch (exception e) {
		SCREENMAN->SystemMessage("Error: Chart contains invalid utf8");
	}
}
void
ETTProtocol::Send(const char* msg)
{
	if (ws != nullptr)
		ws->send(msg);
	if (logPackets) {
		LOG->Trace("Outgoing ETTP:");
		try {
			auto j = nlohmann::json::parse(msg);
			LOG->Trace(j.dump(4).c_str());
		} catch (exception e) {
			LOG->Trace(msg);
		}
	}
}
void
ETTProtocol::ReportHighScore(HighScore* hs, PlayerStageStats& pss)
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
	payload["score"] = hs->GetSSRNormPercent();
	FOREACH_ENUM(Skillset, ss)
	payload[SkillsetToString(ss).c_str()] = hs->GetSkillsetSSR(ss);
	payload["datetime"] = string(hs->GetDateTime().GetString().c_str());
	payload["hitmine"] = hs->GetTapNoteScore(TNS_HitMine);
	payload["held"] = hs->GetHoldNoteScore(HNS_Held);
	payload["letgo"] = hs->GetHoldNoteScore(HNS_LetGo);
	payload["ng"] = hs->GetHoldNoteScore(HNS_Missed);
	payload["chartkey"] = hs->GetChartKey();
	payload["rate"] = hs->GetMusicRate();
	if (GAMESTATE->m_pPlayerState[PLAYER_1] != nullptr)
		payload["options"] = GAMESTATE->m_pPlayerState[PLAYER_1]
							   ->m_PlayerOptions.GetCurrent()
							   .GetString();
	auto chart = SONGMAN->GetStepsByChartkey(hs->GetChartKey());
	payload["negsolo"] = chart->GetTimingData()->HasWarps() ||
						 chart->m_StepsType != StepsType_dance_single;
	payload["nocc"] = static_cast<int>(!hs->GetChordCohesion());
	payload["calc_version"] = hs->GetSSRCalcVersion();
	payload["topscore"] = hs->GetTopScore();
	payload["uuid"] = hs->GetMachineGuid();
	payload["hash"] = hs->GetValidationKey(ValidationKey_Brittle);
	const auto& offsets = pss.GetOffsetVector();
	const auto& noterows = pss.GetNoteRowVector();
	const auto& tracks = pss.GetTrackVector();
	if (offsets.size() > 0) {
		payload["replay"] = json::object();
		payload["replay"]["noterows"] = json::array();
		payload["replay"]["offsets"] = json::array();
		payload["replay"]["tracks"] = json::array();
		for (size_t i = 0; i < noterows.size(); i++)
			payload["replay"]["noterows"].push_back(noterows[i]);
		for (size_t i = 0; i < offsets.size(); i++)
			payload["replay"]["offsets"].push_back(offsets[i]);
		for (size_t i = 0; i < tracks.size(); i++)
			payload["replay"]["tracks"].push_back(tracks[i]);
	}
	Send(j);
}
void
NetworkSyncManager::ReportScore(int playerID,
								int step,
								int score,
								int combo,
								float offset)
{
	if (curProtocol != nullptr)
		curProtocol->ReportScore(this, playerID, step, score, combo, offset);
}
void
NetworkSyncManager::ReportSongOver()
{
	if (curProtocol != nullptr)
		curProtocol->ReportSongOver(this);
}
void
ETTProtocol::ReportSongOver(NetworkSyncManager* n)
{
	if (ws == nullptr)
		return;
	json gameOver;
	gameOver["type"] = ettClientMessageMap[ettpc_gameover];
	gameOver["id"] = msgId++;
	Send(gameOver);
}
void
NetworkSyncManager::ReportStyle()
{
	if (curProtocol != nullptr)
		curProtocol->ReportStyle(this);
}

void
NetworkSyncManager::StartRequest(short position)
{
	// This needs to be reset before ScreenEvaluation could possibly be called
	m_EvalPlayerData.clear();
	if (curProtocol != nullptr)
		curProtocol->StartRequest(this, position);
}


void
NetworkSyncManager::DisplayStartupStatus()
{
	RString sMessage("");

	switch (m_startupStatus) {
		case 0:
			// Networking wasn't attempted
			return;
		case 1:
			if (curProtocol != nullptr)
				sMessage = ssprintf(CONNECTION_SUCCESSFUL.GetValue(),
									curProtocol->serverName.c_str());
			else
				sMessage = CONNECTION_FAILED.GetValue();
			break;
		case 2:
			sMessage = CONNECTION_FAILED.GetValue();
			break;
	}
	SCREENMAN->SystemMessage(sMessage);
}

void
NetworkSyncManager::Update(float fDeltaTime)
{
	if (curProtocol != nullptr)
		curProtocol->Update(this, fDeltaTime);
}

static LocalizedString CONNECTION_DROPPED("NetworkSyncManager",
										  "Connection to server dropped.");

bool
NetworkSyncManager::ChangedScoreboard(int Column)
{
	if (!m_scoreboardchange[Column])
		return false;
	m_scoreboardchange[Column] = false;
	return true;
}

void
NetworkSyncManager::SendChat(const RString& message, string tab, int type)
{
	if (curProtocol != nullptr)
		curProtocol->SendChat(message, tab, type);
}

void
NetworkSyncManager::SendMPLeaderboardUpdate(float wife, RString& jdgstr)
{
	if (curProtocol != nullptr)
		curProtocol->SendMPLeaderboardUpdate(wife, jdgstr);
}

int
NetworkSyncManager::GetServerVersion()
{
	return curProtocol != nullptr ? curProtocol->serverVersion : -1;
}
void
NetworkSyncManager::SelectUserSong()
{
	m_EvalPlayerData.clear();
	if (curProtocol != nullptr)
		curProtocol->SelectUserSong(this, GAMESTATE->m_pCurSong);
}
void
ETTProtocol::SelectUserSong(NetworkSyncManager* n, Song* song)
{
	if (ws == nullptr || song == nullptr)
		return;
	if (song == n->song) {
		if (GAMESTATE->m_pCurSong == nullptr ||
			GAMESTATE->m_pCurSteps[PLAYER_1] == nullptr ||
			GAMESTATE->m_pPlayerState[PLAYER_1] == nullptr)
			return;
		json startChart;
		startChart["type"] = ettClientMessageMap[ettpc_startchart];
		auto& payload = startChart["payload"];
		payload["title"] = GAMESTATE->m_pCurSong->m_sMainTitle;
		payload["subtitle"] = GAMESTATE->m_pCurSong->m_sSubTitle;
		payload["artist"] = GAMESTATE->m_pCurSong->m_sArtist;
		payload["filehash"] = GAMESTATE->m_pCurSong->GetFileHash();
		payload["difficulty"] =
		  DifficultyToString(GAMESTATE->m_pCurSteps[PLAYER_1]->GetDifficulty());
		payload["meter"] = GAMESTATE->m_pCurSteps[PLAYER_1]->GetMeter();
		payload["chartkey"] = GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey();
		payload["options"] = GAMESTATE->m_pPlayerState[PLAYER_1]
							   ->m_PlayerOptions.GetCurrent()
							   .GetString();
		payload["rate"] = static_cast<int>(
		  (GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate * 1000));
		startChart["id"] = msgId++;
		Send(startChart);
	} else {
		if (GAMESTATE->m_pCurSteps[PLAYER_1] == nullptr)
			return;
		json selectChart;
		selectChart["type"] = ettClientMessageMap[ettpc_selectchart];
		auto& payload = selectChart["payload"];
		payload["title"] = n->m_sMainTitle.c_str();
		payload["subtitle"] = n->m_sSubTitle.c_str();
		payload["artist"] = n->m_sArtist.c_str();
		payload["filehash"] = song->GetFileHash().c_str();
		payload["chartkey"] =
		  GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey().c_str();
		payload["difficulty"] =
		  DifficultyToString(GAMESTATE->m_pCurSteps[PLAYER_1]->GetDifficulty());
		payload["meter"] = GAMESTATE->m_pCurSteps[PLAYER_1]->GetMeter();
		payload["options"] = GAMESTATE->m_pPlayerState[PLAYER_1]
							   ->m_PlayerOptions.GetCurrent()
							   .GetString();
		payload["rate"] = static_cast<int>(
		  (GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate * 1000));
		selectChart["id"] = msgId++;
		Send(selectChart);
	}
}

void
NetworkSyncManager::EnterRoom(RString name, RString password)
{
	if (curProtocol != nullptr)
		curProtocol->EnterRoom(name, password);
}

void
NetworkSyncManager::LeaveRoom()
{
	if (curProtocol != nullptr)
		curProtocol->LeaveRoom(this);
}

void
NetworkSyncManager::CreateNewRoom(RString name, RString desc, RString password)
{
	if (curProtocol != nullptr)
		curProtocol->CreateNewRoom(name, desc, password);
}

void
NetworkSyncManager::RequestRoomInfo(RString name)
{
	if (curProtocol != nullptr)
		curProtocol->RequestRoomInfo(name);
}

SMOStepType
NetworkSyncManager::TranslateStepType(int score)
{
	/* Translate from Stepmania's constantly changing TapNoteScore
	 * to SMO's note scores */
	switch (score) {
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
		case HNS_LetGo + TapNoteScore_Invalid:
			return SMOST_LETGO;
		case HNS_Held + TapNoteScore_Invalid:
			return SMOST_HELD;
		default:
			return SMOST_UNUSED;
	}
}

// Packet functions
uint8_t
PacketFunctions::Read1()
{
	if (Position >= NETMAXBUFFERSIZE)
		return 0;

	return Data[Position++];
}

uint16_t
PacketFunctions::Read2()
{
	if (Position >= NETMAXBUFFERSIZE - 1)
		return 0;

	uint16_t Temp;
	memcpy(&Temp, Data + Position, 2);
	Position += 2;
	return ntohs(Temp);
}

uint32_t
PacketFunctions::Read4()
{
	if (Position >= NETMAXBUFFERSIZE - 3)
		return 0;

	uint32_t Temp;
	memcpy(&Temp, Data + Position, 4);
	Position += 4;
	return ntohl(Temp);
}

RString
PacketFunctions::ReadNT()
{
	// int Orig=Packet.Position;
	RString TempStr;
	while ((Position < NETMAXBUFFERSIZE) && (((char*)Data)[Position] != 0))
		TempStr = TempStr + (char)Data[Position++];

	++Position;
	return TempStr;
}

void
PacketFunctions::Write1(uint8_t data)
{
	if (Position >= NETMAXBUFFERSIZE)
		return;
	if (data != 1)
		LOG->Trace("testing");
	memcpy(&Data[Position], &data, 1);
	++Position;
}

void
PacketFunctions::Write2(uint16_t data)
{
	if (Position >= NETMAXBUFFERSIZE - 1)
		return;
	data = htons(data);
	memcpy(&Data[Position], &data, 2);
	Position += 2;
}

void
PacketFunctions::Write4(uint32_t data)
{
	if (Position >= NETMAXBUFFERSIZE - 3)
		return;

	data = htonl(data);
	memcpy(&Data[Position], &data, 4);
	Position += 4;
}

void
PacketFunctions::WriteNT(const RString& data)
{
	size_t index = 0;
	while (Position < NETMAXBUFFERSIZE - 1 && index < data.size())
		Data[Position++] = (unsigned char)(data.c_str()[index++]);
	Data[Position++] = 0;
}

void
PacketFunctions::ClearPacket()
{
	memset((void*)(&Data), 0, NETMAXBUFFERSIZE);
	Position = 0;
}

RString
NetworkSyncManager::MD5Hex(const RString& sInput)
{
	return BinaryToHex(CryptManager::GetMD5ForString(sInput)).MakeUpper();
}

void
NetworkSyncManager::GetListOfLANServers(vector<NetServerInfo>& AllServers)
{
	AllServers = m_vAllLANServers;
}

// Aldo: Please move this method to a new class, I didn't want to create new
// files because I don't know how to properly update the files for each
// platform. I preferred to misplace code rather than cause unneeded headaches
// to non-windows users, although it would be nice to have in the wiki which
// files to update when adding new files and how
// (Xcode/stepmania_xcode4.3.xcodeproj has a really crazy structure :/).
#if defined(CMAKE_POWERED)
unsigned long
NetworkSyncManager::GetCurrentSMBuild(LoadingWindow* ld)
{
	return 0;
}
#else
unsigned long
NetworkSyncManager::GetCurrentSMBuild(LoadingWindow* ld)
{
	// Aldo: Using my own host by now, upload update_check/check_sm5.php to an
	// official URL and change the following constants accordingly:
	const RString sHost = "aldo.mx";
	const unsigned short uPort = 80;
	const RString sResource = "/stepmania/check_sm5.php";
	const RString sUserAgent = PRODUCT_ID;
	const RString sReferer = "http://aldo.mx/stepmania/";

	if (ld) {
		ld->SetIndeterminate(true);
		ld->SetText("Checking for updates...");
	}

	unsigned long uCurrentSMBuild = 0;
	bool bSuccess = false;
	EzSockets* socket = new EzSockets();
	socket->create();
	socket->blocking = true;

	if (socket->connect(sHost, uPort)) {
		RString sHTTPRequest = ssprintf("GET %s HTTP/1.1"
										"\r\n"
										"Host: %s"
										"\r\n"
										"User-Agent: %s"
										"\r\n"
										"Cache-Control: no-cache"
										"\r\n"
										"Referer: %s"
										"\r\n"
										"X-SM-Build: %lu"
										"\r\n"
										"\r\n",
										sResource.c_str(),
										sHost.c_str(),
										sUserAgent.c_str(),
										sReferer.c_str(),
										0);

		socket->SendData(sHTTPRequest);

		// Aldo: EzSocket::pReadData() is a lower level function, I used it
		// because I was having issues with EzSocket::ReadData() in 3.9, feel
		// free to refactor this function, the low lever character manipulation
		// might look scary to people not used to it.
		char* cBuffer = new char[NETMAXBUFFERSIZE];
		// Reading the first NETMAXBUFFERSIZE bytes (usually 1024), should be
		// enough to get the HTTP Header only
		int iBytes = socket->pReadData(cBuffer);
		if (iBytes) {
			// \r\n\r\n = Separator from HTTP Header and Body
			char* cBodyStart = strstr(cBuffer, "\r\n\r\n");
			if (cBodyStart != NULL) {
				// Get the HTTP Header only
				int iHeaderLength = cBodyStart - cBuffer;
				char* cHeader = new char[iHeaderLength + 1];
				strncpy(cHeader, cBuffer, iHeaderLength);
				cHeader[iHeaderLength] =
				  '\0'; // needed to make it a valid C String

				RString sHTTPHeader(cHeader);
				SAFE_DELETE(cHeader);
				Trim(sHTTPHeader);
				// LOG->Trace( sHTTPHeader.c_str() );

				vector<RString> svResponse;
				split(sHTTPHeader, "\r\n", svResponse);

				// Check for 200 OK
				if (svResponse[0].find("200") != RString::npos) {
					// Iterate through every field until an X-SM-Build field is
					// found
					for (unsigned h = 1; h < svResponse.size(); h++) {
						RString::size_type sFieldPos = svResponse[h].find(": ");
						if (sFieldPos != RString::npos) {
							RString sFieldName = svResponse[h].Left(sFieldPos),
									sFieldValue =
									  svResponse[h].substr(sFieldPos + 2);

							Trim(sFieldName);
							Trim(sFieldValue);

							if (0 == strcasecmp(sFieldName, "X-SM-Build")) {
								bSuccess = true;
								uCurrentSMBuild =
								  strtoul(sFieldValue, NULL, 10);
								break;
							}
						}
					}
				} // if( svResponse[0].find("200") != RString::npos )
			}	 // if( cBodyStart != NULL )
		}		  // if( iBytes )
		SAFE_DELETE(cBuffer);
	} // if( socket->connect(sHost, uPort) )

	socket->close();
	SAFE_DELETE(socket);

	if (ld) {
		ld->SetIndeterminate(false);
		ld->SetTotalWork(100);

		if (bSuccess) {
			ld->SetProgress(100);
			ld->SetText("Checking for updates... OK");
		} else {
			ld->SetProgress(0);
			ld->SetText("Checking for updates... ERROR");
		}
	}

	return uCurrentSMBuild;
}
#endif

void
NetworkSyncManager::PushMPLeaderboard(lua_State* L)
{
	lua_newtable(L);
	int i = 1;
	for (auto& pair : mpleaderboard) {
		lua_newtable(L);
		lua_pushnumber(L, pair.second.wife);
		lua_setfield(L, -2, "wife");
		lua_pushstring(L, pair.second.jdgstr.c_str());
		lua_setfield(L, -2, "jdgstr");
		lua_pushstring(L, pair.first.c_str());
		lua_setfield(L, -2, "user");
		lua_rawseti(L, -2, i);
		i++;
	}
	return;
}

static bool
ConnectToServer(const RString& t)
{
	NSMAN->PostStartUp(t);
	return true;
}

LuaFunction(ConnectToServer,
			ConnectToServer((RString(SArg(1)).length() == 0)
							  ? RString(g_sLastServer)
							  : RString(SArg(1))))

#endif

static bool
ReportStyle()
{
	NSMAN->ReportStyle();
	return true;
}
static bool
CloseNetworkConnection()
{
	NSMAN->CloseConnection();
	return true;
}

LuaFunction(IsSMOnlineLoggedIn, NSMAN->loggedIn)
  LuaFunction(IsNetConnected, NSMAN->useSMserver)
	LuaFunction(IsNetSMOnline, NSMAN->isSMOnline)
	  LuaFunction(ReportStyle, ReportStyle())
		LuaFunction(GetServerName, NSMAN->GetServerName())
		  LuaFunction(CloseConnection, CloseNetworkConnection())

// lua start
#include "LuaBinding.h"

			class LunaNetworkSyncManager : public Luna<NetworkSyncManager>
{
  public:
	static int IsETTP(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->IsETTP());
		return 1;
	}
	static int GetMPLeaderboard(T* p, lua_State* L)
	{
		auto& lbd = NSMAN->mpleaderboard;
		NSMAN->PushMPLeaderboard(L);
		return 1;
	}
	static int GetChartRequests(T* p, lua_State* L)
	{
		auto& reqs = p->requests;
		lua_newtable(L);
		int i = 1;
		for (auto& req : reqs) {
			req.PushSelf(L);
			lua_rawseti(L, -2, 0);
			i++;
		}
		return 1;
	}
	static int GetChatMsg(T* p, lua_State* L)
	{
		unsigned int l = IArg(1);
		int tabType = IArg(2);
		string tabName = SArg(3);
		lua_pushstring(L, p->chat[{ tabName, tabType }][l].c_str());
		return 1;
	}
	static int GetCurrentRoomName(T* p, lua_State* L)
	{
		if (!p->IsETTP())
			lua_pushnil(L);
		else if (!p->ETTP.inRoom)
			lua_pushnil(L);
		else
			lua_pushstring(L, p->ETTP.roomName.c_str());
		return 1;
	}
	static int SendChatMsg(T* p, lua_State* L)
	{
		string msg = SArg(1);
		int tabType = IArg(2);
		string tabName = SArg(3);
		p->SendChat(msg, tabName, tabType);
		return 1;
	}
	static int Logout(T* p, lua_State* L)
	{
		p->Logout();
		return 1;
	}
	static int Login(T* p, lua_State* L)
	{
		RString user = SArg(1);
		RString pass = SArg(2);
		p->Login(user, pass);
		return 1;
	}
	static int GetEvalScores(T* p, lua_State* L)
	{
		int i = 1;
		lua_newtable(L);
		for (auto& evalData : NSMAN->m_EvalPlayerData) {
			lua_newtable(L);
			lua_pushstring(L, evalData.nameStr.c_str());
			lua_setfield(L, -2, "user");
			evalData.hs.PushSelf(L);
			lua_setfield(L, -2, "highscore");
			lua_pushstring(L, evalData.playerOptions.c_str());
			lua_setfield(L, -2, "options");
			lua_rawseti(L, -2, i);
			i++;
		}
		return 1;
	}
	static int GetLobbyUserList(T* p, lua_State* L)
	{
		lua_newtable(L);
		int i = 1;
		for (auto& user : NSMAN->lobbyuserlist) {
			lua_pushstring(L, user.c_str());
			lua_rawseti(L, -2, i);
			i++;
		}
		return 1;
	}
	LunaNetworkSyncManager()
	{
		ADD_METHOD(GetEvalScores);
		ADD_METHOD(GetMPLeaderboard);
		ADD_METHOD(GetChartRequests);
		ADD_METHOD(GetChatMsg);
		ADD_METHOD(SendChatMsg);
		ADD_METHOD(Login);
		ADD_METHOD(Logout);
		ADD_METHOD(IsETTP);
		ADD_METHOD(GetCurrentRoomName);
		ADD_METHOD(GetLobbyUserList);
	}
};

LUA_REGISTER_CLASS(NetworkSyncManager)

class LunaChartRequest : public Luna<ChartRequest>
{
  public:
	static int GetChartkey(T* p, lua_State* L)
	{
		lua_pushstring(L, p->chartkey.c_str());
		return 1;
	}
	static int GetUser(T* p, lua_State* L)
	{
		lua_pushstring(L, p->user.c_str());
		return 1;
	}
	static int GetRate(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->rate / 1000);
		return 1;
	}

	LunaChartRequest()
	{
		ADD_METHOD(GetChartkey);
		ADD_METHOD(GetUser);
		ADD_METHOD(GetRate);
	}
};

LUA_REGISTER_CLASS(ChartRequest)

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
