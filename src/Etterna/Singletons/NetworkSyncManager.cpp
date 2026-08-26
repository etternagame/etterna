#include "Etterna/Globals/global.h"
#include "NetworkSyncManager.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/MessageManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Models/Misc/RoomWheel.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Core/Services/Locator.hpp"
#include "arch/LoadingWindow/LoadingWindow.h"

#include "Etterna/Models/Network/ETTProtocol.h"

NetworkSyncManager* NSMAN;

#include <cerrno>
#include <chrono>
#include <cmath>
#include <array>
#ifndef _WIN32
#include <arpa/inet.h>
#endif


extern Preference<std::string> g_sLastServer;
Preference<unsigned int> autoConnectMultiplayer("AutoConnectMultiplayer", 1);


static LocalizedString CONNECTION_SUCCESSFUL("NetworkSyncManager",
											 "Connection to '%s' successful.");
static LocalizedString CONNECTION_FAILED("NetworkSyncManager",
										 "Connection failed.");

NetworkSyncManager::NetworkSyncManager(LoadingWindow* ld)
{
	NSMAN = this;
	useSMserver = false;
	isSMOnline = false;
	loggedIn = false;
	m_startupStatus = NSMANStartupStatus::INIT;
	m_ActivePlayers = 0;
	if (ld) {
		ld->SetIndeterminate(true);
		ld->SetText("\nConnecting to multiplayer server");
	}
	StartUp();

	m_iSelectMode = 0;

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
	if (ShouldSendMessage(true))
		curProtocol->OnMusicSelect();
}

void
NetworkSyncManager::OffMusicSelect()
{
	if (ShouldSendMessage(true))
		curProtocol->OffMusicSelect();
}

void
NetworkSyncManager::OnOptions()
{
	if (ShouldSendMessage(true))
		curProtocol->OnOptions();
}
void
NetworkSyncManager::OffOptions()
{
	if (ShouldSendMessage(true))
		curProtocol->OffOptions();
}

void
NetworkSyncManager::OnEval()
{
	if (ShouldSendMessage(true))
		curProtocol->OnEval();
}
void
NetworkSyncManager::OffEval()
{
	if (ShouldSendMessage(true))
		curProtocol->OffEval();
}

