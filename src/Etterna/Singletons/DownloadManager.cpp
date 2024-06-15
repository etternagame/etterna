#include "Etterna/Globals/global.h"
#include "RageUtil/File/RageFileManager.h"
#include "ScreenManager.h"
#include "Etterna/Models/Misc/Preference.h"
#include "Core/Services/Locator.hpp"
#include "Core/Platform/Platform.hpp"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Utils/RageUtil.h"
#include "DownloadManager.h"
#include "GameState.h"
#include "ScoreManager.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Screen/Network/ScreenNetSelectMusic.h"
#include "ProfileManager.h"
#include "SongManager.h"
#include "ScoreManager.h"
#include "Etterna/Screen/Others/ScreenInstallOverlay.h"
#include "Etterna/Screen/Others/ScreenSelectMusic.h"
#include "Etterna/Globals/SpecialFiles.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Misc/PlayerStageStats.h"
#include "Etterna/Models/Misc/PlayerOptions.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "Etterna/Singletons/CryptManager.h"

#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "jwt-cpp/jwt.h"

#ifdef _WIN32
#include <intrin.h>
#endif

#include <cmath>
#include <regex>
#include <tuple>
#include <unordered_set>
#include <algorithm>

using namespace rapidjson;

std::shared_ptr<DownloadManager> DLMAN = nullptr;
LuaReference DownloadManager::EMPTY_REFERENCE = LuaReference();

static bool runningSequentialScoreUpload = false;
static bool runningSequentialGoalUpload = false;
static bool runningSequentialFavoriteUpload = false;

// Pack API Preferences
static Preference<unsigned int> maxDLPerSecond(
  "maximumBytesDownloadedPerSecond",
  0);
static Preference<unsigned int> maxDLPerSecondGameplay(
  "maximumBytesDownloadedPerSecondDuringGameplay",
  1000000);
static Preference<unsigned int> maxPacksToDownloadAtOnce(
  "parallelDownloadsAllowed",
  1);
static Preference<float> DownloadCooldownTime(
  "secondsBetweenConsecutiveDownloads",
  5.F);

// Score API Preferences
static Preference<std::string> serverURL("BaseAPIUrl",
										 "https://api.beta.etternaonline.com");
static Preference<std::string> searchURL("BaseAPISearchUrl",
										 "https://search.etternaonline.com");
static Preference<std::string> uiHomePage("BaseUIUrl", "https://beta.etternaonline.com");
static Preference<unsigned int> automaticSync("automaticScoreSync", 1);

// 
static const std::string DL_DIR = SpecialFiles::CACHE_DIR + "Downloads/";
static const std::string wife3_rescore_upload_flag = "rescoredw3";

static Preference<unsigned int> UPLOAD_SCORE_BULK_CHUNK_SIZE(
  "bulkScoreUploadChunkSize",
  100);
static Preference<unsigned int> UPLOAD_GOAL_BULK_CHUNK_SIZE(
  "bulkGoalUploadChunkSize",
  100);
static Preference<unsigned int> UPLOAD_FAVORITE_BULK_CHUNK_SIZE(
  "bulkFavoriteUploadChunkSize",
  100);

// endpoint construction constants
// all paths should begin with / and end without /
/// API root path
static const std::string API_ROOT = "/api/client";
static const std::string API_KEY = "CsrvreCj5YnhxYcpfFboFKZvCBf6wbNV2tAX4XAojD7mBFLVojCpEnhicLnRFy7wCqY2LRoocAhSuLcQxMWZvRDewJAzCgA82UFPKvWMQbXp6GjikqqRVNfqopwWk6nFy";
static const std::string TYPESENSE_API_KEY = "uNVBQbmgvnet2LTpT6sE3XYe7JeD8xej";

static const std::string API_LOGIN = "/login";
static const std::string API_RANKED_CHARTKEYS = "/charts/ranked";
static const std::string API_CHART_LEADERBOARD = "/charts/{}/scores";
static const std::string API_UPLOAD_SCORE = "/scores";
static const std::string API_GET_SCORE = "/scores/{}";
static const std::string API_UPLOAD_SCORE_BULK = "/scores/bulk";
static const std::string API_FAVORITES = "/favorites";
static const std::string API_FAVORITES_BULK = "/favorites/bulk";
static const std::string API_GOALS = "/goals";
static const std::string API_GOALS_BULK = "/goals/bulk";
static const std::string API_PLAYLISTS = "/playlists";
static const std::string API_PLAYLIST = "/playlists/{}";
static const std::string API_TAGS = "/tags";
static const std::string API_USER = "/users/{}";
static const std::string API_USER_SCORES = "/users/{}/scores";
static const std::string API_GAME_VERSION = "/settings/version";

static const std::string API_SEARCH = "/multi_search";

static constexpr bool DO_COMPRESS = true;
static constexpr bool DONT_COMPRESS = false;

std::mutex G_MTX_SCORE_UPLOAD;

inline std::string
APIROOT()
{
	return serverURL.Get() + API_ROOT;
}

inline std::string
SEARCHROOT()
{
	return searchURL.Get();
}

size_t
write_memory_buffer(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	std::string tmp(static_cast<char*>(contents), realsize);
	static_cast<std::string*>(userp)->append(tmp);
	return realsize;
}
template<typename T>
inline void
curl_easy_setopt_log_err(CURL *handle, CURLoption option, T param)
{
	// TODO: Once we update curl replace "" with curl_easy_option_by_id(option)->name
	CURLcode ret = curl_easy_setopt(handle, option, param);
	if (ret != CURLE_OK)
		//"Error setting curl option %d(%s): %s(%d)", option,
		// curl_easy_option_by_id(option)->name, curl_easy_strerror(ret), ret);
		Locator::getLogger()->warn("Error setting curl option {}({}): {}({})",
								   option,
								   "",
								   curl_easy_strerror(ret),
								   ret);
}

std::atomic<bool> QUIT_OTHER_THREADS_FLAG = false;
RageSemaphore THREAD_EXIT_COUNT("DLMAN thread exit semaphore", 0);

std::string
ComputerIdentity()
{
	std::string computerName = "";
	std::string userName = "";
#ifdef _WIN32

	TCHAR infoBuf[1024];
	DWORD bufCharCount = 1024;
	if (GetComputerName(infoBuf, &bufCharCount))
		computerName = infoBuf;
	if (GetUserName(infoBuf, &bufCharCount))
		userName = infoBuf;
#else
	char hostname[1024];
	char username[1024];
	gethostname(hostname, 1024);
	getlogin_r(username, 1024);
	computerName = hostname;
	userName = username;
#endif
	return computerName + ":_:" + userName;
}

inline void
EmptyTempDLFileDir()
{
	std::vector<std::string> files;
	FILEMAN->GetDirListing(DL_DIR + "*", files, false, true);
	for (auto& file : files) {
		if (FILEMAN->IsAFile(file))
			FILEMAN->Remove(file);
	}
}

#pragma region curl
inline CURL*
initBasicCURLHandle()
{
	CURL* curlHandle = curl_easy_init();
	curl_easy_setopt_log_err(curlHandle,
					 CURLOPT_USERAGENT,
					 "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
					 "AppleWebKit/537.36 (KHTML, like Gecko) "
					 "Chrome/60.0.3112.113 Safari/537.36");
	curl_easy_setopt_log_err(curlHandle, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt_log_err(curlHandle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
	return curlHandle;
}

inline CURL*
initCURLHandle(bool withBearer, bool acceptJson, bool compressed)
{
	CURL* curlHandle = initBasicCURLHandle();
	struct curl_slist* list = nullptr;
	if (withBearer)
		list = curl_slist_append(
		  list, ("Authorization: Bearer " + DLMAN->authToken).c_str());
	if (acceptJson) {
		list = curl_slist_append(list, "Accept: application/json");
		list = curl_slist_append(list, "Content-Type: application/json");
		list = curl_slist_append(list, "charset: utf-8");
		list = curl_slist_append(list, ("X-TYPESENSE-API-KEY: " + TYPESENSE_API_KEY).c_str());
	}
	if (compressed) {
		// use compress_string on body
		list = curl_slist_append(list, "Content-Encoding: deflate");
		list = curl_slist_append(list, "Accept-Encoding: *");
	}
	curl_easy_setopt_log_err(curlHandle, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_TIMEOUT, 120); // Seconds
	return curlHandle;
}

// Configurees a CURL handle so it writes it's output to a std::string
inline void
SetCURLResultsString(CURL* curlHandle, std::string* str)
{
	curl_easy_setopt_log_err(curlHandle, CURLOPT_WRITEDATA, str);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_WRITEFUNCTION, write_memory_buffer);
}

inline void
SetCURLHeadersString(CURL* curlHandle, std::string* str)
{
	curl_easy_setopt_log_err(curlHandle, CURLOPT_HEADERDATA, str);
	curl_easy_setopt_log_err(
	  curlHandle, CURLOPT_HEADERFUNCTION, write_memory_buffer);
}

inline void
SetCURLURL(CURL* curlHandle, std::string url)
{
	if (!(starts_with(url, "https://") || starts_with(url, "http://")))
		url = std::string("https://").append(url);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_URL, url.c_str());
}

inline void
CURLAPIURL(CURL* curlHandle, std::string endpoint)
{
	SetCURLURL(curlHandle, APIROOT() + endpoint);
}

inline auto
CURLSEARCHURL(CURL* curlHandle, std::string endpoint)
{
	SetCURLURL(curlHandle, SEARCHROOT() + endpoint);
}

void
CURLFormPostField(curl_httppost*& form,
				  curl_httppost*& lastPtr,
				  const char* field,
				  const char* value)
{
	curl_formadd(&form,
				 &lastPtr,
				 CURLFORM_COPYNAME,
				 field,
				 CURLFORM_COPYCONTENTS,
				 value,
				 CURLFORM_END);
}

inline void
SetCURLFormPostField(curl_httppost*& form,
					 curl_httppost*& lastPtr,
					 char* field,
					 char* value)
{
	CURLFormPostField(form, lastPtr, field, value);
}

inline void
SetCURLFormPostField(curl_httppost*& form,
					 curl_httppost*& lastPtr,
					 const char* field,
					 std::string value)
{
	CURLFormPostField(form, lastPtr, field, value.c_str());
}

inline void
SetCURLFormPostField(curl_httppost*& form,
					 curl_httppost*& lastPtr,
					 std::string field,
					 std::string value)
{
	CURLFormPostField(form, lastPtr, field.c_str(), value.c_str());
}

template<typename T>
inline void
SetCURLFormPostField(curl_httppost*& form,
					 curl_httppost*& lastPtr,
					 std::string field,
					 T value)
{
	CURLFormPostField(
	  form, lastPtr, field.c_str(), std::to_string(value).c_str());
}
#pragma endregion curl


inline std::string
jsonObjectToString(Value& doc)
{
	StringBuffer buffer;
	Writer<StringBuffer> w(buffer);
	doc.Accept(w);
	return buffer.GetString();
}

inline float
getJsonFloat(Value& doc, const char* name)
{
	if (doc.HasMember(name)) {
		if (doc[name].IsFloat()) {
			return doc[name].GetFloat();
		} else if (doc[name].IsInt()) {
			return static_cast<float>(doc[name].GetInt());
		} else if (doc[name].IsString()) {
			try {
				return std::stof(doc[name].GetString());
			} catch (...) {}
		}
	}
	return 0.F;
}

inline std::string
getJsonString(Value& doc, const char* name)
{
	if (doc.HasMember(name)) {
		if (doc[name].IsString()) {
			return doc[name].GetString();
		} else if (doc[name].IsFloat()) {
			return std::to_string(doc[name].GetFloat());
		} else if (doc[name].IsInt()) {
			return std::to_string(doc[name].GetInt());
		} else if (doc[name].IsInt64()) {
			return std::to_string(doc[name].GetInt64());
		} else {
			return jsonObjectToString(doc[name]);
		}
	}
	return "";
}

inline int
getJsonInt(Value& doc, const char* name)
{
	if (doc.HasMember(name)) {
		if (doc[name].IsInt()) {
			return doc[name].GetInt();
		} else if (doc[name].IsFloat()) {
			return static_cast<int>(doc[name].GetFloat());
		} else if (doc[name].IsString()) {
			try {
				return std::stoi(doc[name].GetString());
			} catch (...) {}
		}
	}
	return 0;
}

inline int64_t
getJsonInt64(Value& doc, const char* name)
{
	if (doc.HasMember(name)) {
		if (doc[name].IsInt64()) {
			return doc[name].GetInt64();
		} else if (doc[name].IsFloat()) {
			return static_cast<int64_t>(doc[name].GetFloat());
		} else if (doc[name].IsString()) {
			try {
				return std::stol(doc[name].GetString());
			} catch (...) {}
		}
	}
	return 0;
}

inline bool
getJsonBool(Value& doc, const char* name)
{
	if (doc.HasMember(name)) {
		if (doc[name].IsBool()) {
			return doc[name].GetBool();
		} else if (doc[name].IsInt()) {
			return static_cast<bool>(doc[name].GetInt());
		} else if (doc[name].IsString()) {
			try {
				return EqualsNoCase(doc[name].GetString(), "true");
			} catch (...) {}
		}
	}
	return false;
}

inline bool
parseJson(Document& d, HTTPRequest& req, const char* reqName) {
	if (d.Parse(req.result.c_str()).HasParseError()) {
		Locator::getLogger()->error("{} Parse Error: status {} | response: {}",
									reqName,
									req.response_code,
									req.result);
		return true;
	}
	return false;
}

inline Value
stringToVal(const std::string& str,
	Document::AllocatorType& allocator,
	std::string defaultVal = "") {
	Value v;
	if (str.empty()) {
		v.SetString(defaultVal.c_str(), allocator);
	} else {
		v.SetString(str.c_str(), allocator);
	}
	return v;
}

std::string
UrlEncode(const std::string& str)
{
	char* escaped = curl_easy_escape(nullptr, str.data(), str.length());
	std::string ret(escaped);
	curl_free(escaped);
	return ret;
}

inline std::string
encodeDownloadUrl(const std::string& url)
{
	auto last_slash = url.find_last_of('/');
	auto base_url = url.substr(0, last_slash + 1);
	auto filename = url.substr(last_slash + 1);
	int outlength = 0;
	char* unescaped_c_char_filename = curl_easy_unescape(
	  nullptr, filename.c_str(), filename.length(), &outlength);
	std::string unescaped_filename(unescaped_c_char_filename, outlength);
	curl_free(unescaped_c_char_filename);
	return base_url + UrlEncode(unescaped_filename);
}

inline std::unordered_map<std::string, std::string>
extractHeaderMap(const std::string& headersStr) {
	std::vector<std::string> lines{};
	split(headersStr, "\r\n", lines);
	std::unordered_map<std::string, std::string> o{};
	for (auto& x : lines) {
		std::vector<std::string> leftright{};
		split(x, ": ", leftright);
		if (leftright.size() >= 2) {
			o.emplace(leftright.at(0), leftright.at(1));
		}
		if (leftright.size() != 2) {
			Locator::getLogger()->debug(
			  "Header String line in unexpected form: {}", x);
		}
	}
	return o;
	
}

DownloadManager::DownloadManager()
{
	EmptyTempDLFileDir();
	curl_global_init(CURL_GLOBAL_ALL);
	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "DLMAN");
		PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

DownloadManager::~DownloadManager()
{
	QUIT_OTHER_THREADS_FLAG.store(true);
	THREAD_EXIT_COUNT.Wait();
	THREAD_EXIT_COUNT.Wait();
	EmptyTempDLFileDir();
	if (LoggedIn())
		Logout();
	curl_global_cleanup();

	// for completeness, unregister with LUA
	// this is usually triggered at shutdown
	// technically both a leak and not a leak
	if (LUA != nullptr) {
		LUA->UnsetGlobal("DLMAN");
	}
}

