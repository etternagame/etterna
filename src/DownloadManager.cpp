#pragma once

#include "global.h"
#if !defined(WITHOUT_NETWORKING)
#include "RageFileManager.h"
#include "ScreenManager.h"
#include "Preference.h"
#include "RageLog.h"
#include "RageFile.h"
#include "DownloadManager.h"
#include "ScoreManager.h"
#include "RageFileManager.h"
#include "ProfileManager.h"
#include "SongManager.h"
#include "ScreenInstallOverlay.h"
#include "CommandLineActions.h"
#include "ScreenSelectMusic.h"
#include "SpecialFiles.h"
#include "curl/curl.h"
#include "JsonUtil.h"
#include "Foreach.h"
#include "Song.h"

#ifdef _WIN32 
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

shared_ptr<DownloadManager> DLMAN = nullptr;

static Preference<unsigned int> maxDLPerSecond("maximumBytesDownloadedPerSecond", 0);
static Preference<unsigned int> maxDLPerSecondGameplay("maximumBytesDownloadedPerSecondDuringGameplay", 300000);
static Preference<RString> packListURL("packListURL", "https://etternaonline.com/api/pack_list");
static Preference<RString> serverURL("UploadServerURL", "https://api.etternaonline.com/v1/");
static Preference<unsigned int> automaticSync("automaticScoreSync", 1);
static const string TEMP_ZIP_MOUNT_POINT = "/@temp-zip/";

size_t write_memory_buffer(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	string tmp(static_cast<char*>(contents), realsize);
	static_cast<string*>(userp)->append(tmp);
	return realsize;

}

class ReadThis {
public:
	RageFile file;
};

string ComputerIdentity() {
	string computerName = "";
	string userName = "";
#ifdef _WIN32 
	TCHAR  infoBuf[1024];
	DWORD  bufCharCount = 1024;
	if (GetComputerName(infoBuf, &bufCharCount))
		computerName = infoBuf;
	if (GetUserName(infoBuf, &bufCharCount))
		userName = infoBuf;
#else
	char hostname[1024];
	char username[1024];
	gethostname(hostname, 1024);
	getlogin_r(username, 1024);
	computerName - hostname;
	userName = username;
#endif
	return computerName + ":_:" + userName;
}
size_t ReadThisReadCallback(void *dest, size_t size, size_t nmemb, void *userp)
{
	auto rt = static_cast<ReadThis*>(userp);
	size_t buffer_size = size*nmemb;


	return rt->file.Read(dest, buffer_size);
}

int ReadThisSeekCallback(void *arg, curl_off_t offset, int origin)
{
	return static_cast<ReadThis*>(arg)->file.Seek(offset, origin);
}

bool DownloadManager::InstallSmzip(const string &sZipFile)
{
	if (!FILEMAN->Mount("zip", sZipFile, TEMP_ZIP_MOUNT_POINT))
		FAIL_M(static_cast<string>("Failed to mount " + sZipFile).c_str());
	vector<RString> v_packs;
	GetDirListing(TEMP_ZIP_MOUNT_POINT + "*", v_packs, true, true);
	if (v_packs.size() > 1) 
		return false;
	vector<string> vsFiles;
	{
		vector<RString> vsRawFiles;
		GetDirListingRecursive(TEMP_ZIP_MOUNT_POINT, "*", vsRawFiles);

		vector<string> vsPrettyFiles;
		FOREACH_CONST(RString, vsRawFiles, s)
		{
			if (GetExtension(*s).EqualsNoCase("ctl"))
				continue;

			vsFiles.push_back(*s);

			string s2 = s->Right(s->length() - TEMP_ZIP_MOUNT_POINT.length());
			vsPrettyFiles.push_back(s2);
		}
		sort(vsPrettyFiles.begin(), vsPrettyFiles.end());
	}
	string sResult = "Success installing " + sZipFile;
	FOREACH_CONST(string, vsFiles, sSrcFile)
	{
		string sDestFile = *sSrcFile;
		sDestFile = RString(sDestFile.c_str()).Right(sDestFile.length() - TEMP_ZIP_MOUNT_POINT.length());

		RString sDir, sThrowAway;
		splitpath(sDestFile, sDir, sThrowAway, sThrowAway);


		if (!FileCopy(*sSrcFile, "Songs/" + sDestFile))
		{
			sResult = "Error extracting " + sDestFile;
			break;
		}
	}

	FILEMAN->Unmount("zip", sZipFile, TEMP_ZIP_MOUNT_POINT);


	SCREENMAN->SystemMessage(sResult);
	return true;
}



