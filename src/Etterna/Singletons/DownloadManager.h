#pragma once
#ifndef SM_DOWNMANAGER
#define SM_DOWNMANAGER

#include "Etterna/Globals/global.h"
#include "RageUtil/File/RageFile.h"
#include "Etterna/Models/HighScore/HighScore.h"
#include "ScreenManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "Etterna/Models/Misc/Difficulty.h"

#include "curl/curl.h"
#include "rapidjson/document.h"

#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <math.h>

class ScoreGoal;
class DownloadablePack;
class RageTexture;

class ProgressData
{
  public:
	/// total bytes
	std::atomic<curl_off_t> total{ 0 };
	/// bytes downloaded
	std::atomic<curl_off_t> downloaded{ 0 };
	/// seconds passed
	float time{ 0 };
};

class RageFileWrapper
{
  public:
	RageFile file;
	std::atomic<size_t> bytes{ 0 };
	std::atomic<bool> stop{ false };
};

class Download
{
  public:
	Download(
	  std::string url,
	  std::string filename = "");
	~Download();
	void Install();
	void Update(float fDeltaSeconds);
	void Failed();
	std::string StartMessage()
	{
		auto str = "Downloading: " + m_TempFileName;
		if (str.length() > 64) {
			str = str.substr(0, 60) + "...";
		}
		return str;
	};
	std::string Status()
	{
		return m_TempFileName + "\n" + speed + " KB/s\n" + "Downloaded " +
			   std::to_string((progress.downloaded.load() > 0
								 ? progress.downloaded.load()
								 : p_RFWrapper.bytes.load()) /
							  1024) +
			   (progress.total.load() > 0
				  ? "/" + std::to_string(progress.total.load() / 1024) + " (KB)"
				  : "");
	}
	CURL* handle{ nullptr };
	int running{ 1 };
	ProgressData progress;
	std::string speed{ "" };
	curl_off_t downloadedAtLastUpdate{ 0 };
	curl_off_t lastUpdateDone{ 0 };
	std::string m_Url{ "" };
	RageFileWrapper p_RFWrapper;
	DownloadablePack* p_Pack{ nullptr };
	std::string m_TempFileName{ "" };
	// Lua
	void PushSelf(lua_State* L);

  protected:
	std::string MakeTempFileName(std::string s);
};

class DownloadablePack
{
  public:
	std::string name{ "" };
	int64_t size{ 0 };
	int id{ 0 };
	float avgDifficulty{ 0 };
	int plays{ 0 };
	int songs{ 0 };
	bool nsfw{ false };
	std::string url{ "" };
	std::string mirror{ "" };
	std::string bannerUrl{ "" };
	std::string thumbnail{ "" };
	bool downloading{ false };
	bool isQueued();
	RageTexture* GetThumbnailTexture();
	// Lua
	void PushSelf(lua_State* L);
};

class DownloadablePackPaginationKey
{
  public:
	DownloadablePackPaginationKey(const std::string& searchString,
								  std::set<std::string>& tagFilters,
								  int perPage,
								  const std::string& sortField,
								  bool ascendingSort)
	  : searchString(searchString)
	  , tagFilters(tagFilters)
	  , perPage(perPage)
	  , sortByField(sortField)
	  , sortIsAsc(ascendingSort)
	{
	}
	DownloadablePackPaginationKey()
	  : searchString("")
	  , tagFilters({})
	  , perPage(0)
	  , sortByField("")
	  , sortIsAsc(true)
	{
	}

	bool operator==(const DownloadablePackPaginationKey& other) const
	{
		return (perPage == other.perPage) &&
			   (searchString == other.searchString) &&
			   (tagFilters == other.tagFilters) &&
			   (sortByField == other.sortByField) &&
			   (sortIsAsc == other.sortIsAsc);
	}

	std::string searchString{};
	std::set<std::string> tagFilters{};
	int perPage = 0;
	std::string sortByField{};
	bool sortIsAsc = true;
};

