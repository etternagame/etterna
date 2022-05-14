#include "Etterna/Globals/global.h"
#include "NetworkSyncManager.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/MessageManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/HighScore/HighScore.h"
#include "Etterna/Screen/Network/ScreenNetSelectMusic.h"
#include "Etterna/Screen/Network/ScreenNetRoom.h"
#include "Etterna/Screen/Others/ScreenMessage.h"
#include "Core/Services/Locator.hpp"
#include "arch/LoadingWindow/LoadingWindow.h"

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"
using namespace rapidjson;
#include "asio.hpp"

NetworkSyncManager* NSMAN;

#include <cerrno>
#include <chrono>
#include <cmath>

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
	{ "chartrequest", ettps_chartrequest },
	{ "packlist", ettps_roompacklist }
};

AutoScreenMessage(SM_AddToChat);
AutoScreenMessage(SM_GotEval);
AutoScreenMessage(SM_FriendsUpdate);

AutoScreenMessage(ETTP_Disconnect);
AutoScreenMessage(ETTP_LoginResponse);
AutoScreenMessage(ETTP_IncomingChat);
AutoScreenMessage(ETTP_RoomsChange);
AutoScreenMessage(ETTP_SelectChart);
AutoScreenMessage(ETTP_StartChart);

extern Preference<std::string> g_sLastServer;
Preference<unsigned int> autoConnectMultiplayer("AutoConnectMultiplayer", 1);
Preference<unsigned int> logPackets("LogMultiPackets", 0);
static LocalizedString CONNECTION_SUCCESSFUL("NetworkSyncManager",
											 "Connection to '%s' successful.");
static LocalizedString CONNECTION_FAILED("NetworkSyncManager",
										 "Connection failed.");
static LocalizedString LOGIN_TIMEOUT("NetworkSyncManager", "LoginTimeout");

// Utility function (Since json needs to be valid utf8)
string
correct_non_utf_8(string* str)
{
	int i, f_size = str->size();
	unsigned char c = 0, c2 = 0, c3 = 0, c4 = 0;
	string to;
	to.reserve(f_size);

	for (i = 0; i < f_size; i++) {
		c = static_cast<unsigned char>((*str)[i]);
		if (c < 32) {							// control char
			if (c == 9 || c == 10 || c == 13) { // allow only \t \n \r
				to.append(1, c);
			}
			continue;
		}
		if (c < 127) { // normal ASCII
			to.append(1, c);
			continue;
		}
		if (c < 160) { // control char (nothing should be defined here
							  // either ASCI, ISO_8859-1 or UTF8, so skipping)
			if (c2 == 128) {  // fix microsoft mess, add euro
				to.append(1, static_cast<unsigned char>(226));
				to.append(1, static_cast<unsigned char>(130));
				to.append(1, static_cast<unsigned char>(172));
			}
			if (c2 == 133) { // fix IBM mess, add NEL = \n\r
				to.append(1, 10);
				to.append(1, 13);
			}
			continue;
		}
		if (c < 192) { // invalid for UTF8, converting ASCII
			to.append(1, static_cast<unsigned char>(194));
			to.append(1, c);
			continue;
		}
		if (c < 194) { // invalid for UTF8, converting ASCII
			to.append(1, static_cast<unsigned char>(195));
			to.append(1, c - 64);
			continue;
		}
		
		if (c < 224 && i + 1 < f_size) { // possibly 2byte UTF8
			c2 = static_cast<unsigned char>((*str)[i + 1]);
			if (c2 > 127 && c2 < 192) {		// valid 2byte UTF8
				if (c == 194 && c2 < 160) { // control char, skipping
				} else {
					to.append(1, c);
					to.append(1, c2);
				}
				i++;
				continue;
			}
		} else if (c < 240 && i + 2 < f_size) { // possibly 3byte UTF8
			c2 = static_cast<unsigned char>((*str)[i + 1]);
			c3 = static_cast<unsigned char>((*str)[i + 2]);
			if (c2 > 127 && c2 < 192 && c3 > 127 &&
				c3 < 192) { // valid 3byte UTF8
				to.append(1, c);
				to.append(1, c2);
				to.append(1, c3);
				i += 2;
				continue;
			}
		} else if (c < 245 && i + 3 < f_size) { // possibly 4byte UTF8
			c2 = static_cast<unsigned char>((*str)[i + 1]);
			c3 = static_cast<unsigned char>((*str)[i + 2]);
			c4 = static_cast<unsigned char>((*str)[i + 3]);
			if (c2 > 127 && c2 < 192 && c3 > 127 && c3 < 192 && c4 > 127 &&
				c4 < 192) { // valid 4byte UTF8
				to.append(1, c);
				to.append(1, c2);
				to.append(1, c3);
				to.append(1, c4);
				i += 3;
				continue;
			}
		}
		// invalid UTF8, converting ASCII (c>245 || string too short for
		// multi-byte))
		to.append(1, static_cast<unsigned char>(195));
		to.append(1, c - 64);
	}
	return to;
}

string
correct_non_utf_8(const std::string& str)
{
	string stdStr = str.c_str();
	auto utf8ValidStr = correct_non_utf_8(&stdStr);
	return utf8ValidStr;
}

static LocalizedString INITIALIZING_CLIENT_NETWORK(
  "NetworkSyncManager",
  "Initializing Client Network...");
NetworkSyncManager::NetworkSyncManager(LoadingWindow* ld)
{
	NSMAN = this;
	LANserver = nullptr; // So we know if it has been created yet
	useSMserver = false;
	isSMOnline = false;
	loggedIn = false;
	m_startupStatus = 0; // By default, connection not tried.
	m_ActivePlayers = 0;
	if (ld) {
		ld->SetIndeterminate(true);
		ld->SetText("\nConnecting to multiplayer server");
	}
	StartUp();

	m_playerLife = 0;
	m_iSelectMode = 0;
	m_playerID = 0;
	m_step = 0;
	m_score = 0;
	m_combo = 0;

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
ETTProtocol::OnMusicSelect()
{
	state = 0;
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
	if (client == nullptr)
		return;
	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_closeeval].c_str());
	writer.EndObject();
	Send(s.GetString());

	state = 0;
}
void
ETTProtocol::OnEval()
{
	if (client == nullptr)
		return;
	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_openeval].c_str());
	writer.EndObject();
	Send(s.GetString());

	state = 2;
}
void
ETTProtocol::OnOptions()
{
	if (client == nullptr)
		return;
	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_openoptions].c_str());
	writer.EndObject();
	Send(s.GetString());

	state = 3;
}
void
ETTProtocol::OffOptions()
{
	if (client == nullptr)
		return;
	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_closeoptions].c_str());
	writer.EndObject();
	Send(s.GetString());

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
	waitingForTimeout = false;
	inRoom = false;
	if (client) {
		auto hdl = *(this->hdl);
		auto client = this->client;
		client->get_io_service().post([hdl, client]() {
			client->pause_reading(hdl);
			client->close(hdl, websocketpp::close::status::going_away, "");
			client->resume_reading(hdl);
		});
	}
	if (secure_client) {
		auto hdl = *(this->hdl);
		auto secure_client = this->secure_client;
		secure_client->get_io_service().post([hdl, secure_client]() {
			secure_client->pause_reading(hdl);
			secure_client->close(
			  hdl, websocketpp::close::status::going_away, "");
			secure_client->resume_reading(hdl);
		});
	}
	if (this->thread) {
		thread->detach();
		thread = nullptr;
	}
	hdl = nullptr;
	client = nullptr;
	secure_client = nullptr;
}