//Functions used to read/write data
int progressfunc(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	auto ptr = static_cast<ProgressData*>(clientp);
	ptr->total = dltotal;
	ptr->downloaded = dlnow;
	return 0;

}
size_t write_data(void *dlBuffer, size_t size, size_t nmemb, void *pnf)
{
	auto RFW = static_cast<RageFileWrapper*>(pnf);
	size_t b = RFW->stop ? 0 : RFW->file.Write(dlBuffer, size*nmemb);
	RFW->bytes += b;
	return b; 
}
//A couple utility inline string functions
inline bool ends_with(std::string const & value, std::string const & ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}
inline bool starts_with(std::string const & value, std::string const & start)
{
	return value.rfind(start, 0) == 0;
}
inline void checkProtocol(string& url)
{
	if (!(starts_with(url, "https://") || starts_with(url, "http://")))
		url = string("http://").append(url);
}
//Utility inline functions to deal with CURL
inline CURL* initCURLHandle() {
	CURL *curlHandle = curl_easy_init();
	curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYHOST, 0L);
	return curlHandle;
}
inline bool addFileToForm(curl_httppost *&form, curl_httppost *&lastPtr, string field, string fileName, string filePath, RString &contents)
{
	RageFile rFile;
	if (!rFile.Open(filePath))
		return false;
	rFile.Read(contents, rFile.GetFileSize());
	rFile.Close();
	curl_formadd(&form,
		&lastPtr,
		CURLFORM_COPYNAME, field.c_str(),
		CURLFORM_BUFFER, fileName.c_str(),
		CURLFORM_BUFFERPTR, contents.c_str(),
		CURLFORM_BUFFERLENGTH, 0,
		CURLFORM_END);
	return true;
}
inline void DownloadManager::AddSessionCookieToCURL(CURL *curlHandle)
{
	curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, ""); /* start cookie engine */
	curl_easy_setopt(curlHandle, CURLOPT_COOKIE, ("ci_session=" + session + ";").c_str());
}
inline void SetCURLResultsString(CURL *curlHandle, string& str)
{
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &str);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_memory_buffer);
}
inline void DownloadManager::SetCURLURL(CURL *curlHandle, string url)
{
	checkProtocol(url);
	EncodeSpaces(url);
	curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
}
inline void DownloadManager::SetCURLPostToURL(CURL *curlHandle, string url)
{
	SetCURLURL(curlHandle, url);
	curl_easy_setopt(curlHandle, CURLOPT_POST, 1L);
}
void CURLFormPostField(CURL* curlHandle, curl_httppost *&form, curl_httppost *&lastPtr, const char* field, const char* value)
{
	curl_formadd(&form,
		&lastPtr, 
		CURLFORM_COPYNAME, field, 
		CURLFORM_COPYCONTENTS, value,
		CURLFORM_END);
}
inline void SetCURLFormPostField(CURL* curlHandle, curl_httppost *&form, curl_httppost *&lastPtr, char* field, char* value)
{
	CURLFormPostField(curlHandle, form, lastPtr, field, value);
}
inline void SetCURLFormPostField(CURL* curlHandle, curl_httppost *&form, curl_httppost *&lastPtr, const char* field, string value)
{
	CURLFormPostField(curlHandle, form, lastPtr, field, value.c_str());
}
inline void SetCURLFormPostField(CURL* curlHandle, curl_httppost *&form, curl_httppost *&lastPtr, string field, string value)
{
	CURLFormPostField(curlHandle, form, lastPtr, field.c_str(), value.c_str());
}
template<typename T>
inline void SetCURLFormPostField(CURL* curlHandle, curl_httppost *&form, curl_httppost *&lastPtr, string field, T value)
{
	CURLFormPostField(curlHandle, form, lastPtr, field.c_str(), to_string(value).c_str());
}
DownloadManager::DownloadManager() {
	curl_global_init(CURL_GLOBAL_ALL);
	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring(L, "DLMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
	CachePackList(packListURL);
}

DownloadManager::~DownloadManager()
{
	if (mPackHandle != nullptr)
		curl_multi_cleanup(mPackHandle);
	mPackHandle = nullptr;
	if (mHTTPHandle != nullptr)
		curl_multi_cleanup(mHTTPHandle);
	mHTTPHandle = nullptr;
	for (auto &dl : downloads) {
		if (dl.second->handle != nullptr) {
			curl_easy_cleanup(dl.second->handle);
			dl.second->handle = nullptr;
		}
		delete dl.second;
	}
	curl_global_cleanup();
	if (LoggedIn())
		EndSession();
}

Download* DownloadManager::DownloadAndInstallPack(const string &url)
{	
	Download* dl = new Download(url);

	if (mPackHandle == nullptr)
		mPackHandle = curl_multi_init();
	curl_multi_add_handle(mPackHandle, dl->handle);
	downloads[url] = dl;

	UpdateDLSpeed();

	ret = curl_multi_perform(mPackHandle, &downloadingPacks);
	SCREENMAN->SystemMessage(dl->StartMessage());

	return dl;
}

void DownloadManager::UpdateDLSpeed()
{
	size_t maxDLSpeed;
	if (this->gameplay) {
		maxDLSpeed = maxDLPerSecondGameplay;
	}
	else {
		maxDLSpeed = maxDLPerSecond;
	}
	for (auto &x : downloads)
		curl_easy_setopt(x.second->handle, CURLOPT_MAX_RECV_SPEED_LARGE, static_cast<curl_off_t>(maxDLSpeed / downloads.size()));
}

void DownloadManager::UpdateDLSpeed(bool gameplay)
{
	this->gameplay = gameplay;
	UpdateDLSpeed();
}

bool DownloadManager::EncodeSpaces(string& str)
{

	//Parse spaces (curl doesnt parse them properly)
	bool foundSpaces = false;
	size_t index = str.find(" ", 0);
	while (index != string::npos) {

		str.erase(index, 1);
		str.insert(index, "%20");
		index = str.find(" ", index);
		foundSpaces = true;
	}
	return foundSpaces;
}

void Download::Update(float fDeltaSeconds)
{
	progress.time += fDeltaSeconds;
	if (progress.time > 1.0) {
		speed = to_string(progress.downloaded / 1024 - downloadedAtLastUpdate);
		progress.time = 0;
		downloadedAtLastUpdate = progress.downloaded / 1024;
	}
}
Download* DownloadManager::DownloadAndInstallPack(DownloadablePack* pack)
{
	auto songs = SONGMAN->GetAllSongs();
	vector<RString> packs;
	SONGMAN->GetSongGroupNames(packs);
	for (auto packName : packs) {
		if (packName == pack->name) {
			SCREENMAN->SystemMessage("Already have pack " + packName + ", not downloading");
			return nullptr;
		}
	}
	Download* dl = DownloadAndInstallPack(pack->url);
	dl->p_Pack = pack;
	return dl;
}

bool DownloadManager::UpdateAndIsFinished(float fDeltaSeconds)
{
	bool packs = UpdatePacksAndIsFinished(fDeltaSeconds);
	bool http = UpdateHTTPAndIsFinished(fDeltaSeconds);
	return packs && http;
}
bool DownloadManager::UpdateHTTPAndIsFinished(float fDeltaSeconds)
{
	if (!HTTPRunning && HTTPRequests.size() == 0)
		return true;
	timeval timeout;
	int rc, maxfd = -1;
	CURLMcode mc;
	fd_set fdread, fdwrite, fdexcep;
	long curl_timeo = -1;
	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdexcep);
	timeout.tv_sec = 0;
	timeout.tv_usec = 1;
	curl_multi_timeout(mHTTPHandle, &curl_timeo);

	mc = curl_multi_fdset(mHTTPHandle, &fdread, &fdwrite, &fdexcep, &maxfd);
	if (mc != CURLM_OK) {
		error = "curl_multi_fdset() failed, code " + mc;
		return false;
	}
	if (maxfd == -1) {
		rc = 0;
	}
	else {
		rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
	}
	switch (rc) {
	case -1:
		error = "select error" + mc;
		break;
	case 0: /* timeout */
	default: /* action */
		curl_multi_perform(mHTTPHandle, &HTTPRunning);
		break;
	}

	//Check for finished downloads
	CURLMsg *msg;
	int msgs_left;
	while (msg = curl_multi_info_read(mHTTPHandle, &msgs_left)) {
		/* Find out which handle this message is about */
		for (int i = 0; i < HTTPRequests.size();++i) {
			if (msg->easy_handle == HTTPRequests[i]->handle) {
				if (msg->msg == CURLMSG_DONE) {
					HTTPRequests[i]->Done(*(HTTPRequests[i]));
				}
				else {
					HTTPRequests[i]->Failed(*(HTTPRequests[i]));
				}
				if (HTTPRequests[i]->handle != nullptr)
					curl_easy_cleanup(HTTPRequests[i]->handle);
				HTTPRequests[i]->handle = nullptr;
				if (HTTPRequests[i]->form != nullptr)
					curl_formfree(HTTPRequests[i]->form);
				HTTPRequests[i]->form = nullptr;
				delete  HTTPRequests[i];
				HTTPRequests.erase(HTTPRequests.begin()+i);
				break;
			}
		}
	}
	return false;
}
bool DownloadManager::UpdatePacksAndIsFinished(float fDeltaSeconds)
{
	if (reloadPending && !gameplay) {
		auto screen = SCREENMAN->GetScreen(0);
		if (screen && screen->GetName() == "ScreenSelectMusic")
			static_cast<ScreenSelectMusic*>(screen)->DifferentialReload();
		else
			SONGMAN->DifferentialReload();
		reloadPending = false;
	}
	if (!downloadingPacks)
		return true;
	timeval timeout;
	int rc, maxfd = -1;
	CURLMcode mc;
	fd_set fdread, fdwrite, fdexcep;
	long curl_timeo = -1;
	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdexcep);
	timeout.tv_sec = 0;
	timeout.tv_usec = 1;
	curl_multi_timeout(mPackHandle, &curl_timeo);

	mc = curl_multi_fdset(mPackHandle, &fdread, &fdwrite, &fdexcep, &maxfd);
	if (mc != CURLM_OK) {
		error = "curl_multi_fdset() failed, code " + mc;
		return false;
	}
	if (maxfd == -1) {
		rc = 0;
	}
	else {
		rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
	}
	switch (rc) {
	case -1:
		error = "select error" + mc;
		break;
	case 0: /* timeout */
	default: /* action */
		curl_multi_perform(mPackHandle, &downloadingPacks);
		for (auto &dl : downloads) 
			dl.second->Update(fDeltaSeconds);
		break;
	}

	//Check for finished downloads
	CURLMsg *msg;
	int msgs_left;
	bool addedPacks = false;	
	while (msg = curl_multi_info_read(mPackHandle, &msgs_left)) {
		/* Find out which handle this message is about */
		for (auto i = downloads.begin(); i != downloads.end(); i++) {
			if (msg->easy_handle == i->second->handle) {
				if (i->second->p_RFWrapper.file.IsOpen())
					i->second->p_RFWrapper.file.Close();
				if (msg->msg == CURLMSG_DONE) {
					addedPacks = true;
					i->second->Install();
				}
				else {
					i->second->Failed();
				}
				if (i->second->handle != nullptr)
					curl_easy_cleanup(i->second->handle);
				i->second->handle = nullptr;
				if (i->second->p_Pack != nullptr)
					i->second->p_Pack->downloading = false;
				finishedDownloads[i->second->m_Url] = i->second;
				downloads.erase(i);
				break;
			}
		}
	}
	if (addedPacks) {
		curl_off_t maxDLSpeed = 0;
		auto screen = SCREENMAN->GetScreen(0);
		if (screen && screen->GetName() == "ScreenSelectMusic")
			static_cast<ScreenSelectMusic*>(screen)->DifferentialReload();
		else if (!gameplay)
			SONGMAN->DifferentialReload();
		else
			reloadPending = true;
		UpdateDLSpeed();
	}

	return false;

}