class DownloadablePackPagination
{
  public:
	DownloadablePackPagination(const std::string& searchString,
							   std::set<std::string>& tagFilters,
							   int perPage,
							   const std::string& sortByField,
							   bool sortIsAsc)
	  : key(searchString, tagFilters, perPage, sortByField, sortIsAsc)
	{
	}
	DownloadablePackPagination(const DownloadablePackPaginationKey& key)
	  : key(key)
	{
	}
	DownloadablePackPagination()
	  : key({}) // ??
	{
	}

	bool operator==(const DownloadablePackPagination& other)
	{
		// under normal circumstances no two paginations
		// with the same key and different data
		// should exist
		return key == other.key;
	}

	DownloadablePackPaginationKey key;

	int currentPage = 0;
	int totalEntries = 0;
	std::vector<DownloadablePack*> results{};

	// if true, page can't move
	bool pendingRequest = false;

	// if true, a request was made and got no results
	bool noResults = false;

	bool initialized = false;

	void initialize(LuaReference& whenDone = EMPTY_REFERENCE) {
		setPage(0, whenDone);
	}

	// get the cached packs on the current page
	std::vector<DownloadablePack*> get() {
		auto it = results.begin();
		if (results.size() > currentPageStartIndex()) {
			// make sure the start iterator is valid
			it += currentPageStartIndex();
		}

		std::vector<DownloadablePack*> o{};

		while (it != results.end()) {
			o.push_back(*it);
			it++;
		}
		return o;
	}

	// get all the packs that are cached
	std::vector<DownloadablePack*> getCache() { return results; }

	// move to next page and send packs to lua function
	// possibly invoking a search request
	void nextPage(LuaReference& whenDone = EMPTY_REFERENCE)
	{
		setPage(getNextPageNbr(), whenDone);
	}
	// move to prev page and send packs to lua function
	// possibly invoking a search request
	void prevPage(LuaReference& whenDone = EMPTY_REFERENCE)
	{
		setPage(getPrevPageNbr(), whenDone);
	}

	// number of pages, so "1" has only a page "0"
	int getTotalPages() {
		return static_cast<int>(ceilf(static_cast<float>(totalEntries) /
									  static_cast<float>(key.perPage)));
	}

	// Lua
	void PushSelf(lua_State* L);

  private:
	/// Default empty reference for calls allowing Lua functions to be passed
	static LuaReference EMPTY_REFERENCE;

	std::set<int> finishedPageRequests{};

	// return inclusive starting index of page
	int currentPageStartIndex() {
		return key.perPage * currentPage;
	}

	int getNextPageNbr() {
		if (key.perPage >= totalEntries) {
			return 0;
		}
		if (currentPage == getTotalPages() - 1) {
			return 0;
		} else {
			return currentPage + 1;
		}
	}
	int getPrevPageNbr() {
		if (key.perPage >= totalEntries) {
			return 0;
		}
		if (currentPage == 0) {
			return getTotalPages() - 1;
		} else {
			return currentPage - 1;
		}
	}
	// move to n page and send packs to lua function
	// possibly invoking a search request
	void setPage(int page, LuaReference& whenDone = EMPTY_REFERENCE);

	bool mustRequestPage(int page) {
		const auto ind = key.perPage * page;
		if (results.size() <= ind) {
			// dont make requests out of range
			return false;
		}
		// if null, need to request it
		return results.at(ind) == nullptr;
	}
};

// required to actually use the key as a key
// in unordered containers
template<>
struct std::hash<DownloadablePackPaginationKey>
{
	std::size_t operator()(const DownloadablePackPaginationKey& k) const {
		size_t r = 17;
		r = r * 31 + std::hash<int>()(k.perPage);
		r = r * 31 + std::hash<std::string>()(k.searchString);
		size_t s = 0;
		for (const auto& tag : k.tagFilters) {
			std::hash<std::string> h;
			s ^= h(tag) + 0x9e3779b9 + (s << 6) + (s >> 2);
		}
		r = r * 31 + s;
		r = r * 31 + std::hash<std::string>()(k.sortByField);
		r = r * 31 + std::hash<bool>()(k.sortIsAsc);
		return r;
	}
};