ETTProtocol::~ETTProtocol()
{
	close();
	// Wait until the client is closed
	while (this->client != nullptr)
		;
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
	if (curProtocol)
		curProtocol->close();
	curProtocol = nullptr;
	MESSAGEMAN->Broadcast("MultiplayerDisconnection");
}

void
NetworkSyncManager::PostStartUp(const std::string& ServerIP)
{
	std::string sAddress;
	unsigned short iPort;
	m_startupStatus = 2;

	size_t cLoc = ServerIP.find(':');
	if (ServerIP.find(':') != std::string::npos) {
		sAddress = ServerIP.substr(0, cLoc);
		char* cEnd;
		errno = 0;
		auto sub = ServerIP.substr(cLoc + 1);
		iPort = static_cast<unsigned short>(strtol(sub.c_str(), &cEnd, 10));
		if (*cEnd != 0 || errno != 0) {
			Locator::getLogger()->warn("Invalid port {}", sub);
			return;
		}
	} else {
		iPort = 8765;
		sAddress = ServerIP;
	}

	chat.rawMap.clear();
	Locator::getLogger()->info("Attempting to connect to: {}, Port: {}", sAddress.c_str(), iPort);
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
	Locator::getLogger()->info("Server Version: {} {}",
	        curProtocol->serverVersion,
	        curProtocol->serverName.c_str());
	MESSAGEMAN->Broadcast("MultiplayerConnection");
}