enum class RequestResultStatus
{
	Done = 0,
	Failed = 1,
};
struct RequestResult
{
	RequestResultStatus status;
	long response_code;
};
std::mutex G_MTX_HTTP_REQS;
std::vector<CURL*> G_HTTP_REQS;
std::mutex G_MTX_HTTP_RESULT_HANDLES;
std::vector<std::pair<CURL*, RequestResult>> G_HTTP_RESULT_HANDLES;
void AddHttpRequestHandle(CURL* handle) {
	const std::lock_guard<std::mutex> lock(G_MTX_HTTP_REQS);
	G_HTTP_REQS.push_back(handle);
}
std::mutex G_MTX_PACK_REQS;
std::vector<CURL*> G_PACK_REQS;
std::mutex G_MTX_PACK_RESULT_HANDLES;
std::vector<std::pair<CURL*, RequestResult>> G_PACK_RESULT_HANDLES;
void
AddPackDlRequestHandle(CURL* handle)
{
	const std::lock_guard<std::mutex> lock(G_MTX_PACK_REQS);
	G_PACK_REQS.push_back(handle);
}
void
DownloadManager::Init()
{
	RefreshLastVersion();
	RefreshPackTags();
	initialized = true;
	pack_multi_handle = nullptr;
	std::thread([this]() {
		size_t maxDLSpeed = maxDLPerSecond;
		pack_multi_handle = curl_multi_init();
		int HandlesRunning = 0;
		std::vector<CURL*> local_http_reqs_tmp;
		std::vector<CURL*> local_http_reqs;
		std::vector<std::pair<CURL*, RequestResult>> result_handles;
		bool handle_count_changed = false;
		while (true) {
			if (QUIT_OTHER_THREADS_FLAG.load()) break;
			{
				const std::lock_guard<std::mutex> lock(G_MTX_PACK_REQS);
				G_PACK_REQS.swap(local_http_reqs_tmp);
			}
			handle_count_changed =
			  handle_count_changed ||
			  !local_http_reqs_tmp.empty();
			for (auto& curl_handle : local_http_reqs_tmp) {
				curl_multi_add_handle(pack_multi_handle, curl_handle);
			}
			// Add all elements from local_http_reqs_tmp to local_http_reqs
			local_http_reqs.insert(
			  local_http_reqs.end(),
					  std::make_move_iterator(local_http_reqs_tmp.begin()),
					  std::make_move_iterator(local_http_reqs_tmp.end()));
			local_http_reqs_tmp.clear();
			if (handle_count_changed && maxDLSpeed != 0) {
				for (auto& handle : local_http_reqs)
					curl_easy_setopt_log_err(
					  handle,
					  CURLOPT_MAX_RECV_SPEED_LARGE,
									 static_cast<curl_off_t>(
									   maxDLSpeed / local_http_reqs.size()));
			}
			handle_count_changed = false;

			CURLMcode mc =
			  curl_multi_perform(pack_multi_handle, &HandlesRunning);
			if (HandlesRunning == 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			else if (!mc)
				mc = curl_multi_poll(pack_multi_handle, NULL, 0, 10, NULL);

			// Check for finished downloads
			CURLMsg* msg;
			int msgs_left;
			while (
			  (msg = curl_multi_info_read(pack_multi_handle, &msgs_left))) {
				RequestResult res = {};
				res.status =
				  msg->data.result != CURLE_PARTIAL_FILE &&
					  msg->msg == CURLMSG_DONE
					? RequestResultStatus::Done
					: RequestResultStatus::Failed;
				curl_easy_getinfo(
				  msg->easy_handle, CURLINFO_RESPONSE_CODE, &res.response_code);
				result_handles.push_back(
				  std::make_pair(msg->easy_handle, res));
				curl_multi_remove_handle(pack_multi_handle, msg->easy_handle);
				curl_easy_cleanup(msg->easy_handle);
			}
			if (!result_handles.empty()) {
				std::ignore = std::remove_if(
				  local_http_reqs.begin(),
				  local_http_reqs.end(),
				  [result_handles](CURL* x) {
					  return std::find_if(result_handles.begin(),
										  result_handles.end(),
										  [x](auto pair) {
											  return pair.first == x;
										  }) != result_handles.end();
				  });
				handle_count_changed = true;
				{
					const std::lock_guard<std::mutex> lock(
						G_MTX_PACK_RESULT_HANDLES);
					G_PACK_RESULT_HANDLES.insert(
					  G_PACK_RESULT_HANDLES.end(),
						std::make_move_iterator(result_handles.begin()),
						std::make_move_iterator(result_handles.end()));
				}
				result_handles.clear();
			}
		}
		curl_multi_cleanup(pack_multi_handle);
		THREAD_EXIT_COUNT.Post();
	}).detach();

	http_req_handle = nullptr;
	std::thread([this]() {
		http_req_handle = curl_multi_init();
		int HandlesRunning = 0;
		std::vector<CURL*> local_http_reqs;
		std::vector<std::pair<CURL*, RequestResult>> result_handles;
		while (true) {
			if (QUIT_OTHER_THREADS_FLAG.load())
				break;
			{
				const std::lock_guard<std::mutex> lock(G_MTX_HTTP_REQS);
				G_HTTP_REQS.swap(local_http_reqs);

				auto now = std::chrono::steady_clock::now();
				for (auto& pair : endpointRatelimitTimestamps) {
					if (now > pair.second &&
						ratelimitedRequestQueue.count(pair.first) != 0u) {
						// no longer ratelimited, handle the queue
						auto& reqs = ratelimitedRequestQueue.at(pair.first);
						for (auto& req : reqs) {
							G_HTTP_REQS.push_back(req->handle);
							HTTPRequests.push_back(req);
						}
						ratelimitedRequestQueue.erase(pair.first);
					}
				}
			}
			for (auto& curl_handle : local_http_reqs) {
				curl_multi_add_handle(http_req_handle, curl_handle);
			}
			local_http_reqs.clear();

			CURLMcode mc = curl_multi_perform(http_req_handle, &HandlesRunning);
			if (HandlesRunning == 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			else if (!mc)
				mc = curl_multi_poll(http_req_handle, NULL, 0, 10, NULL);

			// Check for finished http requests
			CURLMsg* msg;
			int msgs_left;
			while ((msg = curl_multi_info_read(http_req_handle, &msgs_left))) {
				RequestResult res = {};
				res.status = msg->data.result != CURLE_UNSUPPORTED_PROTOCOL &&
								 msg->msg == CURLMSG_DONE
							   ? RequestResultStatus::Done
							   : RequestResultStatus::Failed;
				curl_easy_getinfo(
				  msg->easy_handle, CURLINFO_RESPONSE_CODE, &res.response_code);
				result_handles.push_back(std::make_pair(msg->easy_handle, res));
				curl_multi_remove_handle(http_req_handle, msg->easy_handle);
				curl_easy_cleanup(msg->easy_handle);
			}
			if (!result_handles.empty()) {
				{
					const std::lock_guard<std::mutex> lock(
					  G_MTX_HTTP_RESULT_HANDLES);
					G_HTTP_RESULT_HANDLES.insert(
					  G_HTTP_RESULT_HANDLES.end(),
					  std::make_move_iterator(result_handles.begin()),
					  std::make_move_iterator(result_handles.end()));
				}
				result_handles.clear();
			}
		}
		curl_multi_cleanup(http_req_handle);
		THREAD_EXIT_COUNT.Post();
	}).detach();
}
void
DownloadManager::Update(float fDeltaSeconds)
{
	if (!initialized)
		Init();
	if (gameplay)
		return;
	UpdatePacks(fDeltaSeconds);
	UpdateHTTP(fDeltaSeconds);
}
void
DownloadManager::UpdateHTTP(float fDeltaSeconds)
{
	if (HTTPRequests.empty() || gameplay)
		return;

	static std::vector<std::pair<CURL*, RequestResult>> result_handles;
	{
		const std::lock_guard<std::mutex> lock(G_MTX_HTTP_RESULT_HANDLES);
		G_HTTP_RESULT_HANDLES.swap(result_handles);
	}
	for (auto& pair : result_handles) {
		CURL* handle = pair.first;
		RequestResultStatus res = pair.second.status;

		/* Find out which handle this message is about */
		int idx_to_delete = -1;
		for (size_t i = 0; i < HTTPRequests.size(); ++i) {
			if (handle == HTTPRequests[i]->handle) {
				// The CURL handle is freed by the other thread, for easier debugging set it to null
				HTTPRequests[i]->handle = nullptr;
				HTTPRequests[i]->response_code = pair.second.response_code;

				switch (res) {
					case RequestResultStatus::Done:
						HTTPRequests[i]->Done(*(HTTPRequests[i]));
						break;
					case RequestResultStatus::Failed:
						HTTPRequests[i]->Failed(*(HTTPRequests[i]));
						break;

					default:
						Locator::getLogger()->warn(
						  "Unknown RequestResultStatus (This indicates a bug)");
				}

				if (HTTPRequests[i]->form != nullptr)
					curl_formfree(HTTPRequests[i]->form);
				HTTPRequests[i]->form = nullptr;
				delete HTTPRequests[i];
				idx_to_delete = i;
				break;
			}

		}
		// Delete this here instead of within the loop to avoid iterator
		// invalidation
		if (idx_to_delete != -1)
			HTTPRequests.erase(HTTPRequests.begin() + idx_to_delete);
	}
	result_handles.clear();
}
void
DownloadManager::UpdatePacks(float fDeltaSeconds)
{
	timeSinceLastDownload += fDeltaSeconds;
	for (auto& x : downloads) {
		/*if (x.second == nullptr) {
			Locator::getLogger()->warn("Pack download was null? URL: {}",
									   dl.first);
			continue;
		}*/
		x.second->Update(fDeltaSeconds);
	}
	if (!pendingInstallDownloads.empty() && !gameplay) {
		// Install all pending packs
		for (auto i = pendingInstallDownloads.begin();
			 i != pendingInstallDownloads.end();
			 i++) {
			i->second->Install();
			finishedDownloads[i->second->m_Url] = i->second;
		}
		pendingInstallDownloads.clear();
		// Reload
		auto screen = SCREENMAN->GetScreen(0);
		if (screen && screen->GetName() == "ScreenSelectMusic")
			static_cast<ScreenSelectMusic*>(screen)->DifferentialReload();
		else if (screen && screen->GetName() == "ScreenNetSelectMusic")
			static_cast<ScreenNetSelectMusic*>(screen)->DifferentialReload();
		else
			SONGMAN->DifferentialReload();
	}
	if (downloads.size() < maxPacksToDownloadAtOnce && !DownloadQueue.empty() &&
		timeSinceLastDownload > DownloadCooldownTime) {
		std::pair<DownloadablePack*, bool> pack = DownloadQueue.front();
		DownloadQueue.pop_front();
		DLMAN->DownloadAndInstallPack(pack.first, pack.second);
	}
	if (downloads.empty())
		return;

	static std::vector<std::pair<CURL*, RequestResult>> result_handles;
	{
		const std::lock_guard<std::mutex> lock(G_MTX_PACK_RESULT_HANDLES);
		G_PACK_RESULT_HANDLES.swap(result_handles);
	}
	bool finishedADownload = false;
	for (auto& pair : result_handles) {
		CURL* handle = pair.first;
		RequestResultStatus res = pair.second.status;

		/* Find out which handle this message is about */
		for (auto i = downloads.begin(); i != downloads.end(); i++) {
			auto& dl = i->second;
			if (handle == dl->handle) {
				if (dl->p_RFWrapper.stop) {
					dl->Failed();
					finishedDownloads[dl->m_Url] = dl;
				} else if (res == RequestResultStatus::Done) {
					timeSinceLastDownload = 0;
					pendingInstallDownloads[dl->m_Url] = dl;
				} else if (res == RequestResultStatus::Failed) {
					i->second->Failed();
					finishedDownloads[dl->m_Url] = dl;
				}
				finishedADownload = true;
				dl->p_RFWrapper.file.Flush();
				if (dl->p_RFWrapper.file.IsOpen())
					dl->p_RFWrapper.file.Close();
				dl->handle = nullptr;
				if (dl->p_Pack != nullptr)
					dl->p_Pack->downloading = false;
				downloads.erase(i);
				break;
			}
		}
	}
	result_handles.clear();

	if (finishedADownload && downloads.empty()) {
		MESSAGEMAN->Broadcast("AllDownloadsCompleted");
	}
}

void
DownloadManager::UpdateGameplayState(bool bGameplay)
{
	if (gameplay != bGameplay) {
		if (bGameplay)
			MESSAGEMAN->Broadcast("PausingDownloads");
		else
			MESSAGEMAN->Broadcast("ResumingDownloads");
	}
	gameplay = bGameplay;
}

std::shared_ptr<Download>
DownloadManager::DownloadAndInstallPack(DownloadablePack* pack, bool mirror)
{
	std::vector<std::string> packs;
	SONGMAN->GetSongGroupNames(packs);
	for (auto packName : packs) {
		if (packName == pack->name) {
			SCREENMAN->SystemMessage("Already have pack " + packName +
									 ", not downloading");
			return nullptr;
		}
	}
	if (downloads.size() >= maxPacksToDownloadAtOnce) {
		DownloadQueue.push_back(std::make_pair(pack, mirror));
		return nullptr;
	}
	std::string& url = mirror ? pack->mirror : pack->url;
	auto encoded_url = encodeDownloadUrl(url);

	auto dl = DownloadAndInstallPack(encoded_url, pack->name + ".zip");
	dl->p_Pack = pack;
	dl->p_Pack->downloading = true;
	return dl;
}

std::shared_ptr<Download>
DownloadManager::DownloadAndInstallPack(const std::string& url,
										std::string filename)
{
	auto dl = std::make_shared<Download>(url, filename);

	AddPackDlRequestHandle(dl->handle);
	downloads[url] = dl;

	SCREENMAN->SystemMessage(dl->StartMessage());

	return dl;
}

HTTPRequest*
DownloadManager::SendRequest(
  std::string requestName,
  std::vector<std::pair<std::string, std::string>> params,
  std::function<void(HTTPRequest&)> done,
  bool requireLogin,
  RequestMethod httpMethod,
  bool compressed,
  bool async,
  bool withBearer)
{
	return SendRequest(requestName,
					   requestName,
					   params,
					   done,
					   requireLogin,
					   httpMethod,
					   compressed,
					   async,
					   withBearer);
}

HTTPRequest*
DownloadManager::SendRequest(
  std::string requestName,
  std::string apiPath,
  std::vector<std::pair<std::string, std::string>> params,
  std::function<void(HTTPRequest&)> done,
  bool requireLogin,
  RequestMethod httpMethod,
  bool compressed,
  bool async,
  bool withBearer)
{
	return SendRequestToURL(APIROOT() + requestName,
							apiPath,
							params,
							done,
							requireLogin,
							httpMethod,
							compressed,
							async,
							withBearer);
}

HTTPRequest*
DownloadManager::SendRequestToURL(
  std::string url,
  std::string apiPath,
  std::vector<std::pair<std::string, std::string>> params,
  std::function<void(HTTPRequest&)> afterDone,
  bool requireLogin,
  RequestMethod httpMethod,
  bool compressed,
  bool async,
  bool withBearer)
{
	if (requireLogin && !LoggedIn())
		return nullptr;
	if (httpMethod == RequestMethod::GET && !params.empty()) {
		url += "?";
		for (auto& param : params)
			url += param.first + "=" + param.second + "&";
		url = url.substr(0, url.length() - 1);
	}
	auto done = [url,
				 params,
				 afterDone,
				 requireLogin,
				 httpMethod,
				 async,
				 withBearer,
				 this](auto& req) {
		if (afterDone)
			afterDone(req);
	};
	CURL* curlHandle = initCURLHandle(withBearer, false, compressed);
	SetCURLURL(curlHandle, url);
	HTTPRequest* req;
	if (httpMethod == RequestMethod::POST) {
		curl_httppost* form = nullptr;
		curl_httppost* lastPtr = nullptr;
		for (auto& param : params)
			CURLFormPostField(
			  form, lastPtr, param.first.c_str(), param.second.c_str());
		curl_easy_setopt_log_err(curlHandle, CURLOPT_HTTPPOST, form);
		req = new HTTPRequest(curlHandle, done, form);
	} else if (httpMethod == RequestMethod::PATCH) {
		// THIS PROBABLY DOESNT WORK
		// USE AT YOUR OWN RISK
		// PROBABLY DO IT ANOTHER WAY
		curl_httppost* form = nullptr;
		curl_httppost* lastPtr = nullptr;
		for (auto& param : params)
			CURLFormPostField(
			  form, lastPtr, param.first.c_str(), param.second.c_str());
		curl_easy_setopt_log_err(curlHandle, CURLOPT_HTTPPOST, form);
		curl_easy_setopt_log_err(curlHandle, CURLOPT_CUSTOMREQUEST, "PATCH");
		req = new HTTPRequest(curlHandle, done, form);
	} else if (httpMethod == RequestMethod::DEL) {
		req = new HTTPRequest(curlHandle, done);
		curl_easy_setopt_log_err(curlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
	} else {
		req = new HTTPRequest(curlHandle, done);
		curl_easy_setopt_log_err(curlHandle, CURLOPT_HTTPGET, 1L);
	}
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (async) {
		if (!QueueRequestIfRatelimited(apiPath, *req)) {
			AddHttpRequestHandle(req->handle);
			HTTPRequests.push_back(req);
		}
	} else {
		Locator::getLogger()->debug("Waiting on synchronous curl call");
		CURLcode res = curl_easy_perform(req->handle);
		curl_easy_cleanup(req->handle);
		done(*req);
		Locator::getLogger()->debug("Synchronous curl call completed");
		delete req;
		return nullptr;
	}
	return req;
}

bool
DownloadManager::HandleRatelimitResponse(const std::string& endpoint,
										 HTTPRequest& req)
{
	const auto& status = req.response_code;
	if (status != 429) {
		return false;
	}

	const auto ratelimitHeader = "Retry-After";
	auto headers = extractHeaderMap(req.headers);

	auto waitSeconds = 60;
	if (headers.count(ratelimitHeader) != 0u) {
		try {
			waitSeconds = std::stoi(headers.at(ratelimitHeader));
		} catch (...) {
			Locator::getLogger()->warn("Failed to turn Retry-After from string "
									   "to number, defaulted to 60 - Value: {}",
									   headers.at(ratelimitHeader));
		}
	}

	Locator::getLogger()->warn("{} {} - Rate Limited. Requeueing request to "
							   "the endpoint after {} seconds",
							   endpoint,
							   status,
							   waitSeconds);

	auto newTimestamp =
	  std::chrono::steady_clock::now() + std::chrono::seconds(waitSeconds);

	{
		const std::lock_guard<std::mutex> lock(G_MTX_HTTP_REQS);
		endpointRatelimitTimestamps[endpoint] = newTimestamp;
	}

	// the calling function needs to call QueueRatelimitedRequest next
	// but with a new copy of the request

	return true;
}

bool
DownloadManager::QueueRequestIfRatelimited(const std::string& endpoint,
										   HTTPRequest& req)
{
	{
		const std::lock_guard<std::mutex> lock(G_MTX_HTTP_REQS);
		if (endpointRatelimitTimestamps.count(endpoint) == 0u) {
			return false;
		}

		auto ratelimitTimestamp = endpointRatelimitTimestamps.at(endpoint);
		if (std::chrono::steady_clock::now() > ratelimitTimestamp) {
			endpointRatelimitTimestamps.erase(endpoint);
			return false;
		}
	}

	QueueRatelimitedRequest(endpoint, req);

	return true;
}

void
DownloadManager::QueueRatelimitedRequest(const std::string& endpoint,
										 HTTPRequest& req)
{
	{
		const std::lock_guard<std::mutex> lock(G_MTX_HTTP_REQS);
		if (ratelimitedRequestQueue.count(endpoint) == 0u) {
			ratelimitedRequestQueue.emplace(endpoint,
											std::vector<HTTPRequest*>());
		}
		ratelimitedRequestQueue.at(endpoint).push_back(&req);
	}
}

bool
DownloadManager::HandleAuthErrorResponse(const std::string& endpoint,
										 HTTPRequest& req)
{
	const auto& status = req.response_code;
	if (status != 401 && status != 403) {
		return false;
	}

	if (status == 403) {
		Locator::getLogger()->warn(
		  "{} {} - Auth Error. You are banned", endpoint, status);
	} else {
		Locator::getLogger()->warn(
		  "{} {} - Auth Error. Logging out automatically", endpoint, status);
	}

	LogoutIfLoggedIn();

	return true;
}

bool
DownloadManager::Handle401And429Response(const std::string& endpoint,
										 HTTPRequest& req,
										 std::function<void()> if401,
										 std::function<void()> if429)
{
	if (HandleAuthErrorResponse(endpoint, req)) {
		if (if401)
			if401();
		return true;
	}
	if (HandleRatelimitResponse(endpoint, req)) {
		if (if429)
			if429();
		return true;
	}
	return false;
}

bool
DownloadManager::LoggedIn()
{
	return !authToken.empty();
}

void
DownloadManager::OnLogin()
{
	Locator::getLogger()->info("Successful login as {}", sessionUser);
	RefreshUserData();
	RefreshFavorites();
	RefreshGoals();
	LoadPlaylists();
	FOREACH_ENUM(Skillset, ss)
	RequestTop25(ss);
	if (ShouldUploadScores()) {
		InitialScoreSync();

		// ok we don't actually want to delete this yet since this is
		// specifically for appending replaydata for a score the site does
		// not have data for without altering the score entry in any other
		// way, but keep disabled for now
		// UpdateOnlineScoreReplayData();
	}
	if (GAMESTATE->m_pCurSteps != nullptr)
		GetChartLeaderboard(GAMESTATE->m_pCurSteps->GetChartKey());
	MESSAGEMAN->Broadcast("Login");
	loggingIn = false;
}

void
DownloadManager::LoginWithToken(const std::string& sessionToken,
								std::function<void(bool)> done)
{
	authToken = sessionToken;
	try {
		auto jwt = jwt::decode(authToken);
		auto name = jwt.get_payload_claim("username");
		sessionUser = name.as_string();
	} catch (...) {
		Locator::getLogger()->error(
		  "Failed to get username from JWT. Set username to 'error'");
		sessionUser = "error";
	}
	OnLogin();
}

void
DownloadManager::LoginRequest(const std::string& user,
							  const std::string& pass,
							  std::function<void(bool loggedIn)> callback)
{
	if (loggingIn || user.empty()) {
		return;
	}

	Locator::getLogger()->info("Creating Login request");

	loggingIn = true;
	LogoutIfLoggedIn();
	CURL* curlHandle = initCURLHandle(false, false, DONT_COMPRESS);
	CURLAPIURL(curlHandle, API_LOGIN);

	curl_easy_setopt_log_err(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt_log_err(
	  curlHandle, CURLOPT_COOKIEFILE, ""); /* start cookie engine */

	curl_httppost* form = nullptr;
	curl_httppost* lastPtr = nullptr;
	CURLFormPostField(form, lastPtr, "email", user.c_str());
	CURLFormPostField(form, lastPtr, "password", pass.c_str());
	CURLFormPostField(form, lastPtr, "key", API_KEY.c_str());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_HTTPPOST, form);

	auto done = [user, pass, callback, this](auto& req) {
		auto loginFailed = [this](std::string reason) {
			authToken = sessionUser = "";
			Message msg("LoginFailed");
			msg.SetParam("why", reason);
			MESSAGEMAN->Broadcast(msg);
			loggingIn = false;
		};

		if (HandleRatelimitResponse(API_LOGIN, req)) {
			LoginRequest(user, pass, callback);
			return;
		}

		Document d;
		// return true if parse failed
		auto parse = [&d, &req]() { return parseJson(d, req, "LoginRequest"); };

		const auto& response = req.response_code;
		if (response == 422) {
			// bad input fields
			parse();

			Locator::getLogger()->error(
			  "Status 422 on LoginRequest. Errors: {}", jsonObjectToString(d));
			if (jsonObjectToString(d).find("out of date") !=
				std::string::npos) {
				loginFailed("Your client is out of date.");
			} else {
				loginFailed("Missing email or password, or email not verified.");
				}
		} else if (response == 404) {
			// user doesnt exist?
			parse();

			Locator::getLogger()->error(
			  "Status 404 on LoginRequest. User may not exist. Errors: {}",
			  jsonObjectToString(d));
			loginFailed("User may not exist.");

		} else if (response == 401) {
			// bad password
			parse();

			Locator::getLogger()->error(
			  "Status 401 on LoginRequest. Bad password? Errors: {}",
			  jsonObjectToString(d));
			loginFailed("Password may be wrong.");

		} else if (response == 403) {
			// user is banned
			parse();

			Locator::getLogger()->error(
			  "Status 403 on LoginRequest. User is forbidden. Errors: {}",
			  jsonObjectToString(d));
			loginFailed("User is banned.");

		} else if (response == 200) {
			// all good
			if (parse()) {
				Locator::getLogger()->warn(
				  "Due to LoginRequest parse error, login failed");
				loginFailed("Unknown error");
				return;
			}

			if (d.HasMember("access_token") &&
				d["access_token"].IsString()) {

				// JWT acquired
				authToken = d["access_token"].GetString();

				try {
					auto jwt = jwt::decode(authToken);
					auto username = jwt.get_payload_claim("username");
					sessionUser = username.as_string();
				} catch (...) {
					Locator::getLogger()->error("Failed to get username from "
												"JWT. Set username to 'error'");
					sessionUser = "error";
				}
				OnLogin();
				callback(LoggedIn());

			} else {
				Locator::getLogger()->error(
				  "Missing access token in LoginRequest response. Content: "
				  "{}",
				  jsonObjectToString(d));
				loginFailed("Missing token in login response from server.");
			}
		} else {
			// ???
			parse();

			Locator::getLogger()->warn(
			  "Unexpected response code {} on LoginRequest. Content: {}",
			  response,
			  jsonObjectToString(d));
			loginFailed("Unknown status");
		}
	};
	HTTPRequest* req = new HTTPRequest(curlHandle, done, form);
	req->Failed = [this](auto& req) {
		authToken = sessionUser = "";
		Message msg("LoginFailed");
		msg.SetParam("why", std::string("Request failed"));
		MESSAGEMAN->Broadcast(msg);
		loggingIn = false;
	};
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(API_LOGIN, *req)) {
		AddHttpRequestHandle(req->handle);
		HTTPRequests.push_back(req);
	}
	Locator::getLogger()->info("Finished creating Login request");
}

void
DownloadManager::Logout()
{
	const std::lock_guard<std::mutex> lock(G_MTX_SCORE_UPLOAD);

	// cancel sequential upload
	ScoreUploadSequentialQueue.clear();
	sequentialScoreUploadTotalWorkload = 0;
	runningSequentialScoreUpload = false;

	// same for goals
	GoalUploadSequentialQueue.clear();
	sequentialGoalUploadTotalWorkload = 0;
	runningSequentialGoalUpload = false;

	// same for favorites
	FavoriteUploadSequentialQueue.clear();
	sequentialFavoriteUploadTotalWorkload = 0;
	runningSequentialFavoriteUpload = false;

	// logout
	sessionUser = authToken = "";
	topScores.clear();
	sessionRatings.clear();
	// This is called on a shutdown, after MessageManager is gone
	if (MESSAGEMAN != nullptr)
		MESSAGEMAN->Broadcast("LogOut");
}

inline std::string
FavoriteVectorToJSON(std::vector<std::string>& v)
{

	Document d;
	Document::AllocatorType& allocator = d.GetAllocator();
	d.SetObject();

	Value arrDoc(kArrayType);
	for (auto& fave : v) {
		// lmao
		Document ckElement;
		ckElement.SetObject();
		Value v;
		v.SetString(fave.c_str(), allocator);
		ckElement.AddMember("key", v, allocator);
		arrDoc.PushBack(ckElement, allocator);
	}
	d.AddMember("data", arrDoc, allocator);

	StringBuffer buffer;
	Writer<StringBuffer> w(buffer);
	d.Accept(w);
	return buffer.GetString();
}

void
DownloadManager::AddFavoriteRequest(const std::string& chartkey)
{
	constexpr auto& CALL_ENDPOINT = API_FAVORITES;
	constexpr auto& CALL_PATH = API_FAVORITES;

	Locator::getLogger()->info("Generating AddFavoriteRequest for {}",
							   chartkey);

	auto done = [chartkey, &CALL_ENDPOINT, this](auto& req) {
		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [chartkey, this]() { AddFavoriteRequest(chartkey); })) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() { return parseJson(d, req, "AddFavorite"); };

		const auto& response = req.response_code;
		if (response == 200) {
			// all good
			Locator::getLogger()->info(
			  "AddFavorite successfully added favorite {} to online profile",
			  chartkey);

		} else if (response == 422) {
			// missing info
			parse();

			Locator::getLogger()->warn(
			  "AddFavorite for {} failed due to input error. Content: {}",
			  chartkey,
			  jsonObjectToString(d));
		} else if (response == 404) {
			parse();

			Locator::getLogger()->warn("AddFavorite for {} failed due to 404. "
									   "Chart may be unranked - Content: {}",
									   chartkey,
									   jsonObjectToString(d));
		} else {
			// ???
			parse();

			Locator::getLogger()->warn(
			  "AddFavorite for {} - unknown response {} - Content: {}",
			  chartkey,
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(CALL_PATH,
				CALL_ENDPOINT,
				{ make_pair("key", UrlEncode(chartkey)) },
				done,
				true,
				RequestMethod::POST);
	Locator::getLogger()->info("Finished creating AddFavorite request for {}",
							   chartkey);
}

void
DownloadManager::BulkAddFavorites(std::vector<std::string> favorites,
	std::function<void()> callback)
{
	constexpr auto& CALL_ENDPOINT = API_FAVORITES_BULK;
	constexpr auto& CALL_PATH = API_FAVORITES_BULK;

	Locator::getLogger()->info("Creating BulkAddGoals request for {} favorites",
		favorites.size());

	if (!LoggedIn()) {
		Locator::getLogger()->warn(
			"Attempted to upload favorites while not logged in. Aborting");
		if (callback)
			callback();
		return;
	}

	CURL* curlHandle = initCURLHandle(true, true, DONT_COMPRESS);
	CURLAPIURL(curlHandle, CALL_PATH);

	auto body = FavoriteVectorToJSON(favorites);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POSTFIELDSIZE, body.length());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_COPYPOSTFIELDS, body.c_str());

	auto done = [callback, favorites, &CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  [callback]() {
				  if (callback)
					  callback();
			  },
			  [favorites, callback, this]() {
				  BulkAddFavorites(favorites, callback);
			  })) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "BulkAddFavorites");
		};

		const auto& response = req.response_code;
		if (response == 200 || response == 207) {
			// mostly good

			if (parse()) {
				if (callback)
					callback();
				return;
			}

			if (d.IsObject() && d.HasMember("failed")) {

				if (d["failed"].IsArray() && d["failed"].Size() > 0) {
					Locator::getLogger()->warn("BulkAddFavorites had {} failed "
											   "uploads out of {}",
											   d["failed"].Size(),
											   favorites.size());
				}

				if (d.HasMember("success") && d["success"].IsArray()) {
					auto& successes = d["success"];
					for (auto it = successes.Begin(); it != successes.End();
						 it++) {
						auto obj = it->GetObj();
						Locator::getLogger()->info(
						  "Favorite {} was accepted by server",
						  obj["key"].GetString());
					}
				}

				if (d["failed"].IsArray()) {
					auto& fails = d["failed"];
					for (auto it = fails.Begin(); it != fails.End(); it++) {
						auto obj = it->GetObj();
						Locator::getLogger()->info(
						  "Favorite {} was rejected by server for: {}",
						  obj["key"].GetString(),
						  jsonObjectToString(obj["errors"].GetArray()));
					}
				}
			} else if (d.IsObject() && d.HasMember("message")) {
				Locator::getLogger()->info(
				  "BulkAddFavorites added all {} favorites successfully", favorites.size());
			} else {
				Locator::getLogger()->warn(
				  "BulkAddFavorites had a successful response status but an "
				  "unexpected response body: {}",
				  jsonObjectToString(d));
			}

		} else if (response == 422) {
			// some validation issue with the request
			parse();

			Locator::getLogger()->warn(
			  "BulkAddFavorites for {} favorites failed due to validation error: {}",
			  favorites.size(),
			  jsonObjectToString(d));

		} else {
			// ???
			parse();

			Locator::getLogger()->warn(
			  "BulkAddFavorites got unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
		if (callback)
			callback();
	};

	HTTPRequest* req =
	  new HTTPRequest(curlHandle, done, nullptr, [callback](auto& req) {
		  if (callback)
			  callback();
	  });
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(CALL_ENDPOINT, *req)) {
		AddHttpRequestHandle(req->handle);
		HTTPRequests.push_back(req);
	}
	Locator::getLogger()->info(
	  "Finished creating BulkAddFavorites request for {} favorites", favorites.size());
}

