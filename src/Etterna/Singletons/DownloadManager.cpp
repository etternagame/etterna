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
#include "Etterna/Models/Songs/SongOptions.h"

#include "curl/curl.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "jwt-cpp/jwt.h"

#ifdef _WIN32
#include <intrin.h>
#endif

#include <regex>
#include <tuple>
#include <unordered_set>
#include <algorithm>

using namespace rapidjson;

std::shared_ptr<DownloadManager> DLMAN = nullptr;
LuaReference DownloadManager::EMPTY_REFERENCE = LuaReference();
static std::atomic<float> timeSinceLastDownload = 0.F;
static bool runningSequentialUpload = false;

// Pack API Preferences
static Preference<unsigned int> maxDLPerSecond(
  "maximumBytesDownloadedPerSecond",
  0);
static Preference<unsigned int> maxDLPerSecondGameplay(
  "maximumBytesDownloadedPerSecondDuringGameplay",
  1000000);
static Preference<unsigned int> downloadPacksToAdditionalSongs(
  "downloadPacksToAdditionalSongs",
  0);
static Preference<unsigned int> maxPacksToDownloadAtOnce(
  "parallelDownloadsAllowed",
  1);
static Preference<float> DownloadCooldownTime(
  "secondsBetweenConsecutiveDownloads",
  5.F);

// Score API Preferences
static Preference<std::string> serverURL("BaseAPIUrl",
										 "https://api.beta.etternaonline.com");
static Preference<std::string> packListURL(
  "PackListUrl",
  "https://api.beta.etternaonline.com/api/client/packs");
static Preference<unsigned int> automaticSync("automaticScoreSync", 1);

// 
static const std::string TEMP_ZIP_MOUNT_POINT = "/@temp-zip/";
static const std::string DL_DIR = SpecialFiles::CACHE_DIR + "Downloads/";
static const std::string wife3_rescore_upload_flag = "rescoredw3";

static Preference<unsigned int> UPLOAD_SCORE_BULK_CHUNK_SIZE(
  "bulkScoreUploadChunkSize",
  100);

// endpoint construction constants
// all paths should begin with / and end without /
/// API root path
static const std::string API_ROOT = "/api/client";
static const std::string API_KEY = "testkey";

static const std::string API_LOGIN = "/login";
static const std::string API_RANKED_CHARTKEYS = "/charts/ranked";
static const std::string API_CHART_LEADERBOARD = "/charts/{}/scores";
static const std::string API_UPLOAD_SCORE = "/scores";
static const std::string API_GET_SCORE = "/scores/{}";
static const std::string API_UPLOAD_SCORE_BULK = "/scores/bulk";
static const std::string API_FAVORITES = "/favorites";
static const std::string API_GOALS = "/goals";
static const std::string API_USER = "/users/{}";
static const std::string API_GAME_VERSION = "/settings/version";

inline std::string
APIROOT()
{
	return serverURL.Get() + API_ROOT;
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

bool
DownloadManager::InstallSmzip(const std::string& sZipFile)
{
	if (!FILEMAN->Mount("zip", sZipFile, TEMP_ZIP_MOUNT_POINT))
		FAIL_M(static_cast<std::string>("Failed to mount " + sZipFile).c_str());
	std::vector<std::string> v_packs;
	FILEMAN->GetDirListing(TEMP_ZIP_MOUNT_POINT + "*", v_packs, ONLY_DIR, true);

	std::string doot = TEMP_ZIP_MOUNT_POINT;
	if (v_packs.size() > 1) {
		// attempt to whitelist pack name, this
		// should be pretty simple/safe solution for
		// a lot of pad packs -mina
		doot += sZipFile.substr(sZipFile.find_last_of('/') + 1);
		doot = doot.substr(0, doot.length() - 4) + "/";
	}

	std::vector<std::string> vsFiles;
	{
		std::vector<std::string> vsRawFiles;
		GetDirListingRecursive(doot, "*", vsRawFiles);

		if (vsRawFiles.empty()) {
			FILEMAN->Unmount("zip", sZipFile, TEMP_ZIP_MOUNT_POINT);
			return false;
		}

		std::vector<std::string> vsPrettyFiles;
		for (auto& s : vsRawFiles) {
			if (EqualsNoCase(GetExtension(s), "ctl"))
				continue;

			vsFiles.push_back(s);

			std::string s2 =
			  tail(s, s.length() - TEMP_ZIP_MOUNT_POINT.length());
			vsPrettyFiles.push_back(s2);
		}
		sort(vsPrettyFiles.begin(), vsPrettyFiles.end());
	}
	std::string sResult = "Success installing " + sZipFile;
	std::string extractTo =
	  downloadPacksToAdditionalSongs ? "AdditionalSongs/" : "Songs/";
	for (auto& sSrcFile : vsFiles) {
		std::string sDestFile = sSrcFile;
		sDestFile = tail(std::string(sDestFile.c_str()),
						 sDestFile.length() - TEMP_ZIP_MOUNT_POINT.length());

		// forcibly convert the path string to ASCII/ANSI/whatever
		// basically remove everything that isnt normal
		// and dont care about locales
		std::vector<unsigned char> bytes(sDestFile.begin(), sDestFile.end());
		bytes.push_back('\0');
		std::string res{};
		for (auto i = 0; i < bytes.size(); i++) {
			auto c = bytes.at(i);
			if (c > 122) {
				res.append(std::to_string(c - 122));
			} else {
				res.push_back(c);
			}
		}
		if (res.length() > 256) {
			res = res.substr(0, 255);
		}
		sDestFile = res;

		std::string sDir, sThrowAway;
		splitpath(sDestFile, sDir, sThrowAway, sThrowAway);

		if (!FileCopy(sSrcFile, extractTo + sDestFile)) {
			sResult = "Error extracting " + sDestFile;
			break;
		}
	}

	FILEMAN->Unmount("zip", sZipFile, TEMP_ZIP_MOUNT_POINT);

	SCREENMAN->SystemMessage(sResult);
	return true;
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
initCURLHandle(bool withBearer, bool acceptJson = false)
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
		} else if (doc[name].IsString()) {
			try {
				return std::stoi(doc[name].GetString());
			} catch (...) {}
		}
	}
	return 0;
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
	RefreshPackList(packListURL);
	RefreshLastVersion();
	RefreshRegisterPage();
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
	timeSinceLastDownload.store(timeSinceLastDownload.load() + fDeltaSeconds);
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
	if (downloads.size() < maxPacksToDownloadAtOnce &&
		!DownloadQueue.empty() &&
		timeSinceLastDownload.load() > DownloadCooldownTime.Get()) {
		std::pair<DownloadablePack*, bool> pack = DownloadQueue.front();
		DownloadQueue.pop_front();
		DownloadAndInstallPack(pack.first, pack.second);
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
			if (handle == dl-> handle) {
				if (dl->p_RFWrapper.stop) {
					dl->Failed();
					finishedDownloads[dl->m_Url] = dl;
				} else if (res == RequestResultStatus::Done) {
					timeSinceLastDownload.store(0);
					pendingInstallDownloads[dl->m_Url] = dl;
				} else if (res == RequestResultStatus::Failed) {
					dl->Failed();
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

	// when download speed is so slow, no data is written
	// when no data is written, the write function is not called
	// when the write function is not called, the download is never cancelled
	// so cancel the download here as well
	for (auto it = downloads.begin(); it != downloads.end(); it++) {
		auto& dl = (*it).second;
		if (dl->p_RFWrapper.stop) {
			if (dl->p_Pack != nullptr && !dl->p_Pack->downloading) {
				continue;
			}

			// dont remove the handle here because other thread wants it
			dl->Failed();
			finishedDownloads[dl->m_Url] = dl;
			dl->p_RFWrapper.file.Flush();
			if (dl->p_RFWrapper.file.IsOpen())
				dl->p_RFWrapper.file.Close();
			if (dl->p_Pack != nullptr)
				dl->p_Pack->downloading = false;

			downloads.erase(it);
			finishedADownload = true;
		}
	}

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
	RefreshCountryCodes();
	FOREACH_ENUM(Skillset, ss)
	RefreshTop25(ss);
	if (ShouldUploadScores()) {
		InitialScoreSync();

		// ok we don't actually want to delete this yet since this is
		// specifically for appending replaydata for a score the site does
		// not have data for without altering the score entry in any other
		// way, but keep disabled for now
		// UpdateOnlineScoreReplayData();
	}
	if (GAMESTATE->m_pCurSteps != nullptr)
		RequestChartLeaderBoard(GAMESTATE->m_pCurSteps->GetChartKey());
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
	loggingIn = true;
	LogoutIfLoggedIn();
	CURL* curlHandle = initCURLHandle(false);
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

	auto done = [user, pass, callback, this](HTTPRequest& req) {
		auto loginFailed = [this]() {
			authToken = sessionUser = "";
			MESSAGEMAN->Broadcast("LoginFailed");
			loggingIn = false;
		};

		if (HandleRatelimitResponse(API_LOGIN, req)) {
			LoginRequest(user, pass, callback);
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "LoginRequest Error: Malformed request response: {}", req.result);
			loginFailed();
			return;
		}

		auto response = req.response_code;
		if (response == 422) {
			// bad input fields

			Locator::getLogger()->error(
			  "Status 422 on LoginRequest. Errors: {}", jsonObjectToString(d));
			loginFailed();

		} else if (response == 401) {
			// bad password

			Locator::getLogger()->error(
			  "Status 401 on LoginRequest. Bad password? Errors: {}",
			  jsonObjectToString(d));
			loginFailed();

		} else if (response == 200) {
			// all good

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
				loginFailed();
			}
		} else {
			// ???
			Locator::getLogger()->warn(
			  "Unexpected response code {} on LoginRequest. Content: {}",
			  response,
			  jsonObjectToString(d));
			loginFailed();
		}
	};
	HTTPRequest* req = new HTTPRequest(curlHandle, done, form);
	req->Failed = [this](HTTPRequest& req) {
		authToken = sessionUser = "";
		MESSAGEMAN->Broadcast("LoginFailed");
		loggingIn = false;
	};
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(API_LOGIN, *req)) {
		AddHttpRequestHandle(req->handle);
		HTTPRequests.push_back(req);
	}
}