string Download::MakeTempFileName(string s)
{
	return SpecialFiles::CACHE_DIR + "Downloads/" + Basename(s);
}
bool DownloadManager::LoggedIn()
{
	return !session.empty();
}

bool DownloadManager::UploadProfile(string file, string profileName)
{
	if (!LoggedIn())
		return false;
	string url = serverURL.Get() + "/upload_xml";
	CURL *curlHandle = initCURLHandle();
	curl_httppost *form = nullptr;
	curl_httppost *lastPtr = nullptr;
	curl_slist *headerlist = nullptr;
	RString contents;
	if (!addFileToForm(form, lastPtr, "xml", "etterna.xml", file, contents))
		return false;
	ComputerIdentity();
	SetCURLFormPostField(curlHandle, form, lastPtr, "origin", ComputerIdentity()+":_:"+profileName);
	SetCURLPostToURL(curlHandle, url);
	AddSessionCookieToCURL(curlHandle);
	string result;
	SetCURLResultsString(curlHandle, result);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form);
	CURLcode ret = curl_easy_perform(curlHandle);
	curl_easy_cleanup(curlHandle);
	curl_formfree(form);
	if (result != "\"Success\"") {
		LOG->Trace(result.c_str());
		return false;
	}
	return ret == 0;
}

bool DownloadManager::ShouldUploadScores()
{
	return LoggedIn() && automaticSync;
}
inline void SetCURLPOSTScore(CURL*& curlHandle, curl_httppost*& form, curl_httppost*& lastPtr, HighScore*& hs)
{
	SetCURLFormPostField(curlHandle, form, lastPtr, "scorekey", hs->GetScoreKey());
	FOREACH_ENUM(Skillset, ss)
		SetCURLFormPostField(curlHandle, form, lastPtr, SkillsetToString(ss), hs->GetSkillsetSSR(ss));
	SetCURLFormPostField(curlHandle, form, lastPtr, "ssr_norm", hs->GetSSRNormPercent());
	SetCURLFormPostField(curlHandle, form, lastPtr, "max_combo", hs->GetMaxCombo());
	SetCURLFormPostField(curlHandle, form, lastPtr, "valid", static_cast<int>(hs->GetEtternaValid()));
	SetCURLFormPostField(curlHandle, form, lastPtr, "mods", hs->GetModifiers());
	SetCURLFormPostField(curlHandle, form, lastPtr, "miss", hs->GetTapNoteScore(TNS_Miss));
	SetCURLFormPostField(curlHandle, form, lastPtr, "bad", hs->GetTapNoteScore(TNS_W5));
	SetCURLFormPostField(curlHandle, form, lastPtr, "good", hs->GetTapNoteScore(TNS_W4));
	SetCURLFormPostField(curlHandle, form, lastPtr, "great", hs->GetTapNoteScore(TNS_W3));
	SetCURLFormPostField(curlHandle, form, lastPtr, "perfect", hs->GetTapNoteScore(TNS_W2));
	SetCURLFormPostField(curlHandle, form, lastPtr, "marv", hs->GetTapNoteScore(TNS_W1));
	SetCURLFormPostField(curlHandle, form, lastPtr, "datetime", string(hs->GetDateTime().GetString().c_str()));
	SetCURLFormPostField(curlHandle, form, lastPtr, "hitmine", hs->GetTapNoteScore(TNS_HitMine));
	SetCURLFormPostField(curlHandle, form, lastPtr, "held", hs->GetHoldNoteScore(HNS_Held));
	SetCURLFormPostField(curlHandle, form, lastPtr, "letgo", hs->GetHoldNoteScore(HNS_LetGo));
	SetCURLFormPostField(curlHandle, form, lastPtr, "ng", hs->GetHoldNoteScore(HNS_Missed));
	SetCURLFormPostField(curlHandle, form, lastPtr, "chartkey", hs->GetChartKey());
	SetCURLFormPostField(curlHandle, form, lastPtr, "rate", hs->GetMusicRate());
	SetCURLFormPostField(curlHandle, form, lastPtr, "cc", static_cast<int>(!hs->GetChordCohesion()));
	SetCURLFormPostField(curlHandle, form, lastPtr, "calc_version", hs->GetSSRCalcVersion());
	SetCURLFormPostField(curlHandle, form, lastPtr, "topscore", hs->GetTopScore());
}
void DownloadManager::UploadScore(HighScore* hs)
{
	if (!LoggedIn())
		return;
	CURL *curlHandle = initCURLHandle();
	string url = serverURL.Get() + "/upload_score";
	curl_httppost *form = nullptr;
	curl_httppost *lastPtr = nullptr;
	SetCURLPOSTScore(curlHandle, form, lastPtr, hs);
	CURLFormPostField(curlHandle, form, lastPtr, "replay_data", "[]");
	SetCURLPostToURL(curlHandle, url);
	AddSessionCookieToCURL(curlHandle);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form);
	function<void(HTTPRequest&)> done = [hs](HTTPRequest& req) {
		if (req.result == "\"Success\"") {
			hs->AddUploadedServer(serverURL.Get());
		}
	};
	HTTPRequest* req = new HTTPRequest(curlHandle, done);
	SetCURLResultsString(curlHandle, req->result);
	curl_multi_add_handle(mHTTPHandle, req->handle);
	HTTPRequests.push_back(req);
	return;
}
void DownloadManager::UploadScoreWithReplayData(HighScore* hs) 
{
	if (!LoggedIn())
		return;
	CURL *curlHandle = initCURLHandle();
	string url = serverURL.Get() + "/upload_score";
	curl_httppost *form = nullptr;
	curl_httppost *lastPtr = nullptr;
	curl_slist *headerlist = nullptr;
	SetCURLPOSTScore(curlHandle, form, lastPtr, hs);
	string replayString = "[";
	vector<float> timestamps = hs->timeStamps;
	vector<float> offsets = hs->GetOffsetVector();
	for (int i = 0; i < offsets.size(); i++) {
		replayString += "[" + to_string(timestamps[i]) + "," + to_string(1000.f * offsets[i]) + "],";
	}
	replayString = replayString.substr(0, replayString.size() - 1); //remove ","
	replayString += "]";
	SetCURLFormPostField(curlHandle, form, lastPtr, "replay_data", replayString);
	SetCURLPostToURL(curlHandle, url);
	AddSessionCookieToCURL(curlHandle);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form);
	function<void(HTTPRequest&)> done = [hs](HTTPRequest& req) {
		if (req.result == "\"Success\"") {
			hs->AddUploadedServer(serverURL.Get());
		}
	};
	HTTPRequest* req = new HTTPRequest(curlHandle, done);
	SetCURLResultsString(curlHandle, req->result);
	curl_multi_add_handle(mHTTPHandle, req->handle);
	HTTPRequests.push_back(req);
	return;
}
void DownloadManager::EndSessionIfExists()
{
	if (!LoggedIn())
		return;
	EndSession();
	MESSAGEMAN->Broadcast("LogOut");
}
void DownloadManager::EndSession()
{
	string url = serverURL.Get() + "/destroy";
	CURL *curlHandle = initCURLHandle();
	SetCURLPostToURL(curlHandle, url);
	AddSessionCookieToCURL(curlHandle);
	CURLcode ret = curl_easy_perform(curlHandle);
	curl_easy_cleanup(curlHandle);
	session = sessionUser = sessionPass = sessionCookie = "";
	scores.clear();
	sessionRatings.clear();
}