struct ApiSearchCriteria
{
	// request body
	std::string packName{};
	std::string songName{};
	std::string chartAuthor{};
	std::string songArtist{};
	std::vector<std::string> packTags{};

	std::string sortBy{};
	bool sortIsAscending = true;

	// request params (actual names)
	int page = 0;
	int per_page = 0;

	auto DebugString() const -> std::string
	{
		std::string o = "(ApiSearchCriteria ";
		o += fmt::format("page: {}, per_page: {}", page, per_page);
		if (!packName.empty()) {
			o += fmt::format(", packName: {}", packName);
		}
		if (!songName.empty()) {
			o += fmt::format(", songName: {}", songName);
		}
		if (!chartAuthor.empty()) {
			o += fmt::format(", chartAuthor: {}", chartAuthor);
		}
		if (!songArtist.empty()) {
			o += fmt::format(", songArtist: {}", songArtist);
		}
		if (!packTags.empty()) {
			o += ", tags: [";
			for (auto& s : packTags) {
				o += fmt::format("'{}'", s);
			}
			o += "]";
		}
		if (!sortBy.empty()) {
			o += fmt::format(
			  ", sortBy: {}:{}", sortBy, sortIsAscending ? "asc" : "desc");
		}
		return o + ")";
	}
};


class HTTPRequest
{
  public:
	HTTPRequest(
	  CURL* h,
	  std::function<void(HTTPRequest&)> done = [](HTTPRequest& req) {},
	  curl_httppost* postform = nullptr,
	  std::function<void(HTTPRequest&)> fail = [](HTTPRequest& req) {},
	  bool requiresLogin = false)
	  : handle(h)
	  , form(postform)
	  , Done(done)
	  , Failed(fail)
	  , requiresLogin(requiresLogin){};
	long response_code{ 0 };
	CURL* handle{ nullptr };
	curl_httppost* form{ nullptr };
	std::string result;
	std::string headers;
	std::function<void(HTTPRequest&)> Done;
	std::function<void(HTTPRequest&)> Failed;
	bool requiresLogin = false;
};
enum class RequestMethod
{
	GET = 0,
	POST = 1,
	PATCH = 2,
	DEL = 3,
};
class OnlineTopScore
{
  public:
	float wifeScore{ 0.0f };
	std::string songName;
	float rate{ 0.0f };
	float ssr{ 0.0f };
	float overall{ 0.0f };
	std::string chartkey;
	std::string scorekey;
	Difficulty difficulty;
	std::string steps;
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
	std::map<Skillset, float> SSRs;
	float rate{ 0.0f };
	float wife{ 0.0f };
	int wifeversion{ 0 };
	int maxcombo{ 0 };
	int miss{ 0 };
	int bad{ 0 };
	int good{ 0 };
	int great{ 0 };
	int perfect{ 0 };
	int marvelous{ 0 };
	int minehits{ 0 };
	int held{ 0 };
	int holdmiss{ 0 };
	std::string songId;
	int letgo{ 0 };
	bool valid{ false };
	bool nocc{ false };
	std::string username;
	float playerRating{ 0.0f };
	std::string modifiers;
	std::string scoreid;
	std::string avatar;
	int userid;
	DateTime datetime;
	bool hasReplay{ false };
	std::vector<std::pair<float, float>> replayData;
	std::string countryCode;
	OnlineHighScore hs;
	void Push(lua_State* L) { hs.PushSelf(L); }
	bool HasReplayData() { return hasReplay; }
};

class DownloadManager
{
  // new
  public:
	DownloadManager();
	~DownloadManager();

	/////
	// Upkeep
	void Init();
	void Update(float fDeltaSeconds);
	void UpdatePacks(float fDeltaSeconds);
	void UpdateHTTP(float fDeltaSeconds);
	void UpdateGameplayState(bool gameplay);

	/////
	// Lua external access
	void PushSelf(lua_State* L);

	/////
	// State getters
	bool LoggedIn();
	bool ShouldUploadScores();
	bool InGameplay() { return gameplay; }