void
DownloadManager::RemoveFavoriteRequest(const std::string& chartkey)
{
	constexpr auto& CALL_ENDPOINT = API_FAVORITES;
	const auto CALL_PATH = API_FAVORITES + "/" + URLEncode(chartkey);

	Locator::getLogger()->info("Generating RemoveFavoriteRequest for {}",
							   chartkey);

	auto done = [chartkey, &CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [chartkey, this]() {
				RemoveFavoriteRequest(chartkey);
			})) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "RemoveFavorite");
		};

		const auto& response = req.response_code;
		if (response == 200) {
			// all good
			Locator::getLogger()->info("RemoveFavorite successfully removed "
									   "favorite {} from online profile",
									   chartkey);

		} else if (response == 404) {
			parse();

			Locator::getLogger()->warn(
			  "RemoveFavorite for {} failed due to 404. Favorite may be "
			  "missing - Content: {}",
			  chartkey,
			  jsonObjectToString(d));
		} else {
			// ???
			parse();

			Locator::getLogger()->warn(
			  "RemoveFavorite for {} unknown response {} - Content: {}",
			  chartkey,
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(CALL_PATH,
				CALL_ENDPOINT,
				{},
				done,
				true,
				RequestMethod::DEL);
	Locator::getLogger()->info(
	  "Finished creating RemoveFavorite request for {}", chartkey);
}

void
DownloadManager::GetFavoritesRequest(std::function<void(std::set<std::string>)> onSuccess,
									const DateTime start,
									const DateTime end)
{
	constexpr auto& CALL_ENDPOINT = API_FAVORITES;
	constexpr auto& CALL_PATH = API_FAVORITES;

	std::string startstr = fmt::format(
	  "{}-{}-{}", start.tm_year + 1900, start.tm_mon + 1, start.tm_mday);
	std::string endstr =
	  fmt::format("{}-{}-{}", end.tm_year + 1900, end.tm_mon + 1, end.tm_mday);
	Locator::getLogger()->info(
	  "Generating GetFavoritesRequest {} to {}", startstr, endstr);

	std::vector<std::pair<std::string, std::string>> params = {
		std::make_pair("start", startstr),
		std::make_pair("end", endstr),
	};

	auto done = [onSuccess, start, end, &CALL_ENDPOINT, this, startstr, endstr](
				  auto& req) {

		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [onSuccess, start, end, this]() {
				  GetFavoritesRequest(onSuccess, start, end);
			  })) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "GetFavorites");
		};

		const auto& response = req.response_code;
		if (response == 200) {
			// all good
			if (parse())
				return;

			if (d.HasMember("data") && d["data"].IsArray()) {
				auto& data = d["data"];

				std::set<std::string> onlineFavorites{};
				for (auto it = data.Begin(); it != data.End(); it++) {
					onlineFavorites.insert(it->GetString());
				}

				Locator::getLogger()->info(
				  "GetFavoritesRequest returned successfully. Found {} online "
				  "favorites",
				  onlineFavorites.size());
				onSuccess(onlineFavorites);

			} else {
				Locator::getLogger()->warn(
				  "GetFavoritesRequest status 200 got unexpected "
				  "response body - Content: {}",
				  jsonObjectToString(d));
			}
		} else {
			parse();
			Locator::getLogger()->warn(
			  "GetFavoritesRequest unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(CALL_PATH, CALL_ENDPOINT, params, done, true);
	Locator::getLogger()->info(
	  "Finished creating GetFavorites request for {} - {}", startstr, endstr);
}

void
uploadFavoritesSequentially()
{
	Message msg("FavoriteUploadProgress");
	msg.SetParam(
	  "percent",
	  1.f - (static_cast<float>(DLMAN->FavoriteUploadSequentialQueue.size()) /
			 static_cast<float>(DLMAN->sequentialFavoriteUploadTotalWorkload)));
	MESSAGEMAN->Broadcast(msg);

	if (!DLMAN->FavoriteUploadSequentialQueue.empty()) {

		std::vector<std::string> favoritesToUpload{};
		for (auto i = 0u; i < UPLOAD_FAVORITE_BULK_CHUNK_SIZE &&
						  !DLMAN->FavoriteUploadSequentialQueue.empty();
			 i++) {
			favoritesToUpload.push_back(
			  DLMAN->FavoriteUploadSequentialQueue.front());
			DLMAN->FavoriteUploadSequentialQueue.pop_front();

			if (favoritesToUpload.size() >= UPLOAD_FAVORITE_BULK_CHUNK_SIZE) {
				break;
			}
		}
		runningSequentialFavoriteUpload = true;
		DLMAN->BulkAddFavorites(favoritesToUpload, uploadFavoritesSequentially);

	} else {
		Locator::getLogger()->info(
		  "Sequential favorite upload queue empty - uploads finished");
		runningSequentialFavoriteUpload = false;
		DLMAN->sequentialFavoriteUploadTotalWorkload = 0;
		MESSAGEMAN->Broadcast("SequentialFavoriteUploadFinished");
	}
}

void
startSequentialFavoriteUpload()
{
	if (DLMAN->FavoriteUploadSequentialQueue.empty()) {
		Locator::getLogger()->info("Could not start sequential favorite upload "
								   "process - nothing to upload");
		return;
	}

	if (!runningSequentialFavoriteUpload) {
		Locator::getLogger()->info(
		  "Starting sequential favorite upload process - "
		  "{} favorites split into chunks of {}",
		  DLMAN->FavoriteUploadSequentialQueue.size(),
		  UPLOAD_FAVORITE_BULK_CHUNK_SIZE);
		uploadFavoritesSequentially();
	}
}

void
DownloadManager::RefreshFavorites(
  const DateTime start,
  const DateTime end)
{
	Locator::getLogger()->info(
	  "Refreshing Favorites - {} to {}", start.GetString(), end.GetString());

	auto onSuccess = [this](std::set<std::string> onlineFavorites) {
		auto* profile = PROFILEMAN->GetProfile(PLAYER_1);

		if (profile == nullptr) {
			Locator::getLogger()->warn("Profile for PLAYER_1 came back null. Favorites cannot be synced");
			return;
		}

		auto& localFavorites = profile->FavoritedCharts;
		std::set<std::string> toUpload{};

		// figure out what is missing online
		std::set_difference(localFavorites.begin(),
							localFavorites.end(),
							onlineFavorites.begin(),
							onlineFavorites.end(),
							std::inserter(toUpload, toUpload.begin()));

		// now save everything that was missing locally
		auto favoriteSavedCount = 0u;
		for (auto& favorite : onlineFavorites) {
			if (localFavorites.contains(favorite))
				continue;

			auto* song = SONGMAN->GetSongByChartkey(favorite);
			if (song != nullptr) {
				song->SetHasFavoritedChart(true);
			}
			auto* steps = SONGMAN->GetStepsByChartkey(favorite);
			if (steps != nullptr) {
				steps->SetFavorited(true);
			}

			profile->AddToFavorites(favorite);
			favoriteSavedCount++;
		}

		// update favorites in other data...
		if (favoriteSavedCount >= 1) {
			profile->allplaylists.erase("Favorites");
			SONGMAN->MakePlaylistFromFavorites(profile->FavoritedCharts,
											   profile->allplaylists);
			MESSAGEMAN->Broadcast(Message_FavoritesUpdated);
		}

		// logging here to occur before the AddFavorite logs
		Locator::getLogger()->info("Found {} online favorites to save locally, "
								   "and {} favorites to upload online",
								   favoriteSavedCount,
								   toUpload.size());

		// upload favorites
		if (toUpload.size() > 0) {
			FavoriteUploadSequentialQueue.insert(
			  FavoriteUploadSequentialQueue.end(), toUpload.begin(), toUpload.end());
			sequentialFavoriteUploadTotalWorkload += toUpload.size();
			startSequentialFavoriteUpload();
		}
	};
	GetFavoritesRequest(onSuccess, start, end);
}

inline Document
GoalToJSON(ScoreGoal* goal, Document::AllocatorType& allocator) {

	Document d;
	d.SetObject();

	if (goal == nullptr) {
		Locator::getLogger()->warn("Null ScoreGoal passed to GoalToJSON. Skipped");
		return d;
	}

	d.AddMember(
	  "key", stringToVal(URLEncode(goal->chartkey), allocator), allocator);
	d.AddMember(
	  "rate", stringToVal(std::to_string(goal->rate), allocator), allocator);
	d.AddMember(
	  "wife", stringToVal(std::to_string(goal->percent), allocator), allocator);
	d.AddMember("set_date",
				stringToVal(goal->timeassigned.GetString(), allocator),
				allocator);

	if (goal->achieved) {
		std::string timeAchievedString = "0000-00-00 00:00:00";
		timeAchievedString = goal->timeachieved.GetString();
		d.AddMember("achieved",
					stringToVal(std::to_string(goal->achieved), allocator),
					allocator);
		d.AddMember("achieved_date",
					stringToVal(timeAchievedString, allocator),
					allocator);
	}

	return d;
}

inline std::string
GoalVectorToJSON(std::vector<ScoreGoal*>& v) {

	Document d;
	Document::AllocatorType& allocator = d.GetAllocator();
	d.SetObject();

	Value arrDoc(kArrayType);
	for (auto& g : v) {
		arrDoc.PushBack(GoalToJSON(g, allocator), allocator);
	}
	d.AddMember("data", arrDoc, allocator);

	StringBuffer buffer;
	Writer<StringBuffer> w(buffer);
	d.Accept(w);
	return buffer.GetString();
}

void
DownloadManager::AddGoalRequest(ScoreGoal* goal)
{
	constexpr auto& CALL_ENDPOINT = API_GOALS;
	constexpr auto& CALL_PATH = API_GOALS;

	if (goal == nullptr) {
		Locator::getLogger()->warn(
		  "Null goal passed to AddGoalRequest. Skipped");
		return;
	}

	Locator::getLogger()->info("Generating AddGoalRequest for {}",
							   goal->DebugString());

	auto done = [goal, &CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [goal, this]() { AddGoalRequest(goal);
			})) {
			return;
		}

		Document d;
		// return true if parse failure
		auto parse = [&d, &req]() { return parseJson(d, req, "AddGoal"); };

		const auto& response = req.response_code;
		if (response == 200) {
			// all good
			Locator::getLogger()->info(
			  "AddGoal successfully added goal for {} to online profile",
			  goal->DebugString());

		} else if (response == 422) {
			// some validation issue with the request
			parse();

			Locator::getLogger()->warn(
			  "AddGoal for {} failed due to validation error: {}",
			  goal->DebugString(),
			  jsonObjectToString(d));
		} else if (response == 404) {
			parse();

			Locator::getLogger()->warn("AddGoal for {} failed due to 404. "
									   "Chart may be unranked - Content: {}",
									   goal->DebugString(),
									   jsonObjectToString(d));
		} else {
			parse();

			Locator::getLogger()->warn(
			  "AddGoal for {} unexpected response {} - Content: {}",
			  goal->DebugString(),
			  response,
			  jsonObjectToString(d));
		}
	};

	std::vector<std::pair<std::string, std::string>> postParams = {
		std::make_pair("key", UrlEncode(goal->chartkey)),
		std::make_pair("rate", std::to_string(goal->rate)),
		std::make_pair("wife", std::to_string(goal->percent)),
		std::make_pair("set_date", goal->timeassigned.GetString())
	};

	SendRequest(
	  CALL_PATH, CALL_ENDPOINT, postParams, done, true, RequestMethod::POST);
	Locator::getLogger()->info("Finished creating AddGoal request for {}",
							   goal->DebugString());
}

void
DownloadManager::BulkAddGoals(std::vector<ScoreGoal*> goals,
							  std::function<void()> callback)
{
	constexpr auto& CALL_ENDPOINT = API_GOALS_BULK;
	constexpr auto& CALL_PATH = API_GOALS_BULK;

	Locator::getLogger()->info("Creating BulkAddGoals request for {} goals",
							   goals.size());

	if (!LoggedIn()) {
		Locator::getLogger()->warn(
		  "Attempted to upload goals while not logged in. Aborting");
		if (callback)
			callback();
		return;
	}

	CURL* curlHandle = initCURLHandle(true, true, DONT_COMPRESS);
	CURLAPIURL(curlHandle, CALL_PATH);

	auto body = GoalVectorToJSON(goals);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POSTFIELDSIZE, body.length());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_COPYPOSTFIELDS, body.c_str());

	auto done = [callback, goals, &CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			CALL_ENDPOINT,
			req,
			[callback]() {
				if (callback)
					callback();
			},
			[goals, callback, this]() {
				BulkAddGoals(goals, callback);
			})) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req, &callback]() {
			if (parseJson(d, req, "BulkAddGoals")) {
				if (callback)
					callback();
				return true;
			}
			return false;
		};


		const auto& response = req.response_code;
		if (response == 200 || response == 207) {
			// mostly good

			if (parse())
				return;

			if (d.IsObject() && d.HasMember("failed")) {

				if (d["failed"].IsArray() && d["failed"].Size() > 0) {
					Locator::getLogger()->warn(
					  "BulkAddGoals had {} failed uploads out of {}",
					  d["failed"].Size(),
					  goals.size());
				}

				if (d.HasMember("success") && d["success"].IsArray()) {
					auto& successes = d["success"];
					for (auto it = successes.Begin(); it != successes.End(); it++) {
						auto obj = it->GetObj();
						Locator::getLogger()->info(
						  "Goal {} - {}x was accepted by server",
						  obj["key"].GetString(),
						  obj["rate"].GetString());
					}
				}

				if (d["failed"].IsArray()) {
					auto& fails = d["failed"];
					for (auto it = fails.Begin(); it != fails.End();
						 it++) {
						auto obj = it->GetObj();
						Locator::getLogger()->info(
						  "Goal {} - {}x was rejected by server for: {}",
						  obj["key"].GetString(),
						  obj["rate"].GetString(),
						  jsonObjectToString(obj["errors"].GetArray()));
					}
				}
			}
			else if (d.IsObject() && d.HasMember("message")) {
				Locator::getLogger()->info(
				  "BulkAddGoals added all {} goals successfully", goals.size());
			}
			else {
				Locator::getLogger()->warn(
				  "BulkAddGoals had a successful response status but an "
				  "unexpected response body: {}",
				  jsonObjectToString(d));
			}

		} else if (response == 422) {
			// some validation issue with the request
			parse();

			Locator::getLogger()->warn(
			  "BulkAddGoals for {} goals failed due to validation error: {}",
			  goals.size(),
			  jsonObjectToString(d));
		} else {
			// ???
			parse();

			Locator::getLogger()->warn(
			  "BulkAddGoals got unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
		if (callback)
			callback();
	};

	HTTPRequest* req =
	  new HTTPRequest(curlHandle, done, nullptr, [callback](auto& req) {
		  if (callback)
			  callback();
	  });
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(CALL_ENDPOINT, *req)) {
		AddHttpRequestHandle(req->handle);
		HTTPRequests.push_back(req);
	}
	Locator::getLogger()->info(
	  "Finished creating BulkAddGoals request for {} goals", goals.size());
}

void
DownloadManager::RemoveGoalRequest(ScoreGoal* goal)
{
	if (goal == nullptr) {
		Locator::getLogger()->warn("Null goal passed to RemoveGoalRequest. Skipped");
		return;
	}

	constexpr auto& CALL_ENDPOINT = API_GOALS;
	const auto CALL_PATH = API_GOALS + "/" + UrlEncode(goal->chartkey) + "/" +
						   std::to_string(goal->rate) + "/" +
						   std::to_string(goal->percent);

	Locator::getLogger()->info("Generating RemoveGoalRequest for {}",
							   goal->DebugString());

	auto done = [goal, &CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [goal, this]() { RemoveGoalRequest(goal);
			})) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() { return parseJson(d, req, "RemoveGoal"); };

		const auto& response = req.response_code;
		if (response == 200) {
			// all good
			Locator::getLogger()->info(
			  "RemoveGoal successfully removed goal for {} from online profile",
			  goal->DebugString());

		} else if (response == 422) {
			// some validation issue with the request
			parse();

			Locator::getLogger()->warn(
			  "RemoveGoal for {} failed due to validation error: {}",
			  goal->DebugString(),
			  jsonObjectToString(d));
		} else if (response == 404) {
			// chart unranked
			parse();

			Locator::getLogger()->warn("RemoveGoal for {} failed due to 404. "
									   "Chart may be unranked or Goal "
									   "missing - Content: {}",
									   goal->DebugString(),
									   jsonObjectToString(d));
		} else {
			parse();

			Locator::getLogger()->warn(
			  "RemoveGoal for {} unexpected response {} - Content: {}",
			  goal->DebugString(),
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(CALL_PATH, CALL_ENDPOINT, {}, done, true, RequestMethod::DEL);
	Locator::getLogger()->info("Finished creating RemoveGoal request for {}",
							   goal->DebugString());
}

void
DownloadManager::UpdateGoalRequest(ScoreGoal* goal)
{
	if (goal == nullptr) {
		Locator::getLogger()->warn("Null goal passed to UpdateGoalRequest. Skipped");
		return;
	}

	constexpr auto& CALL_ENDPOINT = API_GOALS;
	constexpr auto& CALL_PATH = API_GOALS;

	Locator::getLogger()->info("Generating UpdateGoalRequest for {}",
							   goal->DebugString());

	CURL* curlHandle = initCURLHandle(true, true, DONT_COMPRESS);
	CURLAPIURL(curlHandle, CALL_PATH);

	// this seems very illegal
	Document d;
	Document::AllocatorType& allocator = d.GetAllocator();
	d = GoalToJSON(goal, allocator);

	StringBuffer buffer;
	Writer<StringBuffer> w(buffer);
	d.Accept(w);
	std::string body = buffer.GetString();

	curl_easy_setopt_log_err(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POSTFIELDSIZE, body.length());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_COPYPOSTFIELDS, body.c_str());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_CUSTOMREQUEST, "PATCH");

	auto done = [goal, &CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [goal, this]() { UpdateGoalRequest(goal);
			})) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() { return parseJson(d, req, "UpdateGoal"); };

		const auto& response = req.response_code;
		if (response == 200) {
			// all good
			Locator::getLogger()->info(
			  "UpdateGoal successfully updated goal for {} on online profile",
			  goal->DebugString());

		} else if (response == 422) {
			// some validation issue with the request
			parse();

			Locator::getLogger()->warn(
			  "UpdateGoal for {} failed due to validation error: {}",
			  goal->DebugString(),
			  jsonObjectToString(d));
		} else if (response == 404) {
			parse();

			Locator::getLogger()->warn(
			  "UpdateGoal for {} failed due to 404. Goal may "
			  "be missing online - Content: {}",
			  goal->DebugString(),
			  jsonObjectToString(d));
		} else {
			parse();

			Locator::getLogger()->warn(
			  "UpdateGoal for {} unexpected response {} - Content: {}",
			  goal->DebugString(),
			  response,
			  jsonObjectToString(d));
		}
	};

	HTTPRequest* req =
	  new HTTPRequest(curlHandle, done, nullptr, [](auto& req) {
	  });
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(CALL_ENDPOINT, *req)) {
		AddHttpRequestHandle(req->handle);
		HTTPRequests.push_back(req);
	}
	Locator::getLogger()->info("Finished creating UpdateGoal request for {}",
							   goal->DebugString());
}

void
DownloadManager::GetGoalsRequest(std::function<void(std::vector<ScoreGoal>)> onSuccess,
								 const DateTime start,
								 const DateTime end)
{
	constexpr auto& CALL_ENDPOINT = API_GOALS;
	constexpr auto& CALL_PATH = API_GOALS;

	std::string startstr = fmt::format(
	  "{}-{}-{}", start.tm_year + 1900, start.tm_mon + 1, start.tm_mday);
	std::string endstr =
	  fmt::format("{}-{}-{}", end.tm_year + 1900, end.tm_mon + 1, end.tm_mday);
	Locator::getLogger()->info(
	  "Generating GetGoalsRequest {} to {}", startstr, endstr);

	std::vector<std::pair<std::string, std::string>> params = {
		std::make_pair("start", startstr),
		std::make_pair("end", endstr),
	};

	auto done = [onSuccess, start, end, &CALL_ENDPOINT, this, startstr, endstr](
				  auto& req) {
		
		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [onSuccess, start, end, this]() {
				  GetGoalsRequest(onSuccess, start, end);
			  })) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "GetGoals");
		};

		const auto& response = req.response_code;
		if (response == 200) {
			// all good
			if (parse())
				return;

			if (d.HasMember("goals") && d["goals"].IsArray()) {
				auto& data = d["goals"];

				std::vector<ScoreGoal> onlineGoals{};
				for (auto it = data.Begin(); it != data.End(); it++) {
					auto obj = it->GetObj();
					ScoreGoal tmpgoal;

					std::string ck = "";
					if (obj.HasMember("chart")) {
						ck = getJsonString(obj["chart"], "key");
					}

					tmpgoal.achieved = getJsonBool(obj, "achieved");
					tmpgoal.chartkey = ck;
					tmpgoal.percent = getJsonFloat(obj, "wife");
					tmpgoal.rate = getJsonFloat(obj, "rate");
					tmpgoal.timeachieved =
					  DateTime::GetFromString(getJsonString(obj, "achieved_date"));
					tmpgoal.timeassigned =
					  DateTime::GetFromString(getJsonString(obj, "set_date"));

					onlineGoals.push_back(tmpgoal);
				}

				Locator::getLogger()->info(
				  "GetGoalsRequest returned successfully. Found {} online "
				  "goals",
				  onlineGoals.size());
				onSuccess(onlineGoals);

			} else {
				Locator::getLogger()->warn("GetGoalsRequest got unexpected "
										   "response body - Content: {}",
										   jsonObjectToString(d));
			}
		} else {
			parse();

			Locator::getLogger()->warn(
			  "GetGoalsRequest unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(CALL_PATH, CALL_ENDPOINT, params, done, true);
	Locator::getLogger()->info(
	  "Finished creating GetGoals request for {} - {}", startstr, endstr);
}

void
uploadGoalsSequentially()
{
	Message msg("GoalUploadProgress");
	msg.SetParam(
	  "percent",
	  1.f - (static_cast<float>(DLMAN->GoalUploadSequentialQueue.size()) /
			 static_cast<float>(DLMAN->sequentialGoalUploadTotalWorkload)));
	MESSAGEMAN->Broadcast(msg);

	if (!DLMAN->GoalUploadSequentialQueue.empty()) {

		std::vector<ScoreGoal*> goalsToUpload{};
		for (auto i = 0u; i < UPLOAD_GOAL_BULK_CHUNK_SIZE &&
						  !DLMAN->GoalUploadSequentialQueue.empty();
			 i++) {
			goalsToUpload.push_back(DLMAN->GoalUploadSequentialQueue.front());
			DLMAN->GoalUploadSequentialQueue.pop_front();

			if (goalsToUpload.size() >= UPLOAD_GOAL_BULK_CHUNK_SIZE) {
				break;
			}
		}
		runningSequentialGoalUpload = true;
		DLMAN->BulkAddGoals(goalsToUpload, uploadGoalsSequentially);

	} else {
		Locator::getLogger()->info(
		  "Sequential goal upload queue empty - uploads finished");
		runningSequentialGoalUpload = false;
		DLMAN->sequentialGoalUploadTotalWorkload = 0;
		MESSAGEMAN->Broadcast("SequentialGoalUploadFinished");
	}
}

void
startSequentialGoalUpload()
{
	if (DLMAN->GoalUploadSequentialQueue.empty()) {
		Locator::getLogger()->info(
		  "Could not start sequential goal upload process - nothing to upload");
		return;
	}

	if (!runningSequentialGoalUpload) {
		Locator::getLogger()->info("Starting sequential goal upload process - "
								   "{} goals split into chunks of {}",
								   DLMAN->GoalUploadSequentialQueue.size(),
								   UPLOAD_GOAL_BULK_CHUNK_SIZE);
		uploadGoalsSequentially();
	}
}

void
DownloadManager::RefreshGoals(const DateTime start, const DateTime end)
{
	Locator::getLogger()->info(
	  "Refreshing Goals - {} to {}", start.GetString(), end.GetString());

	auto onSuccess = [this](std::vector<ScoreGoal> onlineGoals) {
		auto* profile = PROFILEMAN->GetProfile(PLAYER_1);

		if (profile == nullptr) {
			Locator::getLogger()->warn(
			  "Profile for PLAYER_1 came back null. Goals cannot be synced");
			return;
		}

		auto& goalmap = profile->goalmap;
		std::unordered_map<std::string, std::vector<ScoreGoal>>
		  onlineGoalsByChartkey{};
		std::deque<ScoreGoal*> goalsToUpload{};
		std::vector<ScoreGoal> goalsToSave{};
		std::vector<ScoreGoal*> goalsToUpdate{};

		for (auto& goal : onlineGoals) {
			if (goal.chartkey.empty())
				continue;
			if (onlineGoalsByChartkey.count(goal.chartkey) == 0) {
				onlineGoalsByChartkey[goal.chartkey] = std::vector<ScoreGoal>();
			}
			onlineGoalsByChartkey[goal.chartkey].push_back(goal);
		}

		// find the goals that must be saved locally
		for (auto& it : onlineGoalsByChartkey) {
			const auto& ck = it.first;
			const auto& goalsForChart = it.second;

			if (goalmap.count(ck) == 0) {
				// these online goals are not local
				// so upload all of them
				for (auto& goal : goalsForChart) {
					goalsToSave.push_back(goal);
				}
			} else {
				// these goals might be local
				// see if it should be saved
				auto& localGoalsForChart = goalmap.at(ck).goals;
				for (auto& goal : goalsForChart) {
					auto found = false;
					for (auto& localGoal : localGoalsForChart) {
						// if the goal is found
						// either save or do nothing
						if (fabsf(goal.rate - localGoal.rate) < 0.001) {
							if (goal.achieved && !localGoal.achieved &&
								goal.timeassigned >= localGoal.timeassigned) {
								Locator::getLogger()->info(
								  "About to override local goal {} with "
								  "online goal {}",
								  localGoal.DebugString(),
								  goal.DebugString());
								goalsToSave.push_back(goal);
							}
							found = true;
							break;
						}
					}
					// if the goal is not found, save it
					if (!found) {
						goalsToSave.push_back(goal);
					}
				}
			}
		}

		// have to load the goals here because
		// container reallocation can invalidate pointers
		// to local goals
		for (auto& goal : goalsToSave) {
			if (!profile->LoadGoalIfNew(goal)) {
				Locator::getLogger()->info(
				  "Goal to save locally was a duplicate: {}",
				  goal.DebugString());
			}
		}

		// find the goals that must be uploaded
		for (auto& it : goalmap) {
			const auto& ck = it.first;
			auto& goalsForChart = it.second;

			if (onlineGoalsByChartkey.count(ck) == 0) {
				// these local goals are not online
				// so upload all of them
				for (auto& goal : goalsForChart.goals) {
					Locator::getLogger()->info("ToUpload goal: {}",
											   goal.DebugString());
					goalsToUpload.push_back(&goal);
				}
			} else {
				// these goals might be online
				// see if it should be uploaded
				auto& onlineGoalsForChart = onlineGoalsByChartkey[ck];
				for (auto& goal : goalsForChart.goals) {
					auto found = false;
					for (auto& onlineGoal : onlineGoalsForChart) {
						// if the goal is found
						// either update or do nothing
						if (fabsf(goal.rate - onlineGoal.rate) < 0.001) {
							if (goal.achieved && !onlineGoal.achieved &&
								goal.timeassigned >= onlineGoal.timeassigned) {
								Locator::getLogger()->info(
								  "ToUpdate goal: {} (to replace online goal: "
								  "{})",
								  goal.DebugString(),
								  onlineGoal.DebugString());
								goalsToUpdate.push_back(&goal);
							}
							found = true;
							break;
						}
					}
					// if the goal is not found, upload it
					if (!found) {
						Locator::getLogger()->info("ToUpload goal: {}",
													goal.DebugString());
						goalsToUpload.push_back(&goal);
					}
				}
			}
		}

		// logging here to occur before the AddGoal and UpdateGoal logs
		Locator::getLogger()->info("Found {} online goals to save locally, {} "
								   "local goals to update online, and "
								   "{} local goals to upload as new ones",
								   goalsToSave.size(),
								   goalsToUpdate.size(),
								   goalsToUpload.size());
		if (goalsToSave.size() > 0) {
			MESSAGEMAN->Broadcast(Message_GoalsUpdated);
		}

		// upload goals
		if (goalsToUpload.size() > 0) {
			GoalUploadSequentialQueue.insert(GoalUploadSequentialQueue.end(),
											 goalsToUpload.begin(),
											 goalsToUpload.end());
			sequentialGoalUploadTotalWorkload += goalsToUpload.size();
			startSequentialGoalUpload();
		}

		// update goals
		for (auto& goal : goalsToUpdate) {
			UpdateGoal(goal);
		}

	};
	GetGoalsRequest(onSuccess, start, end);
}

inline std::string
ChartlistToJSON(const Playlist& playlist) {

	Document d;
	Document::AllocatorType& allocator = d.GetAllocator();
	d.SetObject();

	Value arrDoc(kArrayType);
	for (auto& chart : playlist.chartlist) {
		Document element;
		element.SetObject();
		element.AddMember(
		  "chartkey", stringToVal(chart.key, allocator), allocator);
		element.AddMember("rate", chart.rate, allocator);
		arrDoc.PushBack(element, allocator);
	}
	d.AddMember("charts", arrDoc, allocator);

	StringBuffer buffer;
	Writer<StringBuffer> w(buffer);
	d.Accept(w);
	return buffer.GetString();
}

inline std::string
PlaylistToJSON(const Playlist& playlist) {

	Document d;
	Document::AllocatorType& allocator = d.GetAllocator();
	d.SetObject();

	d.AddMember(
	  "charts", stringToVal(ChartlistToJSON(playlist), allocator), allocator);
	d.AddMember("name", stringToVal(playlist.name, allocator), allocator);

	StringBuffer buffer;
	Writer<StringBuffer> w(buffer);
	d.Accept(w);
	return buffer.GetString();
}

void
DownloadManager::AddPlaylistRequest(const std::string& name)
{
	constexpr auto& CALL_ENDPOINT = API_PLAYLISTS;
	constexpr auto& CALL_PATH = API_PLAYLISTS;

	const auto& playlists = SONGMAN->GetPlaylists();
	if (!playlists.contains(name)) {
		Locator::getLogger()->warn(
		  "AddPlaylistRequest couldn't find local Playlist named {}", name);
		return;
	}

	const auto& playlist = playlists.at(name);

	Locator::getLogger()->info(
	  "Generating AddPlaylistRequest for Playlist {} ({} charts)",
	  name,
	  playlist.chartlist.size());

	std::vector<std::pair<std::string, std::string>> postParams = {
		std::make_pair("name", playlist.name),
		std::make_pair("charts", ChartlistToJSON(playlist))
	};

	auto done = [name, playlist, &CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			CALL_ENDPOINT,
			req,
			[]() {},
			[name, this]() { AddPlaylistRequest(name); })) {
			return;
		}

		Document d;
		// return true if parse failure
		auto parse = [&d, &req]() { return parseJson(d, req, "AddPlaylist"); };

		const auto& response = req.response_code;
		if (response == 200) {
			// it worked
			Locator::getLogger()->info(
			  "AddPlaylist successfully added playlist {} to online profile. "
			  "This doesn't guarantee that all charts were uploaded, and they "
			  "may resync later",
			  name);
		}
		else if (response == 422) {
			// some validation issue with the request
			parse();

			Locator::getLogger()->warn(
			  "AddPlaylist for playlist {} failed due to validation error: {}",
			  name,
			  jsonObjectToString(d));
		}
		else {
			// ???
			parse();

			Locator::getLogger()->warn("AddPlaylist for playlist {} unexpected "
									   "response {} - Content: {}",
									   name,
									   response,
									   jsonObjectToString(d));
		}

	};

	SendRequest(
	  CALL_PATH, CALL_ENDPOINT, postParams, done, true, RequestMethod::POST);

	Locator::getLogger()->info(
	  "Finished creating AddPlaylist request for playlist {}", playlist.name);
}