bool
ETTProtocol::Connect(NetworkSyncManager* n,
					 unsigned short port,
					 std::string address)
{
	close();
	n->isSMOnline = false;
	msgId = 0;
	error = false;
	bool finished_connecting = false;

	bool ws = true;
	bool wss = true;
	bool prepend = true;
	if (starts_with(address, "ws://")) {
		wss = false;
		prepend = false;
	} else if (starts_with(address, "wss://")) {
		ws = false;
		prepend = false;
	}
	auto msgHandler = [this](websocketpp::connection_hdl hdl,
							 ws_message_ptr message) {
		// Ignore if we're closing the program and already deleted nsman
		if (NSMAN == nullptr)
			return;
		std::unique_ptr<Document> d(new Document);
		if (d->Parse(message->get_payload().c_str()).HasParseError())
			Locator::getLogger()->error("Error while processing ettprotocol json (message: {} )",
					   message->get_payload().c_str());
		else {
			std::lock_guard<std::mutex> l(this->messageBufferMutex);
			this->newMessages.push_back(std::move(d));
		}
	};
	auto openHandler = [n, this, address, &finished_connecting](
						 websocketpp::connection_hdl hdl) {
		if (NSMAN == nullptr)
			return;
		finished_connecting = true;
		this->hdl = std::make_shared<websocketpp::connection_hdl>(hdl);
		n->isSMOnline = true;
		Locator::getLogger()->info("Connected to ett server: {}", address.c_str());
	};
	auto failHandler = [n, address, &finished_connecting](
						 websocketpp::connection_hdl hdl) {
		if (NSMAN == nullptr)
			return;
		finished_connecting = true;
		n->isSMOnline = false;
	};
	auto closeHandler = [this](websocketpp::connection_hdl hdl) {
		if (NSMAN == nullptr)
			return;
		this->client = nullptr;
	};
	if (wss) {
		std::shared_ptr<wss_client> client(new wss_client());
		try {
			client->init_asio();
			client->clear_access_channels(websocketpp::log::alevel::all);
			client->set_message_handler(msgHandler);
			client->set_open_handler(openHandler);
			client->set_close_handler(closeHandler);
		} catch (std::exception& e) {
			Locator::getLogger()->warn("Failed to initialize ettp connection due to exception: {}", e.what());
		}
		finished_connecting = false;
		websocketpp::lib::error_code ec;
		wss_client::connection_ptr con =
		  client->get_connection(((prepend ? "wss://" + address : address) +
								  ":" + std::to_string(port))
								   .c_str(),
								 ec);
		if (ec) {
			Locator::getLogger()->error("Could not create ettp connection because: {}",
					   ec.message().c_str());
		} else {
			try {
				client->connect(con);
				while (!finished_connecting)
					client->poll_one();
				if (n->isSMOnline)
					this->secure_client = std::move(client);
			} catch (websocketpp::http::exception& e) {
				Locator::getLogger()->warn("Failed to create ettp connection due to exception: "
						  "{} --- {}",
						  e.m_error_code,
						  e.what());
			}
		}
	}
	if (ws && !n->isSMOnline) {
		std::shared_ptr<ws_client> client(new ws_client());
		try {
			client->init_asio();
			client->clear_access_channels(websocketpp::log::alevel::all);
			client->set_message_handler(msgHandler);
			client->set_open_handler(openHandler);
			client->set_fail_handler(failHandler);
			client->set_close_handler(closeHandler);
		} catch (std::exception& e) {
			Locator::getLogger()->warn("Failed to initialize ettp connection due to exception: {}", e.what());
		}

		finished_connecting = false;
		websocketpp::lib::error_code ec;
		ws_client::connection_ptr con = client->get_connection(
		  ((prepend ? "ws://" + address : address) + ":" + std::to_string(port))
			.c_str(),
		  ec);
		if (ec) {
			Locator::getLogger()->error("Could not create ettp connection because: {}",
					   ec.message().c_str());
		} else {
			try {
				client->connect(con);
				while (!finished_connecting) {
					client->poll_one();
				}
				if (n->isSMOnline)
					this->client = std::move(client);
			} catch (websocketpp::http::exception& e) {
				Locator::getLogger()->warn("Failed to create ettp connection due to exception: "
						  "{} --- {}",
						  e.m_error_code,
						  e.what());
			}
		}
	}
	if (n->isSMOnline) {
		auto client = this->client;
		this->thread = std::unique_ptr<std::thread>(
		  new std::thread([client]() { client->run(); }));
	} else
		Locator::getLogger()->error("Failed to connect to ettp server: {}", address.c_str());
	return n->isSMOnline;
}
RoomData
jsonToRoom(Value& room)
{
	RoomData tmp;
	string s = room.HasMember("name") && room["name"].IsString()
				 ? room["name"].GetString()
				 : "";
	tmp.SetName(s);
	s = room.HasMember("desc") && room["desc"].IsString()
		  ? room["desc"].GetString()
		  : "";
	tmp.SetDescription(s);
	unsigned int state = room.HasMember("state") && room["state"].IsUint()
						   ? room["state"].GetUint()
						   : 0;
	tmp.SetState(state);
	tmp.SetHasPassword(room.HasMember("pass") && room["pass"].IsBool()
						 ? room["pass"].GetBool()
						 : false);
	for (auto& player : room["players"].GetArray())
		if (player.IsString())
			tmp.players.push_back(player.GetString());
	return tmp;
}
void
ETTProtocol::FindJsonChart(NetworkSyncManager* n, Value& ch)
{
	n->song = nullptr;
	n->steps = nullptr;
	n->rate =
	  ch.HasMember("rate") && ch["rate"].IsInt() ? ch["rate"].GetInt() : 0;
	n->chartkey = ch.HasMember("chartkey") && ch["chartkey"].IsString()
					? ch["chartkey"].GetString()
					: "";
	n->m_sFileHash = ch.HasMember("filehash") && ch["filehash"].IsString()
					   ? ch["filehash"].GetString()
					   : "";
	n->m_sMainTitle = ch.HasMember("title") && ch["title"].IsString()
						? ch["title"].GetString()
						: "";
	n->m_sSubTitle = ch.HasMember("subtitle") && ch["subtitle"].IsString()
					   ? ch["subtitle"].GetString()
					   : "";
	n->m_sArtist = ch.HasMember("artist") && ch["artist"].IsString()
					 ? ch["artist"].GetString()
					 : "";
	n->difficulty = StringToDifficulty(ch.HasMember("difficulty") &&
										   ch["difficulty"].IsString()
										 ? ch["difficulty"].GetString()
										 : "Invalid");
	n->meter =
	  ch.HasMember("meter") && ch["meter"].IsInt() ? ch["meter"].GetInt() : -1;

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
		std::vector<Song*> AllSongs = SONGMAN->GetAllSongs();
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
				if (n->meter > 0 || n->difficulty != Difficulty_Invalid)
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
	if (this->client == nullptr) {
		Locator::getLogger()->info("Disconnected from ett server {}", serverName.c_str());
		n->isSMOnline = false;
		n->CloseConnection();
		SCREENMAN->SendMessageToTopScreen(ETTP_Disconnect);
	}
	if (waitingForTimeout) {
		clock_t now = clock();
		double elapsed_secs =
		  static_cast<double>(now - timeoutStart) / CLOCKS_PER_SEC;
		if (elapsed_secs > timeout) {
			onTimeout();
			waitingForTimeout = false;
		}
	}
	std::lock_guard<std::mutex> l(this->messageBufferMutex);
	for (auto iterator = newMessages.begin(); iterator != newMessages.end();
		 iterator++) {
		try {
			Document& d = **iterator;
			if (!d.HasMember("type") || !d["type"].IsString()) {
				StringBuffer buffer;
				Writer<StringBuffer> writer(buffer);
				d.Accept(writer);
				Locator::getLogger()->warn(
				  "Recieved ETTP message with no type: {}", buffer.GetString());
				continue;
			}
			if (d.HasMember("error") && d["error"].IsString()) {
				Locator::getLogger()->error("Error on ETTP message {}: {}",
											d["type"].GetString(),
											d["error"].GetString());
				continue;
			}
			auto type = ettServerMessageMap.find(d["type"].GetString());
			if (ettServerMessageMap.end() == type) {
				Locator::getLogger()->warn("Unknown ETTP message type {}",
											d["type"].GetString());
				continue;
			}
			switch (type->second) {
				case ettps_loginresponse: {
					auto& payload = d["payload"];
					waitingForTimeout = false;
					if (!(n->loggedIn = payload.HasMember("logged") &&
										payload["logged"].IsBool() &&
										payload["logged"].GetBool())) {
						if (payload.HasMember("msg") &&
							payload["msg"].IsString())
							n->loginResponse = payload["msg"].GetString();
						else
							n->loginResponse = "";
						n->loggedInUsername.clear();
					} else {
						n->loginResponse = "";
					}
					SCREENMAN->SendMessageToTopScreen(ETTP_LoginResponse);
				} break;
				case ettps_hello: {
					auto& payload = d["payload"];
					if (payload.HasMember("name") && payload["name"].IsString())
						serverName = payload["name"].GetString();
					else
						serverName = "";
					if (payload.HasMember("version") &&
						payload["version"].IsInt())
						serverVersion = payload["version"].GetInt();
					else
						serverVersion = 1;
					Locator::getLogger()->info("Ettp server identified: {} (Version: {})",
							   serverName.c_str(),
							   serverVersion);
					n->DisplayStartupStatus();
					if (client != nullptr) {
						StringBuffer s;
						Writer<StringBuffer> writer(s);
						writer.StartObject();
						writer.Key("id");
						writer.Uint(msgId++);
						writer.Key("type");
						writer.String(ettClientMessageMap[ettpc_hello].c_str());
						writer.Key("payload");
						writer.StartObject();
						writer.Key("version");
						writer.Int(ETTPCVERSION);
						writer.Key("client");
						writer.String(GAMESTATE->GetEtternaVersion().c_str());
						writer.Key("packs");
						writer.StartArray();
						auto& packs = SONGMAN->GetSongGroupNames();
						for (auto& pack : packs) {
							writer.String(correct_non_utf_8(pack).c_str());
						}
						writer.EndArray();
						writer.EndObject();
						writer.EndObject();
						Send(s.GetString());
					}
				} break;
				case ettps_recievescore: {
					auto& payload = d["payload"];
					auto& score = payload["score"];
					HighScore hs;
					EndOfGame_PlayerData result;

					hs.SetScoreKey(score.HasMember("scorekey") &&
									   score["scorekey"].IsString()
									 ? score["scorekey"].GetString()
									 : "");
					hs.SetSSRNormPercent(score.HasMember("ssr_norm") &&
											 score["ssr_norm"].IsNumber()
										   ? score["ssr_norm"].GetFloat()
										   : 0);
					hs.SetEtternaValid(score.HasMember("valid") &&
										   score["valid"].IsInt()
										 ? score["valid"].GetInt() != 0
										 : true);
					hs.SetModifiers(score.HasMember("mods") &&
										score["mods"].IsString()
									  ? score["mods"].GetString()
									  : "");
					if (score.HasMember("wifever") && score["wifever"].IsInt())
						hs.SetWifeVersion(score["wifever"].GetInt());
					RadarValues rv;
					FOREACH_ENUM(RadarCategory, rc)
					{
						auto rcs = RadarCategoryToString(rc).c_str();
						if (score.HasMember(rcs) && score[rcs].IsInt()) {
							rv[rc] = score[rcs].GetInt();
						}
					}
					hs.SetRadarValues(rv);

					FOREACH_ENUM(Skillset, ss)
					{
						auto str = SkillsetToString(ss);
						hs.SetSkillsetSSR(ss,
										  score.HasMember(str.c_str()) &&
											  score[str.c_str()].IsNumber()
											? score[str.c_str()].GetFloat()
											: 0);
					}
					auto wife_score =
					  score.HasMember("score") && score["score"].IsNumber()
						? score["score"].GetFloat()
						: 0.0f;
					hs.SetSSRNormPercent(wife_score);
					hs.SetWifeScore(wife_score);
					auto marv = score.HasMember("marv") && score["marv"].IsInt()
								  ? score["marv"].GetInt()
								  : 0;
					result.tapScores[0] = marv;
					hs.SetTapNoteScore(TNS_W1, marv);
					auto perfect =
					  score.HasMember("perfect") && score["perfect"].IsInt()
						? score["perfect"].GetInt()
						: 0;
					result.tapScores[1] = perfect;
					hs.SetTapNoteScore(TNS_W2, perfect);
					auto great =
					  score.HasMember("great") && score["great"].IsInt()
						? score["great"].GetInt()
						: 0;
					result.tapScores[2] = great;
					hs.SetTapNoteScore(TNS_W3, great);
					auto good = score.HasMember("good") && score["good"].IsInt()
								  ? score["good"].GetInt()
								  : 0;
					result.tapScores[3] = good;
					hs.SetTapNoteScore(TNS_W4, good);
					auto bad = score.HasMember("bad") && score["bad"].IsInt()
								 ? score["bad"].GetInt()
								 : 0;
					result.tapScores[4] = bad;
					hs.SetTapNoteScore(TNS_W5, bad);
					auto miss = score.HasMember("miss") && score["miss"].IsInt()
								  ? score["miss"].GetInt()
								  : 0;
					result.tapScores[5] = miss;
					hs.SetTapNoteScore(TNS_Miss, miss);
					result.tapScores[6] = 0;
					auto max_combo =
					  score.HasMember("max_combo") && score["max_combo"].IsInt()
						? score["max_combo"].GetInt()
						: 0;
					result.tapScores[7] = max_combo;
					hs.SetMaxCombo(max_combo);
					hs.SetGrade(
					  PlayerStageStats::GetGrade(hs.GetSSRNormPercent()));
					hs.SetDateTime(DateTime());
					hs.SetTapNoteScore(TNS_HitMine,
									   score.HasMember("hitmine") &&
										   score["hitmine"].IsInt()
										 ? score["hitmine"].GetInt()
										 : 0);
					hs.SetHoldNoteScore(HNS_Held,
										score.HasMember("held") &&
											score["held"].IsInt()
										  ? score["held"].GetInt()
										  : 0);
					hs.SetChartKey(score.HasMember("chartkey") &&
									   score["chartkey"].IsString()
									 ? score["chartkey"].GetString()
									 : "");
					hs.SetHoldNoteScore(HNS_LetGo,
										score.HasMember("letgo") &&
											score["letgo"].IsInt()
										  ? score["letgo"].GetInt()
										  : 0);
					hs.SetHoldNoteScore(HNS_Missed,
										score.HasMember("ng") &&
											score["ng"].IsInt()
										  ? score["ng"].GetInt()
										  : 0);
					hs.SetChordCohesion(!(score.HasMember("nocc") &&
										  score["nocc"].IsBool() &&
										  score["nocc"].GetBool()));
					hs.SetMusicRate(score.HasMember("rate") &&
										score["rate"].IsNumber()
									  ? score["rate"].GetFloat()
									  : 0.1f);
					if (score.HasMember("replay") &&
						score["replay"].IsObject() &&
						score["replay"].HasMember("offsets") &&
						score["replay"]["offsets"].IsArray() &&
						score["replay"].HasMember("noterows") &&
						score["replay"]["noterows"].IsArray() &&
						score["replay"].HasMember("tracks") &&
						score["replay"]["tracks"].IsArray()) {
						auto& replay = score["replay"];
						auto& offsets = replay["offsets"];
						auto& noterows = replay["noterows"];
						auto& tracks = replay["tracks"];
						std::vector<float> v_offsets;
						std::vector<int> v_noterows;
						std::vector<int> v_tracks;
						for (auto& offset : offsets.GetArray())
							if (offset.IsNumber())
								v_offsets.push_back(offset.GetFloat());
						for (auto& noterow : noterows.GetArray())
							if (noterow.IsInt())
								v_noterows.push_back(noterow.GetInt());
						for (auto& track : tracks.GetArray())
							if (track.IsInt())
								v_tracks.push_back(track.GetInt());
						hs.SetOffsetVector(v_offsets);
						hs.SetNoteRowVector(v_noterows);
						hs.SetTrackVector(v_tracks);

						// add some backwards compatibility with pre 0.71
						// multi users
						if (replay.HasMember("notetypes") &&
							replay["notetypes"].IsArray())
						{
							auto& notetypes = replay["notetypes"];
							std::vector<TapNoteType> v_types;
							for (auto& type : notetypes.GetArray())
								if (type.IsInt()) 
									v_types.push_back(static_cast<TapNoteType>(type.GetInt()));
							hs.SetTapNoteTypeVector(v_types);
						}
					}
					result.nameStr = payload["name"].GetString();
					result.hs = hs;
					result.playerOptions = payload.HasMember("options") &&
											   payload["options"].IsString()
											 ? payload["options"].GetString()
											 : "";
					n->m_EvalPlayerData.push_back(result);
					n->m_ActivePlayers = n->m_EvalPlayerData.size();
					MESSAGEMAN->Broadcast("NewMultiScore");
				} break;
				case ettps_ping: {
					if (client != nullptr) {
						StringBuffer s;
						Writer<StringBuffer> writer(s);
						writer.StartObject();
						writer.Key("id");
						writer.Uint(msgId++);
						writer.Key("type");
						writer.String(ettClientMessageMap[ettpc_ping].c_str());
						writer.EndObject();
						Send(s.GetString());
					}
				} break;
				case ettps_selectchart: {
					auto& payload = d["payload"];
					n->mpleaderboard.clear();
					if (!payload.HasMember("chart") ||
						!payload["chart"].IsObject())
						continue;
					auto& ch = payload["chart"];
					FindJsonChart(n, ch);
					StringBuffer s;
					Writer<StringBuffer> writer(s);
					writer.StartObject();
					writer.Key("id");
					writer.Uint(msgId++);
					writer.Key("type");
					if (n->song != nullptr) {
						SCREENMAN->SendMessageToTopScreen(ETTP_SelectChart);
						writer.String(
						  ettClientMessageMap[ettpc_haschart].c_str());
					} else {
						writer.String(
						  ettClientMessageMap[ettpc_missingchart].c_str());
					}
					writer.EndObject();
					Send(s.GetString());
				} break;
				case ettps_startchart: {
					auto& payload = d["payload"];
					n->mpleaderboard.clear();
					n->m_EvalPlayerData.clear();
					if (!payload.HasMember("chart") ||
						!payload["chart"].IsObject())
						continue;
					auto& ch = payload["chart"];
					FindJsonChart(n, ch);

					StringBuffer s;
					Writer<StringBuffer> writer(s);
					writer.StartObject();
					writer.Key("id");
					writer.Uint(msgId++);
					writer.Key("type");
					if (n->song != nullptr && state == 0) {
						SCREENMAN->SendMessageToTopScreen(ETTP_StartChart);
						writer.String(
						  ettClientMessageMap[ettpc_startingchart].c_str());
					} else {
						writer.String(
						  ettClientMessageMap[ettpc_notstartingchart].c_str());
					}
					writer.EndObject();
					Send(s.GetString());
				} break;
				case ettps_recievechat: {
					auto& payload = d["payload"];
					if (!payload.HasMember("msgtype") ||
						!payload["msgtype"].IsInt() ||
						!payload.HasMember("tab") ||
						!payload["tab"].IsString() ||
						!payload.HasMember("msg") || !payload["msg"].IsString())
						continue;
					// chat[tabname, tabtype] = msg
					int type = payload["msgtype"].GetInt();
					const char* tab = payload["tab"].GetString();
					n->chat[{ tab, type }].push_back(
					  payload["msg"].GetString());
					SCREENMAN->SendMessageToTopScreen(ETTP_IncomingChat);
					Message msg("Chat");
					msg.SetParam("tab", std::string(tab));
					msg.SetParam("msg",
								 std::string(payload["msg"].GetString()));
					msg.SetParam("type", type);
					MESSAGEMAN->Broadcast(msg);
				} break;
				case ettps_mpleaderboardupdate: {
					if (PREFSMAN->m_bEnableScoreboard) {
						auto& payload = d["payload"];
						auto& scores = payload["scores"];
						for (auto& score : scores.GetArray()) {
							if (!score.HasMember("wife") ||
								!score["wife"].IsNumber() ||
								!score.HasMember("jdgstr") ||
								!score["jdgstr"].IsString() ||
								!score.HasMember("user") ||
								!score["user"].IsString())
								continue;
							float wife = score["wife"].GetFloat();
							std::string jdgstr = score["jdgstr"].GetString();
							string user = score["user"].GetString();
							n->mpleaderboard[user].wife = wife;
							n->mpleaderboard[user].jdgstr = jdgstr;
						}
						Message msg("MPLeaderboardUpdate");
						MESSAGEMAN->Broadcast(msg);
					}
				} break;
				case ettps_createroomresponse: {
					auto& payload = d["payload"];
					bool created = payload.HasMember("created") &&
								   payload["created"].IsBool() &&
								   payload["created"].GetBool();
					inRoom = created;
					if (created) {
						Message msg(
						  MessageIDToString(Message_UpdateScreenHeader));
						msg.SetParam("Header", roomName);
						msg.SetParam("Subheader", roomDesc);
						MESSAGEMAN->Broadcast(msg);
						std::string SMOnlineSelectScreen = THEME->GetMetric(
						  "ScreenNetRoom", "MusicSelectScreen");
						SCREENMAN->SendMessageToTopScreen(SM_GoToNextScreen);
					}
				} break;
				case ettps_chartrequest: {
					auto& payload = d["payload"];
					if (!payload.HasMember("rate") ||
						!payload["rate"].IsInt() ||
						!payload.HasMember("chartkey") ||
						!payload["chartkey"].IsString() ||
						!payload.HasMember("requester") ||
						!payload["requester"].IsString())
						continue;
					n->requests.push_back(
					  new ChartRequest(payload["chartkey"].GetString(),
									   payload["requester"].GetString(),
									   payload["rate"].GetInt()));
					Message msg("ChartRequest");
					MESSAGEMAN->Broadcast(msg);
				} break;
				case ettps_enterroomresponse: {
					auto& payload = d["payload"];
					bool entered = payload["entered"].GetBool();
					inRoom = false;
					if (entered) {
						try {
							Message msg(
							  MessageIDToString(Message_UpdateScreenHeader));
							msg.SetParam("Header", roomName);
							msg.SetParam("Subheader", roomDesc);
							MESSAGEMAN->Broadcast(msg);
							inRoom = true;
							std::string SMOnlineSelectScreen = THEME->GetMetric(
							  "ScreenNetRoom", "MusicSelectScreen");
							SCREENMAN->SetNewScreen(SMOnlineSelectScreen);
						} catch (std::exception& e) {
							Locator::getLogger()->error("Error while parsing ettp json enter "
									   "room response: {}",
									   e.what());
						}
					} else {
						roomDesc = "";
						roomName = "";
					}
				} break;
				case ettps_newroom: {
					auto& payload = d["payload"];
					if (!payload.HasMember("room") ||
						!payload["room"].IsObject())
						continue;
					auto tmp = jsonToRoom(payload["room"]);
					n->m_Rooms.push_back(tmp);
					SCREENMAN->SendMessageToTopScreen(ETTP_RoomsChange);
				} break;
				case ettps_deleteroom: {
					auto& payload = d["payload"];
					if (!payload.HasMember("room") ||
						!payload["room"].IsObject() ||
						!payload["room"].HasMember("name") ||
						!payload["room"]["name"].IsString()) {
						Locator::getLogger()->warn("Invalid ETTP deleteroom room message");
						continue;
					}
					string name = payload["room"]["name"].GetString();
					n->m_Rooms.erase(std::remove_if(n->m_Rooms.begin(),
													n->m_Rooms.end(),
													[&](RoomData const& room) {
														return room.Name() ==
															   name;
													}),
									 n->m_Rooms.end());
					SCREENMAN->SendMessageToTopScreen(ETTP_RoomsChange);
				} break;
				case ettps_updateroom: {
					auto& payload = d["payload"];
					if (!payload.HasMember("room") ||
						!payload["room"].IsObject())
						continue;
					auto updated = jsonToRoom(payload["room"]);

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
				} break;
				case ettps_lobbyuserlist: {
					auto& payload = d["payload"];
					NSMAN->lobbyuserlist.clear();
					if (!payload.HasMember("users") ||
						!payload["users"].IsArray())
						continue;
					auto& users = payload["users"];
					for (auto& user : users.GetArray()) {
						if (!user.IsString())
							continue;
						NSMAN->lobbyuserlist.insert(user.GetString());
					}
				} break;
				case ettps_lobbyuserlistupdate: {
					auto& payload = d["payload"];
					if (payload.HasMember("on") && payload["on"].IsArray()) {
						auto& newUsers = payload["on"];
						for (auto& user : newUsers.GetArray()) {
							if (!user.IsString())
								continue;
							NSMAN->lobbyuserlist.insert(user.GetString());
						}
					}
					if (payload.HasMember("off") && payload["off"].IsArray()) {
						auto& removedUsers = payload["off"];
						for (auto& user : removedUsers.GetArray()) {
							if (!user.IsString())
								continue;
							NSMAN->lobbyuserlist.erase(user.GetString());
						}
					}
					MESSAGEMAN->Broadcast("UsersUpdate");
				} break;
				case ettps_roomlist: {
					auto& payload = d["payload"];
					RoomData tmp;
					n->m_Rooms.clear();
					if (!payload.HasMember("rooms") ||
						!payload["rooms"].IsArray())
						continue;
					auto& rooms = payload["rooms"];
					for (auto& room : rooms.GetArray()) {
						if (room.IsObject())
							n->m_Rooms.push_back(jsonToRoom(room));
					}
					SCREENMAN->SendMessageToTopScreen(ETTP_RoomsChange);
				} break;
				case ettps_roompacklist: {
					auto& payload = d["payload"];
					if (!payload.HasMember("commonpacks"))
						continue;
					auto& packlist = payload["commonpacks"];
					n->commonpacks.clear();
					if (packlist.IsArray()) {
						for (auto& pack : packlist.GetArray()) {
							if (!pack.IsString())
								continue;
							n->commonpacks.push_back(pack.GetString());
						}
					}
				} break;
				case ettps_roomuserlist: {
					auto& payload = d["payload"];
					n->m_ActivePlayer.clear();
					n->m_PlayerNames.clear();
					n->m_PlayerStatus.clear();
					n->m_PlayerReady.clear();
					if (!payload.HasMember("players") ||
						!payload["players"].IsArray())
						continue;
					auto& players = payload["players"];
					int i = 0;
					for (auto& player : players.GetArray()) {
						if (!player.HasMember("name") ||
							!player["name"].IsString() ||
							!player.HasMember("status") ||
							!player["status"].IsInt() ||
							!player.HasMember("ready") ||
							!player["ready"].IsBool())
							continue;
						n->m_PlayerNames.push_back(player["name"].GetString());
						n->m_PlayerStatus.push_back(player["status"].GetInt());
						n->m_PlayerReady.push_back(player["ready"].GetBool());
						n->m_ActivePlayer.push_back(i++);
					}
					MESSAGEMAN->Broadcast("UsersUpdate");
				} break;
				case ettps_end:
				default:
					break;
			}
		} catch (std::exception& e) {
			Locator::getLogger()->error("Error while parsing ettp json message: {}", e.what());
		}
	}
	newMessages.clear();
}