std::vector<std::string> split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}
// User rank
void DownloadManager::RefreshUserRank() 
{
	if (!LoggedIn())
		return;
	string url = serverURL.Get() + "/user_rank";
	CURL *curlHandle = initCURLHandle();
	url += "?username=" + sessionUser;
	SetCURLURL(curlHandle, url);
	function<void(HTTPRequest&)> done = [](HTTPRequest& req) {
		Json::Value json;
		RString error;
		if (!JsonUtil::LoadFromString(json, req.result, error)) {
			FOREACH_ENUM(Skillset, ss)
				(DLMAN->sessionRanks)[ss] = 0.0f;
			return;
		}
		FOREACH_ENUM(Skillset, ss)
			(DLMAN->sessionRanks)[ss] = atoi(json.get(SkillsetToString(ss), "0").asCString());
		MESSAGEMAN->Broadcast("OnlineUpdate");
	};
	HTTPRequest* req = new HTTPRequest(curlHandle, done);
	SetCURLResultsString(curlHandle, req->result);
	AddSessionCookieToCURL(curlHandle);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPGET, 1L);
	curl_multi_add_handle(mHTTPHandle, req->handle);
	HTTPRequests.push_back(req);
	return;
}
OnlineScore DownloadManager::GetTopSkillsetScore(unsigned int rank, Skillset ss, bool &result)
{
	unsigned int index = rank - 1;
	if (index < scores[ss].size()) {
		result = true;
		return scores[ss][index];
	}
	result=false;
	return OnlineScore();
}