void
DownloadManager::UpdatePlaylistRequest(const std::string& name)
{
	constexpr auto& CALL_ENDPOINT = API_PLAYLISTS;
	constexpr auto& CALL_PATH = API_PLAYLISTS;

	const auto& playlists = SONGMAN->GetPlaylists();
	if (!playlists.contains(name)) {
		Locator::getLogger()->warn(
		  "UpdatePlaylistRequest couldn't find local Playlist named {}", name);
		return;
	}

	const auto& playlist = playlists.at(name);

	Locator::getLogger()->info(
	  "Generating UpdatePlaylistRequest for Playlist {} ({} charts)",
	  name,
	  playlist.chartlist.size());

	CURL* curlHandle = initCURLHandle(true, true, DONT_COMPRESS);
	CURLAPIURL(curlHandle, CALL_PATH);
	
	auto body = PlaylistToJSON(playlist);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POSTFIELDSIZE, body.length());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_COPYPOSTFIELDS, body.c_str());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_CUSTOMREQUEST, "PATCH");

	auto done = [name, playlist, &CALL_ENDPOINT, this](auto& req) {
		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [name, this]() { UpdatePlaylistRequest(name); })) {
			return;
		}

		Document d;
		// return true if parse failure
		auto parse = [&d, &req]() { return parseJson(d, req, "UpdatePlaylist"); };

		const auto& response = req.response_code;
		if (response == 200) {
			// it worked
			Locator::getLogger()->info(
			  "UpdatePlaylist successfully updated playlist {} on online profile. "
			  "This doesn't guarantee that all charts were uploaded, and they "
			  "may resync later",
			  name);
		} else if (response == 422) {
			// some validation issue with the request
			parse();

			Locator::getLogger()->warn(
			  "UpdatePlaylist for playlist {} failed due to validation error: {}",
			  name,
			  jsonObjectToString(d));
		} else {
			// ???
			parse();

			Locator::getLogger()->warn("UpdatePlaylist for playlist {} unexpected "
									   "response {} - Content: {}",
									   name,
									   response,
									   jsonObjectToString(d));
		}
	};

	HTTPRequest* req =
	  new HTTPRequest(curlHandle, done, nullptr, [](auto& req) {});
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(CALL_ENDPOINT, *req)) {
		AddHttpRequestHandle(req->handle);
		HTTPRequests.push_back(req);
	}
	Locator::getLogger()->info(
	  "Finished creating UpdatePlaylist request for playlist {}", name);
}

void
DownloadManager::RemovePlaylistRequest(const std::string& name)
{
	constexpr auto& CALL_ENDPOINT = API_PLAYLISTS;
	constexpr auto& CALL_PATH = API_PLAYLISTS;

	const auto& playlists = SONGMAN->GetPlaylists();
	if (!playlists.contains(name)) {
		Locator::getLogger()->warn("RemovePlaylistRequest couldn't find local Playlist named {}",
								   name);
		return;
	}

	Locator::getLogger()->info(
	  "Generating RemovePlaylistRequest for Playlist {}", name);

	CURL* curlHandle = initCURLHandle(true, true, DONT_COMPRESS);
	CURLAPIURL(curlHandle, CALL_PATH);

	Document d;
	Document::AllocatorType& allocator = d.GetAllocator();
	d.SetObject();
	d.AddMember("name", stringToVal(name, allocator), allocator);
	StringBuffer buffer;
	Writer<StringBuffer> w(buffer);
	d.Accept(w);

	std::string body = buffer.GetString();
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POSTFIELDSIZE, body.length());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_COPYPOSTFIELDS, body.c_str());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");


	auto done = [name, &CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [name, this]() { RemovePlaylistRequest(name); })) {
			return;
		}

		Document d;
		// return true if parse failure
		auto parse = [&d, &req]() {
			return parseJson(d, req, "RemovePlaylist");
		};

		const auto& response = req.response_code;
		if (response == 200) {
			// success
			Locator::getLogger()->info(
			  "RemovePlaylist for playlist {} successfully deleted from online "
			  "profile",
			  name);
		}
		else {
			// ???
			parse();

			Locator::getLogger()->warn("RemovePlaylist for playlist {} "
									   "unexpected response {} - Content: {}",
									   name,
									   response,
									   jsonObjectToString(d));
		}

	};

	HTTPRequest* req =
	  new HTTPRequest(curlHandle, done, nullptr, [](auto& req) {});
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(CALL_ENDPOINT, *req)) {
		AddHttpRequestHandle(req->handle);
		HTTPRequests.push_back(req);
	}
	Locator::getLogger()->info(
	  "Finished creating RemovePlaylist request for playlist {}", name);
}

void
DownloadManager::GetPlaylistsRequest(
  std::function<void(std::unordered_map<std::string, Playlist>)> onSuccess,
  const DateTime start,
  const DateTime end)
{
	constexpr auto& CALL_ENDPOINT = API_PLAYLISTS;
	constexpr auto& CALL_PATH = API_PLAYLISTS;

	std::string startstr = fmt::format(
	  "{}-{}-{}", start.tm_year + 1900, start.tm_mon + 1, start.tm_mday);
	std::string endstr =
	  fmt::format("{}-{}-{}", end.tm_year + 1900, end.tm_mon + 1, end.tm_mday);
	Locator::getLogger()->info(
	  "Generating GetPlaylistsRequest {} to {}", startstr, endstr);

	std::vector<std::pair<std::string, std::string>> params = {
		std::make_pair("start", startstr),
		std::make_pair("end", endstr),
	};

	auto done = [onSuccess, start, end, &CALL_ENDPOINT, this, startstr, endstr](
				  auto& req) {
		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [onSuccess, start, end, this]() {
				  GetPlaylistsRequest(onSuccess, start, end);
			  })) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "GetPlaylists");
		};

		const auto& response = req.response_code;
		if (response == 200) {
			// all good
			if (parse())
				return;

			if (d.HasMember("playlists") && d["playlists"].IsArray()) {
				auto& data = d["playlists"];

				std::unordered_map<std::string, Playlist> onlinePlaylists{};
				for (auto it = data.Begin(); it != data.End(); it++) {
					auto obj = it->GetObj();
					Playlist tmpPlaylist;

					tmpPlaylist.name = getJsonString(obj, "name");
					tmpPlaylist.onlineId = getJsonInt(obj, "id");

					std::vector<Chart> chartlist{};
					tmpPlaylist.chartlist = chartlist;
					onlinePlaylists[tmpPlaylist.name] = tmpPlaylist;

					Locator::getLogger()->info("Found online playlist {}",
											   tmpPlaylist.name);
				}

				Locator::getLogger()->info(
				  "GetPlaylistsRequest returned successfully. Found {} online "
				  "playlists",
				  onlinePlaylists.size());
				onSuccess(onlinePlaylists);

			} else {
				Locator::getLogger()->warn("GetPlaylistsRequest got unexpected "
										   "response body - Content: {}",
										   jsonObjectToString(d));
			}
		} else {
			parse();

			Locator::getLogger()->warn(
			  "GetPlaylistsRequest unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(CALL_PATH, CALL_ENDPOINT, params, done, true);
	Locator::getLogger()->info(
	  "Finished creating GetPlaylistsRequest request for {} - {}",
	  startstr,
	  endstr);
}

void
DownloadManager::GetPlaylistRequest(std::function<void(Playlist)> onSuccess, int id)
{
	constexpr auto& CALL_ENDPOINT = API_PLAYLIST;
	const auto CALL_PATH = fmt::format(API_PLAYLIST, id);

	Locator::getLogger()->info(
	  "Generating GetPlaylistRequest for playlist id {}", id);

	auto done = [onSuccess, id, &CALL_ENDPOINT, this](auto& req){
		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [onSuccess, id, this]() {
				  GetPlaylistRequest(onSuccess, id);
			  })) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "GetPlaylist");
		};

		const auto& response = req.response_code;
		if (response == 200) {
			// all good
			if (parse())
				return;

			if (d.HasMember("data") && d["data"].IsArray()) {
				auto& data = d["data"];

				Playlist tmpPlaylist;
				tmpPlaylist.name = "YOU_SHOULDNT_SEE_THIS";

				std::vector<Chart> chartlist{};
				for (auto chartIt = data.Begin();
						chartIt != data.End();
						chartIt++) {
					auto chartObj = chartIt->GetObj();
					Chart chart;

					chart.key = getJsonString(chartObj, "key");
					chart.rate = getJsonFloat(chartObj, "rate");
					chart.FromKey(chart.key);

					chartlist.push_back(chart);
				}
				tmpPlaylist.chartlist = chartlist;

				Locator::getLogger()->info(
				  "GetPlaylistRequest for id {} returned successfully. Found "
				  "{} charts in online playlist",
				  id,
				  chartlist.size());
				onSuccess(tmpPlaylist);

			} else {
				Locator::getLogger()->warn("GetPlaylistRequest got unexpected "
										   "response body - Content: {}",
										   jsonObjectToString(d));
			}
		} else {
			parse();

			Locator::getLogger()->warn(
			  "GetPlaylistRequest unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(CALL_PATH, CALL_ENDPOINT, {}, done, true);
	Locator::getLogger()->info(
	  "Finished creating GetPlaylistRequest request for playlist id {}", id);
}

void
DownloadManager::DownloadMissingPlaylists(const DateTime start, const DateTime end)
{
	Locator::getLogger()->info("Downloading missing playlists");

	auto onSuccess = [this](std::unordered_map<std::string, Playlist> onlinePlaylists) {
		auto& localPlaylists = SONGMAN->GetPlaylists();

		std::vector<Playlist> toDownload{};
		for (auto& plit : onlinePlaylists) {
			const auto& name = plit.first;
			const auto existsLocally = localPlaylists.contains(name);

			if (!existsLocally) {
				toDownload.push_back(plit.second);
			}
		}

		if (toDownload.size() > 0) {
			Locator::getLogger()->info(
			  "Found {} playlists which must be downloaded from online. "
			  "Queueing requests...",
			  toDownload.size());

			for (auto pl : toDownload) {
				auto handlePlaylist = [&localPlaylists, pl](Playlist playlist) {
					playlist.name = pl.name;
					localPlaylists.emplace(pl.name, playlist);
					Locator::getLogger()->info(
					  "Saved online playlist '{}' with {} charts locally",
					  pl.name,
					  playlist.chartlist.size());

					// horrible idea (compatibility)
					MESSAGEMAN->Broadcast("DisplayAllPlaylists");

					Message msg("DownloadedPlaylist");
					msg.SetParam("new", true);
					msg.SetParam("playlist",
								 LuaReference::CreateFromPush(playlist));
					MESSAGEMAN->Broadcast(msg);
				};
				GetPlaylistRequest(handlePlaylist, pl.onlineId);
			}
		}
		else {
			Locator::getLogger()->info(
			  "Found no playlists which needed to be download from online");
		}
	};

	GetPlaylistsRequest(onSuccess, start, end);
}

void
DownloadManager::DownloadPlaylist(const std::string& name)
{
	Locator::getLogger()->info("Downloading playlist data for playlist '{}'", name);

	auto& playlists = SONGMAN->GetPlaylists();
	if (!playlists.contains(name)) {
		Locator::getLogger()->warn(
		  "DownloadPlaylist did not find playlist '{}' locally to sync with",
		  name);
		// lazily quit. we could do GetPlaylists and then GetPlaylist
		// but naahhhh
		return;
	}

	auto& localPlaylist = playlists.at(name);

	if (localPlaylist.onlineId == 0) {
		Locator::getLogger()->warn(
		  "DownloadPlaylist did not find onlineId for local playlist '{}'",
		  name);
		// lazily quit. same as above
		return;
	}

	auto onSuccess = [&localPlaylist, name, this](Playlist onlinePlaylist){
		Locator::getLogger()->info(
		  "DownloadPlaylist replaced local playlist '{}' chartlist of length "
		  "{} with chartlist of length {}",
		  name,
		  localPlaylist.chartlist.size(),
		  onlinePlaylist.chartlist.size());

		localPlaylist.chartlist = onlinePlaylist.chartlist;

		// horrible idea (compatibility)
		MESSAGEMAN->Broadcast("DisplayAllPlaylists");

		Message msg("DownloadedPlaylist");
		msg.SetParam("new", false);
		msg.SetParam("playlist", LuaReference::CreateFromPush(localPlaylist));
		MESSAGEMAN->Broadcast(msg);
	};

	GetPlaylistRequest(onSuccess, localPlaylist.onlineId);
}

void
DownloadManager::LoadPlaylists(const DateTime start, const DateTime end)
{
	Locator::getLogger()->info(
	  "Loading Playlists - {} to {}", start.GetString(), end.GetString());

	auto onSuccess =
	  [this](std::unordered_map<std::string, Playlist> onlinePlaylists) {
		auto& localPlaylists = SONGMAN->GetPlaylists();

		auto idUpdatedCount = 0;
		auto missingCount = 0;
		for (auto& plit : onlinePlaylists) {
			const auto& name = plit.first;
			const auto existsLocally = localPlaylists.contains(name);

			if (name == "Favorites") {
				// this should be impossible, but just in case
				continue;
			}

			if (existsLocally) {
				auto& localPlaylist = localPlaylists[name];
				localPlaylist.onlineId = plit.second.onlineId;

				idUpdatedCount++;
			}
			else {
				// playlist needs to be saved
				// do nothing...
				missingCount++;
			}
		}

		Locator::getLogger()->info("Found {} online playlists which are "
									 "saved locally, and {} which are not",
									 idUpdatedCount,
									 missingCount);
	};
	GetPlaylistsRequest(onSuccess, start, end);
}

void
DownloadManager::GetRankedChartkeysRequest(std::function<void(void)> callback,
										   const DateTime start,
										   const DateTime end)
{
	constexpr auto& CALL_ENDPOINT = API_RANKED_CHARTKEYS;
	constexpr auto& CALL_PATH = API_RANKED_CHARTKEYS;

	std::string startstr =
	  fmt::format("{}-{}-{}", start.tm_year + 1900, start.tm_mon + 1, start.tm_mday);
	std::string endstr =
	  fmt::format("{}-{}-{}", end.tm_year + 1900, end.tm_mon + 1, end.tm_mday);
	Locator::getLogger()->info(
	  "Generating ranked chartkeys request {} to {}", startstr, endstr);

	std::vector<std::pair<std::string, std::string>> params = {
		std::make_pair("start", startstr),
		std::make_pair("end", endstr),
	};

	auto done = [callback, start, end, &CALL_ENDPOINT, this, startstr, endstr](
				  auto& req) {

		if (Handle401And429Response(
			CALL_ENDPOINT,
			req,
			[callback]() {
				if (callback)
					callback();
			},
			[callback, start, end, this]() {
				GetRankedChartkeysRequest(callback, start, end);
			})) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "GetRankedChartkeys");
		};

		const auto& response = req.response_code;
		if (response == 200) {
			// all good
			if (parse()) {
				if (callback)
					callback();
				return;
			}

			if (d.HasMember("data") && d["data"].IsArray()) {

				/* expected data:
				{ "data" : [ "Xabc123", "Xabc124", .... ] }
				*/

				std::unordered_set<std::string> newKeys{};
				auto& data = d["data"];
				for (auto it = data.Begin(); it != data.End(); it++) {
					newKeys.insert(it->GetString());
				}
				newlyRankedChartkeys.insert(newKeys.begin(), newKeys.end());

				Locator::getLogger()->info(
				  "GetRankedChartkeys succeeded - {} "
				  "keys found for range {} - {} | {} total keys",
				  newKeys.size(),
				  startstr,
				  endstr,
				  newlyRankedChartkeys.size());

			} else {
				Locator::getLogger()->warn("GetRankedChartkeys got unexpected "
										   "response body - Content: {}",
										   jsonObjectToString(d));
			}
		} else {
			parse();

			Locator::getLogger()->warn(
			  "GetRankedChartkeys unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
		if (callback)
			callback();
	};

	SendRequest(CALL_PATH, CALL_ENDPOINT, params, done, true);
	Locator::getLogger()->info(
	  "Finished creating GetRankedChartkeys request for {} - {}",
	  startstr,
	  endstr);
}

bool
DownloadManager::ShouldUploadScores()
{
	return LoggedIn() && automaticSync &&
		   GamePreferences::m_AutoPlay == PC_HUMAN;
}

inline OnlineScore
jsonToOnlineScore(Value& score, const std::string& chartkey)
{
	OnlineScore tmp;

	std::string scorekey = "";
	if (score.HasMember("key") && score["key"].IsString())
		scorekey = score["key"].GetString();

	auto& user = score["user"];
	tmp.songId = "";
	if (user.HasMember("username") && user["username"].IsString())
		tmp.username = user["username"].GetString();
	else
		tmp.username = "";
	if (user.HasMember("avatar_path") && user["avatar_path"].IsString())
		tmp.avatar = user["avatar_path"].GetString();
	else
		tmp.avatar = "";
	tmp.userid = 0;
	if (user.HasMember("country") && user["country"].IsString())
		tmp.countryCode = user["country"].GetString();
	else
		tmp.countryCode = "";
	if (user.HasMember("overall") && user["overall"].IsNumber())
		tmp.playerRating = user["overall"].GetFloat();
	else
		tmp.playerRating = 0.f;
	if (score.HasMember("wife") && score["wife"].IsNumber())
		tmp.wife = static_cast<float>(score["wife"].GetFloat()) / 100.f;
	else
		tmp.wife = 0.f;
	if (score.HasMember("modifiers") && score["modifiers"].IsString())
		tmp.modifiers = score["modifiers"].GetString();
	else
		tmp.modifiers = "";
	if (score.HasMember("combo") && score["combo"].IsInt())
		tmp.maxcombo = score["combo"].GetInt();
	else
		tmp.maxcombo = 0;
	if (score.HasMember("marvelous") &&
		score["marvelous"].IsInt())
		tmp.marvelous = score["marvelous"].GetInt();
	else
		tmp.marvelous = 0;
	if (score.HasMember("perfect") && score["perfect"].IsInt())
		tmp.perfect = score["perfect"].GetInt();
	else
		tmp.perfect = 0;
	if (score.HasMember("great") && score["great"].IsInt())
		tmp.great = score["great"].GetInt();
	else
		tmp.great = 0;
	if (score.HasMember("good") && score["good"].IsInt())
		tmp.good = score["good"].GetInt();
	else
		tmp.good = 0;
	if (score.HasMember("bad") && score["bad"].IsInt())
		tmp.bad = score["bad"].GetInt();
	else
		tmp.bad = 0;
	if (score.HasMember("miss") && score["miss"].IsInt())
		tmp.miss = score["miss"].GetInt();
	else
		tmp.miss = 0;
	if (score.HasMember("hit_mine") && score["hit_mine"].IsInt())
		tmp.minehits = score["hit_mine"].GetInt();
	else
		tmp.minehits = 0;
	if (score.HasMember("held") && score["held"].IsInt())
		tmp.held = score["held"].GetInt();
	else
		tmp.held = 0;
	if (score.HasMember("let_go") &&
		score["let_go"].IsInt())
		tmp.letgo = score["let_go"].GetInt();
	else
		tmp.letgo = 0;
	if (score.HasMember("missed_hold") &&
		score["missed_hold"].IsInt())
		tmp.holdmiss = score["missed_hold"].GetInt();
	else
		tmp.holdmiss = 0;
	if (score.HasMember("datetime") && score["datetime"].IsString())
		tmp.datetime.FromString(score["datetime"].GetString());
	else
		tmp.datetime.FromString("0");
	if (score.HasMember("id") && score["id"].IsInt())
		tmp.scoreid = std::to_string(score["id"].GetInt());
	else
		tmp.scoreid = "";

	// filter scores not on the current rate out if enabled...
	// dunno if we need this precision -mina
	if (score.HasMember("rate") && score["rate"].IsString())
		tmp.rate = std::stof(score["rate"].GetString());
	else if (score.HasMember("rate") && score["rate"].IsNumber())
		tmp.rate = score["rate"].GetFloat();
	else {
		Locator::getLogger()->warn(
		  "Bad rate in leaderboard? ck {} sk {} scoreid {}",
		  chartkey,
		  scorekey,
		  tmp.scoreid);
		tmp.rate = 0.0;
	}
	if (score.HasMember("chord_cohesion") && score["chord_cohesion"].IsBool())
		tmp.nocc = !score["chord_cohesion"].GetBool();
	else
		tmp.nocc = false;
	if (score.HasMember("valid") && score["valid"].IsBool())
		tmp.valid = score["valid"].GetBool();
	else
		tmp.valid = false;
	if (score.HasMember("wife_version") && score["wife_version"].IsInt()) {
		auto v = score["wife_version"].GetInt();
		if (v == 3)
			tmp.wifeversion = 3;
		else
			tmp.wifeversion = 2;
	} else
		tmp.wifeversion = 2;

	if (score.HasMember("overall") && score["overall"].IsNumber())
		tmp.SSRs[Skill_Overall] = score["overall"].GetFloat();
	else
		tmp.SSRs[Skill_Overall] = 0.0;
	if (score.HasMember("stream") && score["stream"].IsNumber())
		tmp.SSRs[Skill_Stream] = score["stream"].GetFloat();
	else
		tmp.SSRs[Skill_Stream] = 0.0;
	if (score.HasMember("jumpstream") && score["jumpstream"].IsNumber())
		tmp.SSRs[Skill_Jumpstream] = score["jumpstream"].GetFloat();
	else
		tmp.SSRs[Skill_Jumpstream] = 0.0;
	if (score.HasMember("handstream") && score["handstream"].IsNumber())
		tmp.SSRs[Skill_Handstream] = score["handstream"].GetFloat();
	else
		tmp.SSRs[Skill_Handstream] = 0.0;
	if (score.HasMember("stamina") && score["stamina"].IsNumber())
		tmp.SSRs[Skill_Stamina] = score["stamina"].GetFloat();
	else
		tmp.SSRs[Skill_Stamina] = 0.0;
	if (score.HasMember("jacks") && score["jacks"].IsNumber())
		tmp.SSRs[Skill_JackSpeed] = score["jacks"].GetFloat();
	else
		tmp.SSRs[Skill_JackSpeed] = 0.0;
	if (score.HasMember("chordjacks") && score["chordjacks"].IsNumber())
		tmp.SSRs[Skill_Chordjack] = score["chordjacks"].GetFloat();
	else
		tmp.SSRs[Skill_Chordjack] = 0.0;
	if (score.HasMember("technical") && score["technical"].IsNumber())
		tmp.SSRs[Skill_Technical] = score["technical"].GetFloat();
	else
		tmp.SSRs[Skill_Technical] = 0.0;
	if (score.HasMember("replay") && score["replay"].IsBool())
		tmp.hasReplay = score["replay"].GetBool();
	else
		tmp.hasReplay = false;

	if (score.HasMember("calculator_version") &&
		score["calculator_version"].IsInt())
		tmp.hs.SetSSRCalcVersion(score["calculator_version"].GetInt());

	auto& hs = tmp.hs;
	hs.SetDateTime(tmp.datetime);
	hs.SetMaxCombo(tmp.maxcombo);
	hs.SetName(tmp.username);
	hs.SetModifiers(tmp.modifiers);
	hs.SetChordCohesion(tmp.nocc);
	hs.SetEtternaValid(tmp.valid);
	hs.SetWifeScore(tmp.wife);
	hs.SetWifeVersion(tmp.wifeversion);
	hs.SetSSRNormPercent(tmp.wife);
	hs.SetMusicRate(tmp.rate);
	hs.SetChartKey(chartkey);
	hs.SetScoreKey("Online_" + scorekey);
	hs.SetGrade(hs.GetWifeGrade());

	hs.SetTapNoteScore(TNS_W1, tmp.marvelous);
	hs.SetTapNoteScore(TNS_W2, tmp.perfect);
	hs.SetTapNoteScore(TNS_W3, tmp.great);
	hs.SetTapNoteScore(TNS_W4, tmp.good);
	hs.SetTapNoteScore(TNS_W5, tmp.bad);
	hs.SetTapNoteScore(TNS_Miss, tmp.miss);
	hs.SetTapNoteScore(TNS_HitMine, tmp.minehits);

	hs.SetHoldNoteScore(HNS_Held, tmp.held);
	hs.SetHoldNoteScore(HNS_LetGo, tmp.letgo);
	hs.SetHoldNoteScore(HNS_Missed, tmp.holdmiss);

	FOREACH_ENUM(Skillset, ss)
	hs.SetSkillsetSSR(ss, tmp.SSRs[ss]);

	hs.userid = tmp.userid;
	hs.scoreid = tmp.scoreid;
	hs.avatar = tmp.avatar;
	hs.countryCode = tmp.countryCode;
	hs.hasReplay = tmp.hasReplay;

	return tmp;
}

inline Document
ScoreToJSON(HighScore* hs, bool includeReplayData, Document::AllocatorType& allocator) {

	Document d;
	d.SetObject();

	if (hs == nullptr) {
		Locator::getLogger()->warn("Null HighScore passed to ScoreToJSON. Skipped");
		return d;
	}

	Locator::getLogger()->trace(
	  "ScoreToJSON :: score {} | ck {}", hs->GetScoreKey(), hs->GetChartKey());

	auto validity = hs->GetEtternaValid() && hs->HasReplayData();

	d.AddMember("key", stringToVal(hs->GetScoreKey(), allocator), allocator);
	d.AddMember("wife", hs->GetSSRNormPercent(), allocator);
	d.AddMember("max_combo", hs->GetMaxCombo(), allocator);
	d.AddMember(
	  "modifiers", stringToVal(hs->GetModifiers(), allocator), allocator);
	if (!hs->NormalizeJudgments()) {
		Locator::getLogger()->info("Score {} will NOT use Normalized TNS",
									hs->GetScoreKey());
		d.AddMember("marvelous", hs->GetTapNoteScore(TNS_W1), allocator);
		d.AddMember("perfect", hs->GetTapNoteScore(TNS_W2), allocator);
		d.AddMember("great", hs->GetTapNoteScore(TNS_W3), allocator);
		d.AddMember("good", hs->GetTapNoteScore(TNS_W4), allocator);
		d.AddMember("bad", hs->GetTapNoteScore(TNS_W5), allocator);
		d.AddMember("miss", hs->GetTapNoteScore(TNS_Miss), allocator);
	} else {
		Locator::getLogger()->debug("Score {} will use Normalized TNS",
									hs->GetScoreKey());
		d.AddMember("marvelous", hs->GetTNSNormalized(TNS_W1), allocator);
		d.AddMember("perfect", hs->GetTNSNormalized(TNS_W2), allocator);
		d.AddMember("great", hs->GetTNSNormalized(TNS_W3), allocator);
		d.AddMember("good", hs->GetTNSNormalized(TNS_W4), allocator);
		d.AddMember("bad", hs->GetTNSNormalized(TNS_W5), allocator);
		d.AddMember("miss", hs->GetTNSNormalized(TNS_Miss), allocator);
	}
	d.AddMember("hit_mine", hs->GetTapNoteScore(TNS_HitMine), allocator);
	d.AddMember("held", hs->GetHoldNoteScore(HNS_Held), allocator);
	d.AddMember("let_go", hs->GetHoldNoteScore(HNS_LetGo), allocator);
	d.AddMember("missed_hold", hs->GetHoldNoteScore(HNS_Missed), allocator);
	d.AddMember(
	  "rate",
	  stringToVal(
		fmt::format("{:.3f}", std::round(hs->GetMusicRate() * 1000.0) / 1000.0),
		allocator),
	  allocator);
	d.AddMember("datetime",
				stringToVal(hs->GetDateTime().GetString(), allocator),
				allocator);
	d.AddMember(
	  "chord_cohesion", static_cast<int>(hs->GetChordCohesion()), allocator);
	d.AddMember("double_setup", static_cast<int>(hs->GetDSFlag()), allocator);
	d.AddMember("calculator_version", hs->GetSSRCalcVersion(), allocator);
	d.AddMember("top_score", hs->GetTopScore(), allocator);
	d.AddMember("wife_version", hs->GetWifeVersion(), allocator);
	d.AddMember(
	  "validation_key",
	  stringToVal(hs->GetValidationKey(ValidationKey_Brittle), allocator),
	  allocator);
	d.AddMember("machine_guid",
				stringToVal(hs->GetMachineGuid(), allocator, "MISSING_GUID"),
				allocator);
	d.AddMember(
	  "chart_key", stringToVal(hs->GetChartKey(), allocator), allocator);
	d.AddMember("grade",
				stringToVal(GradeToOldString(hs->GetWifeGrade()), allocator),
				allocator);

	if (hs->GetJudgeScale() == 0.F) {
		Locator::getLogger()->info(
		  "Score {} will default to J4 and upload as invalid",
		  hs->GetScoreKey());
		d.AddMember("judge", 1.F, allocator);
		validity = false;
	}
	else {
		d.AddMember(
		  "judge",
		  stringToVal(
			fmt::format("{:.3f}",
						std::round(hs->GetJudgeScale() * 1000.0) / 1000.0),
			allocator),
		  allocator);
	}

	// comprehensive checks for forced invalidation
	if (validity) {
		// if the score is valid, check the mods...
		PlayerOptions po;
		po.Init();
		po.SetForReplay(true);
		po.FromString(hs->GetModifiers());

		const auto& tfs = po.m_bTransforms;
		const auto& turns = po.m_bTurns;
		const auto* steps = SONGMAN->GetStepsByChartkey(hs->GetChartKey());

		// if ccon, invalid
		if (validity) {
			validity &= !hs->GetChordCohesion();
			if (!validity)
				Locator::getLogger()->info(
				  "Score {} will upload as invalid due to CC On",
				  hs->GetScoreKey());
		}

		// if No Mines, disqualify if mines should be present
		if (validity && tfs[PlayerOptions::TRANSFORM_NOMINES]) {
			validity &= steps != nullptr &&
						steps->GetRadarValues()[RadarCategory_Mines] > 0;
			if (!validity)
				Locator::getLogger()->info(
				  "Score {} will upload as invalid due to NoMines",
				  hs->GetScoreKey());
		}

		// if No Holds, disqualify if holds/rolls should be present
		if (validity && (tfs[PlayerOptions::TRANSFORM_NOHOLDS] ||
						 tfs[PlayerOptions::TRANSFORM_NOROLLS])) {
			auto& radars = steps->GetRadarValues();
			validity &= steps != nullptr && (radars[RadarCategory_Holds] > 0 ||
											 radars[RadarCategory_Rolls] > 0);
			if (!validity)
				Locator::getLogger()->info(
				  "Score {} will upload as invalid due to NoHolds or NoRolls",
				  hs->GetScoreKey());
		}

		// invalidate if any transforms are on (Insert and Remove mods)
		// (this starts after nomines/noholds/norolls)
		for (int tfi = PlayerOptions::TRANSFORM_LITTLE;
			 validity && tfi < PlayerOptions::NUM_TRANSFORMS;
			 tfi++) {
			PlayerOptions::Transform tf =
			  static_cast<PlayerOptions::Transform>(tfi);

			// if any transform is turned on, validity becomes false
			validity &= !tfs[tf];
			if (!validity)
				Locator::getLogger()->info(
				  "Score {} will upload as invalid due to Transform {}",
				  hs->GetScoreKey(),
				  tf);
		}

		// invalidate if any turns are on other than Mirror (shuffle)
		// (this starts after mirror)
		for (int ti = PlayerOptions::TURN_BACKWARDS;
			 validity && ti < PlayerOptions::NUM_TURNS;
			 ti++) {
			PlayerOptions::Turn t = static_cast<PlayerOptions::Turn>(ti);

			validity &= !turns[t];
			if (!validity)
				Locator::getLogger()->info(
				  "Score {} will upload as invalid due to Turn {}",
				  hs->GetScoreKey(),
				  t);
		}

		// impossible for this to happen but just in case
		if (validity) {
			validity &= !po.m_bPractice;
			if (!validity)
				Locator::getLogger()->info(
				  "Score {} will upload as invalid due to Practice Mode",
				  hs->GetScoreKey());
		}

		// if the score has less total judgments than the number of notes
		// by a large margin
		// this is broken by CC On
		// but if CC On was used, this would not be reached
		if (validity) {
			const auto notes = steps->GetRadarValues()[RadarCategory_Notes];
			auto total = 0;
			FOREACH_ENUM(TapNoteScore, tns) {
				total += hs->GetTapNoteScore(tns);
			}
			// would rather be 100% but 90% is reasonable
			// score becomes auto invalid if judgments are
			// less than 90% total notes (fails/bugs)
			// or greater than 110% total notes (inserts/bugs)
			validity &= (total >= static_cast<float>(notes) * 0.9F &&
						 total <= static_cast<float>(notes) * 1.1F);
			if (!validity)
				Locator::getLogger()->info(
				  "Score {} will upload as invalid because the Judgment Total "
				  "{} is not close enough to the Note Total {}",
				  hs->GetScoreKey(),
				  total,
				  notes);
		}
	}
	d.AddMember("valid", static_cast<int>(validity), allocator);

	d.AddMember("exe_hash",
				stringToVal(GAMESTATE->ProgramHash, allocator),
				allocator);
	d.AddMember(
	  "os", stringToVal(Core::Platform::getSystem(), allocator), allocator);


	Document replayVector;
	replayVector.SetArray();
	Document inputDataObject;
	inputDataObject.SetObject();
	if (includeReplayData) {
		Locator::getLogger()->trace("Adding replay data JSON to score {}",
									hs->GetScoreKey());
		auto fval = [](const float f) {
			Value v;
			v.SetFloat(f);
			return v;
		};

		auto* replay = hs->GetReplay();
		auto usingReprioritized = replay->UsingReprioritizedNoteRows();
		if (usingReprioritized) {
			replay->SetUseReprioritizedNoteRows(false);
		}

		bool hadToLoadReplayData = false;

		// load replay data if we need it
		// basically, in one case we care about the fact that we loaded or not
		if (replay->GetOffsetVector().empty()) {
			// this handles loading from disk and then generating needed information
			// would return false if impossible to work with
			hadToLoadReplayData = replay->GeneratePrimitiveVectors();
		} else {
			// this handles loading from disk if necessary and generating if necessary
			replay->GeneratePrimitiveVectors();
		}

		// attempt to get load input data directly.
		// if it doesnt exist, generate it
		if (replay->GetInputDataVector().empty()) {
			hadToLoadReplayData |= replay->GenerateInputData();
		}

		// Online Replay (timestamps) version
		const auto& offsets = replay->GetOffsetVector();
		const auto& columns = replay->GetTrackVector();
		const auto& types = replay->GetTapNoteTypeVector();
		const auto& rows = replay->GetNoteRowVector();
		if (!offsets.empty()) {
			auto steps = SONGMAN->GetStepsByChartkey(hs->GetChartKey());
			if (steps == nullptr) {
				Locator::getLogger()->warn(
				  "Attempted to upload score with no loaded steps "
				  "(scorekey: {} chartkey: {})",
				  hs->GetScoreKey(),
				  hs->GetChartKey());
			} else {
				std::vector<float> timestamps =
				  steps->GetTimingData()->ConvertReplayNoteRowsToTimestamps(
					rows, hs->GetMusicRate());
				for (size_t i = 0; i < offsets.size(); i++) {
					Document thisData;
					thisData.SetArray();
					thisData.PushBack(fval(timestamps[i]), allocator);
					thisData.PushBack(fval(1000.f * offsets[i]), allocator);
					if (hs->HasColumnData()) {
						thisData.PushBack(fval(columns[i]), allocator);
						thisData.PushBack(fval(types[i]), allocator);
					}
					thisData.PushBack(fval(rows[i]), allocator);
					replayVector.PushBack(thisData, allocator);
				}
			}
		}

		// Input Data
		const auto& inputdata = replay->GetInputDataVector();
		const auto& missdata = replay->GetMissReplayDataVector();
		const auto& holddata = replay->GetHoldReplayDataVector();
		const auto& minedata = replay->GetMineReplayDataVector();
		if (!inputdata.empty()) {
			const auto& replay = hs->GetReplay();
			replay->SetHighScoreMods(); // load the mods if they arent loaded..
			inputDataObject.AddMember("mods", stringToVal(replay->GetModifiers(), allocator), allocator);
			inputDataObject.AddMember(
			  "chartkey",
			  stringToVal(replay->GetChartKey(), allocator),
			  allocator);
			inputDataObject.AddMember(
			  "scorekey",
			  stringToVal(replay->GetScoreKey(), allocator),
			  allocator);
			inputDataObject.AddMember(
			  "music_rate", replay->GetMusicRate(), allocator);
			inputDataObject.AddMember(
			  "song_offset", replay->GetSongOffset(), allocator);
			inputDataObject.AddMember(
			  "global_offset", replay->GetGlobalOffset(), allocator);
			inputDataObject.AddMember(
			  "rng_seed", replay->GetRngSeed(), allocator);

			Document inputDataArr;
			inputDataArr.SetArray();
			for (const auto& input : inputdata) {
				// there is a crash in here. dont know where
				Document inputObj;
				inputObj.SetObject();
				inputObj.AddMember("column", input.column, allocator);
				inputObj.AddMember("is_press", input.is_press, allocator);
				inputObj.AddMember(
				  "timestamp", input.songPositionSeconds, allocator);
				inputObj.AddMember("nearest_noterow", input.nearestTapNoterow, allocator);
				inputObj
				  .AddMember("offset_from_nearest_noterow", input.offsetFromNearest, allocator);
				inputObj.AddMember("nearest_notetype",
								   static_cast<int>(input.nearestTapNoteType),
								   allocator);
				inputObj.AddMember("nearest_notesubtype",
								   static_cast<int>(input.nearestTapNoteSubType),
								   allocator);

				inputDataArr.PushBack(inputObj, allocator);
			}
			inputDataObject.AddMember("data", inputDataArr, allocator);

			Document missDataArr;
			missDataArr.SetArray();
			for (const auto& miss : missdata) {
				Document missObj;
				missObj.SetObject();
				missObj.AddMember("column", miss.track, allocator);
				missObj.AddMember("row", miss.row, allocator);
				missObj.AddMember(
				  "notetype", static_cast<int>(miss.tapNoteType), allocator);
				missObj.AddMember("notesubtype",
								  static_cast<int>(miss.tapNoteSubType),
								  allocator);
				missDataArr.PushBack(missObj, allocator);
			}
			inputDataObject.AddMember("misses", missDataArr, allocator);

			Document mineDataArr;
			mineDataArr.SetArray();
			for (const auto& mine : minedata) {
				Document mineObj;
				mineObj.SetObject();
				mineObj.AddMember("column", mine.track, allocator);
				mineObj.AddMember("row", mine.row, allocator);
				mineDataArr.PushBack(mineObj, allocator);
			}
			inputDataObject.AddMember("mine_hits", mineDataArr, allocator);

			Document holdDataArr;
			holdDataArr.SetArray();
			for (const auto& hold : holddata) {
				Document holdObj;
				holdObj.SetObject();
				holdObj.AddMember("column", hold.track, allocator);
				holdObj.AddMember("row", hold.row, allocator);
				holdObj.AddMember("subtype", hold.subType, allocator);
				holdDataArr.PushBack(holdObj, allocator);
			}
			inputDataObject.AddMember("hold_drops", holdDataArr, allocator);
		}

		if (usingReprioritized) {
			replay->SetUseReprioritizedNoteRows(true);
			// this means replay data was already loaded, probably
			hadToLoadReplayData = false;
		}
		if (hadToLoadReplayData) {
			hs->UnloadReplayData();
		}
	}
	d.AddMember("replay_data", replayVector, allocator);
	d.AddMember("input_data", inputDataObject, allocator);

	return d;
}

inline std::string
ScoreVectorToJSON(std::vector<HighScore*>& v, bool includeReplayData) {

	Document d;
	Document::AllocatorType& allocator = d.GetAllocator();
	d.SetObject();

	Value arrDoc(kArrayType);
	for (auto& hs : v) {
		arrDoc.PushBack(ScoreToJSON(hs, includeReplayData, allocator), allocator);
	}
	d.AddMember("data", arrDoc, allocator);


	StringBuffer buffer;
	Writer<StringBuffer> w(buffer);
	d.Accept(w);
	return buffer.GetString();
}

void
DownloadManager::UploadBulkScores(std::vector<HighScore*> hsList,
								  std::function<void()> callback)
{
	constexpr auto& CALL_ENDPOINT = API_UPLOAD_SCORE_BULK;
	constexpr auto& CALL_PATH = API_UPLOAD_SCORE_BULK;

	Locator::getLogger()->info("Creating BulkUploadScore request for {} scores",
							   hsList.size());
	if (!LoggedIn()) {
		Locator::getLogger()->warn(
		  "Attempted to upload scores while not logged in. Aborting");
		if (callback)
			callback();
		return;
	}

	CURL* curlHandle = initCURLHandle(true, true, DO_COMPRESS);
	CURLAPIURL(curlHandle, CALL_PATH);

	auto body = base64_encode(compress_string(ScoreVectorToJSON(hsList, true)));
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POSTFIELDSIZE, body.length());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_COPYPOSTFIELDS, body.c_str());

	// body json
	// Locator::getLogger()->warn("{}", body);

	auto done = [callback, hsList, &CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			CALL_ENDPOINT,
			req,
			[callback]() {
				if (callback)
					callback();
			},
			[hsList, callback, this]() {
				UploadBulkScores(hsList, callback);
			})) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "BulkScoreUpload");
		};

		const auto& response = req.response_code;
		if (response == 200) {
			// kind of good.
			// we can have successes and failures.

			if (parse()) {
				if (callback)
					callback();
				return;
			}

			std::unordered_set<std::string> failedUploadKeys{};
			std::unordered_set<std::string> unrankedUploadKeys{};
			if (d.IsObject() && d.HasMember("failedUploads")) {
				auto& fails = d["failedUploads"];
				/*// Verbose output
				Locator::getLogger()->warn(
				  "BulkScoreUpload had failed uploads but continued: {}",
				  jsonObjectToString(fails));
				*/

				if (fails.IsArray() && fails.Size() > 0) {
					Locator::getLogger()->warn("BulkScoreUpload had {} failed "
											   "uploads out of {}",
											   fails.Size(),
											   hsList.size());
				}

				// collect the scorekeys for the failed uploads
				// the return data is in this format:
				/*
				[
					{ "Sabcdef1234567890" : ["Reason for failure1",] },
					{ "S1234567890abcdef" : ["Reason for failure1",] },
				]
				*/
				if (fails.IsArray()) {
					for (auto it = fails.Begin();
						it != fails.End(); it++) {
						for (auto objIt = it->MemberBegin();
							 objIt != it->MemberEnd();
							 objIt++) {
							auto scorekey = objIt->name.GetString();
							auto strReasons = jsonObjectToString(objIt->value);

							if (strReasons.find("not ranked") != std::string::npos) {
								// the chart isnt ranked, so dont say it was uploaded
								unrankedUploadKeys.insert(scorekey);
							} else {
								// behaves like a normally uploaded score
								failedUploadKeys.insert(scorekey);
							}

							Locator::getLogger()->warn(
							  "Score {} was rejected by server for: {}",
							  objIt->name.GetString(),
							  strReasons);
						}
					}
				}
			}

			if (!d.HasMember("failedUploads") && !d.HasMember("playerRating")) {
				Locator::getLogger()->info(
				  "BulkScoreUpload received no useful response? Content: {}",
				  jsonObjectToString(d));
			}

			// set the uploaded server for the scores that were uploaded
			// except for unranked charts
			for (auto& hs : hsList) {

				hs->forceuploadedthissession = true;
				if (unrankedUploadKeys.contains(hs->GetScoreKey())) {
					// unranked files just cant be force reuploaded
					// (later: commented this to allow fuller uploading)
					// continue;
				}
				// everything else successfully had an upload attempt
				// so record it
				if (hs->GetWifeVersion() == 3)
					hs->AddUploadedServer(wife3_rescore_upload_flag);
				hs->AddUploadedServer(serverURL.Get());
			}

			if (d.IsObject() && d.HasMember("playerRating") &&
				d["playerRating"].IsObject()) {
				// we know the new player rating details and had a successful
				// upload
				auto get = [&d](const char* name) {
					auto& pr = d["playerRating"];
					if (pr.HasMember(name) && pr[name].IsDouble()) {
						return pr[name].GetDouble();
					} else {
						return 0.0;
					}
				};

				auto stream = get("stream");
				auto jumpstream = get("jumpstream");
				auto handstream = get("handstream");
				auto jacks = get("jacks");
				auto chordjacks = get("chordjacks");
				auto stamina = get("stamina");
				auto tech = get("technical");
				auto overall = get("overall");

				sessionRatings[Skill_Overall] = overall;
				sessionRatings[Skill_Stream] = stream;
				sessionRatings[Skill_Jumpstream] = jumpstream;
				sessionRatings[Skill_Handstream] = handstream;
				sessionRatings[Skill_JackSpeed] = jacks;
				sessionRatings[Skill_Chordjack] = chordjacks;
				sessionRatings[Skill_Stamina] = stamina;
				sessionRatings[Skill_Technical] = tech;

				Locator::getLogger()->info(
				  "BulkScoreUpload for {} scores completed, and {} of those "
				  "were rejected - \n\tOverall {}\n\tStream "
				  "{}\n\tJS "
				  "{}\n\tHS {}\n\tJacks {}\n\tCJ {}\n\tStamina {}\n\tTech {}",
				  hsList.size(),
				  failedUploadKeys.size() + unrankedUploadKeys.size(),
				  overall,
				  stream,
				  jumpstream,
				  handstream,
				  jacks,
				  chordjacks,
				  stamina,
				  tech);
			}

		} else {
			// ???
			parse();

			Locator::getLogger()->warn(
			  "BulkUploadScore got unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));

		}
		if (callback)
			callback();
	};

	HTTPRequest* req =
	  new HTTPRequest(curlHandle, done, nullptr, [callback](auto& req) {
		  if (callback)
			  callback();
	  });
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(CALL_ENDPOINT, *req)) {
		AddHttpRequestHandle(req->handle);
		HTTPRequests.push_back(req);
	}
	Locator::getLogger()->info(
	  "Finished creating BulkUploadScore request for {} scores", hsList.size());
}

