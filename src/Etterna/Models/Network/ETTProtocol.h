#pragma once

#include "NetProtocol.h"
#include "NetworkConstants.h"

#include "rapidjson/document.h"
#include <curl/curl.h>

#include <string>
#include <mutex>
#include <thread>
#include <functional>

class NetworkSyncManager;
struct HighScore;

// Websockets using websocketpp sending json
class ETTProtocol : public NetProtocol
{

	std::unique_ptr<std::thread> thread;
	std::mutex messageBufferMutex;
	std::vector<std::unique_ptr<rapidjson::Document>> newMessages;
	unsigned int msgId{ 0 };
	bool error{ false };
	std::string errorMsg;

	CURL* curl;
	std::mutex curlMutex;

	void FindJsonChart(NetworkSyncManager* n, rapidjson::Value& ch);
	std::atomic_bool stopRequest = false;
	void LaunchPollingThread();
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

	// triggered by button presses in gameplay
	void ReportReplayInput(NetworkSyncManager* n,
						   bool isPress,
						   int col,
						   int row,
						   float fMusicSeconds,
						   float fNoteOffset,
						   int tapNoteType,
						   int tapNoteSubType) override;

	// triggered by a miss in gameplay
	void ReportReplayMiss(NetworkSyncManager* n,
						  int col,
						  int row,
						  int tapNoteType,
						  int tapNoteSubType) override;

	// triggered by completing or dropping a hold in gameplay
	void ReportReplayHold(NetworkSyncManager* n,
						  int col,
						  int row,
						  int subType) override;

	// triggered by hitting a mine in gameplay
	void ReportReplayMine(NetworkSyncManager* n, int row, int col) override;
	void Send(const std::string& str);

private:
	rapidjson::Document newMsg(const ETTClientMessageTypes& msgType);
	void completeAndSend(rapidjson::Document& doc);

	void handleLogin(rapidjson::Value& payload);
	void handleHello(rapidjson::Value& payload);
	void handleReceiveScore(rapidjson::Value& payload);
	void handlePing();
	void handleSelectChart(rapidjson::Value& payload);
	void handleStartChart(rapidjson::Value& payload);
	void handleReceiveChat(rapidjson::Value& payload);
	void handleMPLeaderboardUpdate(rapidjson::Value& payload);
	void handleCreateRoomResponse(rapidjson::Value& payload);
	void handleChartRequest(rapidjson::Value& payload);
	void handleEnterRoomResponse(rapidjson::Value& payload);
	void handleNewRoom(rapidjson::Value& payload);
	void handleDeleteRoom(rapidjson::Value& payload);
	void handleUpdateRoom(rapidjson::Value& payload);
	void handleLobbyUserlist(rapidjson::Value& payload);
	void handleLobbyUserlistUpdate(rapidjson::Value& payload);
	void handleRoomlist(rapidjson::Value& payload);
	void handleRoomPacklist(rapidjson::Value& payload);
	void handleRoomUserlist(rapidjson::Value& payload);

};