void
NetworkSyncManager::CloseConnection()
{
	if (!useSMserver)
		return;
	m_sChatText = "";
	need_to_disconnect = false;
	useSMserver = false;
	isSMOnline = false;
	loggedIn = false;
	loginResponse = "";
	m_startupStatus = NSMANStartupStatus::INIT;
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
	m_startupStatus = NSMANStartupStatus::NOT_SUCCESSFUL;

	size_t cLoc = ServerIP.find(':');

	auto tmp = ServerIP;
	if (ServerIP.starts_with("wss://")) {
		tmp = ServerIP.substr(6);
	} else if (ServerIP.starts_with("ws://")) {
		tmp = ServerIP.substr(5);
	}

	if (tmp.find(':') != std::string::npos) {
		sAddress = tmp.substr(0, cLoc);
		char* cEnd;
		errno = 0;
		auto sub = tmp.substr(cLoc + 1);
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
	Locator::getLogger()->info(
	  "Attempting to connect to: {}, Port: {}", sAddress.c_str(), iPort);
	curProtocol = nullptr;
	CloseConnection();

	auto ETTP = new ETTProtocol;

	if (ETTP->Connect(this, iPort, sAddress))
		curProtocol = ETTP;
	if (curProtocol == nullptr)
		return;
	g_sLastServer.Set(ServerIP);
	loggedIn = false;
	useSMserver = true;
	m_startupStatus = NSMANStartupStatus::SUCCESSFUL;
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
NetworkSyncManager::IsETTP()
{
	return true;
}

bool
NetworkSyncManager::ShouldSendMessage(bool requiresLogin) const
{
	return curProtocol != nullptr && isSMOnline &&
		   (!requiresLogin || (requiresLogin && loggedIn));
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

std::string
NetworkSyncManager::GetServerName() const
{
	return curProtocol != nullptr ? curProtocol->serverName : "";
}

void
NetworkSyncManager::Logout()
{
	if (ShouldSendMessage(true))
		curProtocol->Logout();
}

void
NetworkSyncManager::Login(std::string user, std::string pass)
{
	if (ShouldSendMessage(false))
		curProtocol->Login(user, pass);
}

void
NetworkSyncManager::ReportHighScore(HighScore* hs, PlayerStageStats& pss)
{
	if (ShouldSendMessage(true))
		curProtocol->ReportHighScore(hs, pss);
}

void
NetworkSyncManager::ReportReplayInput(bool isPress,
									  int col,
									  int row,
									  float fMusicSeconds,
									  float fNoteOffset,
									  int tapNoteType,
									  int tapNoteSubType)
{
	if (ShouldSendMessage(true) && !spectating)
		curProtocol->ReportReplayInput(this,
									   isPress,
									   col,
									   row,
									   fMusicSeconds,
									   fNoteOffset,
									   tapNoteType,
									   tapNoteSubType);
}

void
NetworkSyncManager::ReportReplayMiss(int col,
									 int row,
									 int tapNoteType,
									 int tapNoteSubType)
{
	if (ShouldSendMessage(true) && !spectating)
		curProtocol->ReportReplayMiss(
		  this, col, row, tapNoteType, tapNoteSubType);
}

void
NetworkSyncManager::ReportReplayHold(int col, int row, int subType)
{
	if (ShouldSendMessage(true) && !spectating)
		curProtocol->ReportReplayHold(this, col, row, subType);
}

void
NetworkSyncManager::ReportReplayMine(int row, int col)
{
	if (ShouldSendMessage(true) && !spectating)
		curProtocol->ReportReplayMine(this, row, col);
}

void
NetworkSyncManager::ReportV2Data(int col,
								 int row,
								 float offset,
								 int tapNoteType)
{
	if (ShouldSendMessage(true) && !spectating)
		curProtocol->ReportV2Data(col, row, offset, tapNoteType);
}

void
NetworkSyncManager::ReportSongOver()
{
	if (ShouldSendMessage(true))
		curProtocol->ReportSongOver(this);
}

void
NetworkSyncManager::StartRequest(short position)
{
	// This needs to be reset before ScreenEvaluation could possibly be
	// called
	m_EvalPlayerData.clear();
	if (ShouldSendMessage(true))
		curProtocol->StartRequest(this, position);
}

void
NetworkSyncManager::DisplayStartupStatus() const
{
	std::string sMessage("");

	switch (m_startupStatus) {
		case NSMANStartupStatus::INIT:
			return;
		case NSMANStartupStatus::SUCCESSFUL:
			if (curProtocol != nullptr)
				sMessage = ssprintf(CONNECTION_SUCCESSFUL.GetValue(),
									curProtocol->serverName.c_str());
			else
				sMessage = CONNECTION_FAILED.GetValue();
			break;
		case NSMANStartupStatus::NOT_SUCCESSFUL:
			sMessage = CONNECTION_FAILED.GetValue();
			break;
	}
	SCREENMAN->SystemMessage(sMessage);
}

void
NetworkSyncManager::Update(float fDeltaTime)
{
	if (curProtocol != nullptr) {
		if (need_to_disconnect) {
			CloseConnection();
		} else {
			curProtocol->Update(this, fDeltaTime);
		}
	}
}

void
NetworkSyncManager::SendChat(const std::string& message,
							 std::string tab,
							 int type)
{
	if (ShouldSendMessage(true))
		curProtocol->SendChat(message, tab, type);
}

void
NetworkSyncManager::SendMPLeaderboardUpdate(float wife, std::string& jdgstr)
{
	if (ShouldSendMessage(true) && !spectating)
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
	if (ShouldSendMessage(true))
		curProtocol->SelectUserSong(this, GAMESTATE->m_pCurSong);
}

void
NetworkSyncManager::EnterRoom(std::string name, std::string password)
{
	if (ShouldSendMessage(true))
		curProtocol->EnterRoom(name, password);
}

void
NetworkSyncManager::LeaveRoom()
{
	if (ShouldSendMessage (true))
		curProtocol->LeaveRoom(this);
}

void
NetworkSyncManager::CreateNewRoom(std::string name,
								  std::string desc,
								  std::string password)
{
	if (ShouldSendMessage(true))
		curProtocol->CreateNewRoom(name, desc, password);
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

std::string
NetworkSyncManager::GetRoomName()
{
	if (curProtocol == nullptr) {
		return "";
	}
	if (!IsETTP()) {
		return "";
	}
	auto ettp = dynamic_cast<ETTProtocol*>(curProtocol);
	if (!ettp->InRoom()) {
		return "";
	}
	return ettp->GetRoomName();
}

bool
NetworkSyncManager::CreatingRoom()
{
	if (curProtocol == nullptr) {
		return false;
	}
	if (!IsETTP()) {
		return false;
	}
	auto ettp = dynamic_cast<ETTProtocol*>(curProtocol);
	return ettp->CreatingRoom();
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

static bool
CloseNetworkConnection()
{
	NSMAN->CloseConnection();
	return true;
}

LuaFunction(IsSMOnlineLoggedIn, NSMAN->loggedIn)
LuaFunction(IsNetConnected, NSMAN->useSMserver)
LuaFunction(IsNetSMOnline, NSMAN->isSMOnline)
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
		std::string tabName = SArg(3);
		lua_pushstring(L, p->chat[{ tabName, tabType }][l].c_str());
		return 1;
	}
	static int GetCurrentRoomName(T* p, lua_State* L)
	{
		auto roomname = p->GetRoomName();
		if (roomname == "") {
			lua_pushnil(L);
		}
		lua_pushstring(L, roomname.c_str());
		return 1;
	}
	static int SendChatMsg(T* p, lua_State* L)
	{
		std::string msg = SArg(1);
		int tabType = IArg(2);
		std::string tabName = SArg(3);
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