void
DownloadManager::UploadScore(HighScore* hs,
							 std::function<void()> callback,
							 bool load_from_disk)
{
	if (hs == nullptr) {
		Locator::getLogger()->warn("Null HighScore passed to UploadScore. Skipped");
		return;
	}

	constexpr auto& CALL_ENDPOINT = API_UPLOAD_SCORE;
	constexpr auto& CALL_PATH = API_UPLOAD_SCORE;

	Locator::getLogger()->info("Creating UploadScore request for score {}",
							   hs->GetScoreKey());
	if (!LoggedIn()) {
		Locator::getLogger()->warn(
		  "Attempted to upload score when not logged in (scorekey: \"{}\")",
		  hs->GetScoreKey().c_str());
		if (callback)
			callback();
		return;
	}

	if (load_from_disk)
		hs->LoadReplayData();

	CURL* curlHandle = initCURLHandle(true, true, DO_COMPRESS);
	CURLAPIURL(curlHandle, CALL_PATH);

	Document jsonDoc;
	auto scoreDoc = ScoreToJSON(hs, true, jsonDoc.GetAllocator());
	auto json = base64_encode(compress_string(jsonObjectToString(scoreDoc)));
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POSTFIELDSIZE, json.length());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_COPYPOSTFIELDS, json.c_str());

	auto done = [hs, callback, load_from_disk, &CALL_ENDPOINT, this](
				  auto& req) {

		if (Handle401And429Response(
			CALL_ENDPOINT,
			req,
			[callback]() {
				if (callback)
					callback();
			},
			[hs, callback, load_from_disk, this]() {
				UploadScore(hs, callback, load_from_disk);
			})) {
			return;
		}

		Document d;
		auto parse = [&d, &req]() { return parseJson(d, req, "UploadScore"); };

		const auto& response = req.response_code;
		if (response == 200) {
			// all good
			if (parse()) {
				if (callback)
					callback();
				return;
			}

			if (d.HasMember("data")) {
				auto& data = d["data"];

				auto get = [&data](const char* name) {
					if (data.HasMember(name) && data[name].IsDouble()) {
						return data[name].GetDouble();
					} else {
						return 0.0;
					}
				};

				auto stream = get("stream");
				auto jumpstream = get("jumpstream");
				auto handstream = get("handstream");
				auto jacks = get("jacks");
				auto chordjacks = get("chordjacks");
				auto stamina = get("stamina");
				auto tech = get("technical");
				auto overall = get("overall");

				sessionRatings[Skill_Overall] = overall;
				sessionRatings[Skill_Stream] = stream;
				sessionRatings[Skill_Jumpstream] = jumpstream;
				sessionRatings[Skill_Handstream] = handstream;
				sessionRatings[Skill_JackSpeed] = jacks;
				sessionRatings[Skill_Chordjack] = chordjacks;
				sessionRatings[Skill_Stamina] = stamina;
				sessionRatings[Skill_Technical] = tech;

				Locator::getLogger()->info(
				  "UploadScore completed for score {} - \n\tOverall "
				  "{}\n\tStream {}\n\tJS "
				  "{}\n\tHS {}\n\tJacks {}\n\tCJ {}\n\tStamina {}\n\tTech {}",
				  hs->GetScoreKey(),
				  overall,
				  stream,
				  jumpstream,
				  handstream,
				  jacks,
				  chordjacks,
				  stamina,
				  tech);

				if (hs->GetWifeVersion() == 3)
					hs->AddUploadedServer(wife3_rescore_upload_flag);
				hs->AddUploadedServer(serverURL.Get());
				hs->forceuploadedthissession = true;

			} else {
				Locator::getLogger()->warn(
				  "Score upload response missing data field. Content: {}",
				  jsonObjectToString(d));
			}

		} else if (response == 422) {
			// missing info
			parse();

			if (d.HasMember("data") && d["data"].HasMember("errors")) {
				auto strReasons = jsonObjectToString(d["data"]["errors"]);
				Locator::getLogger()->error("UploadScore for {} has errors: {}",
											hs->GetScoreKey(),
											strReasons);

				hs->forceuploadedthissession = true;
				if (strReasons.find("not ranked") != std::string::npos) {
					// unranked. dont say it was uploaded
				} else {
					// even if the score was rejected, we tried our best
					if (hs->GetWifeVersion() == 3)
						hs->AddUploadedServer(wife3_rescore_upload_flag);
					hs->AddUploadedServer(serverURL.Get());
				}

				if (callback)
					callback();
				return;
			}

			Locator::getLogger()->warn(
				"UploadScore failed due to input error. Content: {}",
				jsonObjectToString(d));
		} else if (response == 404) {
			parse();

			Locator::getLogger()->warn(
				"UploadScore failed due to 404. Chart may be unranked - Content: {}",
				jsonObjectToString(d));
		} else {
			parse();

			Locator::getLogger()->warn("Unexpected status {} - Content: {}",
									   response,
									   jsonObjectToString(d));
		}

		if (callback)
			callback();
	};
	HTTPRequest* req = new HTTPRequest(
	  curlHandle, done, nullptr, [callback](auto& req) {
			if (callback)
			  callback();
	  });
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(CALL_ENDPOINT, *req)) {
		AddHttpRequestHandle(req->handle);
		HTTPRequests.push_back(req);
	}
	Locator::getLogger()->info("Finished creating UploadScore request for {}",
							   hs->GetScoreKey());
}

// this is for new/live played scores that have replaydata in memory
void
DownloadManager::UploadScoreWithReplayData(HighScore* hs)
{
	// (Without replay data loading from disk)
	UploadScore(
	  hs, []() {}, false);
}

// for older scores or newer scores that failed to upload using the above
// function we should probably do some refactoring of this
void
DownloadManager::UploadScoreWithReplayDataFromDisk(
  HighScore* hs,
  std::function<void()> callback)
{
	// (With replay data loading from disk)
	UploadScore(hs, callback, true);
}