void DownloadManager::RefreshTop25(Skillset ss)
{
	if (!LoggedIn())
		return;
	string url = serverURL.Get() + "/user_top_scores?num=25";
	CURL *curlHandle = initCURLHandle();
	url += "&username=" + sessionUser;
	if(ss!= Skill_Overall)
		url += "&ss="+SkillsetToString(ss);
	SetCURLURL(curlHandle, url);
	function<void(HTTPRequest&)> done = [ss](HTTPRequest& req) {
		Json::Value json;
		RString error;
		if (!JsonUtil::LoadFromString(json, req.result, error) || (json.isObject() && json.isMember("error")))
			return;
		vector<OnlineScore> & vec = DLMAN->scores[ss];
		LOG->Trace(req.result.c_str());
		LOG->Flush();
		for (auto it = json.begin(); it != json.end(); ++it) {
			OnlineScore tmp;
			tmp.songName = (*it).get("songname", "").asString();
			tmp.wifeScore = atof((*it).get("wifescore", "0.0").asCString());
			tmp.ssr = atof((*it).get("Overall", "0.0").asCString());
			tmp.chartkey = (*it).get("chartkey", "").asString(); 
			tmp.scorekey = (*it).get("scorekey", "").asString();
			tmp.rate = atof((*it).get("user_chart_rate_rate", "0.0").asCString());
			tmp.difficulty = StringToDifficulty((*it).get("difficulty", "Invalid").asCString());
			vec.push_back(tmp);
		}
		MESSAGEMAN->Broadcast("OnlineUpdate");
	};
	HTTPRequest* req= new HTTPRequest(curlHandle, done);
	SetCURLResultsString(curlHandle, req->result);
	AddSessionCookieToCURL(curlHandle);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPGET, 1L);
	curl_multi_add_handle(mHTTPHandle, req->handle);
	HTTPRequests.push_back(req);
	return;
}
// Skillset ratings (we dont care about mod lvl, username, about, etc)
void DownloadManager::RefreshUserData()
{
	if (!LoggedIn())
		return;
	string url = serverURL.Get() + "/user_data";
	CURL *curlHandle = initCURLHandle();
	url += "?username=" + sessionUser;
	SetCURLURL(curlHandle, url);
	function<void(HTTPRequest&)> done = [](HTTPRequest& req) {
		Json::Value json;
		RString error;
		if (!JsonUtil::LoadFromString(json, req.result, error)) {
			FOREACH_ENUM(Skillset, ss)
				(DLMAN->sessionRatings)[ss] = 0.0f;
			return;
		}
		FOREACH_ENUM(Skillset, ss)
			(DLMAN->sessionRatings)[ss] = atof(json.get(SkillsetToString(ss), "0.0").asCString());
	};
	HTTPRequest* req = new HTTPRequest(curlHandle, done);
	SetCURLResultsString(curlHandle, req->result);
	AddSessionCookieToCURL(curlHandle);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPGET, 1L);
	curl_multi_add_handle(mHTTPHandle, req->handle);
	HTTPRequests.push_back(req);
	return;
}
DateTime DownloadManager::GetLastUploadDate(string profileName)
{
	string url = serverURL.Get() + "/last_upload";
	DateTime t;
	if (!LoggedIn())
		return DateTime::GetNowDateTime();
	CURL *curlHandle = initCURLHandle();
	curl_httppost *form = nullptr;
	curl_httppost *lastPtr = nullptr;
	curl_slist *headerlist = nullptr;
	SetCURLFormPostField(curlHandle, form, lastPtr, "origin", ComputerIdentity() + ":_:" + profileName);
	SetCURLPostToURL(curlHandle, url);
	AddSessionCookieToCURL(curlHandle);
	string result;
	SetCURLResultsString(curlHandle, result);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form);
	CURLcode ret = curl_easy_perform(curlHandle);
	curl_easy_cleanup(curlHandle);
	curl_formfree(form);
	t.FromString(result.c_str());
	return t;
}
void DownloadManager::StartSession(string user, string pass)
{
	string url = serverURL.Get() + "/login";
	if (loggingIn || (user == sessionUser && pass == sessionPass)) {
		return;
	}
	DLMAN->loggingIn = true;
	EndSessionIfExists();
	CURL *curlHandle = initCURLHandle();
	SetCURLPostToURL(curlHandle, url);
	curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, ""); /* start cookie engine */

	curl_httppost *form = nullptr;
	curl_httppost *lastPtr = nullptr;
	CURLFormPostField(curlHandle, form, lastPtr, "username", user.c_str());
	CURLFormPostField(curlHandle, form, lastPtr, "password", pass.c_str());
	curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form);

	function<void(HTTPRequest&)> done = [user, pass](HTTPRequest& req) {
		vector<string> v_cookies;
		if (req.result == "\"Success\"") {
			struct curl_slist *cookies;
			struct curl_slist *cookieIterator;
			curl_easy_getinfo(req.handle, CURLINFO_COOKIELIST, &cookies);
			cookieIterator = cookies;
			while (cookieIterator) {
				v_cookies.push_back(cookieIterator->data);
				cookieIterator = cookieIterator->next;
			}
			curl_slist_free_all(cookies);
			for (auto& cook : v_cookies) {
				vector<string> parts = split(cook, '\t');
				for (auto x = parts.begin(); x != parts.end(); x++) {
					if (*x == "ci_session") {
						DLMAN->session = *(x + 1);
						DLMAN->sessionCookie = cook;
						break;
					}
				}
				if (!DLMAN->session.empty())
					break;
			}
			DLMAN->sessionUser = user;
			DLMAN->sessionPass = pass;
			DLMAN->RefreshUserRank();
			DLMAN->RefreshUserData();
			FOREACH_ENUM(Skillset, ss)
				DLMAN->RefreshTop25(ss);
			DLMAN->UploadScores();
			MESSAGEMAN->Broadcast("Login");
		}
		else {
			DLMAN->session = DLMAN->sessionUser = DLMAN->sessionPass = DLMAN->sessionCookie = "";
			MESSAGEMAN->Broadcast("LoginFailed");
		}
		DLMAN->loggingIn = false;
	};
	HTTPRequest* req = new HTTPRequest(curlHandle, done, form);
	SetCURLResultsString(curlHandle, req->result);
	if (mHTTPHandle == nullptr)
		mHTTPHandle = curl_multi_init();
	curl_multi_add_handle(mHTTPHandle, req->handle);
	HTTPRequests.push_back(req);
}