	/////
	// External Facing API Request Generators
	void Login(
	  const std::string& username,
	  const std::string& password,
	  std::function<void(bool)> done = [](bool loggedIn) {})
	{
		LoginRequest(username, password, done);
	}
	void LoginWithToken(
	  const std::string& sessionToken,
	  std::function<void(bool)> done = [](bool loggedIn) {});
	void Logout();
	void LogoutIfLoggedIn()
	{
		if (LoggedIn())
			Logout();
	}

	void GetRankedChartkeys(
	  std::function<void(void)> callback = []() {},
	  const DateTime start = DateTime::GetFromString("1990-01-01 12:00:00"),
	  const DateTime end = DateTime::GetFromString("2100-01-01 12:00:00"))
	{
		GetRankedChartkeysRequest(callback, start, end);
	}
	void AddFavorite(const std::string& chartKey)
	{
		AddFavoriteRequest(chartKey);
	}
	void BulkAddFavorites(
	  std::vector<std::string> chartKeys,
	  std::function<void()> = []() {});
	void RemoveFavorite(const std::string& chartKey)
	{
		RemoveFavoriteRequest(chartKey);
	}
	void RefreshFavorites(
	  const DateTime start = DateTime::GetFromString("1990-01-01 12:00:00"),
	  const DateTime end = DateTime::GetFromString("2100-01-01 12:00:00"));
	void AddGoal(ScoreGoal* goal) {
		AddGoalRequest(goal);
	}
	void BulkAddGoals(
	  std::vector<ScoreGoal*> goals,
	  std::function<void()> callback = []() {});
	void UpdateGoal(ScoreGoal* goal) {
		UpdateGoalRequest(goal);
	}
	void RemoveGoal(ScoreGoal* goal, bool oldGoal = false) {
		RemoveGoalRequest(goal, oldGoal);
	}
	void RefreshGoals(
	  const DateTime start = DateTime::GetFromString("1990-01-01 12:00:00"),
	  const DateTime end = DateTime::GetFromString("2100-01-01 12:00:00"));
	void AddPlaylist(const std::string& name) {
		AddPlaylistRequest(name);
	}
	void UpdatePlaylist(const std::string& name)
	{
		UpdatePlaylistRequest(name);
	}
	void RemovePlaylist(const std::string& name)
	{
		RemovePlaylistRequest(name);
	}
	// the point of this is to maintain the online id of playlists
	void LoadPlaylists(
	  const DateTime start = DateTime::GetFromString("1990-01-01 12:00:00"),
	  const DateTime end = DateTime::GetFromString("2100-01-01 12:00:00"));
	void DownloadMissingPlaylists(
	  const DateTime start = DateTime::GetFromString("1990-01-01 12:00:00"),
	  const DateTime end = DateTime::GetFromString("2100-01-01 12:00:00"));
	void DownloadPlaylist(const std::string& name);

	// Score upload functions
	void UploadScore(HighScore* hs,
					 std::function<void()> callback,
					 bool load_from_disk);
	void UploadScoreWithReplayData(HighScore* hs);
	void UploadScoreWithReplayDataFromDisk(
	  HighScore* hs,
	  std::function<void()> callback = []() {});
	void UploadBulkScores(
	  std::vector<HighScore*> hsList,
	  std::function<void()> callback = []() {});

	void RefreshPackTags();
	void SearchForPacks(ApiSearchCriteria searchCriteria,
						std::function<void(rapidjson::Document&)> packSearchParser)
	{
		MultiSearchRequest(searchCriteria, packSearchParser);
	}

	// Mass score upload functions
	bool ForceUploadPBsForChart(const std::string& ck, bool startNow = false);
	bool ForceUploadPBsForPack(const std::string& pack, bool startNow = false);
	bool ForceUploadAllPBs();
	bool InitialScoreSync();

	void GetChartLeaderboard(const std::string& chartkey,
							 LuaReference& ref = EMPTY_REFERENCE)
	{
		GetChartLeaderboardRequest(chartkey, ref);
	}
	void GetReplayData(const std::string& scorekey,
					   int userid,
					   const std::string& username,
					   const std::string& chartkey,
					   LuaReference& callback = EMPTY_REFERENCE)
	{
		GetReplayDataRequest(scorekey, userid, username, chartkey, callback);
	}