// This function begins uploading the given list (deque) of scores
// It does so one score at a time, sequentially (But without blocking)
// So as to not spam the server with possibly hundreds or thousands of scores
// the way it does that is by using a callback and moving the remaining scores
// into the callback which calls this function again
// (So it is essentially kind of recursive, with the base case of an empty
// deque)
void
uploadScoresSequentially()
{
	Message msg("UploadProgress");
	msg.SetParam(
	  "percent",
	  1.f - (static_cast<float>(DLMAN->ScoreUploadSequentialQueue.size()) /
			 static_cast<float>(DLMAN->sequentialScoreUploadTotalWorkload)));
	MESSAGEMAN->Broadcast(msg);

	if (!DLMAN->ScoreUploadSequentialQueue.empty()) {

		auto work = []() {

			if (DLMAN == nullptr) {
				// a detached thread can be alive after the rest of the game
				return;
			}

			std::vector<HighScore*> hsToUpload{};
			{
				const std::lock_guard<std::mutex> lock(G_MTX_SCORE_UPLOAD);
				for (auto i = 0u; i < UPLOAD_SCORE_BULK_CHUNK_SIZE &&
								  !DLMAN->ScoreUploadSequentialQueue.empty();
					 i++) {
					hsToUpload.push_back(
					  DLMAN->ScoreUploadSequentialQueue.front());
					DLMAN->ScoreUploadSequentialQueue.pop_front();

					if (hsToUpload.size() >= UPLOAD_SCORE_BULK_CHUNK_SIZE) {
						break;
					}
				}
				runningSequentialScoreUpload = true;
			}

			DLMAN->UploadBulkScores(hsToUpload, uploadScoresSequentially);
		};
		std::thread worker(work);
		worker.detach();
	} else {
		Locator::getLogger()->info(
		  "Sequential score upload queue empty - uploads finished");
		runningSequentialScoreUpload = false;
		DLMAN->sequentialScoreUploadTotalWorkload = 0;
		MESSAGEMAN->Broadcast("SequentialScoreUploadFinished");
	}
}

// This starts the sequential uploading process if it has not already begun
void
startSequentialScoreUpload()
{
	if (DLMAN->ScoreUploadSequentialQueue.empty()) {
		Locator::getLogger()->info("Could not start sequential score upload "
								   "process - nothing to upload");
		return;
	}

	if (!runningSequentialScoreUpload) {
		Locator::getLogger()->info(
		  "Starting sequential score upload process - {} "
		  "scores split into chunks of {}",
		  DLMAN->ScoreUploadSequentialQueue.size(),
		  UPLOAD_SCORE_BULK_CHUNK_SIZE);
		uploadScoresSequentially();
	}
}

bool
DownloadManager::InitialScoreSync()
{
	if (!LoggedIn())
		return false;

	auto* profile =
	  PROFILEMAN->GetProfile(PLAYER_1);
	if (profile == nullptr) {
		return false;
	}

	auto lastCheckDT = profile->m_lastRankedChartkeyCheck;
	auto year0 = DateTime();
	if (lastCheckDT.tm_year == year0.tm_year &&
		lastCheckDT.tm_mon == year0.tm_mon &&
		lastCheckDT.tm_mday == year0.tm_mday) {
		// if the check never happened at all, set to a valid date

		// sets to 1900-01-01
		lastCheckDT.tm_year = 0;
		lastCheckDT.tm_mon = 0;
		lastCheckDT.tm_mday = 1;
	}

	// this runs after the GetRankedChartkeys request
	auto callback = [this, lastCheckDT, profile]() {
		// First we accumulate scores that have not been uploaded and have
		// replay data. There is no reason to upload updated calc versions to
		// the site anymore - the site uses its own calc and afaik ignores the
		// provided values, we only need to upload scores that have not been
		// uploaded, and scores that have been rescored from wife2 to wife3
		auto scores = SCOREMAN->GetAllPBsPreferringReplays();
		auto& newly_rescored = SCOREMAN->rescores;
		std::vector<HighScore*> toUpload;
		for (auto& s : scores) {
			// probably not worth uploading fails, they get rescored now
			if (s->GetGrade() == Grade_Failed)
				continue;

			// handle rescores, ignore upload check
			if (newly_rescored.count(s))
				toUpload.push_back(s);
			// normal behavior, upload scores that haven't been uploaded
			else if (!s->IsUploadedToServer(serverURL.Get()))
				toUpload.push_back(s);
			// if the chart was recently ranked, upload
			else if (newlyRankedChartkeys.contains(s->GetChartKey()))
				toUpload.push_back(s);
		}

		// set to yesterday because timezones and stuff
		profile->m_lastRankedChartkeyCheck = DateTime::GetYesterday();

		if (!toUpload.empty()) {
			Locator::getLogger()->info(
			  "Updating online scores. (Uploading {} scores)", toUpload.size());
		} else {
			Locator::getLogger()->info(
			  "Didn't find any scores to automatically upload");
			return false;
		}

		{
			const std::lock_guard<std::mutex> lock(G_MTX_SCORE_UPLOAD);
			ScoreUploadSequentialQueue.insert(ScoreUploadSequentialQueue.end(),
											  toUpload.begin(),
											  toUpload.end());
			sequentialScoreUploadTotalWorkload += toUpload.size();
		}
		startSequentialScoreUpload();
		return true;
	};

	// this will search the date range [start, inf]
	GetRankedChartkeys(callback, lastCheckDT);

	return true;
}

// manual upload function that will upload all scores for a chart
// that skips some of the constraints of the auto uploaders
bool
DownloadManager::ForceUploadPBsForChart(const std::string& ck, bool startNow)
{
	if (startNow)
		Locator::getLogger()->info("Trying ForceUploadPBsForChart - {}", ck);

	auto* cs = SCOREMAN->GetScoresForChart(ck);
	if (cs) {
		bool successful = false;
		auto& scores = cs->GetTopScoresForUploading();
		for (auto& s : scores) {
			if (!s->forceuploadedthissession) {
				if (s->GetGrade() != Grade_Failed) {
					// don't add stuff we're already uploading
					const std::lock_guard<std::mutex> lock(G_MTX_SCORE_UPLOAD);
					auto res = std::find(ScoreUploadSequentialQueue.begin(),
										 ScoreUploadSequentialQueue.end(),
										 s);
					if (res != ScoreUploadSequentialQueue.end())
						continue;

					ScoreUploadSequentialQueue.push_back(s);
					sequentialScoreUploadTotalWorkload += 1;
					successful = true;
				}
			}
		}
		if (!successful && startNow) {
			Locator::getLogger()->info(
			  "ForceUploadPBsForChart did not queue any scores for chart {}",
			  ck);
		} else if (startNow) {
			startSequentialScoreUpload();
		}
		return successful;
	} else {
		if (startNow)
			Locator::getLogger()->info(
			  "ForceUploadPBsForChart found no scores for chart {}", ck);
		return false;
	}
}
bool
DownloadManager::ForceUploadPBsForPack(const std::string& pack, bool startNow)
{
	Locator::getLogger()->info("Trying ForceUploadPBsForPack - {}", pack);

	bool successful = false;
	
	auto exec = [&successful,
				this](std::pair<vectorIt<std::string>, vectorIt<std::string>> workload,
					ThreadData* data) {
		for (auto it = workload.first; it != workload.second; it++) {
			auto& ck = *it;
			successful |= ForceUploadPBsForChart(ck, false);
		}
	};
	std::set<std::string> s{};
	for (auto& song : SONGMAN->GetSongs(pack)) {
		for (auto& steps : song->GetAllSteps()) {
			s.emplace(steps->GetChartKey());
		}
	}
	std::vector<std::string> sv(s.begin(), s.end());
	parallelExecution<std::string>(sv, exec);

	if (!successful) {
		Locator::getLogger()->info(
		  "ForceUploadPBsForPack did not queue any scores for pack {}", pack);
	} else if (startNow) {
		startSequentialScoreUpload();
	}
	return successful;
}
bool
DownloadManager::ForceUploadAllPBs()
{
	Locator::getLogger()->info("Trying ForceUploadAllPBs");

	bool successful = false;

	auto exec = [&successful, this](std::pair<vectorIt<std::string>, vectorIt<std::string>> workload, ThreadData* data) {
		for (auto it = workload.first; it != workload.second; it++) {
			auto& ck = *it;
			successful |= ForceUploadPBsForChart(ck, false);
		}
	};
	std::vector<std::string> s{};
	s.reserve(SONGMAN->StepsByKey.size());
	for (auto it = SONGMAN->StepsByKey.begin(); it != SONGMAN->StepsByKey.end(); it++) {
		s.push_back(it->first);
	}
	parallelExecution<std::string>(s, exec);

	if (!successful) {
		Locator::getLogger()->info(
		  "ForceUploadAllPBs did not queue any scores");
	} else {
		startSequentialScoreUpload();
	}
	return successful;
}

OnlineTopScore
DownloadManager::GetTopSkillsetScore(unsigned int rank,
									 Skillset ss,
									 bool& result)
{
	unsigned int index = rank - 1;
	if (index < topScores[ss].size()) {
		result = true;
		return topScores[ss][index];
	}
	result = false;
	return OnlineTopScore();
}

void
DownloadManager::GetReplayDataRequest(const std::string& scoreid,
								   int userid,
								   const std::string& username,
								   const std::string& chartkey,
								   LuaReference& callback)
{
	constexpr auto& CALL_ENDPOINT = API_GET_SCORE;
	const auto CALL_PATH = fmt::format(API_GET_SCORE, scoreid);

	Locator::getLogger()->info(
	  "Generating GetReplayData request for scoreid {} - {}", scoreid, chartkey);

	auto runLuaFunc =
	  [callback](std::vector<std::pair<float, float>>& replayData,
				 const std::vector<OnlineScore>::iterator& it,
				 const std::vector<OnlineScore>::iterator& itEnd) {
		  
		  if (!callback.IsNil() && callback.IsSet()) {
			  Locator::getLogger()->info(
				"GetReplayData finished - running callback function");
			  auto L = LUA->Get();
			  callback.PushSelf(L);
			  std::string Error =
				"Error running GetReplayData Finish Function: ";
			  if (it != itEnd)
				  it->hs.PushSelf(L);
			  else
				  lua_pushnil(L);
			  // 1 args, 0 results
			  LuaHelpers::RunScriptOnStack(L, Error, 1, 0, true);
			  LUA->Release(L);
		  } else {
			  Locator::getLogger()->info(
				"GetReplayData finished, but no callback was set");
		  }
	  };

	auto done = [runLuaFunc,
				 scoreid,
				 userid,
				 username,
				 chartkey,
				 &callback,
				 &CALL_ENDPOINT,
				 this](auto& req) {
		std::vector<std::pair<float, float>> replayData{};
		auto& lbd = chartLeaderboards[chartkey];

		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  [runLuaFunc, &replayData, &lbd]() {
				  runLuaFunc(replayData, lbd.end(), lbd.end());
			  },
			  [scoreid, userid, username, chartkey, &callback, this]() {
				  GetReplayData(
				scoreid, userid, username, chartkey, callback);
			  })) {
			return;
		}

		const auto& response = req.response_code;
		if (response == 404) {
			// no score was found
			Locator::getLogger()->warn(
			  "GetReplayData 404'd because there is no score id {} (ck {})",
			  scoreid, chartkey);
			runLuaFunc(replayData, lbd.end(), lbd.end());
			return;
		}

		
		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "GetReplayData");
		};
		// for some reason this function never exited early on parse error
		// so dont change that ..?
		parse();

		if (response == 200) {
			// found a score. maybe it has replay data

			if (d.HasMember("data") && d["data"].IsObject()) {

				auto& data = d["data"];
				if (data.HasMember("replay_data") &&
					data["replay_data"].IsArray()) {
					//
					// Old replays.
					// The site will return this if no InputData was uploaded
					//
					auto replayArr = data["replay_data"].GetArray();

					auto it =
					  find_if(lbd.begin(),
							  lbd.end(),
							  [userid, username, scoreid](OnlineScore& a) {
								  return a.userid == userid &&
										 a.username == username &&
										 a.scoreid == scoreid;
							  });

					if (it == lbd.end()) {
						Locator::getLogger()->warn(
						  "GetReplayData could not save replay data "
						  "because scoreid {} was not found stored in a chart "
						  "leaderboard for {}",
						  scoreid,
						  chartkey);
					} else {

						auto& score = it->hs;
						std::vector<float> timestamps;
						std::vector<float> offsets;
						std::vector<int> tracks;
						std::vector<int> rows;
						std::vector<TapNoteType> types;

						for (auto& note : replayArr) {
							if (!note.IsArray() || note.Size() < 2 ||
								!note[0].IsNumber() || !note[1].IsNumber())
								continue;
							replayData.push_back(std::make_pair(
							  note[0].GetFloat(), note[1].GetFloat()));

							timestamps.push_back(note[0].GetFloat());
							// horrid temp hack --
							// EO keeps misses as 180ms bads for not a great
							// reason convert them back to misses here
							auto offset = note[1].GetFloat() / 1000.F;
							if (offset == .18F)
								offset = 1.F;
							offsets.push_back(offset);
							if (note.Size() == 3 &&
								note[2].IsInt()) { // pre-0.6 with noterows
								rows.push_back(note[2].GetInt());
							}
							if (note.Size() > 3 && note[2].IsInt() &&
								note[3].IsInt()) { // 0.6 without noterows
								tracks.push_back(note[2].GetInt());
								types.push_back(
								  static_cast<TapNoteType>(note[3].GetInt()));
							}
							if (note.Size() == 5 &&
								note[4].IsInt()) { // 0.6 with noterows
								rows.push_back(note[4].GetInt());
							}
						}
						score.SetOnlineReplayTimestampVector(timestamps);
						score.SetOffsetVector(offsets);
						score.SetTrackVector(tracks);
						score.SetTapNoteTypeVector(types);
						score.SetNoteRowVector(rows);

						runLuaFunc(replayData, it, lbd.end());
						return;
					}
				} else if (data.HasMember("replay_data") && data["replay_data"].IsObject()) {
					//
					// InputData
					// The site sends this if it receives it on upload
					//
					auto replayObj = data["replay_data"].GetObj();

					auto it =
					  find_if(lbd.begin(),
							  lbd.end(),
							  [userid, username, scoreid](OnlineScore& a) {
								  return a.userid == userid &&
										 a.username == username &&
										 a.scoreid == scoreid;
							  });

					if (it == lbd.end()) {
						Locator::getLogger()->warn(
						  "GetReplayData could not save "
						  "replay data "
						  "because scoreid {} was not "
						  "found stored in a chart "
						  "leaderboard for {}",
						  scoreid,
						  chartkey);
					} else {
						auto& score = it->hs;
						auto* replay = score.GetReplay();

						auto mods = getJsonString(replayObj, "mods");
						auto chartkey = getJsonString(replayObj, "chartkey");
						auto scorekey = "Online_" + getJsonString(replayObj, "scorekey");
						auto musicrate = getJsonFloat(replayObj, "music_rate");
						auto songoffset = getJsonFloat(replayObj, "song_offset");
						auto globaloffset = getJsonFloat(replayObj, "global_offset");
						auto rngseed = getJsonInt(replayObj, "rng_seed");
						std::vector<InputDataEvent> inputDataEvents{};
						std::vector<MissReplayResult> missDataEvents{};
						std::vector<HoldReplayResult> holdDataEvents{};
						std::vector<MineReplayResult> mineDataEvents{};

						if (replayObj.HasMember("data") &&
							replayObj["data"].IsArray()) {
							auto inputArr = replayObj["data"].GetArray();
							for (auto inIt = inputArr.Begin();
								inIt != inputArr.End(); inIt++) {
								auto evt = inIt->GetObj();

								auto column = getJsonInt(evt, "column");
								auto ispress = getJsonBool(evt, "is_press");
								auto musicseconds = getJsonFloat(evt, "timestamp");
								auto nearestnoterow = getJsonInt(evt, "nearest_noterow");
								auto offsetfromnearest = getJsonFloat(
								  evt, "offset_from_nearest_noterow");
								auto nearestnotetype = static_cast<TapNoteType>(
								  getJsonInt(evt, "nearest_notetype"));
								auto nearestnotesubtype =
								  static_cast<TapNoteSubType>(
									getJsonInt(evt, "nearest_notesubtype"));

								inputDataEvents.emplace_back(
								  ispress,
								  column,
								  musicseconds,
								  nearestnoterow,
								  offsetfromnearest,
								  nearestnotetype,
								  nearestnotesubtype);
							}
						}

						if (replayObj.HasMember("misses") &&
							replayObj["misses"].IsArray()) {
							auto missArr = replayObj["misses"].GetArray();
							for (auto missIt = missArr.Begin();
								 missIt != missArr.End();
								 missIt++) {
								auto evt = missIt->GetObj();

								auto column = getJsonInt(evt, "column");
								auto row = getJsonInt(evt, "row");
								auto notetype =
								  static_cast<TapNoteType>(getJsonInt(evt, "notetype"));
								auto notesubtype = static_cast<TapNoteSubType>(
								  getJsonInt(evt, "notesubtype"));

								missDataEvents.emplace_back(
								  row, column, notetype, notesubtype);
							}
						}

						if (replayObj.HasMember("mine_hits") &&
							replayObj["mine_hits"].IsArray()) {
							auto mineArr = replayObj["mine_hits"].GetArray();
							for (auto mineIt = mineArr.Begin();
								 mineIt != mineArr.End();
								 mineIt++) {
								auto evt = mineIt->GetObj();

								auto column = getJsonInt(evt, "column");
								auto row = getJsonInt(evt, "row");

								MineReplayResult mrr;
								mrr.row = row;
								mrr.track = column;
								mineDataEvents.push_back(mrr);
							}
						}

						if (replayObj.HasMember("hold_drops") &&
							replayObj["hold_drops"].IsArray()) {
							auto holdArr = replayObj["hold_drops"].GetArray();
							for (auto holdIt = holdArr.Begin();
								 holdIt != holdArr.End();
								 holdIt++) {
								auto evt = holdIt->GetObj();

								auto column = getJsonInt(evt, "column");
								auto row = getJsonInt(evt, "row");
								auto subtype = static_cast<TapNoteSubType>(
								  getJsonInt(evt, "subtype"));

								HoldReplayResult hrr;
								hrr.row = row;
								hrr.track = column;
								hrr.subType = subtype;
								holdDataEvents.push_back(hrr);
							}
						}

						replay->SetModifiers(mods);
						replay->SetChartKey(chartkey);
						replay->SetScoreKey(scorekey);
						replay->SetMusicRate(musicrate);
						replay->SetSongOffset(songoffset);
						replay->SetGlobalOffset(globaloffset);
						replay->SetRngSeed(rngseed);
						replay->SetInputDataVector(inputDataEvents);
						replay->SetMissReplayDataVector(missDataEvents);
						replay->SetHoldReplayDataVector(holdDataEvents);
						replay->SetMineReplayDataVector(mineDataEvents);

						Locator::getLogger()->info(
						  "Read online InputData Replay with info: mods {} | "
						  "ck {} | sk {} | rate {} | offset {} | globaloffset "
						  "{} | rng {}",
						  mods,
						  chartkey,
						  scorekey,
						  musicrate,
						  songoffset,
						  globaloffset,
						  rngseed);

						if (!replay->GeneratePrimitiveVectors()) {
							Locator::getLogger()->warn(
							  "Failed to convert online InputData for score {} "
							  "({}) to usable "
							  "data ingame",
							  scoreid,
							  scorekey);
						}

						runLuaFunc(replayData, it, lbd.end());
						return;
					}
				} else {
					Locator::getLogger()->warn(
					  "GetReplayData didn't find replay data for score {}  "
					  "(ck {}) - Object Missing? {}",
					  scoreid,
					  chartkey,
					  data.HasMember("replay_data"));
				}

			} else {
				Locator::getLogger()->warn("GetReplayData got unexpected "
										   "response body - Content: {}",
										   jsonObjectToString(d));
			}
		} else {
			Locator::getLogger()->warn(
			  "GetReplayData unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}

		// anything falling through to here has no replay data
		runLuaFunc(replayData, lbd.end(), lbd.end());
	};

	SendRequest(CALL_PATH, CALL_ENDPOINT, {}, done, true);
	Locator::getLogger()->info(
	  "Finished creating GetReplayData request for scoreid {} - {}",
	  scoreid,
	  chartkey);
}

void
DownloadManager::GetChartLeaderboardRequest(const std::string& chartkey,
										 LuaReference& ref)
{
	constexpr auto& CALL_ENDPOINT = API_CHART_LEADERBOARD;
	const auto CALL_PATH = fmt::format(API_CHART_LEADERBOARD, chartkey);

	Locator::getLogger()->info("Generating GetChartLeaderboard request for {}",
							   chartkey);

	std::vector<std::pair<std::string, std::string>> params = {
		//std::make_pair("sort", ""),
		std::make_pair("limit", "999999"),
		//std::make_pair("page", "")
	};

	std::vector<OnlineScore>& vec = chartLeaderboards[chartkey];

	auto runLuaFunc = [ref](HTTPRequest& req, std::vector<OnlineScore>& vec) {
		if (!ref.IsNil() && ref.IsSet()) {
			Locator::getLogger()->info(
			  "GetChartLeaderboard finished - running callback function");
			auto L = LUA->Get();
			ref.PushSelf(L);
			if (!lua_isnil(L, -1)) {
				std::string Error =
				  "Error running GetChartLeaderboard Finish Function: ";
				const auto& response = req.response_code;

				// 404: Chart not ranked
				// 401: Invalid login token
				// 403: User is banned
				// 429: Rate Limited
				if (response == 404 || response == 401 || response == 403) {
					lua_pushnil(L);
					// nil output means unranked to Lua
				} else {
					// expecting only 200 as the alternative
					// 200: success
					lua_newtable(L);
					for (unsigned i = 0; i < vec.size(); ++i) {
						auto& s = vec[i];
						s.Push(L);
						lua_rawseti(L, -2, i + 1);
					}
					// table size of 0 means ranked but no scores
					// any larger table size means ranked with scores (duh)
				}
				LuaHelpers::RunScriptOnStack(
				  L, Error, 1, 0, true); // 1 args, 0 results
			}
			LUA->Release(L);
		} else {
			Locator::getLogger()->info(
			  "GetChartLeaderboard finished, but no callback was set");
		}
	};

	auto done = [&ref, chartkey, runLuaFunc, &vec, &CALL_ENDPOINT, this](
				  auto& req) {

		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  [runLuaFunc, &req, &vec]() {
				runLuaFunc(req, vec);
			  },
			  [chartkey, &ref, this]() {
				  GetChartLeaderboardRequest(chartkey, ref);
			  })) {
			return;
		}

		const auto& response = req.response_code;
		if (response == 404) {
			// chart is unranked
			unrankedCharts.emplace(chartkey);
			runLuaFunc(req, vec);

			Locator::getLogger()->warn("GetChartLeaderboard 404'd because "
									   "this chart is unranked: {}",
									   chartkey);
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "GetChartLeaderboard");
		};

		if (response == 200) {
			// chart is ranked. leaderboard has [0,inf] scores
			if (parse()) {
				runLuaFunc(req, vec);
				return;
			}

			if (d.HasMember("data") && d["data"].IsArray()) {

				vec.clear();
				auto count = 0;
				auto& data = d["data"];
				for (auto it = data.Begin(); it != data.End(); it++) {
					count++;
					vec.push_back(jsonToOnlineScore(*it, chartkey));
				}
				unrankedCharts.erase(chartkey);
				runLuaFunc(req, vec);

				Locator::getLogger()->info(
				  "GetChartLeaderboard for {} succeeded - {} scores found",
				  chartkey,
				  count);
			} else {
				Locator::getLogger()->warn(
				  "GetChartLeaderboard got unexpected response body - "
				  "Content: {}",
				  jsonObjectToString(d));
			}
		} else {
			parse();

			Locator::getLogger()->warn(
			  "GetChartLeaderboard unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(CALL_PATH, CALL_ENDPOINT, params, done, true);
	Locator::getLogger()->info(
	  "Finished creating GetChartLeaderboard request for {}", chartkey);
}

std::vector<DownloadablePack*>
DownloadManager::GetCoreBundle(const std::string& whichoneyo)
{
	return std::vector<DownloadablePack*>();
}

void
DownloadManager::DownloadCoreBundle(const std::string& whichoneyo, bool mirror)
{
	auto bundle = GetCoreBundle(whichoneyo);
	sort(bundle.begin(),
		 bundle.end(),
		 [](DownloadablePack* x1, DownloadablePack* x2) {
			 return x1->size < x2->size;
		 });
	for (auto pack : bundle)
		DownloadQueue.push_back(std::make_pair(pack, mirror));
}

void
DownloadManager::RefreshLastVersion()
{
	Locator::getLogger()->info("RefreshLastVersion beginning to determine "
							   "latest game version according to API");

	std::vector<std::pair<std::string, std::string>> params = {};

	auto done = [this](auto& req) {
		if (HandleRatelimitResponse(API_USER, req)) {
			RefreshUserData();
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "RefreshLastVersion");
		};
		if (parse()) {
			return;
		}

		const auto& response = req.response_code;
		if (response == 200) {

			lastVersion = getJsonString(d, "game_version");

			Locator::getLogger()->info(
			  "RefreshLastVersion successful - Last Version is {}",
			  lastVersion);
		} else {
			Locator::getLogger()->error(
			  "RefreshLastVersion Error: Unexpected status {} - content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(API_GAME_VERSION,
				params,
				done,
				false,
				RequestMethod::GET,
				DONT_COMPRESS,
				true,
				false);
}

void
DownloadManager::RequestTop25(Skillset ss)
{
	constexpr auto& CALL_ENDPOINT = API_USER_SCORES;
	const auto CALL_PATH = fmt::format(API_USER_SCORES, sessionUser);
	
	std::string ssstr = "";
	switch (ss) {
		case Skill_Stream:
			ssstr = "stream";
			break;
		case Skill_Jumpstream:
			ssstr = "jumpstream";
			break;
		case Skill_Handstream:
			ssstr = "handstream";
			break;
		case Skill_Stamina:
			ssstr = "stamina";
			break;
		case Skill_JackSpeed:
			ssstr = "jacks";
			break;
		case Skill_Chordjack:
			ssstr = "chordjacks";
			break;
		case Skill_Technical:
			ssstr = "technical";
			break;
		default:
			ssstr = "overall";
			break;
	}

	Locator::getLogger()->info(
	  "Generating RequestTop25 request for skillset {} ({})",
	  SkillsetToString(ss),
	  ssstr);

	std::vector<std::pair<std::string, std::string>> params = {
		std::make_pair("sort", fmt::format("-{}", ssstr)),
	};

	auto done = [ss, ssstr, &CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [ss, this]() { RequestTop25(ss); })) {
			return;
		}

		const auto& response = req.response_code;

		auto& vec = topScores[ss];
		vec.clear();

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "RequestTop25");
		};

		if (response == 200) {

			// there is a problem..
			if (parse()) {
				return;
			}

			if (d.HasMember("data") && d["data"].IsArray()) {
				auto datarr = d["data"].GetArray();

				for (auto& hs : datarr) {
					if (!hs.IsObject() || !hs.HasMember("song") ||
						!hs.HasMember("chart")) {
						Locator::getLogger()->warn(
						  "RequestTop25 score in response is wrong type or "
						  "missing metadata");
						continue;
					}

					auto song = hs["song"].GetObj();
					auto chart = hs["chart"].GetObj();

					OnlineTopScore tmp;
					tmp.songName = getJsonString(song, "name");
					tmp.wifeScore = getJsonFloat(hs, "wife") / 100.F;
					tmp.overall = getJsonFloat(hs, "overall");
					if (ss != Skill_Overall) {
						tmp.ssr = getJsonFloat(hs, ssstr.c_str());
					}
					else {
						tmp.ssr = tmp.overall;
					}
					tmp.chartkey = getJsonString(chart, "key");
					tmp.scorekey = getJsonString(hs, "key");
					tmp.rate = getJsonFloat(hs, "rate");
					tmp.difficulty =
					  StringToDifficulty(getJsonString(chart, "difficulty"));
					
					vec.push_back(tmp);
				}
			}
			else {
				Locator::getLogger()->warn("RequestTop25 for skillset {} - "
										   "return incorrect? Content: {}",
										   SkillsetToString(ss),
										   jsonObjectToString(d));
			}
			Locator::getLogger()->info(
			  "RequestTop25 found {} scores for skillset {}",
			  vec.size(),
			  SkillsetToString(ss));
		}
		else {
			parse();
			Locator::getLogger()->warn("RequestTop25 for skillset {} ({}) "
									   "unexpected response {} - Content: {}",
									   SkillsetToString(ss),
									   ssstr,
									   response,
									   jsonObjectToString(d));
		}
		MESSAGEMAN->Broadcast("OnlineUpdate");
	};

	SendRequest(CALL_PATH, CALL_ENDPOINT, params, done, true);
	Locator::getLogger()->info(
	  "Finished creating RequestTop25 request for skillset {}",
	  SkillsetToString(ss));
}