void
NetworkSyncManager::StartUp()
{
	std::string ServerIP;

	if (GetCommandlineArgument("netip", &ServerIP))
		PostStartUp(ServerIP);
	else if (autoConnectMultiplayer)
		PostStartUp(std::string(g_sLastServer));
}

void
NetworkSyncManager::ReportNSSOnOff(int i)
{
	if (curProtocol != nullptr)
		curProtocol->ReportNSSOnOff(i);
}

std::string
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
	if (client == nullptr)
		return;
	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_logout].c_str());
	writer.EndObject();
	Send(s.GetString());
}
void
NetworkSyncManager::Login(std::string user, std::string pass)
{
	if (curProtocol != nullptr)
		curProtocol->Login(user, pass);
}
void
ETTProtocol::SendChat(const std::string& message, string tab, int type)
{
	if (client == nullptr)
		return;
	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_sendchat].c_str());
	writer.Key("payload");
	writer.StartObject();
	writer.Key("msg");
	if (message.length() > 500)
		writer.String(message.substr(0, 500).c_str());
	else
		writer.String(message.c_str());
	writer.Key("tab");
	writer.String(tab.c_str());
	writer.Key("msgtype");
	writer.Int(type);
	writer.EndObject();
	writer.EndObject();
	Send(s.GetString());
}
void
ETTProtocol::SendMPLeaderboardUpdate(float wife, std::string& jdgstr)
{
	if (client == nullptr)
		return;
	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_mpleaderboardupdate].c_str());
	writer.Key("payload");
	writer.StartObject();
	writer.Key("wife");
	if (std::isfinite(wife))
		writer.Double(wife);
	else
		writer.Null();
	writer.Key("jdgstr");
	writer.String(jdgstr.c_str());
	writer.EndObject();
	writer.EndObject();
	Send(s.GetString());
}
void
ETTProtocol::CreateNewRoom(std::string name,
						   std::string desc,
						   std::string password)
{
	if (client == nullptr || creatingRoom)
		return;
	creatingRoom = true;
	timeoutStart = clock();
	waitingForTimeout = true;
	timeout = 1;
	onTimeout = [this](void) { this->creatingRoom = false; };
	roomName = name.c_str();
	roomDesc = desc.c_str();

	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_createroom].c_str());
	writer.Key("payload");
	writer.StartObject();
	writer.Key("name");
	writer.String(name.c_str());
	writer.Key("pass");
	writer.String(password.c_str());
	writer.Key("desc");
	writer.String(desc.c_str());
	writer.EndObject();
	writer.EndObject();

	Send(s.GetString());
}
void
ETTProtocol::LeaveRoom(NetworkSyncManager* n)
{
	if (client == nullptr)
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

	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_leaveroom].c_str());
	writer.EndObject();
	Send(s.GetString());

	roomName = "";
	roomDesc = "";
	inRoom = false;
}
void
ETTProtocol::EnterRoom(std::string name, std::string password)
{
	if (client == nullptr)
		return;
	auto it = find_if(NSMAN->m_Rooms.begin(),
					  NSMAN->m_Rooms.end(),
					  [&name](const RoomData& r) { return r.Name() == name; });
	if (it == NSMAN->m_Rooms.end())
		return; // Unknown room
	roomName = name.c_str();
	roomDesc = it->Description().c_str();

	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_enterroom].c_str());
	writer.Key("payload");
	writer.StartObject();
	writer.Key("name");
	writer.String(name.c_str());
	writer.Key("pass");
	writer.String(password.c_str());
	writer.EndObject();
	writer.EndObject();

	Send(s.GetString());
}
void
ETTProtocol::Login(std::string user, std::string pass)
{
	if (client == nullptr)
		return;

	NSMAN->loggedInUsername = user.c_str();
	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_login].c_str());
	writer.Key("payload");
	writer.StartObject();
	writer.Key("user");
	writer.String(user.c_str());
	writer.Key("pass");
	writer.String(pass.c_str());
	writer.EndObject();
	writer.EndObject();

	Send(s.GetString());

	timeoutStart = clock();
	waitingForTimeout = true;
	timeout = 5.0;
	onTimeout = [](void) {
		NSMAN->loggedInUsername.clear();
		NSMAN->loginResponse = LOGIN_TIMEOUT.GetValue();
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
ETTProtocol::Send(const char* msg)
{
	if (client != nullptr) {
		client->send(*hdl, msg, websocketpp::frame::opcode::text);
	}
}
void
ETTProtocol::ReportHighScore(HighScore* hs, PlayerStageStats& pss)
{
	if (client == nullptr)
		return;
	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_sendscore].c_str());
	writer.Key("payload");
	writer.StartObject();

	writer.Key("scorekey");
	writer.String(hs->GetScoreKey().c_str());
	writer.Key("ssr_norm");
	writer.Double(hs->GetSSRNormPercent());
	writer.Key("max_combo");
	writer.Int(hs->GetMaxCombo());
	writer.Key("valid");
	writer.Int(hs->GetEtternaValid());
	writer.Key("mods");
	writer.String(hs->GetModifiers().c_str());
	writer.Key("miss");
	writer.Int(hs->GetTapNoteScore(TNS_Miss));
	writer.Key("bad");
	writer.Int(hs->GetTapNoteScore(TNS_W5));
	writer.Key("good");
	writer.Int(hs->GetTapNoteScore(TNS_W4));
	writer.Key("great");
	writer.Int(hs->GetTapNoteScore(TNS_W3));
	writer.Key("perfect");
	writer.Int(hs->GetTapNoteScore(TNS_W2));
	writer.Key("marv");
	writer.Int(hs->GetTapNoteScore(TNS_W1));
	writer.Key("score");
	writer.Double(hs->GetSSRNormPercent());
	writer.Key("wifever");
	writer.Int(hs->GetWifeVersion());
	auto& r = hs->GetRadarValues();
	FOREACH_ENUM(RadarCategory, rc)
	{
		writer.Key(RadarCategoryToString(rc).c_str());
		writer.Int(r[rc]);
	}
	FOREACH_ENUM(Skillset, ss)
	{
		writer.Key(SkillsetToString(ss).c_str());
		writer.Double(hs->GetSkillsetSSR(ss));
	}
	writer.Key("datetime");
	writer.String(hs->GetDateTime().GetString().c_str());
	writer.Key("hitmine");
	writer.Int(hs->GetTapNoteScore(TNS_HitMine));
	writer.Key("held");
	writer.Int(hs->GetHoldNoteScore(HNS_Held));
	writer.Key("letgo");
	writer.Int(hs->GetHoldNoteScore(HNS_LetGo));
	writer.Key("ng");
	writer.Int(hs->GetHoldNoteScore(HNS_Missed));
	writer.Key("chartkey");
	writer.String(hs->GetChartKey().c_str());
	writer.Key("rate");
	writer.Double(hs->GetMusicRate());
	if (GAMESTATE->m_pPlayerState != nullptr) {
		writer.Key("options");
		writer.String(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
						.GetString()
						.c_str());
	}
	auto chart = SONGMAN->GetStepsByChartkey(hs->GetChartKey());
	writer.Key("negsolo");
	writer.Int(chart->GetTimingData()->HasWarps() ||
			   chart->m_StepsType != StepsType_dance_single);
	writer.Key("nocc");
	writer.Int(!hs->GetChordCohesion());
	writer.Key("calc_version");
	writer.Int(hs->GetSSRCalcVersion());
	writer.Key("topscore");
	writer.Int(hs->GetTopScore());
	writer.Key("uuid");
	writer.String(hs->GetMachineGuid().c_str());
	writer.Key("hash");
	writer.String(hs->GetValidationKey(ValidationKey_Brittle).c_str());
	const auto& offsets = pss.GetOffsetVector();
	const auto& noterows = pss.GetNoteRowVector();
	const auto& tracks = pss.GetTrackVector();
	const auto& types = pss.GetTapNoteTypeVector();
	if (offsets.size() > 0) {
		writer.Key("replay");
		writer.StartObject();
		writer.Key("noterows");
		writer.StartArray();
		for (size_t i = 0; i < noterows.size(); i++)
			writer.Int(noterows[i]);
		writer.EndArray();
		writer.Key("offsets");
		writer.StartArray();
		for (size_t i = 0; i < offsets.size(); i++)
			writer.Double(offsets[i]);
		writer.EndArray();
		writer.Key("tracks");
		writer.StartArray();
		for (size_t i = 0; i < tracks.size(); i++)
			writer.Int(tracks[i]);
		writer.EndArray();
		writer.Key("notetypes");
		writer.StartArray();
		for (size_t i = 0; i < types.size(); i++)
			writer.Int(types[i]);
		writer.EndArray();
		writer.EndObject();
	}

	writer.EndObject();
	writer.EndObject();

	Send(s.GetString());
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
	if (client == nullptr)
		return;

	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	writer.String(ettClientMessageMap[ettpc_gameover].c_str());
	writer.EndObject();

	Send(s.GetString());
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
	// This needs to be reset before ScreenEvaluation could possibly be
	// called
	m_EvalPlayerData.clear();
	if (curProtocol != nullptr)
		curProtocol->StartRequest(this, position);
}