	OnlineTopScore GetTopSkillsetScore(unsigned int rank,
									   Skillset ss,
									   bool& result);
	float GetSkillsetRating(Skillset ss);
	int GetSkillsetRank(Skillset ss);

	DownloadablePackPagination& GetPackPagination(
	  const std::string& searchString,
	  std::set<std::string> tagFilters,
	  int perPage,
	  const std::string& sortBy,
	  bool sortIsAsc);
	std::shared_ptr<Download> DownloadAndInstallPack(const std::string& url,
													 std::string filename = "");
	std::shared_ptr<Download> DownloadAndInstallPack(DownloadablePack* pack,
													 bool mirror = false);

	/////
	// User session
	// Session cookie content
	std::string authToken{ "" };
	// Currently logged in username
	std::string sessionUser{ "" };
	// Leaderboard ranks for logged in user by skillset
	std::map<Skillset, int> sessionRanks{};
	// Online profile skillset ratings
	std::map<Skillset, double> sessionRatings{};

	/////
	// Score uploads
	std::deque<HighScore*> ScoreUploadSequentialQueue;
	unsigned int sequentialScoreUploadTotalWorkload{ 0 };

	/////
	// Goal uploads
	std::deque<ScoreGoal*> GoalUploadSequentialQueue;
	unsigned int sequentialGoalUploadTotalWorkload{ 0 };

	/////
	// Favorite uploads
	std::deque<std::string> FavoriteUploadSequentialQueue;
	unsigned int sequentialFavoriteUploadTotalWorkload{ 0 };

	/////
	// Pack downloads
	// Active downloads
	std::map<std::string, std::shared_ptr<Download>> downloads;
	// (pack,isMirror)
	std::deque<std::pair<DownloadablePack*, bool>> DownloadQueue;
	std::map<std::string, std::shared_ptr<Download>> finishedDownloads;
	std::map<std::string, std::shared_ptr<Download>> pendingInstallDownloads;
	std::map<int, DownloadablePack> downloadablePacks;
	std::unordered_map<std::string, std::vector<std::string>> packTags;
	std::unordered_map<DownloadablePackPaginationKey,
					   DownloadablePackPagination>
	  downloadablePackPaginations{};

	/////
	// Chart leaderboards
	std::map<std::string, std::vector<OnlineScore>> chartLeaderboards{};
	std::map<Skillset, std::vector<OnlineTopScore>> topScores{};
	std::set<std::string> unrankedCharts{};
	bool currentrateonly = false;
	bool topscoresonly = true;
	bool validonly = true;

	/////
	// Misc information
	// Last version according to server (Or current if non was obtained)
	std::string lastVersion{ "" };
	std::string countryCode{ "" };

  private:
	/// Default empty reference for calls allowing Lua functions to be passed
	static LuaReference EMPTY_REFERENCE;

	// Events
	void OnLogin();

	// Checks for generic response codes where we have consistent behavior
	// status 401 - log out the user, cancel uploads
	bool HandleAuthErrorResponse(const std::string& endpoint, HTTPRequest& req);
	// status 429 - requeue the request, wait a while
	bool HandleRatelimitResponse(const std::string& endpoint, HTTPRequest& req);
	bool QueueRequestIfRatelimited(const std::string& endpiont,
								   HTTPRequest& req);
	void QueueRatelimitedRequest(const std::string& endpoint, HTTPRequest& req);
	// basically returns HandleAuthErrorResponse || HandleRatelimitResponse
	// and runs the given lambda for the 401 or the 429. Won't run the 429
	// if a 401 is given first.
	bool Handle401And429Response(
	  const std::string& endpoint,
	  HTTPRequest& req,
	  std::function<void()> if401 = []() {},
	  std::function<void()> if429 = []() {});