bool DownloadManager::UploadScores()
{
	if (!LoggedIn())
		return false;
	auto scores = SCOREMAN->GetAllPBPtrs();
	for (auto&vec : scores) {
		for (auto&scorePtr : vec) {
			if (!scorePtr->IsUploadedToServer(serverURL.Get())) {
				UploadScore(scorePtr);
				scorePtr->AddUploadedServer(serverURL.Get());
			}
		}
	}
	return true;
}

bool DownloadManager::CachePackList(string url)
{
	bool result;
	auto ptr = GetPackList(url, result);
	if (!result)
		return false;
	downloadablePacks = *ptr;
	return result;
}

int DownloadManager::GetSkillsetRank(Skillset ss)
{
	if (!LoggedIn())
		return 0;
	return sessionRanks[ss];
}

float DownloadManager::GetSkillsetRating(Skillset ss)
{
	if (!LoggedIn())
		return 0.0f;
	return sessionRatings[ss];
}
vector<DownloadablePack>* DownloadManager::GetPackList(string url, bool &result)
{
	if (url == "") {
		result = false;
		return nullptr;
	}

	CURL *curl = initCURLHandle();

	SetCURLPostToURL(curl, url);

	string response;
	SetCURLResultsString(curl, response);

	CURLcode res = curl_easy_perform(curl);
	
	curl_easy_cleanup(curl);
	if (res != CURLE_OK) {
		result = false;
	}
	Json::Value packs;
	RString error;
	auto packlist = new vector<DownloadablePack>;
	bool parsed = JsonUtil::LoadFromString(packs, response, error);
	if (!parsed) {
		result = false;
		return packlist;
	}
	string baseUrl = "http://simfiles.stepmania-online.com/";

	for (int index = 0; index < packs.size(); ++index) {
		DownloadablePack tmp;

		if (packs[index].isMember("pack"))
			tmp.name = packs[index].get("pack", "").asString();
		else if (packs[index].isMember("packname"))
			tmp.name = packs[index].get("packname", "").asString();
		else if (packs[index].isMember("name"))
			tmp.name = packs[index].get("name", "").asString();
		else
			continue;

		if (packs[index].isMember("url"))
			tmp.url = packs[index].get("url", baseUrl + tmp.name + ".zip").asString();
		else
			tmp.url = baseUrl + tmp.name + ".zip";

		if (packs[index].isMember("average"))
			tmp.avgDifficulty = static_cast<float>(packs[index].get("average", 0).asDouble());
		else
			tmp.avgDifficulty = 0.0;

		if (packs[index].isMember("size"))
			tmp.size = packs[index].get("size", 0).asInt();
		else
			tmp.size = 0;

		tmp.id = ++lastid;
		packlist->push_back(tmp); 
	}
	result = true;
	return packlist;
}