void
DownloadManager::RefreshUserData()
{
	if (!LoggedIn())
		return;

	constexpr auto& CALL_ENDPOINT = API_USER;
	const auto CALL_PATH = fmt::format(API_USER, sessionUser);

	Locator::getLogger()->info("Refreshing UserData for {}", sessionUser);

	std::vector<std::pair<std::string, std::string>> params = {};

	auto done = [&CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			CALL_ENDPOINT,
			req,
			[]() {},
			[this]() { RefreshUserData(); })) {
			return;
		}

		const auto& response = req.response_code;
		if (response == 404) {
			// user doesnt exist :eyebrow_raise:
			Locator::getLogger()->warn(
			  "RefreshUserData 404'd because user does not exist: {}",
			  sessionUser);
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "RefreshUserData");
		};

		if (response == 200) {
			// it probably worked
			if (parse())
				return;

			if (d.HasMember("data") && d["data"].IsObject()) {

				auto count = 0;
				auto& data = d["data"];

				sessionRatings[Skill_Overall] = getJsonFloat(data, "overall");
				sessionRatings[Skill_Stream] = getJsonFloat(data, "stream");
				sessionRatings[Skill_Jumpstream] =
				  getJsonFloat(data, "jumpstream");
				sessionRatings[Skill_Handstream] =
				  getJsonFloat(data, "handstream");
				sessionRatings[Skill_Stamina] =
				  getJsonFloat(data, "stamina");
				sessionRatings[Skill_JackSpeed] = getJsonFloat(data, "jacks");
				sessionRatings[Skill_Chordjack] =
				  getJsonFloat(data, "chordjacks");
				sessionRatings[Skill_Technical] =
				  getJsonFloat(data, "technical");

				countryCode = getJsonString(data, "country");

				sessionRanks[Skill_Overall] = getJsonInt(data, "rank");

				if (data.HasMember("skillset_ranks") &&
					data["skillset_ranks"].IsObject()) {
					auto ssranks = data["skillset_ranks"].GetObj();

					sessionRanks[Skill_Stream] = getJsonInt(ssranks, "stream");
					sessionRanks[Skill_Jumpstream] = getJsonInt(ssranks, "jumpstream");
					sessionRanks[Skill_Handstream] = getJsonInt(ssranks, "handstream");
					sessionRanks[Skill_Stamina] = getJsonInt(ssranks, "stamina");
					sessionRanks[Skill_JackSpeed] =
					  getJsonInt(ssranks, "jacks");
					sessionRanks[Skill_Chordjack] = getJsonInt(ssranks, "chordjacks");
					sessionRanks[Skill_Technical] =
					  getJsonInt(ssranks, "technical");
				}
				else {
					Locator::getLogger()->warn(
					  "RefreshUserData was missing skillsetRanks");
				}

				Locator::getLogger()->info(
				  "RefreshUserData for {} succeeded - Rank {} - Overall {}",
				  sessionUser,
				  sessionRanks[Skill_Overall],
				  sessionRatings[Skill_Overall]);

				MESSAGEMAN->Broadcast("OnlineUpdate");
			} else {
				Locator::getLogger()->warn(
				  "RefreshUserData got unexpected response body - "
				  "Content: {}",
				  jsonObjectToString(d));
			}
		} else {
			parse();

			Locator::getLogger()->warn(
			  "RefreshUserData unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(CALL_PATH, CALL_ENDPOINT, params, done, true);
}

int
DownloadManager::GetSkillsetRank(Skillset ss)
{
	if (!LoggedIn())
		return 0;
	return sessionRanks[ss];
}

float
DownloadManager::GetSkillsetRating(Skillset ss)
{
	if (!LoggedIn())
		return 0.0f;
	return static_cast<float>(sessionRatings[ss]);
}
void
DownloadManager::RefreshPackList(const std::string& url)
{
	if (url.empty())
		return;

	/**
	*****
	*****
	*****
	THIS METHOD IS UNUSED
	*****
	*****
	*****
	*/


	const auto& CALL_ENDPOINT = url;
	const auto& CALL_PATH = url;

	Locator::getLogger()->info("Generating RefreshPackList via API: {}",
							   CALL_PATH);

	auto done = [url, &CALL_ENDPOINT, this](auto& req) {

		if (Handle401And429Response(
			CALL_ENDPOINT,
			req,
			[]() {},
			[url, this]() { RefreshPackList(url); })) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "RefreshPackList");
		};

		const auto& response = req.response_code;
		if (response == 200) {
			parse();
			if (!d.HasMember("data") || !d["data"].IsArray()) {
				Locator::getLogger()->error(
				  "RefreshPackList Error: response data was missing or not an "
				  "array - content: {}",
				  jsonObjectToString(d));
				return;
			}

			auto& data = d["data"];
			auto& packlist = downloadablePacks;

			for (auto& pack : data.GetArray()) {
				try {
					DownloadablePack packDl;

					packDl.id = getJsonInt(pack, "id");
					packDl.name = getJsonString(pack, "name");
					packDl.url = getJsonString(pack, "download");
					packDl.mirror = getJsonString(pack, "mirror");
					if (packDl.url.empty()) {
						packDl.url = packDl.mirror;
					}
					if (packDl.mirror.empty()) {
						packDl.mirror = packDl.url;
					}
					packDl.avgDifficulty =
					  std::stof(getJsonString(pack, "overall"));
					packDl.size = getJsonInt(pack, "size");
					packDl.plays = getJsonInt(pack, "play_count");
					packDl.songs = getJsonInt(pack, "song_count");
					packDl.bannerUrl = getJsonString(pack, "banner_path");

					auto thumbnail = getJsonString(pack, "bannerTinyThumb");
					if (thumbnail.find("base64,") != std::string::npos) {
						packDl.thumbnail = thumbnail;
					}
					else {
						packDl.thumbnail = "";
					}

					if (packDl.name.empty()) {
						Locator::getLogger()->warn(
						  "RefreshPackList missing pack name: {}",
						  jsonObjectToString(pack));
						continue;
					}
					if (packDl.url.empty()) {
						Locator::getLogger()->warn(
						  "RefreshPackList missing pack download: {}",
						  jsonObjectToString(pack));
						continue;
					}

					if (!packlist.contains(packDl.id)) {
						packlist[packDl.id] = packDl;
					}
				}
				catch (std::exception& e) {
					Locator::getLogger()->error(
					  "RefreshPackList parse exception - {} - pack content: {}",
					  e.what(),
					  jsonObjectToString(pack));
				}
			}
			if (MESSAGEMAN != nullptr) {
				MESSAGEMAN->Broadcast("PackListRefreshed");
			}
		} else {
			parse();
			Locator::getLogger()->error(
			  "RefreshPackList unexpected response code {} - content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};
	SendRequestToURL(CALL_PATH,
					 CALL_ENDPOINT,
					 {},
					 done,
					 false,
					 RequestMethod::GET,
					 DONT_COMPRESS,
					 true,
					 false);
}

inline std::string
ApiSearchCriteriaToJSONBody(const ApiSearchCriteria& criteria)
{
	Document d;
	Document::AllocatorType& allocator = d.GetAllocator();
	d.SetObject();

	Value arrDoc(kArrayType);

	std::string sortByField = "name";
	if (!criteria.sortBy.empty()) {
		if (criteria.sortBy != sortByField) {
			// this allows even further custom sorting
			// if you know what you are doing...
			sortByField =
			  fmt::format("{}:{},name:asc",
						  criteria.sortBy,
						  criteria.sortIsAscending ? "asc" : "desc");
		}
		else {
			// this is always "name:XXXX"
			sortByField =
			  fmt::format("{}:{}",
						  criteria.sortBy,
						  criteria.sortIsAscending ? "asc" : "desc");
		}
	}

	if (!criteria.packName.empty() || !criteria.packTags.empty()) {
		Document packSearchDoc;
		packSearchDoc.SetObject();

		packSearchDoc.AddMember("collection", "packs", allocator);
		packSearchDoc.AddMember("q", stringToVal(criteria.packName, allocator), allocator);
		packSearchDoc.AddMember("query_by", "name", allocator);
		packSearchDoc.AddMember("num_typos", "0", allocator);
		packSearchDoc.AddMember(
		  "sort_by", stringToVal(sortByField, allocator), allocator);
		if (!criteria.packTags.empty()) {
			// this should produce tags:=[`4k`,`5k`] ....
			auto tagstr = "tags:=[`" + join("`,`", criteria.packTags) + "`]";
			packSearchDoc.AddMember("filter_by", stringToVal(tagstr, allocator), allocator);
		}
		arrDoc.PushBack(packSearchDoc, allocator);
	}
	if (!criteria.songName.empty()) {
		Document songNameSearchDoc;
		songNameSearchDoc.SetObject();

		songNameSearchDoc.AddMember("collection", "songs", allocator);
		songNameSearchDoc.AddMember(
		  "q", stringToVal(criteria.songName, allocator), allocator);
		songNameSearchDoc.AddMember("query_by", "name", allocator);
		songNameSearchDoc.AddMember(
		  "sort_by", stringToVal(sortByField, allocator), allocator);
		arrDoc.PushBack(songNameSearchDoc, allocator);
	}
	if (!criteria.chartAuthor.empty()) {
		Document authorNameSearchDoc;
		authorNameSearchDoc.SetObject();

		authorNameSearchDoc.AddMember("collection", "songs", allocator);
		authorNameSearchDoc.AddMember(
		  "q", stringToVal(criteria.chartAuthor, allocator), allocator);
		authorNameSearchDoc.AddMember("query_by", "author", allocator);
		authorNameSearchDoc.AddMember(
		  "sort_by", stringToVal(sortByField, allocator), allocator);
		arrDoc.PushBack(authorNameSearchDoc, allocator);
	}
	if (!criteria.songArtist.empty()) {
		Document artistNameSearchDoc;
		artistNameSearchDoc.SetObject();

		artistNameSearchDoc.AddMember("collection", "songs", allocator);
		artistNameSearchDoc.AddMember(
		  "q", stringToVal(criteria.chartAuthor, allocator), allocator);
		artistNameSearchDoc.AddMember("query_by", "artist", allocator);
		artistNameSearchDoc.AddMember(
		  "sort_by", stringToVal(sortByField, allocator), allocator);
		arrDoc.PushBack(artistNameSearchDoc, allocator);
	}

	// if the criteria is empty, just return all packs
	if (arrDoc.Empty()) {
		Document unfilteredSearchDoc;
		unfilteredSearchDoc.SetObject();
		unfilteredSearchDoc.AddMember("collection", "packs", allocator);
		unfilteredSearchDoc.AddMember("q", "", allocator);
		unfilteredSearchDoc.AddMember("query_by", "name", allocator);
		unfilteredSearchDoc.AddMember(
		  "sort_by", stringToVal(sortByField, allocator), allocator);
		arrDoc.PushBack(unfilteredSearchDoc, allocator);
	}

	d.AddMember("searches", arrDoc, allocator);
	StringBuffer buffer;
	Writer<StringBuffer> w(buffer);
	d.Accept(w);
	return buffer.GetString();
}

void
DownloadManager::MultiSearchRequest(
  ApiSearchCriteria searchCriteria,
  std::function<void(Document&)> whenDoneParser)
{
	constexpr auto& CALL_ENDPOINT = API_SEARCH;
	constexpr auto& CALL_PATH = API_SEARCH;

	Locator::getLogger()->info("Generating MultiSearchRequest for {}", searchCriteria.DebugString());

	std::vector<std::pair<std::string, std::string>> params = {
		std::make_pair("page", std::to_string(searchCriteria.page + 1)),
		std::make_pair("per_page", std::to_string(searchCriteria.per_page)),
	};

	auto url = CALL_PATH + "?";
	for (auto& param : params)
		url += param.first + "=" + param.second + "&";
	url = url.substr(0, url.length() - 1);

	CURL* curlHandle = initCURLHandle(true, true, DONT_COMPRESS);
	CURLSEARCHURL(curlHandle, url);

	auto body = ApiSearchCriteriaToJSONBody(searchCriteria);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POSTFIELDSIZE, body.length());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_COPYPOSTFIELDS, body.c_str());

	// Locator::getLogger()->info("url: {}", url);
	// Locator::getLogger()->info("body: {}", body);

	auto done = [searchCriteria, whenDoneParser, &CALL_ENDPOINT, this](
				  auto& req) {

		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [searchCriteria, whenDoneParser, this]() {
				  MultiSearchRequest(searchCriteria, whenDoneParser);
			  })) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "MultiSearch");
		};

		parse();
		whenDoneParser(d);
	};

	HTTPRequest* req = new HTTPRequest(
	  curlHandle, done, nullptr, [whenDoneParser](auto& req) {
		  if (whenDoneParser) {
			  Document d;
			  whenDoneParser(d);
		  }
	  });
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(CALL_ENDPOINT, *req)) {
		AddHttpRequestHandle(req->handle);
		HTTPRequests.push_back(req);
	}
	Locator::getLogger()->info(
	  "Finished creating MultiSearch request for {}", searchCriteria.DebugString());

}

void
DownloadManager::RefreshPackTags() {
	Locator::getLogger()->info("Refreshing Pack Tags");
	if (!packTags.empty()) {
		Locator::getLogger()->info(
		  "Pack tags are already loaded. No request made");
		return;
	}
	GetPackTagsRequest();
}