	// Specialized API Requests
	void LoginRequest(const std::string& username,
					  const std::string& password,
					  std::function<void(bool)> done);
	void GetRankedChartkeysRequest(std::function<void(void)> callback,
								   const DateTime start,
								   const DateTime end);
	void AddFavoriteRequest(const std::string& chartKey);
	void RemoveFavoriteRequest(const std::string& chartKey);
	void GetFavoritesRequest(
	  std::function<void(std::set<std::string>)> onSuccess,
	  const DateTime start,
	  const DateTime end);
	void AddGoalRequest(ScoreGoal* goal);
	void UpdateGoalRequest(ScoreGoal* goal);
	void RemoveGoalRequest(ScoreGoal* goal, bool oldGoal);
	void GetGoalsRequest(std::function<void(std::vector<ScoreGoal>)> onSuccess,
						 const DateTime start,
						 const DateTime end);
	void AddPlaylistRequest(const std::string& name);
	void UpdatePlaylistRequest(const std::string& name);
	void RemovePlaylistRequest(const std::string& name);
	void GetPlaylistsRequest(
	  std::function<void(std::unordered_map<std::string, Playlist>)> onSuccess,
	  const DateTime start,
	  const DateTime end);
	void GetPlaylistRequest(std::function<void(Playlist)> onSuccess, int id);
	void GetChartLeaderboardRequest(const std::string& chartkey,
									LuaReference& ref);
	void GetReplayDataRequest(const std::string& scorekey,
							  int userid,
							  const std::string& username,
							  const std::string& chartkey,
							  LuaReference& callback);

	void RefreshUserData();
	void RequestTop25(Skillset ss);
	void RefreshLastVersion();

	void GetPackTagsRequest();

	void MultiSearchRequest(ApiSearchCriteria searchCriteria,
							std::function<void(rapidjson::Document&)> whenDoneParser);

	// To send a request to API base URL and ratelimit at that URL
	HTTPRequest* SendRequest(
	  std::string requestName,
	  std::vector<std::pair<std::string, std::string>> params,
	  std::function<void(HTTPRequest&)> done,
	  bool requireLogin = true,
	  RequestMethod httpMethod = RequestMethod::GET,
	  bool compressed = false,
	  bool async = true,
	  bool withBearer = true);
	// To send a request to API based URL, ratelimit at different path
	HTTPRequest* SendRequest(
	  std::string requestName,
	  std::string apiPath,
	  std::vector<std::pair<std::string, std::string>> params,
	  std::function<void(HTTPRequest&)> done,
	  bool requireLogin = true,
	  RequestMethod httpMethod = RequestMethod::GET,
	  bool compressed = false,
	  bool async = true,
	  bool withBearer = true);
	// Send a request directly to a given URL, ratelimit at other path
	HTTPRequest* SendRequestToURL(
	  std::string url,
	  std::string apiPath,
	  std::vector<std::pair<std::string, std::string>> params,
	  std::function<void(HTTPRequest&)> done,
	  bool requireLogin,
	  RequestMethod httpMethod,
	  bool compressed,
	  bool async,
	  bool withBearer);

	// Active HTTP requests (async, curlMulti)
	std::vector<HTTPRequest*> HTTPRequests;

	CURLM* pack_multi_handle = nullptr;
	CURLM* http_req_handle = nullptr;

	// Currently logging in (Since it's async, to not try twice)
	bool loggingIn{ false };
	// Currently in gameplay y/n
	bool gameplay{ false };
	bool initialized{ false };

	float timeSinceLastDownload = 0.F;

	std::unordered_map<std::string, std::chrono::steady_clock::time_point>
	  endpointRatelimitTimestamps{};
	std::unordered_map<std::string, std::vector<HTTPRequest*>>
	  ratelimitedRequestQueue{};
	std::unordered_set<std::string> newlyRankedChartkeys{};


  // old
  public:

	void RefreshPackList(const std::string& url);

	void DownloadCoreBundle(const std::string& whichoneyo, bool mirror = false);
	std::vector<DownloadablePack*> GetCoreBundle(const std::string& whichoneyo);
};

extern std::shared_ptr<DownloadManager> DLMAN;

#endif