void
DownloadManager::Logout()
{
	// cancel sequential upload
	ScoreUploadSequentialQueue.clear();
	sequentialScoreUploadTotalWorkload = 0;
	runningSequentialUpload = false;

	// logout
	sessionUser = authToken = "";
	topScores.clear();
	sessionRatings.clear();
	// This is called on a shutdown, after MessageManager is gone
	if (MESSAGEMAN != nullptr)
		MESSAGEMAN->Broadcast("LogOut");
}

bool
DownloadManager::HandleAuthErrorResponse(const std::string& endpoint, HTTPRequest& req)
{
	auto& status = req.response_code;
	if (status != 401) {
		return false;
	}

	Locator::getLogger()->warn(
	  "{} {} - Auth Error. Logging out automatically",
	  endpoint,
	  status);

	LogoutIfLoggedIn();


	return true;
}

void
DownloadManager::AddFavoriteRequest(const std::string& chartkey)
{
	auto req = API_FAVORITES;

	favorites.push_back(chartkey);
	auto done = [chartkey, this](HTTPRequest& req) {

		if (HandleAuthErrorResponse(API_FAVORITES, req)) {
			return;
		}
		if (HandleRatelimitResponse(API_FAVORITES, req)) {
			AddFavoriteRequest(chartkey);
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "AddFavoriteRequest Error: Malformed request response: {}",
			  req.result);
			return;
		}

		auto response = req.response_code;

		if (response == 200) {
			// all good
			Locator::getLogger()->info(
			  "AddFavorite successfully added favorite {} from online profile",
			  chartkey);

		} else if (response == 422) {
			// missing info
			Locator::getLogger()->warn(
			  "AddFavorite failed due to input error. Content: {}",
			  jsonObjectToString(d));
		} else if (response == 404) {
			Locator::getLogger()->warn(
			  "AddFavorite failed due to 404. Chart may be unranked - Content: {}",
			  jsonObjectToString(d));
		} else {
			// ???
			Locator::getLogger()->warn(
			  "AddFavorite unknown response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(req,
				{ make_pair("key", UrlEncode(chartkey)) },
				done,
				true,
				RequestMethod::POST);
}

void
DownloadManager::RemoveFavoriteRequest(const std::string& chartkey)
{
	auto req = API_FAVORITES + "/" + URLEncode(chartkey);

	auto it =
	  std::find(favorites.begin(), favorites.end(), chartkey);
	if (it != favorites.end())
		favorites.erase(it);

	auto done = [chartkey, this](HTTPRequest& req) {

		if (HandleAuthErrorResponse(API_FAVORITES, req)) {
			return;
		}
		if (HandleRatelimitResponse(API_FAVORITES, req)) {
			RemoveFavoriteRequest(chartkey);
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "RemoveFavorite Error: Malformed request response: {}",
			  req.result);
			return;
		}

		auto response = req.response_code;

		if (response == 200) {
			// all good
			Locator::getLogger()->info("RemoveFavorite successfully removed "
									   "favorite {} from online profile",
									   chartkey);

		} else if (response == 404) {
			Locator::getLogger()->warn(
			  "RemoveFavorite failed due to 404. Favorite may be missing - Content: {}",
			  jsonObjectToString(d));
		} else {
			// ???
			Locator::getLogger()->warn(
			  "RemoveFavorite unknown response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(req, {}, done, true, RequestMethod::DEL);
}

void
DownloadManager::AddGoalRequest(ScoreGoal* goal)
{
	auto req = API_GOALS;

	auto done = [goal, this](HTTPRequest& req) {

		if (HandleAuthErrorResponse(API_GOALS, req)) {
			return;
		}
		if (HandleRatelimitResponse(API_GOALS, req)) {
			AddGoalRequest(goal);
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "RemoveFavorite Error: Malformed request response: {}",
			  req.result);
			return;
		}

		auto response = req.response_code;

		if (response == 200) {
			// all good
			Locator::getLogger()->info(
			  "AddGoal successfully added goal for {} to online profile",
			  goal->chartkey);

		} else if (response == 422) {
			// some validation issue with the request
			Locator::getLogger()->warn(
			  "AddGoal failed due to validation error: {}",
			  jsonObjectToString(d));
		} else if (response == 404) {
			Locator::getLogger()->warn(
			  "AddGoal failed due to 404. Chart may be unranked - Content: {}", jsonObjectToString(d));
		} else {
			Locator::getLogger()->warn(
			  "AddGoal unexpected response {} - Content: {}",
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

	SendRequest(req, postParams, done, true, RequestMethod::POST);
}

void
DownloadManager::RemoveGoalRequest(ScoreGoal* goal)
{
	auto req = API_GOALS + "/" + UrlEncode(goal->chartkey) + "/" +
					  std::to_string(goal->rate) + "/" +
					  std::to_string(goal->percent);

	auto done = [goal, this](HTTPRequest& req) {

		if (HandleAuthErrorResponse(API_GOALS, req)) {
			return;
		}
		if (HandleRatelimitResponse(API_GOALS, req)) {
			RemoveGoalRequest(goal);
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "RemoveGoal Error: Malformed request response: {}", req.result);
			return;
		}

		auto response = req.response_code;

		if (response == 200) {
			// all good
			Locator::getLogger()->info(
			  "RemoveGoal successfully removed goal for {} from online profile",
			  goal->chartkey);

		} else if (response == 422) {
			// some validation issue with the request
			Locator::getLogger()->warn(
			  "RemoveGoal failed due to validation error: {}",
			  jsonObjectToString(d));
		} else if (response == 404) {
			Locator::getLogger()->warn(
			  "RemoveGoal failed due to 404. Chart may be unranked or Favorite "
			  "missing - Content: {}",
			  jsonObjectToString(d));
		} else {
			Locator::getLogger()->warn(
			  "RemoveGoal unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(req, {}, done, true, RequestMethod::DEL);
}

void
DownloadManager::UpdateGoalRequest(ScoreGoal* goal)
{
	std::string timeAchievedString = "0000:00:00 00:00:00";
	if (goal->achieved)
		timeAchievedString = goal->timeachieved.GetString();

	auto req = API_GOALS;
	std::vector<std::pair<std::string, std::string>> postParams = {
		std::make_pair("key", UrlEncode(goal->chartkey)),
		std::make_pair("rate", std::to_string(goal->rate)),
		std::make_pair("wife", std::to_string(goal->percent)),
		std::make_pair("achieved", std::to_string(goal->achieved)),
		std::make_pair("set_date", goal->timeassigned.GetString()),
		std::make_pair("achieved_date", timeAchievedString)
	};

	auto done = [ goal, this ](HTTPRequest& req) {

		if (HandleAuthErrorResponse(API_GOALS, req)) {
			return;
		}
		if (HandleRatelimitResponse(API_GOALS, req)) {
			UpdateGoalRequest(goal);
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "UpdateGoal Error: Malformed request response: {}", req.result);
			return;
		}

		auto response = req.response_code;

		if (response == 200) {
			// all good
			Locator::getLogger()->info(
			  "UpdateGoal successfully updated goal for {} on online profile",
			  goal->chartkey);

		} else if (response == 422) {
			// some validation issue with the request
			Locator::getLogger()->warn(
			  "UpdateGoal failed due to validation error: {}",
			  jsonObjectToString(d));
		} else if (response == 404) {
			Locator::getLogger()->warn("UpdateGoal failed due to 404. Goal may "
									   "be missing online - Content: {}",
									   jsonObjectToString(d));
		} else {
			Locator::getLogger()->warn(
			  "UpdateGoal unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(req, postParams, done, true, RequestMethod::PATCH);
}

void
DownloadManager::RefreshFavorites()
{
	Locator::getLogger()->warn("REFRESH FAVORITES UNIMPLEMENTED");
	return;
	/*
	std::string req = "user/" + UrlEncode(sessionUser) + "/favorites";
	auto done = [this](HTTPRequest& req) {
		Document d;
		if (d.Parse(req.result.c_str()).HasParseError() ||
			!d.HasMember("data") || !d["data"].IsArray())
			favorites.clear();
		else {
			auto& favs = d["data"];
			for (auto& fav : favs.GetArray()) {
				if (fav.HasMember("attributes") && fav["attributes"].IsString())
					favorites.push_back(fav["attributes"].GetString());
			}
		}
		MESSAGEMAN->Broadcast("FavouritesUpdate");
	};
	SendRequest(req, {}, done);
	*/
}

void
DownloadManager::GetRankedChartkeysRequest(std::function<void(void)> callback,
										   const DateTime start,
										   const DateTime end)
{
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

	auto done = [callback, start, end, this, startstr, endstr](HTTPRequest& req) {

		if (HandleAuthErrorResponse(API_RANKED_CHARTKEYS, req)) {
			if (callback)
				callback();
			return;
		}
		if (HandleRatelimitResponse(API_RANKED_CHARTKEYS, req)) {
			GetRankedChartkeysRequest(callback, start, end);
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "GetRankedChartkeysRequest Error: Malformed request response: {}",
			  req.result);
			return;
		}

		auto response = req.response_code;

		if (response == 200) {
			// all good

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
			Locator::getLogger()->warn(
			  "GetRankedChartkeys unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
		if (callback)
			callback();
	};

	SendRequest(API_RANKED_CHARTKEYS, params, done, true);
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
	else
		tmp.rate = 0.0;
	if (score.HasMember("chord_cohesion") && score["chord_cohesion"].IsInt())
		tmp.nocc = score["chord_cohesion"].GetInt() == 0;
	else
		tmp.nocc = false;
	if (score.HasMember("valid") && score["valid"].IsInt())
		tmp.valid = score["valid"].GetInt() == 1;
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

	std::string scorekey = "";
	if (score.HasMember("key") && score["key"].IsString())
		scorekey = score["key"].GetString();

	auto& hs = tmp.hs;
	hs.SetDateTime(tmp.datetime);
	hs.SetMaxCombo(tmp.maxcombo);
	hs.SetName(tmp.username);
	hs.SetModifiers(tmp.modifiers);
	hs.SetChordCohesion(tmp.nocc);
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

	// to put a string into a json object
	// we have to put it in a Value instead
	// thats kinda cringe but theres some good technical reason why
	auto val = [&allocator](const std::string& str,
							std::string defaultVal = "") {
		Value v;
		if (str.empty()) {
			v.SetString(defaultVal.c_str(), allocator);
		} else {
			v.SetString(str.c_str(), allocator);
		}
		
		return v;
	};

	d.AddMember("key", val(hs->GetScoreKey()), allocator);
	d.AddMember("wife", hs->GetSSRNormPercent(), allocator);
	d.AddMember("max_combo", hs->GetMaxCombo(), allocator);
	d.AddMember("modifiers", val(hs->GetModifiers()), allocator);
	d.AddMember("marvelous", hs->GetTapNoteScore(TNS_W1), allocator);
	d.AddMember("perfect", hs->GetTapNoteScore(TNS_W2), allocator);
	d.AddMember("great", hs->GetTapNoteScore(TNS_W3), allocator);
	d.AddMember("good", hs->GetTapNoteScore(TNS_W4), allocator);
	d.AddMember("bad", hs->GetTapNoteScore(TNS_W5), allocator);
	d.AddMember("miss", hs->GetTapNoteScore(TNS_Miss), allocator);
	d.AddMember("hit_mine", hs->GetTapNoteScore(TNS_HitMine), allocator);
	d.AddMember("held", hs->GetHoldNoteScore(HNS_Held), allocator);
	d.AddMember("let_go", hs->GetHoldNoteScore(HNS_LetGo), allocator);
	d.AddMember("missed_hold", hs->GetHoldNoteScore(HNS_Missed), allocator);
	d.AddMember(
	  "rate", std::round(hs->GetMusicRate() * 1000.0) / 1000.0, allocator);
	d.AddMember("datetime", val(hs->GetDateTime().GetString()), allocator);
	d.AddMember(
	  "chord_cohesion", static_cast<int>(hs->GetChordCohesion()), allocator);
	d.AddMember("calculator_version", hs->GetSSRCalcVersion(), allocator);
	d.AddMember("top_score", hs->GetTopScore(), allocator);
	d.AddMember("wife_version", hs->GetWifeVersion(), allocator);
	d.AddMember(
	  "validation_key", val(hs->GetValidationKey(ValidationKey_Brittle)), allocator);
	d.AddMember("machine_guid", val(hs->GetMachineGuid(), "MISSING_GUID"), allocator);
	d.AddMember("chart_key", val(hs->GetChartKey()), allocator);
	d.AddMember(
	  "judge", std::round(hs->GetJudgeScale() * 1000.0) / 1000.0, allocator);
	d.AddMember("grade", val(GradeToOldString(hs->GetWifeGrade())), allocator);

	Document replayVector;
	replayVector.SetArray();
	if (includeReplayData) {
		auto fval = [](const float f) {
			Value v;
			v.SetFloat(f);
			return v;
		};

		bool hadToLoadReplayData = false;

		// load replay data if we need it
		// basically, in one case we care about the fact that we loaded or not
		if (hs->GetOffsetVector().empty()) {
			// this handles loading from disk and then generating needed information
			// would return false if impossible to work with
			hadToLoadReplayData = hs->GetReplay()->GeneratePrimitiveVectors();
		} else {
			// this handles loading from disk if necessary and generating if necessary
			hs->GetReplay()->GeneratePrimitiveVectors();
		}

		const auto& offsets = hs->GetOffsetVector();
		const auto& columns = hs->GetTrackVector();
		const auto& types = hs->GetTapNoteTypeVector();
		const auto& rows = hs->GetNoteRowVector();

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

		if (hadToLoadReplayData) {
			hs->UnloadReplayData();
		}
	}
	d.AddMember("replay_data", replayVector, allocator);

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
	Locator::getLogger()->info("Creating BulkUploadScore request for {} scores",
							   hsList.size());
	if (!LoggedIn()) {
		Locator::getLogger()->warn(
		  "Attempted to upload scores while not logged in. Aborting");
		if (callback)
			callback();
		return;
	}

	CURL* curlHandle = initCURLHandle(true, true);
	CURLAPIURL(curlHandle, API_UPLOAD_SCORE_BULK);

	auto body = ScoreVectorToJSON(hsList, true);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POSTFIELDSIZE, body.length());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_COPYPOSTFIELDS, body.c_str());

	// body json
	// Locator::getLogger()->warn("{}", body);

	auto done = [callback, hsList, this](HTTPRequest& req) {

		if (HandleAuthErrorResponse(API_UPLOAD_SCORE_BULK, req)) {
			if (callback)
				callback();
			return;
		}
		if (HandleRatelimitResponse(API_UPLOAD_SCORE_BULK, req)) {
			UploadBulkScores(hsList, callback);
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "Bulk score upload status {} & response parse error. Response "
			  "body: {}",
			  req.response_code,
			  req.result.c_str());
			if (callback)
				callback();
			return;
		}

		auto response = req.response_code;

		if (response == 200) {
			// kind of good.
			// we can have successes and failures.

			std::unordered_set<std::string> failedUploadKeys{};
			if (d.IsObject() && d.HasMember("failedUploads")) {
				auto& fails = d["failedUploads"];
				/*// Verbose output
				Locator::getLogger()->warn(
				  "BulkScoreUpload had failed uploads but continued: {}",
				  jsonObjectToString(fails));
				*/
				Locator::getLogger()->warn(
				  "BulkScoreUpload had failed uploads but continued working");

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
							failedUploadKeys.insert(objIt->name.GetString());
							Locator::getLogger()->warn(
							  "Score {} was rejected by server for: {}",
							  objIt->name.GetString(),
							  jsonObjectToString(objIt->value));
						}
					}
				}
			}

			if (!d.HasMember("failedUploads") && !d.HasMember("playerRating")) {
				Locator::getLogger()->info(
				  "BulkScoreUpload received no useful response? Content: {}",
				  jsonObjectToString(d));
			}

			// record the scores that successfully uploaded
			for (auto& hs : hsList) {
				if (failedUploadKeys.contains(hs->GetScoreKey())) {
					continue;
				}

				if (hs->GetWifeVersion() == 3)
					hs->AddUploadedServer(wife3_rescore_upload_flag);
				hs->AddUploadedServer(serverURL.Get());
				hs->forceuploadedthissession = true;
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
				  failedUploadKeys.size(),
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
			Locator::getLogger()->warn(
			  "BulkUploadScore got unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));

		}
		if (callback)
			callback();
	};

	HTTPRequest* req =
	  new HTTPRequest(curlHandle, done, nullptr, [callback](HTTPRequest& req) {
		  if (callback)
			  callback();
	  });
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(API_UPLOAD_SCORE_BULK, *req)) {
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

	CURL* curlHandle = initCURLHandle(true, true);
	CURLAPIURL(curlHandle, API_UPLOAD_SCORE);

	Document jsonDoc;
	auto scoreDoc = ScoreToJSON(hs, true, jsonDoc.GetAllocator());
	auto json = jsonObjectToString(scoreDoc);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POST, 1L);
	curl_easy_setopt_log_err(curlHandle, CURLOPT_POSTFIELDSIZE, json.length());
	curl_easy_setopt_log_err(curlHandle, CURLOPT_COPYPOSTFIELDS, json.c_str());

	auto done = [hs, callback, load_from_disk, this](HTTPRequest& req) {

		if (HandleAuthErrorResponse(API_UPLOAD_SCORE, req)) {
			if (callback)
				callback();
			return;
		}
		if (HandleRatelimitResponse(API_UPLOAD_SCORE, req)) {
			UploadScore(hs, callback, load_from_disk);
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error("Score upload status {} & response "
										"parse error. Response body: {}",
										req.response_code,
										req.result.c_str());
			if (callback)
				callback();
			return;
		}

		auto response = req.response_code;

		if (response == 200) {

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

			if (d.HasMember("data") && d["data"].HasMember("errors")) {
				Locator::getLogger()->error(
				  "UploadScore for {} has errors: {}",
				  hs->GetScoreKey(),
				  jsonObjectToString(d["data"]["errors"]));
				if (callback)
					callback();
				return;
			}

			Locator::getLogger()->warn(
				"UploadScore failed due to input error. Content: {}",
				jsonObjectToString(d));
		} else if (response == 404) {
			Locator::getLogger()->warn(
				"UploadScore failed due to 404. Chart may be unranked - Content: {}",
				jsonObjectToString(d));
		} else {
			Locator::getLogger()->warn("Unexpected status {} - Content: {}",
									   response,
									   jsonObjectToString(d));
		}

		if (callback)
			callback();
	};
	HTTPRequest* req = new HTTPRequest(
	  curlHandle, done, nullptr, [callback](HTTPRequest& req) {
			if (callback)
			  callback();
	  });
	SetCURLResultsString(curlHandle, &(req->result));
	SetCURLHeadersString(curlHandle, &(req->headers));
	if (!QueueRequestIfRatelimited(API_UPLOAD_SCORE, *req)) {
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
uploadSequentially()
{
	Message msg("UploadProgress");
	msg.SetParam(
	  "percent",
	  1.f - (static_cast<float>(DLMAN->ScoreUploadSequentialQueue.size()) /
			 static_cast<float>(DLMAN->sequentialScoreUploadTotalWorkload)));
	MESSAGEMAN->Broadcast(msg);

	if (!DLMAN->ScoreUploadSequentialQueue.empty()) {
		
		std::vector<HighScore*> hsToUpload{};
		for (auto i = 0u; i < UPLOAD_SCORE_BULK_CHUNK_SIZE &&
						 !DLMAN->ScoreUploadSequentialQueue.empty();
			 i++) {
			hsToUpload.push_back(DLMAN->ScoreUploadSequentialQueue.front());
			DLMAN->ScoreUploadSequentialQueue.pop_front();

			if (hsToUpload.size() >= UPLOAD_SCORE_BULK_CHUNK_SIZE) {
				break;
			}
		}
		runningSequentialUpload = true;
		DLMAN->UploadBulkScores(hsToUpload, uploadSequentially);
		
	} else {
		Locator::getLogger()->info(
		  "Sequential upload queue empty - uploads finished");
		runningSequentialUpload = false;
		DLMAN->sequentialScoreUploadTotalWorkload = 0;
	}
}

// This starts the sequential uploading process if it has not already begun
void
startSequentialUpload()
{
	if (DLMAN->ScoreUploadSequentialQueue.empty()) {
		Locator::getLogger()->info(
		  "Could not start sequential upload process - nothing to upload");
		return;
	}

	if (!runningSequentialUpload) {
		Locator::getLogger()->info("Starting sequential upload process - {} "
								   "scores split into chunks of {}",
								   DLMAN->ScoreUploadSequentialQueue.size(),
								   UPLOAD_SCORE_BULK_CHUNK_SIZE);
		uploadSequentially();
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
		auto scores = SCOREMAN->GetAllPBPtrs();
		auto& newly_rescored = SCOREMAN->rescores;
		std::vector<HighScore*> toUpload;
		for (auto& vec : scores) {
			for (auto& s : vec) {
				// probably not worth uploading fails, they get rescored now
				if (s->GetGrade() == Grade_Failed)
					continue;

				// the chart must be ranked to be uploaded
				if (!newlyRankedChartkeys.contains(s->GetChartKey()))
					continue;

				// handle rescores, ignore upload check
				if (newly_rescored.count(s))
					toUpload.push_back(s);
				// ok so i think we probably do need an upload flag for wife3
				// resyncs, and to actively check it, since if people rescore
				// everything, play 1 song and close their game or whatever,
				// rescore list won't be built again and scores won't auto
				// sync
				else if (s->GetWifeVersion() == 3 &&
						 !s->IsUploadedToServer(wife3_rescore_upload_flag))
					toUpload.push_back(s);
				// normal behavior, upload scores that haven't been uploaded and
				// have replays
				else if (!s->IsUploadedToServer(serverURL.Get()) &&
						 s->HasReplayData())
					toUpload.push_back(s);
			}
		}

		if (!toUpload.empty())
			Locator::getLogger()->info(
			  "Updating online scores. (Uploading {} scores)", toUpload.size());
		else
			return false;

		ScoreUploadSequentialQueue.insert(
		  ScoreUploadSequentialQueue.end(), toUpload.begin(), toUpload.end());
		sequentialScoreUploadTotalWorkload += toUpload.size();
		startSequentialUpload();
		profile->m_lastRankedChartkeyCheck = lastCheckDT;
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
	Locator::getLogger()->info(
	  "Trying ForceUploadPBsForChart - {}", ck);

	auto cs = SCOREMAN->GetScoresForChart(ck);
	if (cs) {
		bool successful = false;
		auto& scores = cs->GetAllScores();
		for (auto& s : scores) {
			if (!s->forceuploadedthissession) {
				if (s->GetGrade() != Grade_Failed && s->HasReplayData()) {
					// don't add stuff we're already uploading
					auto res = std::find(ScoreUploadSequentialQueue.begin(),
										 ScoreUploadSequentialQueue.end(),
										 s);
					if (res != ScoreUploadSequentialQueue.end())
						continue;

					ScoreUploadSequentialQueue.push_back(s);
					sequentialScoreUploadTotalWorkload += 1;
					successful = true;
				}
				else if (s->GetGrade() != Grade_Failed && !s->HasReplayData()) {
					Locator::getLogger()->info("Scorekey {} in chart {} has no "
											   "replay - ForceUpload skipped",
											   s->GetScoreKey(),
											   ck);
				}
			}
		}
		if (!successful) {
			Locator::getLogger()->info(
			  "ForceUploadPBsForChart did not queue any scores for chart {}",
			  ck);
		} else if (startNow) {
			startSequentialUpload();
		}
		return successful;
	} else {
		Locator::getLogger()->info(
		  "ForceUploadPBsForChart found no scores for chart {}", ck);
		return false;
	}
}
bool
DownloadManager::ForceUploadPBsForPack(const std::string& pack, bool startNow)
{
	Locator::getLogger()->info(
	  "Trying ForceUploadPBsForPack - {}", pack);

	bool successful = false;
	auto songs = SONGMAN->GetSongs(pack);
	for (auto so : songs)
		for (auto c : so->GetAllSteps())
			successful |= ForceUploadPBsForChart(c->GetChartKey(), false);
	if (!successful) {
		Locator::getLogger()->info(
		  "ForceUploadPBsForPack did not queue any scores for pack {}", pack);
	} else if (startNow) {
		startSequentialUpload();
	}
	return successful;
}
bool
DownloadManager::ForceUploadAllPBs()
{
	Locator::getLogger()->info("Trying ForceUploadAllPBs");

	bool successful = false;
	auto songs = SONGMAN->GetSongs(GROUP_ALL);
	for (auto so : songs)
		for (auto c : so->GetAllSteps())
			successful |= ForceUploadPBsForChart(c->GetChartKey(), false);
	if (!successful) {
		Locator::getLogger()->info(
		  "ForceUploadAllPBs did not queue any scores");
	} else {
		startSequentialUpload();
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

HTTPRequest*
DownloadManager::SendRequest(
  std::string requestName,
  std::vector<std::pair<std::string, std::string>> params,
  std::function<void(HTTPRequest&)> done,
  bool requireLogin,
  RequestMethod httpMethod,
  bool async,
  bool withBearer)
{
	return SendRequestToURL(APIROOT() + requestName,
							params,
							done,
							requireLogin,
							httpMethod,
							async,
							withBearer);
}

HTTPRequest*
DownloadManager::SendRequestToURL(
  std::string url,
  std::vector<std::pair<std::string, std::string>> params,
  std::function<void(HTTPRequest&)> afterDone,
  bool requireLogin,
  RequestMethod httpMethod,
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
				 this](HTTPRequest& req) {

		if (HandleAuthErrorResponse(url, req)) {
			if (afterDone)
				afterDone(req);
			return;
		}
		if (HandleRatelimitResponse(url, req)) {
			SendRequestToURL(url,
							 params,
							 afterDone,
							 requireLogin,
							 httpMethod,
							 async,
							 withBearer);
			return;
		}

		// for non 404s, check to see if it is malformed
		Document d;
		if (req.response_code != 404 &&
			d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "SendRequestToURL ({}) status {} & Parse Error: {}",
			  url,
			  req.response_code,
			  req.result);
			return;
		}
		if (d.HasMember("errors")) {
			// hmm
		}
		if (afterDone)
			afterDone(req);
	};
	CURL* curlHandle = initCURLHandle(withBearer);
	SetCURLURL(curlHandle, url);
	HTTPRequest* req;
	if (httpMethod == RequestMethod::POST) {
		curl_httppost* form = nullptr;
		curl_httppost* lastPtr = nullptr;
		for (auto& param : params)
			CURLFormPostField(form,
							  lastPtr,
							  param.first.c_str(),
							  param.second.c_str());
		curl_easy_setopt_log_err(curlHandle, CURLOPT_HTTPPOST, form);
		req = new HTTPRequest(curlHandle, done, form);
	} else if (httpMethod == RequestMethod::PATCH) {
		curl_httppost* form = nullptr;
		curl_httppost* lastPtr = nullptr;
		for (auto& param : params)
			CURLFormPostField(form,
							  lastPtr,
							  param.first.c_str(),
							  param.second.c_str());
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
		if (!QueueRequestIfRatelimited(url, *req)) {
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
DownloadManager::HandleRatelimitResponse(const std::string& endpoint, HTTPRequest& req)
{
	auto& status = req.response_code;
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
DownloadManager::QueueRequestIfRatelimited(const std::string& endpoint, HTTPRequest& req) {
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
DownloadManager::QueueRatelimitedRequest(const std::string& endpoint, HTTPRequest& req) {
	{
		const std::lock_guard<std::mutex> lock(G_MTX_HTTP_REQS);
		if (ratelimitedRequestQueue.count(endpoint) == 0u) {
			ratelimitedRequestQueue.emplace(endpoint,
											std::vector<HTTPRequest*>());
		}
		ratelimitedRequestQueue.at(endpoint).push_back(&req);
	}
}


void
DownloadManager::RefreshCountryCodes()
{
	Locator::getLogger()->warn("REFRESH COUNTRY CODES NOT IMPLEMENTED");
	return;

	/*

	auto done = [this](HTTPRequest& req) {
		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "RefreshCountryCodes Error: Malformed request response: {}",
			  req.result);
			return;
		}
		if (d.HasMember("data") && d["data"].IsArray())
			for (auto& code_obj : d["data"].GetArray()) {
				if (code_obj.HasMember("id") && code_obj["id"].IsString())
					countryCodes.push_back(code_obj["id"].GetString());
				else
					countryCodes.push_back("");
			}
		// append the list to global/player country code so
		// we dont have to merge tables in lua -mina
		countryCodes.push_back(std::string("Global"));
	};
	SendRequest("/misc/countrycodes",
				std::vector<std::pair<std::string, std::string>>(),
				done,
				true);
				*/
}

void
DownloadManager::RequestReplayData(const std::string& scoreid,
								   int userid,
								   const std::string& username,
								   const std::string& chartkey,
								   LuaReference& callback)
{
	auto queryPath = fmt::format(API_GET_SCORE, scoreid);
	Locator::getLogger()->info(
	  "Generating replay data request for scoreid {} - {}", scoreid, chartkey);

	auto runLuaFunc =
	  [callback](std::vector<std::pair<float, float>>& replayData,
				 const std::vector<OnlineScore>::iterator& it,
				 const std::vector<OnlineScore>::iterator& itEnd) {
		  
		  if (!callback.IsNil() && callback.IsSet()) {
			  Locator::getLogger()->info(
				"RequestReplayData finished - running callback function");
			  auto L = LUA->Get();
			  callback.PushSelf(L);
			  std::string Error =
				"Error running RequestReplayData Finish Function: ";
			  lua_newtable(L);
			  for (unsigned i = 0; i < replayData.size(); ++i) {
				  auto& pair = replayData[i];
				  lua_newtable(L);
				  lua_pushnumber(L, pair.first);
				  lua_rawseti(L, -2, 1);
				  lua_pushnumber(L, pair.second);
				  lua_rawseti(L, -2, 2);
				  lua_rawseti(L, -2, i + 1);
			  }
			  if (it != itEnd)
				  it->hs.PushSelf(L);
			  // 2 args, 0 results
			  LuaHelpers::RunScriptOnStack(L, Error, 2, 0, true);
			  LUA->Release(L);
		  } else {
			  Locator::getLogger()->info(
				"RequestReplayData finished, but no callback was set");
		  }
	  };

	auto done = [runLuaFunc, scoreid, userid, username, chartkey, &callback, this](HTTPRequest& req) {
		std::vector<std::pair<float, float>> replayData{};

		auto& lbd = chartLeaderboards[chartkey];
		if (HandleAuthErrorResponse(API_GET_SCORE, req)) {
			runLuaFunc(replayData, lbd.end(), lbd.end());
			return;
		}
		if (HandleRatelimitResponse(API_GET_SCORE, req)) {
			RequestReplayData(scoreid, userid, username, chartkey, callback);
			return;
		}

		auto response = req.response_code;
		if (response == 404) {
			// no score was found
			Locator::getLogger()->warn(
			  "RequestReplayData 404'd because there is no score id {} (ck {})",
			  scoreid, chartkey);
			runLuaFunc(replayData, lbd.end(), lbd.end());
			return;
		}

		
		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "RequestReplayData Error: Malformed request response: {}",
			  req.result);
		}

		if (response == 200) {
			// found a score. maybe it has replay data

			if (d.HasMember("data") && d["data"].IsObject()) {

				auto& data = d["data"];
				if (data.HasMember("replay_data") &&
					data["replay_data"].IsArray()) {
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
						  "RequestReplayData could not save replay data "
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
				} else {
					Locator::getLogger()->warn(
					  "RequestReplayData didn't find replay data for score {}  "
					  "(ck {}) - Object Missing? {}",
					  scoreid,
					  chartkey,
					  data.HasMember("replay_data"));
				}

			} else {
				Locator::getLogger()->warn("RequestReplayData got unexpected "
										   "response body - Content: {}",
										   jsonObjectToString(d));
			}
		} else {
			Locator::getLogger()->warn(
			  "RequestReplayData unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}

		// anything falling through to here has no replay data
		runLuaFunc(replayData, lbd.end(), lbd.end());
	};

	SendRequest(queryPath, {}, done, true);
}

void
DownloadManager::RequestChartLeaderBoard(const std::string& chartkey,
										 LuaReference& ref)
{
	auto queryPath = fmt::format(API_CHART_LEADERBOARD, chartkey);
	Locator::getLogger()->info("Generating chart leaderboard request for {}",
							   chartkey);

	std::vector<std::pair<std::string, std::string>> params = {
		//std::make_pair("sort", ""),
		std::make_pair("limit", "999999"),
		//std::make_pair("page", "")
	};

	std::vector<OnlineScore>& vec = chartLeaderboards[chartkey];
	vec.clear();

	auto runLuaFunc = [ref](HTTPRequest& req, std::vector<OnlineScore>& vec) {
		if (!ref.IsNil() && ref.IsSet()) {
			Locator::getLogger()->info(
			  "RequestChartLeaderBoard finished - running callback function");
			auto L = LUA->Get();
			ref.PushSelf(L);
			if (!lua_isnil(L, -1)) {
				std::string Error =
				  "Error running RequestChartLeaderBoard Finish Function: ";
				auto response = req.response_code;

				// 404: Chart not ranked
				// 401: Invalid login token
				// 429: Rate Limited
				if (response == 404 || response == 401) {
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
			  "RequestChartLeaderBoard finished, but no callback was set");
		}
	};

	auto done = [&ref, chartkey, runLuaFunc, &vec, this](HTTPRequest& req) {
		if (HandleAuthErrorResponse(API_CHART_LEADERBOARD, req)) {
			runLuaFunc(req, vec);
			return;
		}
		if (HandleRatelimitResponse(API_CHART_LEADERBOARD, req)) {
			RequestChartLeaderBoard(chartkey, ref);
			return;
		}

		auto response = req.response_code;
		if (response == 404) {
			// chart is unranked
			unrankedCharts.emplace(chartkey);
			runLuaFunc(req, vec);

			Locator::getLogger()->warn("RequestChartLeaderboard 404'd because "
									   "this chart is unranked: {}",
									   chartkey);
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "RequestChartLeaderboard Error: Malformed request response: {}",
			  req.result);
			return;
		}


		if (response == 200) {
			// chart is ranked. leaderboard has [0,inf] scores

			if (d.HasMember("data") && d["data"].IsArray()) {

				auto count = 0;
				auto& data = d["data"];
				for (auto it = data.Begin(); it != data.End(); it++) {
					count++;
					vec.push_back(jsonToOnlineScore(*it, chartkey));
				}
				unrankedCharts.erase(chartkey);
				runLuaFunc(req, vec);

				Locator::getLogger()->info(
				  "RequestChartLeaderboard for {} succeeded - {} scores found",
				  chartkey,
				  count);
			} else {
				Locator::getLogger()->warn(
				  "RequestChartLeaderboard got unexpected response body - "
				  "Content: {}",
				  jsonObjectToString(d));
			}
		} else {
			Locator::getLogger()->warn(
			  "RequestChartLeaderboard unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(queryPath, params, done, true);
}

void
DownloadManager::RefreshCoreBundles()
{
	Locator::getLogger()->warn("REFRESH CORE BUNDLES NOT IMPLEMENTED");
	return;
	/*

	auto done = [this](HTTPRequest& req) {
		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "RefreshCoreBundles Error: Malformed request response: {}",
			  req.result);
			return;
		}

		if (d.HasMember("data") && d["data"].IsArray()) {
			auto& dlPacks = downloadablePacks;
			for (auto& bundleData : d["data"].GetArray()) {
				if (!bundleData.HasMember("id") ||
					!bundleData["id"].IsString() ||
					!bundleData.HasMember("attributes") ||
					!bundleData["attributes"].IsObject() ||
					!bundleData["attributes"].HasMember("packs") ||
					!bundleData["attributes"]["packs"].IsArray())
					continue;
				auto bundleName = bundleData["id"].GetString();
				(bundles)[bundleName] = {};
				auto& bundle = (bundles)[bundleName];
				for (auto& pack :
					 bundleData["attributes"]["packs"].GetArray()) {
					if (!pack.HasMember("packname") ||
						!pack["packname"].IsString())
						continue;
					auto name = pack["packname"].GetString();
					auto dlPack = std::find_if(
					  dlPacks.begin(),
					  dlPacks.end(),
					  [&name](DownloadablePack x) { return x.name == name; });
					if (dlPack != dlPacks.end())
						bundle.push_back(&(*dlPack));
				}
			}
		}
		if (MESSAGEMAN != nullptr)
			MESSAGEMAN->Broadcast("CoreBundlesRefreshed");
	};
	SendRequest("packs/collections/", {}, done, false);
	*/
}

std::vector<DownloadablePack*>
DownloadManager::GetCoreBundle(const std::string& whichoneyo)
{
	return bundles.count(whichoneyo) ? bundles[whichoneyo]
									 : std::vector<DownloadablePack*>();
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

	std::vector<std::pair<std::string, std::string>> params = {};

	auto done = [this](HTTPRequest& req) {
		if (HandleRatelimitResponse(API_USER, req)) {
			RefreshUserData();
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "RefreshLastVersion Error: Malformed request response: {}",
			  req.result);
			return;
		}

		auto response = req.response_code;
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

	SendRequest(
	  API_GAME_VERSION, params, done, false, RequestMethod::GET, true, false);
}

void
DownloadManager::RefreshRegisterPage()
{
	Locator::getLogger()->warn("REFRESH REGISTER PAGE NOT IMPLEMENTED");
	return;
	/*
	auto done = [this](HTTPRequest& req) {
		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "RefreshRegisterPage Error: Malformed request response: {}",
			  req.result);
			return;
		}

		if (d.HasMember("data") && d["data"].IsObject() &&
			d["data"].HasMember("attributes") &&
			d["data"]["attributes"].IsObject() &&
			d["data"]["attributes"].HasMember("url") &&
			d["data"]["attributes"]["url"].IsString())
			registerPage = d["data"]["attributes"]["url"].GetString();
		else
			registerPage = "";
	};
	SendRequest("client/registration",
				std::vector<std::pair<std::string, std::string>>(),
				done,
				false,
				false,
				true);
				*/
}
void
DownloadManager::RefreshTop25(Skillset ss)
{
	topScores[ss].clear();
	if (!LoggedIn())
		return;

	Locator::getLogger()->warn("REFRESH TOP 25 NOT IMPLEMENTED");
	return;
	/*

	std::string req = "user/" + UrlEncode(sessionUser) + "/top/";
	if (ss != Skill_Overall)
		req += SkillsetToString(ss) + "/25";
	auto done = [ss, this](HTTPRequest& req) {
		Document d;
		if (d.Parse(req.result.c_str()).HasParseError() ||
			(d.HasMember("errors") && d["errors"].IsArray() &&
			 d["errors"][0].HasMember("status") &&
			 d["errors"][0]["status"].GetInt() == 404) ||
			!d.HasMember("data") || !d["data"].IsArray()) {
			Locator::getLogger()->error(
			  "Malformed top25 scores request response: {}", req.result);
			return;
		}
		std::vector<OnlineTopScore>& vec = topScores[ss];
		auto& scores = d["data"];
		for (auto& score_obj : scores.GetArray()) {
			if (!score_obj.HasMember("attributes")) {
				Locator::getLogger()->warn(
				  "Malformed single score in top25 scores request response: {}",
				  jsonObjectToString(score_obj));
				continue;
			}
			auto& score = score_obj["attributes"];
			if (!score.HasMember("songName") || !score["songName"].IsString() ||
				!score.HasMember("wife") || !score["wife"].IsNumber() ||
				!score.HasMember("Overall") || !score["Overall"].IsNumber() ||
				!score.HasMember("chartKey") || !score["chartKey"].IsString() ||
				!score_obj.HasMember("id") || !score_obj["id"].IsString() ||
				!score.HasMember("rate") || !score["rate"].IsNumber() ||
				!score.HasMember("difficulty") ||
				!score["difficulty"].IsString() ||
				!score.HasMember("skillsets") ||
				(ss != Skill_Overall &&
				 (!score["skillsets"].HasMember(SkillsetToString(ss).c_str()) ||
				  !score["skillsets"][SkillsetToString(ss).c_str()]
					 .IsNumber()))) {
				Locator::getLogger()->warn(
				  "Malformed single score in top25 scores request response: {}",
				  jsonObjectToString(score_obj));
				continue;
			}
			OnlineTopScore tmp;
			tmp.songName = score["songName"].GetString();
			tmp.wifeScore = score["wife"].GetFloat() / 100.f;
			tmp.overall = score["Overall"].GetFloat();
			if (ss != Skill_Overall)
				tmp.ssr =
				  score["skillsets"][SkillsetToString(ss).c_str()].GetFloat();
			else
				tmp.ssr = tmp.overall;
			tmp.chartkey = score["chartKey"].GetString();
			tmp.scorekey = score_obj["id"].GetString();
			tmp.rate = score["rate"].GetFloat();
			tmp.difficulty =
			  StringToDifficulty(score["difficulty"].GetString());
			vec.push_back(tmp);
		}
		MESSAGEMAN->Broadcast("OnlineUpdate");
	};
	SendRequest(req, {}, done);
	*/
}

void
DownloadManager::RefreshUserData()
{
	if (!LoggedIn())
		return;

	auto queryPath = fmt::format(API_USER, sessionUser);
	Locator::getLogger()->info("Refreshing UserData for {}", sessionUser);

	std::vector<std::pair<std::string, std::string>> params = {};

	auto done = [this](HTTPRequest& req) {
		if (HandleAuthErrorResponse(API_USER, req)) {
			return;
		}
		if (HandleRatelimitResponse(API_USER, req)) {
			RefreshUserData();
			return;
		}

		auto response = req.response_code;
		if (response == 404) {
			// user doesnt exist :eyebrow_raise:
			Locator::getLogger()->warn(
			  "RefreshUserData 404'd because user does not exist: {}",
			  sessionUser);
			return;
		}

		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error(
			  "RefreshUserData Error: Malformed request response: {}",
			  req.result);
			return;
		}

		if (response == 200) {
			// it probably worked

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
			Locator::getLogger()->warn(
			  "RefreshUserData unexpected response {} - Content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};

	SendRequest(queryPath, params, done, true);
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

	auto done = [this](HTTPRequest& req) {
		Document d;
		if (d.Parse(req.result.c_str()).HasParseError()) {
			Locator::getLogger()->error("RefreshPackList Error: status {} & "
										"response parse error - content: {}",
										req.response_code,
										req.result);
			return;
		}

		auto response = req.response_code;
		if (response == 200) {
			if (!d.HasMember("data") || !d["data"].IsArray()) {
				Locator::getLogger()->error(
				  "RefreshPackList Error: response data was missing or not an "
				  "array - content: {}",
				  jsonObjectToString(d));
				return;
			}

			auto& data = d["data"];
			auto& packlist = downloadablePacks;
			packlist.clear();

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

					packlist.push_back(packDl);
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
			RefreshCoreBundles();
		} else {
			Locator::getLogger()->error(
			  "RefreshPackList unexpected response code {} - content: {}",
			  response,
			  jsonObjectToString(d));
		}
	};
	SendRequestToURL(url, {}, done, false, RequestMethod::GET, true, false);
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
	if (!DownloadManager::InstallSmzip(m_TempFileName))
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
						   [this](pair<DownloadablePack*, bool> pair) {
							   return pair.first == this;
						   });
	return it != DLMAN->DownloadQueue.end();
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"
#include "LuaManager.h"
/** @brief Allow Lua to have access to the ProfileManager. */
class LunaDownloadManager : public Luna<DownloadManager>
{
  public:
	static int GetCountryCodes(T* p, lua_State* L)
	{
		auto& codes = p->countryCodes;
		LuaHelpers::CreateTableFromArray(codes, L);
		return 1;
	}
	static int GetUserCountryCode(T* p, lua_State* L)
	{
		lua_pushstring(L, p->countryCode.c_str());
		return 1;
	}
	static int GetAllPacks(T* p, lua_State* L)
	{
		std::vector<DownloadablePack>& packs = p->downloadablePacks;
		lua_createtable(L, packs.size(), 0);
		for (unsigned i = 0; i < packs.size(); ++i) {
			packs[i].PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	static int GetDownloadingPacks(T* p, lua_State* L)
	{
		std::vector<DownloadablePack>& packs = p->downloadablePacks;
		std::vector<DownloadablePack*> dling;
		for (auto& pack : packs) {
			if (pack.downloading)
				dling.push_back(&pack);
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
	static int GetRegisterPage(T* p, lua_State* L)
	{
		lua_pushstring(L, p->registerPage.c_str());
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
				LuaHelpers::RunScriptOnStack(
				  L, Error, 2, 0, true); // 2 args, 0 results
			}
			return 0;
		}

		p->RequestReplayData(scoreid, userid, username, ck, f);
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
					std::string Error = "Error running RequestChartLeaderBoard "
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
		p->RequestChartLeaderBoard(chart, ref);

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
			if (p->ccoffonly && !score.nocc)
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

		LuaHelpers::CreateTableFromArray(filteredLeaderboardScores, L);
		return 1;
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
	static int ToggleCCFilter(T* p, lua_State* L)
	{
		p->ccoffonly = !p->ccoffonly;
		return 0;
	}
	static int GetCCFilter(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->ccoffonly);
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
	LunaDownloadManager()
	{
		ADD_METHOD(GetCountryCodes);
		ADD_METHOD(GetUserCountryCode);
		ADD_METHOD(DownloadCoreBundle);
		ADD_METHOD(GetCoreBundle);
		ADD_METHOD(GetAllPacks);
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
		ADD_METHOD(GetRegisterPage);
		ADD_METHOD(RequestChartLeaderBoardFromOnline);
		ADD_METHOD(RequestOnlineScoreReplayData);
		ADD_METHOD(GetChartLeaderBoard);
		ADD_METHOD(ToggleRateFilter);
		ADD_METHOD(GetCurrentRateFilter);
		ADD_METHOD(ToggleTopScoresOnlyFilter);
		ADD_METHOD(GetTopScoresOnlyFilter);
		ADD_METHOD(ToggleCCFilter);
		ADD_METHOD(GetCCFilter);
		ADD_METHOD(SendReplayDataForOldScore);
		ADD_METHOD(UploadScoresForChart);
		ADD_METHOD(UploadScoresForPack);
		ADD_METHOD(UploadAllScores);
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
		lua_pushnumber(L, p->size);
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
