#pragma once
#ifndef SM_DOWNMANAGER

#define SM_DOWNMANAGER

#include "Etterna/Globals/global.h"
#include "CommandLineActions.h"
#include "RageUtil/File/RageFile.h"
#include "Etterna/Models/Misc/HighScore.h"
#include "ScreenManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "curl/curl.h"
#include "Etterna/Models/Misc/Difficulty.h"
#include <deque>

class DownloadablePack;

class ProgressData
{
  public:
	curl_off_t total{ 0 };		// total bytes
	curl_off_t downloaded{ 0 }; // bytes downloaded
	float time{ 0 };			// seconds passed
};

class RageFileWrapper
{
  public:
	RageFile file;
	size_t bytes{ 0 };
	bool stop{ false };
};

class Download
{
  public:
	function<void(Download*)> Done;
	Download(
	  string url,
	  string filename = "",
	  function<void(Download*)> done = [](Download*) { return; });
	~Download();
	void Install();
	void Update(float fDeltaSeconds);
	void Failed();
	string StartMessage()
	{
		return "Downloading file " + m_TempFileName + " from " + m_Url;
	};
	string Status()
	{
		return m_TempFileName + "\n" + speed + " KB/s\n" + "Downloaded " +
			   to_string((progress.downloaded > 0 ? progress.downloaded
												  : p_RFWrapper.bytes) /
						 1024) +
			   (progress.total > 0
				  ? "/" + to_string(progress.total / 1024) + " (KB)"
				  : "");
	}
	CURL* handle{ nullptr };
	int running{ 1 };
	ProgressData progress;
	string speed{ "" };
	curl_off_t downloadedAtLastUpdate{ 0 };
	curl_off_t lastUpdateDone{ 0 };
	string m_Url{ "" };
	RageFileWrapper p_RFWrapper;
	DownloadablePack* p_Pack{ nullptr };
	string m_TempFileName{ "" };
	// Lua
	void PushSelf(lua_State* L);

  protected:
	string MakeTempFileName(string s);
};

class DownloadablePack
{
  public:
	string name{ "" };
	size_t size{ 0 };
	int id{ 0 };
	float avgDifficulty{ 0 };
	string url{ "" };
	string mirror{ "" };
	bool downloading{ false };
	// Lua
	void PushSelf(lua_State* L);
};

class HTTPRequest
{
  public:
	HTTPRequest(
	  CURL* h,
	  function<void(HTTPRequest&, CURLMsg*)> done = [](HTTPRequest& req,
													   CURLMsg*) { return; },
	  curl_httppost* postform = nullptr,
	  function<void(HTTPRequest&, CURLMsg*)> fail = [](HTTPRequest& req,
													   CURLMsg*) { return; })
	  : handle(h)
	  , form(postform)
	  , Done(done)
	  , Failed(fail){};
	CURL* handle{ nullptr };
	curl_httppost* form{ nullptr };
	string result;
	function<void(HTTPRequest&, CURLMsg*)> Done;
	function<void(HTTPRequest&, CURLMsg*)> Failed;
};
class OnlineTopScore
{
  public:
	float wifeScore{ 0.0f };
	string songName;
	float rate{ 0.0f };
	float ssr{ 0.0f };
	float overall{ 0.0f };
	string chartkey;
	string scorekey;
	Difficulty difficulty;
	string steps;
};
struct OnlineHighScore : HighScore
{
  public:
	bool hasReplay;
	bool HasReplayData() override { return hasReplay; }
};
class OnlineScore
{
  public:
	map<Skillset, float> SSRs;
	float rate{ 0.0f };
	float wife{ 0.0f };
	int maxcombo{ 0 };
	int miss{ 0 };
	int bad{ 0 };
	int good{ 0 };
	int great{ 0 };
	int perfect{ 0 };
	int marvelous{ 0 };
	int minehits{ 0 };
	int held{ 0 };
	string songId;
	int letgo{ 0 };
	bool valid{ false };
	bool nocc{ false };
	string username;
	float playerRating{ 0.0f };
	string modifiers;
	string scoreid;
	string avatar;
	int userid;
	DateTime datetime;
	bool hasReplay{ false };
	vector<pair<float, float>> replayData;
	string countryCode;
	OnlineHighScore hs;
	void Push(lua_State* L) { hs.PushSelf(L); }
	bool HasReplayData() { return hasReplay; }
};

class DownloadManager
{
  public:
	static LuaReference EMPTY_REFERENCE;
	DownloadManager();
	~DownloadManager();
	map<string, Download*> downloads; // Active downloads
	vector<HTTPRequest*>
	  HTTPRequests; // Active HTTP requests (async, curlMulti)