void
NetworkSyncManager::DisplayStartupStatus()
{
	std::string sMessage("");

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
NetworkSyncManager::SendChat(const std::string& message, string tab, int type)
{
	if (curProtocol != nullptr)
		curProtocol->SendChat(message, tab, type);
}

void
NetworkSyncManager::SendMPLeaderboardUpdate(float wife, std::string& jdgstr)
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
	auto curSteps = GAMESTATE->m_pCurSteps;
	if (client == nullptr || song == nullptr || curSteps == nullptr ||
		GAMESTATE->m_pPlayerState == nullptr)
		return;

	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();
	writer.Key("id");
	writer.Uint(msgId++);
	writer.Key("type");
	if (song == n->song) {
		writer.String(ettClientMessageMap[ettpc_startchart].c_str());
	} else {
		writer.String(ettClientMessageMap[ettpc_selectchart].c_str());
	}
	writer.Key("payload");
	writer.StartObject();
	writer.Key("title");
	writer.String(correct_non_utf_8(song->m_sMainTitle).c_str());
	writer.Key("subtitle");
	writer.String(correct_non_utf_8(song->m_sSubTitle).c_str());
	writer.Key("artist");
	writer.String(correct_non_utf_8(song->m_sArtist).c_str());
	writer.Key("filehash");
	writer.String(song->GetFileHash().c_str());
	writer.Key("pack");
	writer.String(correct_non_utf_8(song->m_sGroupName).c_str());
	writer.Key("chartkey");
	writer.String(curSteps->GetChartKey().c_str());
	writer.Key("difficulty");
	writer.String(DifficultyToString(curSteps->GetDifficulty()).c_str());
	writer.Key("meter");
	writer.Int(curSteps->GetMeter());
	writer.Key("options");
	writer.String(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent()
					.GetString()
					.c_str());
	writer.Key("rate");
	writer.Int(static_cast<int>(
	  GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate * 1000));
	writer.EndObject();
	writer.EndObject();

	Send(s.GetString());
}