void
DownloadManager::GetPackTagsRequest() {
	constexpr auto& CALL_ENDPOINT = API_TAGS;
	constexpr auto& CALL_PATH = API_TAGS;

	Locator::getLogger()->info("Generating GetPackTagsRequest");

	auto done = [&CALL_ENDPOINT, this](auto& req) {
		if (Handle401And429Response(
			  CALL_ENDPOINT,
			  req,
			  []() {},
			  [this]() { GetPackTagsRequest(); })) {
			return;
		}

		Document d;
		// return true if parse error
		auto parse = [&d, &req]() {
			return parseJson(d, req, "GetPackTags");
		};

		const auto& response = req.response_code;
		if (response == 200) {
			parse();
			if (!d.HasMember("data") || !d["data"].IsObject()) {
				Locator::getLogger()->error(
				  "GetPackTagsRequest Error: response data was missing or not an "
				  "object - content: {}",
				  jsonObjectToString(d));
				return;
			}

			auto& data = d["data"];
			for (auto it = data.MemberBegin(); it != data.MemberEnd(); it++) {

				auto tagType = it->name.GetString();
				if (!it->value.IsArray()) {
					Locator::getLogger()->warn(
					  "Found tagType {} was not array. Data: {}",
					  tagType,
					  jsonObjectToString(it->value));
					continue;
				}

				auto tags = it->value.GetArray();
				packTags[tagType] = std::vector<std::string>();
				for (auto tit = tags.Begin(); tit != tags.End(); tit++) {
					packTags[tagType].push_back(tit->GetString());
				}

				Locator::getLogger()->info("Loaded tagType {} with {} tags",
										   tagType,
										   packTags[tagType].size());
			}

			if (MESSAGEMAN)
				MESSAGEMAN->Broadcast("PackTagsRefreshed");
		} else {
			parse();
			Locator::getLogger()->error(
			  "GetPackTagsRequest unexpected response code {} - content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};
	SendRequest(CALL_PATH,
				CALL_ENDPOINT,
				{},
				done,
				false,
				RequestMethod::GET,
				DONT_COMPRESS,
				true,
				false);
}

DownloadablePackPagination&
DownloadManager::GetPackPagination(const std::string& searchString,
								   std::set<std::string> tagFilters,
								   int perPage,
								   const std::string& sortBy,
								   bool sortIsAsc)
{
	auto key = DownloadablePackPaginationKey(
	  searchString, tagFilters, perPage, sortBy, sortIsAsc);
	if (!downloadablePackPaginations.contains(key)) {
		downloadablePackPaginations[key] = DownloadablePackPagination(
		  searchString, tagFilters, perPage, sortBy, sortIsAsc);
	}
	return downloadablePackPaginations[key];
}

void
DownloadablePackPagination::setPage(int page, LuaReference& whenDone) {
	// first, see if the page is available. if it is, no work to be done
	// if the page is not cached, retrieve it.
	// when retrieving, queue further request attempts

	auto runLuaFunc = [this, whenDone]() {
		if (!whenDone.IsNil() && whenDone.IsSet()) {
			Locator::getLogger()->info(
			  "DownloadablePackPagination setPage finished - running callback function");

			auto L = LUA->Get();
			whenDone.PushSelf(L);
			std::string error = "Error running DownloadablePackPagination finish function";
			LuaHelpers::CreateTableFromArray(get(), L);

			// 1 args, 0 results
			LuaHelpers::RunScriptOnStack(L, error, 1, 0, true);
			LUA->Release(L);
		} else {
			Locator::getLogger()->info(
			  "DownloadablePackPagination setPage finished - no callback function to run");
		}
	};

	if (noResults) {
		// no reason to make another request
		Locator::getLogger()->info("PackPagination can't repeat a search that "
								   "originally returned 0 results");
		runLuaFunc();
		return;
	}

	if (pendingRequest) {
		// cant request while already waiting for return
		Locator::getLogger()->info("PackPagination must wait for request "
								   "completion before making a new request");
		runLuaFunc();
		return;
	}

	if (finishedPageRequests.contains(page)) {
		// the page is already cached
		Locator::getLogger()->info("PackPagination returning cached page {}",
								   page);
		currentPage = page;
		initialized = true;
		pendingRequest = false;
		runLuaFunc();
		return;
	}

	
	bool pageIsInvalid = initialized && (page > getTotalPages() || page < 0);
	if (pageIsInvalid) {
		// we know the page couldnt have any results
		Locator::getLogger()->info(
		  "PackPagination returned early because page {} is invalid", page);
		pendingRequest = false;
		runLuaFunc();
		return;
	}

	ApiSearchCriteria searchCriteria;
	searchCriteria.page = page;
	searchCriteria.per_page = key.perPage;
	searchCriteria.packTags =
	  std::vector<std::string>(key.tagFilters.begin(), key.tagFilters.end());
	searchCriteria.packName = key.searchString;
	searchCriteria.sortBy = key.sortByField;
	searchCriteria.sortIsAscending = key.sortIsAsc;

	pendingRequest = true;

	auto parseFunc = [this, page, searchCriteria, runLuaFunc](Document& d) {
		// Locator::getLogger()->info("{}", jsonObjectToString(d));

		this->currentPage = page;
		this->initialized = true;
		this->pendingRequest = false;
		this->finishedPageRequests.insert(page);

		// Least Unsafe Json Traversal
		if (d.HasMember("results") && d["results"].IsArray()) {
			auto& resultArr = d["results"];
			auto& packlist = DLMAN->downloadablePacks;
			auto& localPacklist = this->results;
			auto packIndex = this->currentPageStartIndex();
			Locator::getLogger()->info("Starting at pack index {}", packIndex);
			for (auto& results : resultArr.GetArray()) {

				auto resultCount = getJsonInt(results, "found");
				if (resultCount > localPacklist.size()) {
					localPacklist.resize(resultCount);
					this->totalEntries = resultCount;
				}
				if (resultCount == 0 && localPacklist.size() == 0) {
					this->noResults = true;
				}

				if (results.HasMember("hits") &&
					results["hits"].IsArray()) {
					for (auto& docEntry : results["hits"].GetArray()) {
						if (docEntry.HasMember("document") &&
							docEntry["document"].IsObject()) {
							auto& pack = docEntry["document"];

							DownloadablePack packDl;

							packDl.id = getJsonInt(pack, "id");
							packDl.name = getJsonString(pack, "name");
							packDl.url = getJsonString(pack, "download");
							packDl.mirror = getJsonString(pack, "mirror");
							if (packDl.url.empty()) {
								packDl.url = packDl.mirror;
							}
							if (packDl.mirror.empty()) {
								packDl.mirror = packDl.url;
							}
							packDl.avgDifficulty =
							  std::stof(getJsonString(pack, "overall"));
							packDl.size = getJsonInt64(pack, "bytes");
							packDl.plays = getJsonInt(pack, "play_count");
							packDl.songs = getJsonInt(pack, "song_count");
							packDl.bannerUrl =
							  getJsonString(pack, "banner_path");
							packDl.nsfw = getJsonBool(pack, "nsfw");

							auto thumbnail =
							  getJsonString(pack, "bannerTinyThumb");
							if (thumbnail.find("base64,") !=
								std::string::npos) {
								packDl.thumbnail = thumbnail;
							} else {
								packDl.thumbnail = "";
							}

							if (packDl.name.empty()) {
								Locator::getLogger()->warn(
								  "PackPagination missing pack name: {}",
								  jsonObjectToString(pack));
							}
							if (packDl.url.empty()) {
								Locator::getLogger()->warn(
								  "PackPagination missing pack download: {}",
								  jsonObjectToString(pack));
							}

							if (!packlist.contains(packDl.id)) {
								packlist[packDl.id] = packDl;
							}
							localPacklist.at(packIndex++) = &packlist[packDl.id];
							Locator::getLogger()->info("Added pack {}: {}", packIndex-1, packDl.name);
						}
					}
				}
				else {
					continue;
				}
			}
			Locator::getLogger()->info("Finished at pack index {}", packIndex-1);
		}
		else {
			Locator::getLogger()->warn(
			  "Pack search {} seemed to return no results? Content: {}",
			  searchCriteria.DebugString(),
			  jsonObjectToString(d));
		}
		runLuaFunc();
	};

	DLMAN->SearchForPacks(searchCriteria, parseFunc);
}

Download::Download(std::string url, std::string filename)
{
	// remove characters we cant accept
	std::regex remover("[^\\w\\s\\d.]");
	std::string cleanname = std::regex_replace(filename, remover, "_");
	auto tmpFilename =
	  DL_DIR + (!cleanname.empty() ? cleanname : MakeTempFileName(url));
	auto opened = p_RFWrapper.file.Open(tmpFilename, 2);
	m_Url = url;
	handle = initBasicCURLHandle();
	m_TempFileName = tmpFilename;

	// horrible force crash if somehow things still dont work
	ASSERT_M(opened, p_RFWrapper.file.GetError());

	curl_easy_setopt_log_err(handle, CURLOPT_WRITEDATA, &p_RFWrapper);
	curl_easy_setopt_log_err(handle,
					 CURLOPT_WRITEFUNCTION,
	  static_cast<size_t(*)(
		char*, size_t, size_t, void*)>(
		[](char* dlBuffer, size_t size, size_t nmemb, void* pnf) -> size_t {
			auto RFW = static_cast<RageFileWrapper*>(pnf);
			if (RFW->stop) {
				return 0;
			}
			if (RFW->file.IsOpen()) {
				size_t b = RFW->file.Write(dlBuffer, size * nmemb);
				RFW->bytes += b;
				return b;
			}
			return 0;
		}));
	curl_easy_setopt_log_err(handle, CURLOPT_URL, m_Url.c_str());

	curl_easy_setopt_log_err(handle, CURLOPT_XFERINFODATA, &progress);
	curl_easy_setopt_log_err(handle,
					 CURLOPT_XFERINFOFUNCTION,
					 static_cast<int (*)(void *,
                      curl_off_t,
                      curl_off_t,
                      curl_off_t,
                      curl_off_t)>([](void* clientp,
										 curl_off_t dltotal,
										 curl_off_t dlnow,
										 curl_off_t ultotal,
						curl_off_t ulnow) -> int {
						 auto ptr = static_cast<ProgressData*>(clientp);
						 ptr->total = dltotal;
						 ptr->downloaded = dlnow;
						 return 0;
					 }));
	curl_easy_setopt_log_err(handle, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt_log_err(handle, CURLOPT_HTTPGET, 1L);
}

Download::~Download()
{
	FILEMAN->Remove(m_TempFileName);
}

void
Download::Update(float fDeltaSeconds)
{
	progress.time += fDeltaSeconds;
	if (progress.time > 1.0) {
		curl_off_t downloaded = progress.downloaded;
		speed = std::to_string(downloaded / 1024 - downloadedAtLastUpdate);
		progress.time = 0;
		downloadedAtLastUpdate = downloaded / 1024;
	}
}

void
Download::Install()
{
	Core::Platform::requestUserAttention();
	Message* msg;
	if (!SongManager::InstallSmzip(m_TempFileName))
		msg = new Message("DownloadFailed");
	else
		msg = new Message("PackDownloaded");
	msg->SetParam("pack", LuaReference::CreateFromPush(*p_Pack));
	MESSAGEMAN->Broadcast(*msg);
	delete msg;
}

std::string
Download::MakeTempFileName(std::string s)
{
	return Basename(s);
}

void
Download::Failed()
{
	Message msg("DownloadFailed");
	msg.SetParam("pack", LuaReference::CreateFromPush(*p_Pack));
	MESSAGEMAN->Broadcast(msg);
}

bool
DownloadablePack::isQueued() {
	auto it = std::find_if(DLMAN->DownloadQueue.begin(),
						   DLMAN->DownloadQueue.end(),
						   [this](std::pair<DownloadablePack*, bool> pair) {
							   return pair.first == this;
						   });
	return it != DLMAN->DownloadQueue.end();
}

RageTexture*
DownloadablePack::GetThumbnailTexture() {
	if (thumbnail.empty()) {
		return nullptr;
	}

	RageTextureID texid = RageTextureID(thumbnail);
	texid.base64 = true;

	return TEXTUREMAN->LoadTexture(texid);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"
#include "LuaManager.h"
/** @brief Allow Lua to have access to the ProfileManager. */
class LunaDownloadManager : public Luna<DownloadManager>
{
  public:
	static int GetHomePage(T* p, lua_State* L)
	{
		  lua_pushstring(L, uiHomePage.Get().c_str());
		  return 1;
	}
	static int GetUserCountryCode(T* p, lua_State* L)
	{
		lua_pushstring(L, p->countryCode.c_str());
		return 1;
	}
	static int GetPackPagination(T* p, lua_State* L) {
		auto searchString = SArg(1);

		luaL_checktype(L, 2, LUA_TTABLE);
		lua_pushvalue(L, 2);
		std::vector<std::string> tagVec;
		LuaHelpers::ReadArrayFromTable(tagVec, L);
		lua_pop(L, 1);
		std::set<std::string> tags(tagVec.begin(), tagVec.end());

		auto perPage = IArg(3);

		if (lua_isnoneornil(L, 4)) {
			auto& pagination =
			  p->GetPackPagination(searchString, tags, perPage, "name", true);
			pagination.PushSelf(L);
		}
		else {
			auto& pagination = p->GetPackPagination(
			  searchString, tags, perPage, SArg(4), BArg(5));
			pagination.PushSelf(L);
		}

		return 1;
	}
	static int GetAllPacks(T* p, lua_State* L)
	{
		auto& packs = p->downloadablePacks;
		lua_createtable(L, packs.size(), 0);
		auto it = packs.begin();
		for (unsigned i = 0; i < packs.size(); ++i) {
			it->second.PushSelf(L);
			it++;
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	static int GetDownloadingPacks(T* p, lua_State* L)
	{
		auto& packs = p->downloadablePacks;
		std::vector<DownloadablePack*> dling;
		for (auto& pack : packs) {
			if (pack.second.downloading)
				dling.push_back(&pack.second);
		}
		lua_createtable(L, dling.size(), 0);
		for (unsigned i = 0; i < dling.size(); ++i) {
			dling[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	static int GetQueuedPacks(T* p, lua_State* L)
	{
		lua_createtable(L, p->DownloadQueue.size(), 0);
		for (unsigned i = 0; i < p->DownloadQueue.size(); i++) {
			p->DownloadQueue[i].first->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	static int GetUsername(T* p, lua_State* L)
	{
		lua_pushstring(L, p->sessionUser.c_str());
		return 1;
	}
	static int GetSkillsetRank(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetSkillsetRank(Enum::Check<Skillset>(L, 1)));
		return 1;
	}
	static int GetSkillsetRating(T* p, lua_State* L)
	{
		lua_pushnumber(L,
					   p->GetSkillsetRating(Enum::Check<Skillset>(L, 1)));
		return 1;
	}
	static int GetDownloads(T* p, lua_State* L)
	{
		std::map<std::string, std::shared_ptr<Download>>& dls = p->downloads;
		lua_createtable(L, dls.size(), 0);
		int j = 0;
		for (auto& dl : dls) {
			dl.second->PushSelf(L);
			lua_rawseti(L, -2, j + 1);
			j++;
		}
		return 1;
	}
	static int IsLoggedIn(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->LoggedIn());
		return 1;
	}
	static int Login(T* p, lua_State* L)
	{
		std::string user = SArg(1);
		std::string pass = SArg(2);
		p->Login(user, pass);
		return 0;
	}
	static int LoginWithToken(T* p, lua_State* L)
	{
		std::string user = SArg(1);
		std::string token = SArg(2);
		p->LogoutIfLoggedIn();
		p->LoginWithToken(token);
		return 0;
	}
	static int Logout(T* p, lua_State* L)
	{
		p->LogoutIfLoggedIn();
		return 0;
	}
	static int GetLastVersion(T* p, lua_State* L)
	{
		lua_pushstring(L, p->lastVersion.c_str());
		return 1;
	}
	static int GetTopSkillsetScore(T* p, lua_State* L)
	{
		int rank = IArg(1);
		auto ss = Enum::Check<Skillset>(L, 2);
		bool result;
		auto onlineScore = p->GetTopSkillsetScore(rank, ss, result);
		if (!result) {
			lua_pushnil(L);
			return 1;
		}
		lua_createtable(L, 0, 7);
		lua_pushstring(L, onlineScore.songName.c_str());
		lua_setfield(L, -2, "songName");
		lua_pushnumber(L, onlineScore.rate);
		lua_setfield(L, -2, "rate");
		lua_pushnumber(L, onlineScore.ssr);
		lua_setfield(L, -2, "ssr");
		lua_pushnumber(L, onlineScore.wifeScore);
		lua_setfield(L, -2, "wife");
		lua_pushstring(L, onlineScore.chartkey.c_str());
		lua_setfield(L, -2, "chartkey");
		LuaHelpers::Push(L, onlineScore.difficulty);
		lua_setfield(L, -2, "difficulty");
		LuaHelpers::Push(L, PlayerStageStats::GetGrade(onlineScore.wifeScore));
		lua_setfield(L, -2, "grade");
		return 1;
	}
	static int GetTopChartScoreCount(T* p, lua_State* L)
	{
		std::string ck = SArg(1);
		if (p->chartLeaderboards.count(ck))
			lua_pushnumber(L, p->chartLeaderboards[ck].size());
		else
			lua_pushnumber(L, 0);
		return 1;
	}
	static int GetTopChartScore(T* p, lua_State* L)
	{
		std::string chartkey = SArg(1);
		int rank = IArg(2);
		int index = rank - 1;
		if (index < 0 || !p->chartLeaderboards.count(chartkey) ||
			index >=
			  static_cast<int>(p->chartLeaderboards[chartkey].size())) {
			lua_pushnil(L);
			return 1;
		}
		auto& score = p->chartLeaderboards[chartkey][index];
		lua_createtable(
		  L, 0, 17 + NUM_Skillset + (score.replayData.empty() ? 0 : 1));
		FOREACH_ENUM(Skillset, ss)
		{
			lua_pushnumber(L, score.SSRs[ss]);
			lua_setfield(L, -2, SkillsetToString(ss).c_str());
		}
		lua_pushboolean(L, score.valid);
		lua_setfield(L, -2, "valid");
		lua_pushnumber(L, score.rate);
		lua_setfield(L, -2, "rate");
		lua_pushnumber(L, score.wife);
		lua_setfield(L, -2, "wife");
		lua_pushnumber(L, score.wifeversion);
		lua_setfield(L, -2, "wifeversion");
		lua_pushnumber(L, score.miss);
		lua_setfield(L, -2, "miss");
		lua_pushnumber(L, score.marvelous);
		lua_setfield(L, -2, "marvelous");
		lua_pushnumber(L, score.perfect);
		lua_setfield(L, -2, "perfect");
		lua_pushnumber(L, score.bad);
		lua_setfield(L, -2, "bad");
		lua_pushnumber(L, score.good);
		lua_setfield(L, -2, "good");
		lua_pushnumber(L, score.great);
		lua_setfield(L, -2, "great");
		lua_pushnumber(L, score.maxcombo);
		lua_setfield(L, -2, "maxcombo");
		lua_pushnumber(L, score.held);
		lua_setfield(L, -2, "held");
		lua_pushnumber(L, score.letgo);
		lua_setfield(L, -2, "letgo");
		lua_pushnumber(L, score.minehits);
		lua_setfield(L, -2, "minehits");
		lua_pushboolean(L, score.nocc);
		lua_setfield(L, -2, "nocc");
		lua_pushstring(L, score.modifiers.c_str());
		lua_setfield(L, -2, "modifiers");
		lua_pushstring(L, score.username.c_str());
		lua_setfield(L, -2, "username");
		lua_pushnumber(L, score.playerRating);
		lua_setfield(L, -2, "playerRating");
		lua_pushstring(L, score.datetime.GetString().c_str());
		lua_setfield(L, -2, "datetime");
		lua_pushstring(L, score.scoreid.c_str());
		lua_setfield(L, -2, "scoreid");
		lua_pushnumber(L, score.userid);
		lua_setfield(L, -2, "userid");
		lua_pushstring(L, score.avatar.c_str());
		lua_setfield(L, -2, "avatar");
		if (!score.replayData.empty()) {
			lua_createtable(L, 0, score.replayData.size());
			int i = 1;
			for (auto& pair : score.replayData) {
				lua_createtable(L, 0, 2);
				lua_pushnumber(L, pair.first);
				lua_rawseti(L, -2, 1);
				lua_pushnumber(L, pair.second);
				lua_rawseti(L, -2, 2);
				lua_rawseti(L, -2, i++);
			}
			lua_setfield(L, -2, "replaydata");
		}
		return 1;
	}
	static int GetCoreBundle(T* p, lua_State* L)
	{
		// don't remove this yet or at all idk yet -mina
		auto bundle = p->GetCoreBundle(SArg(1));
		lua_createtable(L, bundle.size(), 0);
		for (size_t i = 0; i < bundle.size(); ++i) {
			bundle[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}

		size_t totalsize = 0;
		float avgpackdiff = 0.f;

		for (auto p : bundle) {
			totalsize += p->size / 1024 / 1024;
			avgpackdiff += p->avgDifficulty;
		}

		if (!bundle.empty())
			avgpackdiff /= bundle.size();

		// this may be kind of unintuitive but lets roll with it for now
		// -mina
		lua_pushnumber(L, totalsize);
		lua_setfield(L, -2, "TotalSize");
		lua_pushnumber(L, avgpackdiff);
		lua_setfield(L, -2, "AveragePackDifficulty");

		return 1;
	}
	static int DownloadCoreBundle(T* p, lua_State* L)
	{
		bool bMirror = false;
		if (!lua_isnoneornil(L, 2)) {
			bMirror = BArg(2);
		}
		p->DownloadCoreBundle(SArg(1), bMirror);
		return 0;
	}
	static int GetToken(T* p, lua_State* L)
	{
		lua_pushstring(L, p->authToken.c_str());
		return 1;
	}

	static int RequestOnlineScoreReplayData(T* p, lua_State* L)
	{
		OnlineHighScore* hs =
		  (OnlineHighScore*)GetPointerFromStack(L, "HighScore", 1);
		int userid = hs->userid;
		std::string username = hs->GetDisplayName();
		std::string scoreid = hs->scoreid;
		std::string ck = hs->GetChartKey();

		bool alreadyHasReplay = false;
		alreadyHasReplay |= !hs->GetNoteRowVector().empty();
		alreadyHasReplay |=
		  !hs->GetCopyOfSetOnlineReplayTimestampVector().empty();
		alreadyHasReplay |= !hs->GetOffsetVector().empty();
		alreadyHasReplay |= !hs->GetInputDataVector().empty();

		LuaReference f;
		if (lua_isfunction(L, 2))
			f = GetFuncArg(2, L);

		if (alreadyHasReplay) {
			if (!f.IsNil() && f.IsSet()) {
				auto L = LUA->Get();
				f.PushSelf(L);
				std::string Error =
				  "Error running RequestChartLeaderBoard Finish Function: ";
				hs->PushSelf(L);
				// 1 args, 0 results
				LuaHelpers::RunScriptOnStack(L, Error, 1, 0, true);
			}
			return 0;
		}

		p->GetReplayData(scoreid, userid, username, ck, f);
		return 0;
	}

	// This requests the leaderboard from online. This may cause lag.
	// Use this sparingly.
	// This will NOT request a leaderboard if it already exists for the
	// chartkey. This needs to be updated in the future to do so. That means
	// this will not update a leaderboard to a new state.
	static int RequestChartLeaderBoardFromOnline(T* p, lua_State* L)
	{
		// an unranked chart check could be done here
		// but just in case, don't check
		// allow another request for those -- if it gets ranked during a session
		std::string chart = SArg(1);
		LuaReference ref;
		auto& leaderboardScores = p->chartLeaderboards[chart];
		if (lua_isfunction(L, 2)) {
			lua_pushvalue(L, 2);
			ref.SetFromStack(L);
		}
		if (!leaderboardScores.empty()) {
			if (!ref.IsNil()) {
				ref.PushSelf(L);
				if (!lua_isnil(L, -1)) {
					std::string Error = "Error running GetChartLeaderboard "
										"Finish Function: ";
					lua_newtable(L);
					for (unsigned i = 0; i < leaderboardScores.size(); ++i) {
						auto& s = leaderboardScores[i];
						s.Push(L);
						lua_rawseti(L, -2, i + 1);
					}
					LuaHelpers::RunScriptOnStack(
					  L, Error, 1, 0, true); // 1 args, 0 results
				}
			}
			return 0;
		}
		p->GetChartLeaderboard(chart, ref);

		return 0;
	}

	static int GetChartLeaderBoard(T* p, lua_State* L)
	{
		std::vector<HighScore*> filteredLeaderboardScores;
		std::unordered_set<std::string> userswithscores;
		auto ck = SArg(1);
		auto& leaderboardScores = p->chartLeaderboards[ck];
		std::string country = "";
		if (!lua_isnoneornil(L, 2)) {
			country = SArg(2);
		}
		float currentrate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

		// empty chart leaderboards return empty lists
		// unranked charts return NO lists
		if (p->unrankedCharts.count(ck)) {
			lua_pushnil(L);
			return 1;
		}

		for (auto& score : leaderboardScores) {
			auto& leaderboardHighScore = score.hs;
			if (p->validonly && (!score.nocc || !score.valid))
				continue;
			if (p->currentrateonly &&
				lround(leaderboardHighScore.GetMusicRate() * 10000.f) !=
				  lround(currentrate * 10000.f))
				continue;
			if (p->topscoresonly &&
				userswithscores.count(leaderboardHighScore.GetName()) == 1)
				continue;
			if (!country.empty() && country != "Global" &&
				leaderboardHighScore.countryCode != country)
				continue;

			filteredLeaderboardScores.push_back(&(score.hs));
			userswithscores.emplace(leaderboardHighScore.GetName());
		}

		if (!filteredLeaderboardScores.empty() && p->currentrateonly) {
			std::sort(filteredLeaderboardScores.begin(),
					  filteredLeaderboardScores.end(),
					  [](const HighScore* a, const HighScore* b) -> bool {
						  return a->GetWifeScore() > b->GetWifeScore();
					  });
		}
		else if (!filteredLeaderboardScores.empty()) {
			std::sort(filteredLeaderboardScores.begin(),
					  filteredLeaderboardScores.end(),
					  [](const HighScore* a, const HighScore* b) -> bool {
						  if (a->GetMusicRate() == b->GetMusicRate()) {
							  return a->GetWifeScore() > b->GetWifeScore();
						  }
						  auto assr = a->GetSkillsetSSR(Skill_Overall);
						  auto bssr =
							b->GetSkillsetSSR(Skill_Overall);
						  if (fabsf(assr - bssr) < 0.001F) {
							  return a->GetMusicRate() > b->GetMusicRate();
						  } else {
							  return assr > bssr;
						  }
					  });
		}

		LuaHelpers::CreateTableFromArray(filteredLeaderboardScores, L);
		return 1;
	}
	static int GetPackTags(T* p, lua_State* L) {

		lua_createtable(L, 0, p->packTags.size());
		for (auto& x : p->packTags) {
			LuaHelpers::CreateTableFromArray(x.second, L);
			lua_setfield(L, -2, x.first.c_str());
		}

		return 1;
	}

	static int DownloadMissingPlaylists(T* p, lua_State* L) {
		p->DownloadMissingPlaylists();
		return 0;
	}
	static int ToggleRateFilter(T* p, lua_State* L)
	{
		p->currentrateonly = !p->currentrateonly;
		return 0;
	}
	static int GetCurrentRateFilter(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->currentrateonly);
		return 1;
	}
	static int ToggleTopScoresOnlyFilter(T* p, lua_State* L)
	{
		p->topscoresonly = !p->topscoresonly;
		return 0;
	}
	static int GetTopScoresOnlyFilter(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->topscoresonly);
		return 1;
	}
	static int ToggleValidFilter(T* p, lua_State* L)
	{
		p->validonly = !p->validonly;
		return 0;
	}
	static int GetValidFilter(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->validonly);
		return 1;
	}
	static int SendReplayDataForOldScore(T* p, lua_State* L)
	{
		p->UploadScoreWithReplayDataFromDisk(
		  SCOREMAN->GetScoresByKey().at(SArg(1)));
		return 0;
	}
	static int UploadScoresForChart(T* p, lua_State* L)
	{
		p->ForceUploadPBsForChart(SArg(1), true);
		return 0;
	}
	static int UploadScoresForPack(T* p, lua_State* L)
	{
		p->ForceUploadPBsForPack(SArg(1), true);
		return 0;
	}
	static int UploadAllScores(T* p, lua_State* L)
	{
		p->ForceUploadAllPBs();
		return 0;
	}
	static int GetQueuedScoreUploadsRemaining(T* p, lua_State* L)
	{
		lua_pushnumber(L, DLMAN->ScoreUploadSequentialQueue.size());
		return 1;
	}
	static int GetQueuedScoreUploadTotal(T* p, lua_State* L)
	{
		lua_pushnumber(L, DLMAN->sequentialScoreUploadTotalWorkload);
		return 1;
	}
	LunaDownloadManager()
	{
		ADD_METHOD(GetHomePage);
		ADD_METHOD(GetUserCountryCode);
		ADD_METHOD(DownloadCoreBundle);
		ADD_METHOD(GetCoreBundle);
		ADD_METHOD(GetPackPagination);
		ADD_METHOD(GetAllPacks);
		ADD_METHOD(GetPackTags);
		ADD_METHOD(GetDownloadingPacks);
		ADD_METHOD(GetQueuedPacks);
		ADD_METHOD(GetDownloads);
		ADD_METHOD(GetToken);
		ADD_METHOD(IsLoggedIn);
		ADD_METHOD(Login);
		ADD_METHOD(LoginWithToken);
		ADD_METHOD(GetUsername);
		ADD_METHOD(GetSkillsetRank);
		ADD_METHOD(GetSkillsetRating);
		ADD_METHOD(GetTopSkillsetScore);
		ADD_METHOD(GetTopChartScore);
		ADD_METHOD(GetTopChartScoreCount);
		ADD_METHOD(GetLastVersion);
		ADD_METHOD(RequestChartLeaderBoardFromOnline);
		ADD_METHOD(RequestOnlineScoreReplayData);
		ADD_METHOD(GetChartLeaderBoard);
		ADD_METHOD(DownloadMissingPlaylists);
		ADD_METHOD(ToggleRateFilter);
		ADD_METHOD(GetCurrentRateFilter);
		ADD_METHOD(ToggleTopScoresOnlyFilter);
		ADD_METHOD(GetTopScoresOnlyFilter);
		ADD_METHOD(ToggleValidFilter);
		ADD_METHOD(GetValidFilter);
		ADD_METHOD(SendReplayDataForOldScore);
		ADD_METHOD(UploadScoresForChart);
		ADD_METHOD(UploadScoresForPack);
		ADD_METHOD(UploadAllScores);
		ADD_METHOD(GetQueuedScoreUploadsRemaining);
		ADD_METHOD(GetQueuedScoreUploadTotal);
		ADD_METHOD(Logout);
	}
};
LUA_REGISTER_CLASS(DownloadManager)

class LunaDownloadablePack : public Luna<DownloadablePack>
{
  public:
	static int DownloadAndInstall(T* p, lua_State* L)
	{
		bool mirror = false;
		if (lua_gettop(L) > 0)
			mirror = BArg(1);
		if (p->downloading) {
			p->PushSelf(L);
			return 1;
		}
		if (p->isQueued()) {
			p->PushSelf(L);
			return 1;
		}

		std::shared_ptr<Download> dl = DLMAN->DownloadAndInstallPack(p, mirror);
		if (dl) {
			dl->PushSelf(L);
		} else
			lua_pushnil(L);
		return 1;
	}
	static int GetName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->name.c_str());
		return 1;
	}
	static int GetSize(T* p, lua_State* L)
	{
		lua_pushinteger(L, p->size);
		return 1;
	}
	static int GetAvgDifficulty(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->avgDifficulty);
		return 1;
	}
	static int GetPlayCount(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->plays);
		return 1;
	}
	static int GetSongCount(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->songs);
		return 1;
	}
	static int IsQueued(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->isQueued());
		return 1;
	}
	static int RemoveFromQueue(T* p, lua_State* L)
	{
		auto it = std::find_if(
		  DLMAN->DownloadQueue.begin(),
		  DLMAN->DownloadQueue.end(),
							   [p](std::pair<DownloadablePack*, bool> pair) {
								   return pair.first == p;
							   });
		if (it == DLMAN->DownloadQueue.end())
			// does not exist
			lua_pushboolean(L, false);
		else {
			DLMAN->DownloadQueue.erase(it);
			// success?
			lua_pushboolean(L, true);
		}
		return 1;
	}
	static int IsDownloading(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->downloading == 0);
		return 1;
	}
	static int GetDownload(T* p, lua_State* L)
	{
		if (p->downloading) {
			// using GetDownload on a download started by a Mirror isn't keyed
			// by the Mirror url have to check both
			auto u = encodeDownloadUrl(p->url);
			auto m = encodeDownloadUrl(p->mirror);
			if (DLMAN->downloads.count(u))
				DLMAN->downloads[u]->PushSelf(L);
			else if (DLMAN->downloads.count(m))
				DLMAN->downloads[m]->PushSelf(L);
			else
				lua_pushnil(L); // this shouldnt happen
		} else
			lua_pushnil(L);
		return 1;
	}
	static int GetID(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->id);
		return 1;
	}
	static int GetURL(T* p, lua_State* L)
	{
		lua_pushstring(L, p->url.c_str());
		return 1;
	}
	static int GetMirror(T* p, lua_State* L)
	{
		lua_pushstring(L, p->mirror.c_str());
		return 1;
	}
	static int IsNSFW(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->nsfw);
		return 1;
	}
	/*
	static int GetThumbnailTexture(T* p, lua_State* L)
	{
		auto* pTexture = p->GetThumbnailTexture();
		if (pTexture != nullptr) {
			pTexture->PushSelf(L);
		} else
			lua_pushnil(L);
		return 1;
	}
	*/
	LunaDownloadablePack()
	{
		ADD_METHOD(DownloadAndInstall);
		ADD_METHOD(IsDownloading);
		ADD_METHOD(IsQueued);
		ADD_METHOD(RemoveFromQueue);
		ADD_METHOD(GetAvgDifficulty);
		ADD_METHOD(GetPlayCount);
		ADD_METHOD(GetSongCount);
		ADD_METHOD(GetName);
		ADD_METHOD(GetSize);
		ADD_METHOD(GetDownload);
		ADD_METHOD(GetID);
		ADD_METHOD(GetURL);
		ADD_METHOD(GetMirror);
		ADD_METHOD(IsNSFW);
		// ADD_METHOD(GetThumbnailTexture);
	}
};

LUA_REGISTER_CLASS(DownloadablePack)

class LunaDownload : public Luna<Download>
{
  public:
	static int GetKBDownloaded(T* p, lua_State* L)
	{
		lua_pushnumber(L, static_cast<int>(p->progress.downloaded));
		return 1;
	}
	static int GetKBPerSecond(T* p, lua_State* L)
	{
		lua_pushnumber(L, atoi(p->speed.c_str()));
		return 1;
	}
	static int GetTotalKB(T* p, lua_State* L)
	{
		lua_pushnumber(L, static_cast<int>(p->progress.total));
		return 1;
	}
	static int Stop(T* p, lua_State* L)
	{
		p->p_RFWrapper.stop = true;
		return 0;
	}
	LunaDownload()
	{
		ADD_METHOD(GetTotalKB);
		ADD_METHOD(GetKBDownloaded);
		ADD_METHOD(GetKBPerSecond);
		ADD_METHOD(Stop);
	}
};

LUA_REGISTER_CLASS(Download)

class LunaDownloadablePackPagination : public Luna<DownloadablePackPagination>
{
  public:
	static int GetResults(T* p, lua_State* L) {

		LuaReference func = GetFuncArg(1, L);

		if (!p->initialized) {
			p->initialize(func);
		}
		else {
			func.PushSelf(L);
			std::string error = "Error running GetResults callback";
			LuaHelpers::CreateTableFromArray(p->get(), L);

			// 1 args, 0 results
			LuaHelpers::RunScriptOnStack(L, error, 1, 0, true);
		}

		return 0;
	}
	static int GetCachedResults(T* p, lua_State* L) {
		LuaHelpers::CreateTableFromArray(p->getCache(), L);
		return 1;
	}
	static int GetTotalResults(T* p, lua_State* L) {
		lua_pushnumber(L, p->totalEntries);
		return 1;
	}
	static int NextPage(T* p, lua_State* L) {
		LuaReference func;
		if (lua_isfunction(L, 1))
			func = GetFuncArg(1, L);
		p->nextPage(func);
		return 0;
	}
	static int PrevPage(T* p, lua_State* L) {
		LuaReference func;
		if (lua_isfunction(L, 1))
			func = GetFuncArg(1, L);
		p->prevPage(func);
		return 0;
	}
	static int GetTotalPages(T* p, lua_State* L) {
		lua_pushnumber(L, p->getTotalPages());
		return 1;
	}
	static int GetCurrentPage(T* p, lua_State* L) {
		lua_pushnumber(L, p->currentPage+1);
		return 1;
	}
	static int IsAwaitingRequest(T* p, lua_State* L) {
		lua_pushboolean(L, p->pendingRequest);
		return 1;
	}
	LunaDownloadablePackPagination() {
		ADD_METHOD(GetResults);
		ADD_METHOD(GetCachedResults);
		ADD_METHOD(GetTotalResults);
		ADD_METHOD(NextPage);
		ADD_METHOD(PrevPage);
		ADD_METHOD(GetTotalPages);
		ADD_METHOD(GetCurrentPage);
		ADD_METHOD(IsAwaitingRequest);
	}
};

LUA_REGISTER_CLASS(DownloadablePackPagination)