Download::Download(string url)
{
	m_Url = url;
	handle = initCURLHandle();
	m_TempFileName = MakeTempFileName(url);
	p_RFWrapper.file.Open(m_TempFileName, 2);
	DLMAN->EncodeSpaces(m_Url);

	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &p_RFWrapper);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_URL, m_Url.c_str());
	curl_easy_setopt(handle, CURLOPT_XFERINFODATA, &progress);
	curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, progressfunc);
	curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);
}

Download::~Download()
{
	FILEMAN->Remove(m_TempFileName);
	if (p_Pack) 
		p_Pack->downloading = false;
}

void Download::Install()
{
	Message* msg;
	if(!DLMAN->InstallSmzip(m_TempFileName))
		msg = new Message("DownloadFailed");
	else
		msg = new Message("PackDownloaded");
	msg->SetParam("pack", LuaReference::CreateFromPush(*p_Pack));
	MESSAGEMAN->Broadcast(*msg);
	delete msg;
}

void Download::Failed()
{
	Message msg("DownloadFailed");
	msg.SetParam("pack", LuaReference::CreateFromPush(*p_Pack));
	MESSAGEMAN->Broadcast(msg);
}
/// Try to find in the Haystack the Needle - ignore case
bool findStringIC(const std::string & strHaystack, const std::string & strNeedle)
{
	auto it = std::search(
		strHaystack.begin(), strHaystack.end(),
		strNeedle.begin(), strNeedle.end(),
		[](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
	);
	return (it != strHaystack.end());
}

// lua start
#include "LuaBinding.h"
#include "LuaManager.h"
/** @brief Allow Lua to have access to the ProfileManager. */
class LunaDownloadManager : public Luna<DownloadManager>
{
public:
	static int GetPackList(T* p, lua_State* L)
	{
		vector<DownloadablePack>& packs = DLMAN->downloadablePacks;
		lua_createtable(L, packs.size(), 0);
		for (unsigned i = 0; i < packs.size(); ++i) {
			packs[i].PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}

		return 1;
	}
	static int GetDownloadingPacks(T* p, lua_State* L)
	{
		vector<DownloadablePack>& packs = DLMAN->downloadablePacks;
		vector<DownloadablePack*> dling;
		for (unsigned i = 0; i < packs.size(); ++i) {
			if (packs[i].downloading) 
				dling.push_back(&(packs[i]));
		}
		lua_createtable(L, dling.size(), 0);
		for (unsigned i = 0; i < dling.size(); ++i) {
			dling[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	static int GetUsername(T* p, lua_State* L)
	{
		lua_pushstring(L, DLMAN->sessionUser.c_str());
		return 1;
	}
	static int GetSkillsetRank(T* p, lua_State* L)
	{
		lua_pushnumber(L, DLMAN->GetSkillsetRank(Enum::Check<Skillset>(L, 1)));
		return 1;
	}
	static int GetSkillsetRating(T* p, lua_State* L)
	{
		lua_pushnumber(L, DLMAN->GetSkillsetRating(Enum::Check<Skillset>(L, 1)));
		return 1;
	}
	static int GetDownloads(T* p, lua_State* L)
	{
		map<string, Download*>& dls = DLMAN->downloads;
		lua_createtable(L, dls.size(), 0);
		int j = 0;
		for (auto it = dls.begin(); it != dls.end(); ++it) {
			it->second->PushSelf(L);
			lua_rawseti(L, -2, j + 1);
			j++;
		}
		return 1;
	}
	static int IsLoggedIn(T* p, lua_State* L)
	{
		lua_pushboolean(L, DLMAN->LoggedIn());
		return 1;
	}
	static int Login(T* p, lua_State* L)
	{
		string user = SArg(1);
		string pass = SArg(2);
		DLMAN->StartSession(user, pass); 
		return 1;
	}
	static int Logout(T* p, lua_State* L)
	{
		DLMAN->EndSessionIfExists();
		return 1;
	}
	static int GetTopSkillsetScore(T* p, lua_State* L)
	{
		int rank = IArg(1);
		auto ss = Enum::Check<Skillset>(L, 2);
		bool result;
		OnlineScore onlineScore = DLMAN->GetTopSkillsetScore(rank, ss, result);
		if (!result) {
			lua_pushnil(L);
			return 1;
		}
		lua_createtable(L, 0, 6);
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
		return 1;
	}
	static int GetFilteredAndSearchedPackList(T* p, lua_State* L)
	{
		if (lua_gettop(L) < 5) {
			return luaL_error(L, "GetFilteredAndSearchedPackList expects exactly 5 arguments(packname, lower diff, upper diff, lower size, upper size)");
		}
		string name = SArg(1);
		double avgLower = max(luaL_checknumber(L, 2), 0);
		double avgUpper = max(luaL_checknumber(L, 3),0);
		size_t sizeLower = luaL_checknumber(L, 4);
		size_t sizeUpper = luaL_checknumber(L, 5);
		vector<DownloadablePack>& packs = DLMAN->downloadablePacks;
		vector<DownloadablePack*> retPacklist;
		for (unsigned i = 0; i < packs.size(); ++i) {
			if(packs[i].avgDifficulty >= avgLower &&
				findStringIC(packs[i].name, name) &&
				(avgUpper <= 0 || packs[i].avgDifficulty < avgUpper) &&
				packs[i].size >= sizeLower &&
				(sizeUpper <= 0 || packs[i].size < sizeUpper))
				retPacklist.push_back(&(packs[i]));
		}
		lua_createtable(L, retPacklist.size(), 0);
		for (unsigned i = 0; i < retPacklist.size(); ++i) {
			retPacklist[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	LunaDownloadManager()
	{
		ADD_METHOD(GetPackList);
		ADD_METHOD(GetDownloadingPacks);
		ADD_METHOD(GetDownloads);
		ADD_METHOD(GetFilteredAndSearchedPackList);
		ADD_METHOD(IsLoggedIn);
		ADD_METHOD(Login);
		ADD_METHOD(GetUsername);
		ADD_METHOD(GetSkillsetRank);
		ADD_METHOD(GetSkillsetRating);
		ADD_METHOD(GetTopSkillsetScore);
		ADD_METHOD(Logout);
	}
};
LUA_REGISTER_CLASS(DownloadManager) 


class LunaDownloadablePack : public Luna<DownloadablePack>
{
public:
	static int DownloadAndInstall(T* p, lua_State* L)
	{
		if (p->downloading)
			return 1;
		p->downloading = true;
		Download * dl = DLMAN->DownloadAndInstallPack(p);
		if (dl)
			dl->PushSelf(L);
		else
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
	static int IsDownloading(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->downloading ==0);
		return 1;
	}
	static int GetDownload(T* p, lua_State* L)
	{
		if (p->downloading)
			DLMAN->downloads[p->url]->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	LunaDownloadablePack()
	{
		ADD_METHOD(DownloadAndInstall);
		ADD_METHOD(IsDownloading);
		ADD_METHOD(GetAvgDifficulty);
		ADD_METHOD(GetName);
		ADD_METHOD(GetSize);
		ADD_METHOD(GetDownload);
	}
};

LUA_REGISTER_CLASS(DownloadablePack) 


class LunaDownload : public Luna<Download>
{
public:
	static int GetKBDownloaded(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->progress.downloaded);
		return 1;
	}
	static int GetKBPerSecond(T* p, lua_State* L)
	{
		lua_pushnumber(L, atoi(p->speed.c_str()));
		return 1;
	}
	static int GetTotalKB(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->progress.total);
		return 1;
	}
	static int Stop(T* p, lua_State* L)
	{
		p->p_RFWrapper.stop = true;
		return 1;
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

#endif