void
NetworkSyncManager::EnterRoom(std::string name, std::string password)
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
NetworkSyncManager::CreateNewRoom(std::string name,
								  std::string desc,
								  std::string password)
{
	if (curProtocol != nullptr)
		curProtocol->CreateNewRoom(name, desc, password);
}

void
NetworkSyncManager::RequestRoomInfo(std::string name)
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

std::string
PacketFunctions::ReadNT()
{
	// int Orig=Packet.Position;
	std::string TempStr;
	while ((Position < NETMAXBUFFERSIZE) && (((char*)Data)[Position] != 0))
		TempStr = TempStr + static_cast<char>(Data[Position++]);

	++Position;
	return TempStr;
}

void
PacketFunctions::Write1(uint8_t data)
{
	if (Position >= NETMAXBUFFERSIZE)
		return;
	if (data != 1)
		Locator::getLogger()->trace("testing");
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
PacketFunctions::WriteNT(const std::string& data)
{
	size_t index = 0;
	while (Position < NETMAXBUFFERSIZE - 1 && index < data.size())
		Data[Position++] = static_cast<unsigned char>(data.c_str()[index++]);
	Data[Position++] = 0;
}

void
PacketFunctions::ClearPacket()
{
	memset(static_cast<void*>(&Data), 0, NETMAXBUFFERSIZE);
	Position = 0;
}

void
NetworkSyncManager::GetListOfLANServers(std::vector<NetServerInfo>& AllServers)
{
	AllServers = m_vAllLANServers;
}

// Aldo: Please move this method to a new class, I didn't want to create new
// files because I don't know how to properly update the files for each
// platform. I preferred to misplace code rather than cause unneeded
// headaches to non-windows users, although it would be nice to have in the
// wiki which files to update when adding new files and how
// (Xcode/stepmania_xcode4.3.xcodeproj has a really crazy structure :/).
unsigned long
NetworkSyncManager::GetCurrentSMBuild(LoadingWindow* ld)
{
	return 0;
}

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
}