	map<string, Download*> finishedDownloads;
	map<string, Download*> pendingInstallDownloads;
	CURLM* mPackHandle{ nullptr }; // Curl multi handle for packs downloads
	CURLM* mHTTPHandle{ nullptr }; // Curl multi handle for httpRequests
	CURLMcode ret = CURLM_CALL_MULTI_PERFORM;
	int downloadingPacks{ 0 };
	int HTTPRunning{ 0 };
	bool loggingIn{
		false
	}; // Currently logging in (Since it's async, to not try twice)
	bool gameplay{ false }; // Currently in gameplay
	bool initialized{ false };
	string error{ "" };
	vector<DownloadablePack> downloadablePacks;
	string authToken{ "" };	  // Session cookie content
	string sessionUser{ "" }; // Currently logged in username
	string sessionPass{ "" }; // Currently logged in password
	string lastVersion{
		""
	}; // Last version according to server (Or current if non was obtained)
	string registerPage{
		""
	}; // Register page from server (Or empty if non was obtained)
	map<string, vector<OnlineScore>> chartLeaderboards;
	vector<string> countryCodes;
	map<Skillset, int>
	  sessionRanks; // Leaderboard ranks for logged in user by skillset
	map<Skillset, double> sessionRatings;
	map<Skillset, vector<OnlineTopScore>> topScores;
	bool LoggedIn();

	void AddFavorite(const string& chartkey);
	void RemoveFavorite(const string& chartkey);
	void RefreshFavourites();
	vector<string> favorites;

	void AddGoal(const string& chartkey,
				 float wife,
				 float rate,
				 DateTime& timeAssigned);
	void UpdateGoal(const string& chartkey,
					float wife,
					float rate,
					bool achieved,
					DateTime& timeAssigned,
					DateTime& timeAchieved);
	void RemoveGoal(const string& chartkey, float wife, float rate);

	void EndSessionIfExists(); // Calls EndSession if logged in
	void EndSession();		   // Sends session destroy request
	void StartSession(string user,
					  string pass,
					  function<void(bool loggedIn)>
						done); // Sends login request if not already logging in
	void OnLogin();
	bool UploadScores(); // Uploads all scores not yet uploaded to current
	void ForceUploadScoresForChart(
	  const std::string& ck,
	  bool startnow = true); // forced upload wrapper for charts
	void ForceUploadScoresForPack(
	  const std::string& pack,
	  bool startnow = true); // forced upload wrapper for packs
	void ForceUploadAllScores();
	void RefreshPackList(const string& url);

	void init();
	Download* DownloadAndInstallPack(const string& url, string filename = "");
	Download* DownloadAndInstallPack(DownloadablePack* pack,
									 bool mirror = false);
	void Update(float fDeltaSeconds);
	void UpdatePacks(float fDeltaSeconds);
	void UpdateHTTP(float fDeltaSeconds);
	bool InstallSmzip(const string& sZipFile);

	void UpdateDLSpeed();
	void UpdateDLSpeed(bool gameplay);

	string GetError() { return error; }
	bool Error() { return error == ""; }
	bool EncodeSpaces(string& str);

	void UploadScore(HighScore* hs,
					 function<void()> callback,
					 bool load_from_disk);
	void UploadScoreWithReplayData(HighScore* hs);
	void UploadScoreWithReplayDataFromDisk(
	  HighScore* hs,
	  function<void()> callback = []() {});

	bool ShouldUploadScores();

	inline void AddSessionCookieToCURL(CURL* curlHandle);
	inline void SetCURLPostToURL(CURL* curlHandle, string url);
	inline void SetCURLURL(CURL* curlHandle, string url);

	HTTPRequest* SendRequest(string requestName,
							 vector<pair<string, string>> params,
							 function<void(HTTPRequest&, CURLMsg*)> done,
							 bool requireLogin = true,
							 bool post = false,
							 bool async = true,
							 bool withBearer = true);
	HTTPRequest* SendRequestToURL(string url,
								  vector<pair<string, string>> params,
								  function<void(HTTPRequest&, CURLMsg*)> done,
								  bool requireLogin,
								  bool post,
								  bool async,
								  bool withBearer);
	void RefreshLastVersion();
	void RefreshRegisterPage();
	bool currentrateonly = false;
	bool topscoresonly = true;
	bool ccoffonly = true;
	void RefreshCountryCodes();
	void RequestReplayData(const string& scorekey,
						   int userid,
						   const string& username,
						   const string& chartkey,
						   LuaReference& callback = EMPTY_REFERENCE);
	void RequestChartLeaderBoard(const string& chartkey,
								 LuaReference& ref = EMPTY_REFERENCE);
	void RefreshUserData();
	string countryCode;
	void RefreshUserRank();
	void RefreshTop25(Skillset ss);
	void DownloadCoreBundle(const string& whichoneyo, bool mirror = true);
	map<string, vector<DownloadablePack*>> bundles;
	void RefreshCoreBundles();
	vector<DownloadablePack*> GetCoreBundle(const string& whichoneyo);
	OnlineTopScore GetTopSkillsetScore(unsigned int rank,
									   Skillset ss,
									   bool& result);
	float GetSkillsetRating(Skillset ss);
	int GetSkillsetRank(Skillset ss);

	// most recent single score upload result -mina
	RString mostrecentresult = "";
	deque<pair<DownloadablePack*, bool>> DownloadQueue; // (pack,isMirror)
	deque<HighScore*> ScoreUploadSequentialQueue;
	unsigned int sequentialScoreUploadTotalWorkload{ 0 };
	const int maxPacksToDownloadAtOnce = 1;
	const float DownloadCooldownTime = 5.f;
	float timeSinceLastDownload = 0.f;

	// Lua
	void PushSelf(lua_State* L);
};

extern shared_ptr<DownloadManager> DLMAN;

#endif