static bool
ConnectToServer(const std::string& t)
{
	NSMAN->PostStartUp(t);
	return true;
}

LuaFunction(ConnectToServer,
			ConnectToServer((std::string(SArg(1)).length() == 0)
							  ? std::string(g_sLastServer)
							  : std::string(SArg(1))))

  static bool ReportStyle()
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
#include "Etterna/Models/Lua/LuaBinding.h"

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
		NSMAN->PushMPLeaderboard(L);
		return 1;
	}
	static int RemoveChartRequest(T* p, lua_State* L)
	{
		auto& reqs = p->requests;
		auto reqPtrToRemove = Luna<ChartRequest>::check(L, 1, true);
		auto new_end = remove_if(
		  reqs.begin(), reqs.end(), [reqPtrToRemove](ChartRequest* req) {
			  return req == reqPtrToRemove;
		  });
		reqs.erase(new_end, reqs.end());
		// Keep it in case lua keeps a reference to it
		p->staleRequests.push_back(reqPtrToRemove);
		return 0;
	}
	static int GetChartRequests(T* p, lua_State* L)
	{
		auto& reqs = p->requests;
		lua_newtable(L);
		int i = 1;
		for (auto& req : reqs) {
			req->PushSelf(L);
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
		std::string user = SArg(1);
		std::string pass = SArg(2);
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
	static int GetLoggedInUsername(T* p, lua_State* L)
	{
		lua_pushstring(L, NSMAN->loggedInUsername.c_str());
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
		ADD_METHOD(GetLoggedInUsername);
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
