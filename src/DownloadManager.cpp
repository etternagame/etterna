#include "global.h"
#if !defined(WITHOUT_NETWORKING)
#include "RageFileManager.h"
#include "ScreenManager.h"
#include "Preference.h"
#include "RageLog.h"
#include "RageFile.h"
#include "DownloadManager.h"
#include "GameState.h"
#include "ScoreManager.h"
#include "ScreenNetSelectMusic.h"
#include "RageFileManager.h"
#include "ProfileManager.h"
#include "SongManager.h"
#include "ScreenInstallOverlay.h"
#include "CommandLineActions.h"
#include "ScreenSelectMusic.h"
#include "SpecialFiles.h"
#include "curl/curl.h"
#include "Foreach.h"
#include "Song.h"
#include "RageString.h"
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <FilterManager.h>
#include "PlayerStageStats.h">
#include "Grade.h"
using json = nlohmann::json;
#ifdef _WIN32 
#include <intrin.h>
#endif
shared_ptr<DownloadManager> DLMAN = nullptr;

static Preference<unsigned int> maxDLPerSecond("maximumBytesDownloadedPerSecond", 0);
static Preference<unsigned int> maxDLPerSecondGameplay("maximumBytesDownloadedPerSecondDuringGameplay", 1000000);
static Preference<RString> packListURL("PackListURL", "https://api.etternaonline.com/v2/packs");
static Preference<RString> serverURL("BaseAPIURL", "https://api.etternaonline.com/v2");
static Preference<unsigned int> automaticSync("automaticScoreSync", 1);
static Preference<unsigned int> downloadPacksToAdditionalSongs("downloadPacksToAdditionalSongs", 0);
static const string TEMP_ZIP_MOUNT_POINT = "/@temp-zip/";
static const string CLIENT_DATA_KEY = "4406B28A97B326DA5346A9885B0C9DEE8D66F89B562CF5E337AC04C17EB95C40";
static const string DL_DIR = SpecialFiles::CACHE_DIR + "Downloads/";

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

	int cpuinfo[4] = { 0, 0, 0, 0 };
	__cpuid(cpuinfo, 0);
	uint16_t cpuHash = 0;
	uint16_t* ptr = (uint16_t*)(&cpuinfo[0]);
	for (uint32_t i = 0; i < 8; i++)
		cpuHash += ptr[i];

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
	computerName = hostname;
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
	return static_cast<ReadThis*>(arg)->file.Seek(static_cast<int>(offset), origin);
}

bool DownloadManager::InstallSmzip(const string &sZipFile)
{
	if (!FILEMAN->Mount("zip", sZipFile, TEMP_ZIP_MOUNT_POINT))
		FAIL_M(static_cast<string>("Failed to mount " + sZipFile).c_str());
	vector<RString> v_packs;
	GetDirListing(TEMP_ZIP_MOUNT_POINT + "*", v_packs, true, true);

	string doot = TEMP_ZIP_MOUNT_POINT;
	if (v_packs.size() > 1) {
		doot += sZipFile.substr(sZipFile.find_last_of("/") + 1);// attempt to whitelist pack name, this should be pretty simple/safe solution for a lot of pad packs -mina
		doot = doot.substr(0, doot.length() - 4) + "/";
	}
	
	vector<string> vsFiles;
	{
		vector<RString> vsRawFiles;
		GetDirListingRecursive(doot, "*", vsRawFiles);

		if (vsRawFiles.empty()) {
			FILEMAN->Unmount("zip", sZipFile, TEMP_ZIP_MOUNT_POINT);
			return false;
		}

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
	string extractTo = downloadPacksToAdditionalSongs ? "AdditionalSongs/" : "Songs/";
	FOREACH_CONST(string, vsFiles, sSrcFile)
	{
		string sDestFile = *sSrcFile;
		sDestFile = RString(sDestFile.c_str()).Right(sDestFile.length() - TEMP_ZIP_MOUNT_POINT.length());

		RString sDir, sThrowAway;
		splitpath(sDestFile, sDir, sThrowAway, sThrowAway);


		if (!FileCopy(*sSrcFile, extractTo + sDestFile))
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
inline CURL* initBasicCURLHandle() {
	CURL *curlHandle = curl_easy_init();
	curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/60.0.3112.113 Safari/537.36");
	curl_easy_setopt(curlHandle, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
	return curlHandle;
}
//Utility inline functions to deal with CURL
inline CURL* initCURLHandle(bool withBearer) {
	CURL *curlHandle = initBasicCURLHandle();
	struct curl_slist *list = NULL;
	if(withBearer)
		list = curl_slist_append(list, ("Authorization: Bearer " + DLMAN->authToken).c_str());
	curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT, 120);//Seconds
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
inline void SetCURLResultsString(CURL *curlHandle, string* str)
{
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, str);
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
inline void EmptyTempDLFileDir() 
{
	vector<RString> files;
	FILEMAN->GetDirListing(DL_DIR + "*", files, false, true);
	for (auto& file : files) {
		if (FILEMAN->IsAFile(file))
			FILEMAN->Remove(file);
	}
}
DownloadManager::DownloadManager() {
	EmptyTempDLFileDir();
	curl_global_init(CURL_GLOBAL_ALL);
	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring(L, "DLMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

DownloadManager::~DownloadManager()
{
	if (mPackHandle != nullptr)
		curl_multi_cleanup(mPackHandle);
	mPackHandle = nullptr;
	if (mHTTPHandle != nullptr)
		curl_multi_cleanup(mHTTPHandle);
	mHTTPHandle = nullptr;
	EmptyTempDLFileDir();
	for (auto &dl : downloads) {
		if (dl.second->handle != nullptr) {
			curl_easy_cleanup(dl.second->handle);
			dl.second->handle = nullptr;
		}
		delete dl.second;
	}
	if (LoggedIn())
		EndSession();
	curl_global_cleanup();
}

Download* DownloadManager::DownloadAndInstallPack(const string &url, string filename)
{	
	Download* dl = new Download(url, filename);

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
	vector<RString> packs;
	SONGMAN->GetSongGroupNames(packs);
	for (auto packName : packs) {
		if (packName == pack->name) {
			SCREENMAN->SystemMessage("Already have pack " + packName + ", not downloading");
			return nullptr;
		}
	}
	if (downloadingPacks >= maxPacksToDownloadAtOnce) {
		DLMAN->DownloadQueue.emplace_back(pack);
		return nullptr;
	}
	Download* dl = DownloadAndInstallPack(pack->url, pack->name+".zip");
	dl->p_Pack = pack;
	return dl;
}
void DownloadManager::init()
{
	RefreshPackList(packListURL);
	RefreshLastVersion();
	RefreshRegisterPage();
	initialized = true;
}
void DownloadManager::Update(float fDeltaSeconds)
{
	if (!initialized)
		init();
	UpdatePacks(fDeltaSeconds);
	UpdateHTTP(fDeltaSeconds);
	return;
}
void DownloadManager::UpdateHTTP(float fDeltaSeconds)
{
	if (!HTTPRunning && HTTPRequests.size() == 0)
		return;
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
		error = "curl_multi_fdset() failed, code " + to_string(mc);
		return;
	}
	if (maxfd == -1) {
		rc = 0;
	}
	else {
		rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
	}
	switch (rc) {
	case -1:
		error = "select error" + to_string(mc);
		break;
	case 0: /* timeout */
	default: /* action */
		curl_multi_perform(mHTTPHandle, &HTTPRunning);
		break;
	}

	//Check for finished downloads
	CURLMsg *msg;
	int msgs_left;
	while ((msg = curl_multi_info_read(mHTTPHandle, &msgs_left))) {
		/* Find out which handle this message is about */
		for (size_t i = 0; i < HTTPRequests.size();++i) {
			if (msg->easy_handle == HTTPRequests[i]->handle) {
				if (msg->data.result == CURLE_UNSUPPORTED_PROTOCOL) {
					HTTPRequests[i]->Failed(*(HTTPRequests[i]), msg);
					LOG->Trace("CURL UNSUPPORTED PROTOCOL (Probably https)");
				}
				else if (msg->msg == CURLMSG_DONE) {
					HTTPRequests[i]->Done(*(HTTPRequests[i]), msg);
				}
				else
					HTTPRequests[i]->Failed(*(HTTPRequests[i]), msg);
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
	return;
}
void DownloadManager::UpdatePacks(float fDeltaSeconds)
{
	timeSinceLastDownload += fDeltaSeconds;
	if (pendingInstallDownloads.size() > 0 && !gameplay) {
		//Install all pending packs
		for (auto i = pendingInstallDownloads.begin(); i != pendingInstallDownloads.end(); i++) {
			i->second->Install();
			finishedDownloads[i->second->m_Url] = i->second;
			pendingInstallDownloads.erase(i);
		}
		//Reload
		auto screen = SCREENMAN->GetScreen(0);
		if (screen && screen->GetName() == "ScreenSelectMusic")
			static_cast<ScreenSelectMusic*>(screen)->DifferentialReload();
		else
			if (screen && screen->GetName() == "ScreenNetSelectMusic")
				static_cast<ScreenNetSelectMusic*>(screen)->DifferentialReload();
			else
				SONGMAN->DifferentialReload();
	}
	if (downloadingPacks < maxPacksToDownloadAtOnce && !DownloadQueue.empty() && timeSinceLastDownload > DownloadCooldownTime) {
		auto it = DownloadQueue.begin();
		DownloadQueue.pop_front();
		auto pack = *it;
		auto dl = DLMAN->DownloadAndInstallPack(pack);
	}
	if (!downloadingPacks)
		return;
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
		error = "curl_multi_fdset() failed, code " + to_string(mc);
		return;
	}
	if (maxfd == -1) {
		rc = 0;
	}
	else {
		rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
	}
	switch (rc) {
	case -1:
		error = "select error" + to_string(mc);
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
	bool installedPacks = false;
	bool finishedADownload = false;
	while ((msg = curl_multi_info_read(mPackHandle, &msgs_left))) {
		/* Find out which handle this message is about */
		for (auto i = downloads.begin(); i != downloads.end(); i++) {
			if (msg->easy_handle == i->second->handle && msg->msg == CURLMSG_DONE && msg->data.result != CURLE_PARTIAL_FILE) {
				finishedADownload = true;
				i->second->p_RFWrapper.file.Flush();
				if (i->second->p_RFWrapper.file.IsOpen())
					i->second->p_RFWrapper.file.Close();
				if (msg->msg == CURLMSG_DONE && i->second->progress.total <= i->second->progress.downloaded) {
					timeSinceLastDownload = 0;
					i->second->Done(i->second);
					if (!gameplay) {
						installedPacks = true;
						i->second->Install();
						finishedDownloads[i->second->m_Url] = i->second;
					}
					else {
						pendingInstallDownloads[i->second->m_Url] = i->second;
					}
				}
				else {
					i->second->Failed();
					finishedDownloads[i->second->m_Url] = i->second;
				}
				if (i->second->handle != nullptr)
					curl_easy_cleanup(i->second->handle);
				i->second->handle = nullptr;
				if (i->second->p_Pack != nullptr)
					i->second->p_Pack->downloading = false;
				downloads.erase(i);
				break;
			}
		}
	}
	if (finishedADownload)
	{
		UpdateDLSpeed();
		if (downloads.empty())
			MESSAGEMAN->Broadcast("AllDownloadsCompleted");
	}
	if (installedPacks) {
		auto screen = SCREENMAN->GetScreen(0);
		if (screen && screen->GetName() == "ScreenSelectMusic")
			static_cast<ScreenSelectMusic*>(screen)->DifferentialReload();
		else
			if (screen && screen->GetName() == "ScreenNetSelectMusic")
				static_cast<ScreenNetSelectMusic*>(screen)->DifferentialReload();
			else
				SONGMAN->DifferentialReload();
	}
	return;
}

string Download::MakeTempFileName(string s)
{
	return DL_DIR + Basename(s);
}
bool DownloadManager::LoggedIn()
{
	return !authToken.empty();
}

void DownloadManager::AddFavorite(string chartkey)
{
	string req = "user/" + DLMAN->sessionUser + "/favorites";
	DLMAN->favorites.emplace_back(chartkey);
	auto done = [req](HTTPRequest& requ, CURLMsg *) {
		LOG->Warn((requ.result+req+DLMAN->sessionUser).c_str());
	};
	SendRequest(req, {make_pair("chartkey", chartkey)}, done, true, true);
}

void DownloadManager::RemoveFavorite(string chartkey)
{
	auto it = std::find(DLMAN->favorites.begin(), DLMAN->favorites.end(), chartkey);
	if (it != DLMAN->favorites.end())
		DLMAN->favorites.erase(it);
	string req = "user/" + DLMAN->sessionUser + "/favorites/"+chartkey;
	auto done = [](HTTPRequest& req, CURLMsg *) {

	};
	auto r = SendRequest(req, {}, done);
	if(r)
		curl_easy_setopt(r->handle, CURLOPT_CUSTOMREQUEST, "DELETE");
}

// we could pass scoregoal objects instead..? -mina
void DownloadManager::RemoveGoal(string chartkey, float wife, float rate)
{
	string req = "user/" + DLMAN->sessionUser + "/goals/" + chartkey+"/"+to_string(wife)+"/"+to_string(rate);
	auto done = [](HTTPRequest& req, CURLMsg *) {

	};
	auto r = SendRequest(req, {}, done);
	if (r)
		curl_easy_setopt(r->handle, CURLOPT_CUSTOMREQUEST, "DELETE");
}

void DownloadManager::AddGoal(string chartkey, float wife, float rate, DateTime timeAssigned)
{
	string req = "user/" + DLMAN->sessionUser + "/goals";
	auto done = [](HTTPRequest& req, CURLMsg *) {

	};
	vector<pair<string, string>> postParams = { make_pair("chartkey", chartkey),
		make_pair("rate", to_string(rate)), 
		make_pair("wife", to_string(wife)),
		make_pair("timeAssigned", timeAssigned.GetString()) };
	SendRequest(req, postParams, done, true, true);
}

void DownloadManager::UpdateGoal(string chartkey, float wife, float rate, bool achieved, DateTime timeAssigned, DateTime timeAchieved)
{
	string doot = "0000:00:00 00:00:00";
	if (achieved)
		doot = timeAchieved.GetString();

	string req = "user/" + DLMAN->sessionUser + "/goals/update";
	auto done = [](HTTPRequest& req, CURLMsg *) {

	};
	vector<pair<string, string>> postParams = { make_pair("chartkey", chartkey),
		make_pair("rate", to_string(rate)),
		make_pair("wife", to_string(wife)),
		make_pair("achieved", to_string(achieved)),
		make_pair("timeAssigned", timeAssigned.GetString()),
		make_pair("timeAchieved", doot) };
	SendRequest(req, postParams, done, true, true);
}

void DownloadManager::RefreshFavourites()
{
	string req = "user/" + DLMAN->sessionUser + "/favorites";
	auto done = [](HTTPRequest& req, CURLMsg *) {
		json j;
		bool parsed = true;
		try {
			j = json::parse(req.result);
			auto favs = j.find("data");
			for (auto fav : *favs)
				DLMAN->favorites.emplace_back(fav["attributes"].value("chartkey", ""));
		}
		catch (exception e) {
			DLMAN->favorites.clear();
		}
		MESSAGEMAN->Broadcast("FavouritesUpdate");
	};
	SendRequest(req, {}, done);
}

bool DownloadManager::ShouldUploadScores()
{
	return LoggedIn() && automaticSync;
}
inline void SetCURLPOSTScore(CURL*& curlHandle, curl_httppost*& form, curl_httppost*& lastPtr, HighScore*& hs)
{
	SetCURLFormPostField(curlHandle, form, lastPtr, "scorekey", hs->GetScoreKey());
	hs->GenerateValidationKeys();
	SetCURLFormPostField(curlHandle, form, lastPtr, "ssr_norm", hs->norms);
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
	SetCURLFormPostField(curlHandle, form, lastPtr, "rate", hs->musics);
	auto chart = SONGMAN->GetStepsByChartkey(hs->GetChartKey());
	SetCURLFormPostField(curlHandle, form, lastPtr, "negsolo", chart->GetTimingData()->HasWarps() || chart->m_StepsType != StepsType_dance_single);
	SetCURLFormPostField(curlHandle, form, lastPtr, "nocc", static_cast<int>(!hs->GetChordCohesion()));
	SetCURLFormPostField(curlHandle, form, lastPtr, "calc_version", hs->GetSSRCalcVersion());
	SetCURLFormPostField(curlHandle, form, lastPtr, "topscore", hs->GetTopScore());
	SetCURLFormPostField(curlHandle, form, lastPtr, "hash", hs->GetValidationKey(ValidationKey_Brittle));
	SetCURLFormPostField(curlHandle, form, lastPtr, "wife", hs->GetWifeScore());
	SetCURLFormPostField(curlHandle, form, lastPtr, "wifePoints", hs->GetWifePoints());
	SetCURLFormPostField(curlHandle, form, lastPtr, "judgeScale", hs->judges);
	SetCURLFormPostField(curlHandle, form, lastPtr, "machineGuid", hs->GetMachineGuid());
	SetCURLFormPostField(curlHandle, form, lastPtr, "grade", hs->GetGrade());
	SetCURLFormPostField(curlHandle, form, lastPtr, "wifeGrade", string(GradeToString(hs->GetWifeGrade()).c_str()));	
}
void DownloadManager::UploadScore(HighScore* hs)
{
	if (!LoggedIn())
		return;
	CURL *curlHandle = initCURLHandle(true);
	string url = serverURL.Get() + "/score";
	curl_httppost *form = nullptr;
	curl_httppost *lastPtr = nullptr;
	SetCURLPOSTScore(curlHandle, form, lastPtr, hs);
	CURLFormPostField(curlHandle, form, lastPtr, "replay_data", "[]");
	SetCURLPostToURL(curlHandle, url);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form);
	auto done = [hs](HTTPRequest& req, CURLMsg *) {
		json j;
		try {
			j = json::parse(req.result);
			auto errors = j["errors"];
			bool delay = false;
			for (auto error : errors) {
				int status = error["status"];
				if (status == 22) {
					delay = true;
					DLMAN->StartSession(DLMAN->sessionUser, DLMAN->sessionPass, [hs](bool logged) {
						if (logged) {
							DLMAN->UploadScore(hs);
						}
					});
				}
				else if (status == 404 || status == 405 || status == 406) {
					hs->AddUploadedServer(serverURL.Get());
				}
			}
			if (!delay && j["data"]["type"] == "ssrResults") {
				hs->AddUploadedServer(serverURL.Get());
			}
		}
		catch (exception e) { }
	};
	HTTPRequest* req = new HTTPRequest(curlHandle, done);
	SetCURLResultsString(curlHandle, &(req->result));
	curl_multi_add_handle(mHTTPHandle, req->handle);
	HTTPRequests.push_back(req);
	return;
}
void DownloadManager::UploadScoreWithReplayData(HighScore* hs) 
{
	if (!LoggedIn())
		return;
	CURL *curlHandle = initCURLHandle(true);
	string url = serverURL.Get() + "/score";
	curl_httppost *form = nullptr;
	curl_httppost *lastPtr = nullptr;
	SetCURLPOSTScore(curlHandle, form, lastPtr, hs);
	string replayString;
	vector<float> offsets = hs->GetOffsetVector();
	vector<int> columns = hs->GetTrackVector();
	vector<TapNoteType> types = hs->GetTapNoteTypeVector();
	if (offsets.size() > 0) {
		replayString = "[";
		vector<float>& timestamps = hs->timeStamps;
		for (size_t i = 0; i < offsets.size(); i++) {
			replayString += "[";
			replayString += to_string(timestamps[i]) + ",";
			replayString += to_string(1000.f * offsets[i]) + ",";
			replayString += to_string(columns[i]) + ",";
			replayString += to_string(types[i]);
			replayString += "],";
		}
		replayString = replayString.substr(0, replayString.size() - 1); //remove ","
		replayString += "]";
	}
	else
		replayString = "";
	SetCURLFormPostField(curlHandle, form, lastPtr, "replay_data", replayString);
	SetCURLPostToURL(curlHandle, url);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form); 
	auto done = [this,hs](HTTPRequest& req, CURLMsg *) {
		long response_code;
		curl_easy_getinfo(req.handle, CURLINFO_RESPONSE_CODE, &response_code);
		json j;
		try {
			j = json::parse(req.result);
			bool delay = false;
			try {
				auto errors = j["errors"];
				for (auto error : errors) {
					int status = error["status"];
					if (status == 22) {
						delay = true;
						DLMAN->StartSession(DLMAN->sessionUser, DLMAN->sessionPass, [hs](bool logged) {
							if (logged) {
								DLMAN->UploadScoreWithReplayData(hs);
							}
						});
					}
					else if (status == 404 || status == 405 || status == 406) {
						hs->AddUploadedServer(serverURL.Get());
					}
				}
			}
			catch (exception e) {}
			if (!delay && j["data"]["type"] == "ssrResults") {
				auto diffs = j["data"]["attributes"]["diff"];
				FOREACH_ENUM(Skillset, ss)
					if (ss != Skill_Overall)
						(DLMAN->sessionRatings)[ss] += diffs.value(SkillsetToString(ss), 0.0);
				(DLMAN->sessionRatings)[Skill_Overall] += diffs.value("Rating", 0.0);
				hs->AddUploadedServer(serverURL.Get());
				HTTPRunning = response_code;
			}
		}
		catch (exception e) {
		}
	};
	HTTPRequest* req = new HTTPRequest(curlHandle, done);
	SetCURLResultsString(curlHandle, &(req->result));
	curl_multi_add_handle(mHTTPHandle, req->handle);
	HTTPRequests.push_back(req);
	return;
}
void DownloadManager::EndSessionIfExists()
{
	if (!LoggedIn())
		return;
	EndSession();
}
void DownloadManager::EndSession()
{
	sessionUser = sessionPass = authToken = "";
	topScores.clear();
	sessionRatings.clear();
	MESSAGEMAN->Broadcast("LogOut");
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
	auto done = [](HTTPRequest& req, CURLMsg *) {
		json j;
		try {
			j = json::parse(req.result);
			try { 
				if (j["errors"]["status"] == 404) 
					return; 
			} catch (exception e) {}
			auto skillsets = j.find("data")->find("attributes");
			FOREACH_ENUM(Skillset, ss)
				(DLMAN->sessionRanks)[ss] = skillsets->value(SkillsetToString(ss).c_str(), 0);
		}
		catch (exception e) {
			FOREACH_ENUM(Skillset, ss)
				(DLMAN->sessionRanks)[ss] = 0;
		}
		MESSAGEMAN->Broadcast("OnlineUpdate");
	};
	SendRequest("user/" + sessionUser + "/ranks", {}, done, true, false, true);
	return;
}
OnlineTopScore DownloadManager::GetTopSkillsetScore(unsigned int rank, Skillset ss, bool &result)
{
	unsigned int index = rank - 1;
	if (index < topScores[ss].size()) {
		result = true;
		return topScores[ss][index];
	}
	result=false;
	return OnlineTopScore();
}

HTTPRequest* DownloadManager::SendRequest(string requestName, vector<pair<string, string>> params, function<void(HTTPRequest&, CURLMsg *)> done, bool requireLogin, bool post, bool async, bool withBearer)
{
	return SendRequestToURL(serverURL.Get() + "/" + requestName, params, done, requireLogin, post, async, withBearer);
}

HTTPRequest* DownloadManager::SendRequestToURL(string url, vector<pair<string, string>> params, function<void(HTTPRequest&, CURLMsg *)> afterDone, bool requireLogin, bool post, bool async, bool withBearer)
{
	if (requireLogin && !LoggedIn())
		return nullptr;
	if (!post && !params.empty()) {
		url += "?";
		for (auto& param : params)
			url += param.first + "=" + param.second + "&";
		url = url.substr(0, url.length() - 1);
	}
	function<void(HTTPRequest&, CURLMsg *)> done = [afterDone](HTTPRequest& req, CURLMsg * msg) {
		try {
			json tmp = json::parse(req.result);
			auto errors = tmp["errors"];
			bool delay = false;
			for (auto error : errors) { 
				if (error["status"] == 22) {
					delay = true;
					DLMAN->StartSession(DLMAN->sessionUser, DLMAN->sessionPass, [req, msg, afterDone](bool logged) {
						if (logged) {
							auto r = req;
							afterDone(r, msg);
						}
					});
				}
			}
			if(!delay)
				afterDone(req, msg);
		}
		catch (exception e) {
			afterDone(req, msg);
		}
	};
	CURL *curlHandle = initCURLHandle(withBearer);
	SetCURLURL(curlHandle, url);
	HTTPRequest* req;
	if (post) {
		curl_httppost *form = nullptr;
		curl_httppost *lastPtr = nullptr;
		for (auto& param : params)
			CURLFormPostField(curlHandle, form, lastPtr, param.first.c_str(), param.second.c_str());
		curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form);
		req = new HTTPRequest(curlHandle, done, form);
	}
	else {
		req = new HTTPRequest(curlHandle, done);
		curl_easy_setopt(curlHandle, CURLOPT_HTTPGET, 1L);
	}
	SetCURLResultsString(curlHandle, &(req->result));
	if (async) {
		if (mHTTPHandle == nullptr)
			mHTTPHandle = curl_multi_init();
		curl_multi_add_handle(mHTTPHandle, req->handle);
		HTTPRequests.push_back(req);
	}
	else {
		CURLcode res = curl_easy_perform(req->handle);
		curl_easy_cleanup(req->handle);
		done(*req, nullptr);
		delete req;
	}
	return req;
}

float mythicalmathymathsProbablyUnderratedness(string chartkey) {
	auto &onlineScores = DLMAN->chartLeaderboards[chartkey];

	float dsum = 0.f;
	int num = 4;
	for (auto &s : onlineScores) {
		float adjRating = s.playerRating - 1.f;
		if (s.playerRating > 1.f && s.SSRs[Skill_Overall] > 1.f && abs(s.playerRating - s.SSRs[Skill_Overall]) < 7.5f) {
			if (s.playerRating > s.SSRs[Skill_Overall]) {
				float diff = adjRating - s.SSRs[Skill_Overall];
				dsum += diff;
			}
			else {
				float mcdoot = s.SSRs[Skill_Overall] + 1.f - s.playerRating;
				dsum -= mcdoot * mcdoot;
			}
			++num;
		}	
	}
	return dsum/num;
}

float overratedness(string chartkey) {
	auto &onlineScores = DLMAN->chartLeaderboards[chartkey];
	vector<float> values;
	float offset = 5.f;
	float dsum = 0.f;
	int num = onlineScores.size();
	if (num == 0)
		return 0.0f;
	
	for (auto &s : onlineScores) {
		if (s.playerRating > 1.f && s.SSRs[Skill_Overall] > 1.f) {
			float adjrating = s.playerRating * 1.026f;
			float value = static_cast<float>((2.0 / erfc(0.1 * (s.SSRs[Skill_Overall] - adjrating))));
			if (s.SSRs[Skill_Overall] - s.playerRating > 40)
				value = 100.f;
			if (value < 0)
				value = 0.f;
			values.emplace_back(value);
			dsum += value;
		}
	}
	
	float zeeaverage = dsum / num;

	float overratedness = (dsum - offset) / num;
	
	overratedness /= zeeaverage;
	overratedness -= 1.5f;

	float multiplier = 1.f - overratedness;
	float mcdootMin = 1.f;
	if (multiplier  < mcdootMin-0.2f)
		multiplier = mcdootMin - 0.2f;

	float nerfE = (4.f*mcdootMin + multiplier) / 5.f;
	nerfE = min(1.f, nerfE);
	return overratedness;
}

// fill dummy highscores so we can use the same lua displays -mina
void DownloadManager::MakeAThing(string chartkey) {
	athing.clear();
	HighScore hs;

	for (auto& ohs : DLMAN->chartLeaderboards[chartkey]) {
		hs.SetDateTime(ohs.datetime);
		hs.SetMaxCombo(ohs.maxcombo);
		hs.SetName(ohs.username);
		hs.SetModifiers(ohs.modifiers);
		hs.SetChordCohesion(ohs.nocc);
		hs.SetWifeScore(ohs.wife);
		hs.SetMusicRate(ohs.rate);

		hs.SetTapNoteScore(TNS_W1, ohs.marvelous);
		hs.SetTapNoteScore(TNS_W2, ohs.perfect);
		hs.SetTapNoteScore(TNS_W3, ohs.great);
		hs.SetTapNoteScore(TNS_W4, ohs.good);
		hs.SetTapNoteScore(TNS_W5, ohs.bad);
		hs.SetTapNoteScore(TNS_Miss, ohs.miss);
		hs.SetTapNoteScore(TNS_HitMine, ohs.minehits);

		hs.SetHoldNoteScore(HNS_Held, ohs.held);
		hs.SetHoldNoteScore(HNS_LetGo, ohs.letgo);

		FOREACH_ENUM(Skillset, ss)
			hs.SetSkillsetSSR(ss, ohs.SSRs[ss]);

		hs.userid = ohs.userid;
		hs.scoreid = ohs.scoreid;
		hs.avatar = ohs.avatar;
		athing.push_back(hs);
	}
	
}

void DownloadManager::RequestChartLeaderBoard(string chartkey)
{
	auto done = [chartkey](HTTPRequest& req, CURLMsg *) {
		vector<OnlineScore> & vec = DLMAN->chartLeaderboards[chartkey];
		vec.clear();
		//unordered_set<string> userswithscores;
		Message msg("ChartLeaderboardUpdate");
		try {
			auto j = json::parse(req.result);
			if (j.find("errors") != j.end())
				throw exception();
			auto scores = j.find("data");
			for (auto scoreJ : (*scores)) {
				auto score = *(scoreJ.find("attributes"));
				
				// i don't really want to do this very iteration but oh well -mina
				msg.SetParam("songid", RString(score.value("songId", "").c_str()));

				OnlineScore tmp;
				auto user = *(score.find("user"));
				tmp.username = user.value("userName", "").c_str();
				tmp.avatar = user.value("avatar", "").c_str();
				tmp.userid = user.value("userId", 0);				
				tmp.playerRating = static_cast<float>(user.value("playerRating", 0.0));
				tmp.wife = static_cast<float>(score.value("wife", 0.0)/100.0);
				tmp.modifiers = score.value("modifiers", "").c_str();
				tmp.maxcombo = score.value("maxCombo", 0);
				{
					auto judgements = *(score.find("judgements"));
					tmp.marvelous = judgements.value("marvelous", 0);
					tmp.perfect = judgements.value("perfect", 0);
					tmp.great = judgements.value("great", 0);
					tmp.good = judgements.value("good", 0);
					tmp.bad = judgements.value("bad", 0);
					tmp.miss = judgements.value("miss", 0);
					tmp.minehits = judgements.value("hitMines", 0);
					tmp.held = judgements.value("heldHold", 0);
					tmp.letgo = judgements.value("letGoHold", 0);
				}
				tmp.datetime.FromString(score.value("datetime", "0"));
				tmp.scoreid = scoreJ.value("id", "").c_str();
				
				// filter scores not on the current rate out if enabled... dunno if we need this precision -mina
				tmp.rate = static_cast<float>(score.value("rate", 0.0));
				tmp.nocc = score.value("noCC", 0) != 0;
				tmp.valid = score.value("valid", 0) != 0;

				auto ssrs = *(score.find("skillsets"));
				FOREACH_ENUM(Skillset, ss)
					tmp.SSRs[ss] = static_cast<float>(ssrs.value(SkillsetToString(ss).c_str(), 0.0));
				/*
				try {
					auto replay = score["replay"];
					if (replay.size() > 1)
						LOG->Trace(tmp.modifiers.c_str());
					for (auto pair : replay)
						tmp.replayData.emplace_back(make_pair(pair[0].get<float>(), pair[1].get<float>()));
				}
				catch (exception e) {
					//replaydata failed
				}
				*/

				// eo still has some old profiles with various edge issues that unfortunately need to be handled here
				// screen out old 11111 flags (my greatest mistake) and it's probably a safe bet to throw out below 25% scores -mina
				if (tmp.wife > 1.f || tmp.wife < 0.25f || !tmp.valid)
					continue;

				// it seems prudent to maintain the eo functionality in this way and screen out multiple scores from the same user 
				// even more prudent would be to put this last where it belongs, we don't want to screen out scores for players who wouldn't
				// have had them registered in the first place -mina
				// Moved this filtering to the Lua call. -poco
				//if (userswithscores.count(tmp.username) == 1)
				//	continue;

				//userswithscores.emplace(tmp.username);
				vec.emplace_back(tmp);
			}
		}
		catch (exception e) {
			//json failed
		}
		
		//float ProbablyUnderratedness = mythicalmathymathsProbablyUnderratedness(chartkey);
		//float coop = overratedness(chartkey);
		// Renaming these 2 requires renaming them in lua wherever theyre used
		//msg.SetParam("mmm", ProbablyUnderratedness);
		//msg.SetParam("ixmixblixb", 2);
		MESSAGEMAN->Broadcast(msg);	// see start of function
	};
	SendRequest("/charts/"+chartkey+"/leaderboards", vector<pair<string, string>>(), done, true);
}

void DownloadManager::RefreshCoreBundles() {
	auto done = [](HTTPRequest& req, CURLMsg *) {
		try {
			json j = json::parse(req.result);
			auto bundles = j.find("data");
			if (bundles == j.end())
				return;
			auto& dlPacks = DLMAN->downloadablePacks;
			for (auto bundleData : (*bundles)) {
				auto bundleName = bundleData.value("id", "");
				auto packs = bundleData["attributes"]["packs"];
				(DLMAN->bundles)[bundleName] = {};
				auto& bundle = (DLMAN->bundles)[bundleName];
				for (auto pack : packs) {
					auto name = pack.value("packname", "");
					auto dlPack = find_if(dlPacks.begin(), dlPacks.end(),
						[&name](DownloadablePack x) { return x.name == name; });
					if (dlPack != dlPacks.end())
						bundle.emplace_back(&(*dlPack));
				}
			}
		}
		catch (exception e) {}
	};
	SendRequest("packs/collections/", {}, done, false);
}

vector<DownloadablePack*> DownloadManager::GetCoreBundle(string whichoneyo) {
	return bundles.count(whichoneyo) ? bundles[whichoneyo] : vector<DownloadablePack*>();
}

void DownloadManager::DownloadCoreBundle(string whichoneyo) {
	auto bundle = GetCoreBundle(whichoneyo);
	sort(bundle.begin(), bundle.end(), [](DownloadablePack* x1, DownloadablePack* x2) {return x1->size < x2->size; });
	for (auto pack : bundle)
		DLMAN->DownloadQueue.emplace_back(pack);
}

void DownloadManager::RefreshLastVersion()
{
	auto done = [this](HTTPRequest& req, CURLMsg *) {
		json j;
		bool parsed = true;
		try {
			j = json::parse(req.result);
		}
		catch (exception e) {
			parsed = false;
		}
		if (!parsed)
			return;
		try {
			this->lastVersion = j["data"]["attributes"].value("version", GAMESTATE->GetEtternaVersion().c_str());
		}
		catch (exception e) {}
	};
	SendRequest("client/version", vector<pair<string, string>>(), done, false, false, true);
}
void DownloadManager::RefreshRegisterPage()
{
	auto done = [this](HTTPRequest& req, CURLMsg *) {
		json j;
		bool parsed = true;
		try {
			j = json::parse(req.result);
		}
		catch (exception e) {
			parsed = false;
		}
		if (!parsed)
			return;
		try {
			this->registerPage = j["data"]["attributes"].value("url", "");
		} catch(exception e) { }
	};
	SendRequest("client/registration", vector<pair<string, string>>(), done, false, false, true);
}
void DownloadManager::RefreshTop25(Skillset ss)
{
	DLMAN->topScores[ss].clear();
	if (!LoggedIn())
		return;
	string req = "user/"+DLMAN->sessionUser+"/top/";
	CURL *curlHandle = initCURLHandle(true);
	if(ss!= Skill_Overall)
		req += SkillsetToString(ss)+"/25";
	auto done = [ss](HTTPRequest& req, CURLMsg *) {
		try {
			auto j = json::parse(req.result);
			try {
				if (j["errors"]["status"] == 404)
					return;
			}
			catch (exception e) { }
			auto scores = j.find("data");
			vector<OnlineTopScore> & vec = DLMAN->topScores[ss];
			
			if(scores == j.end()) {
			  return;
			}
			
			for (auto scoreJ : (*scores)) {
				try {
					auto score = *(scoreJ.find("attributes"));
					OnlineTopScore tmp;
					tmp.songName = score.value("songName", "");
					tmp.wifeScore = static_cast<float>(score.value("wife", 0.0)/100.0);
					tmp.overall = static_cast<float>(score.value("Overall", 0.0));
					if(ss!=Skill_Overall)
						tmp.ssr = static_cast<float>(score["skillsets"].value(SkillsetToString(ss), 0.0));
					else
						tmp.ssr = tmp.overall;
					tmp.chartkey = score.value("chartKey", "");
					tmp.scorekey = scoreJ.value("id", "");
					tmp.rate = static_cast<float>(score.value("rate", 0.0));
					tmp.difficulty = StringToDifficulty(score.value("difficulty", "Invalid").c_str());
					vec.push_back(tmp);
				}
				catch (exception e) {
					//score failed
				}
			}
			MESSAGEMAN->Broadcast("OnlineUpdate");
		}
		catch (exception e) { /* We already cleared the vector */}
	};
	SendRequest(req, {}, done);
	return;
}
// Skillset ratings (we dont care about mod lvl, username, about, etc)
void DownloadManager::RefreshUserData()
{
	if (!LoggedIn())
		return;
	auto done = [](HTTPRequest& req, CURLMsg *) {
		json j;
		bool parsed = true;
		try {
			j = json::parse(req.result);
		}
		catch (exception e) {
			parsed = false;
		}
		try {
			auto attr = j.find("data")->find("attributes");
			auto skillsets = attr->find("skillsets");
			FOREACH_ENUM(Skillset, ss)
				(DLMAN->sessionRatings)[ss] = skillsets->value(SkillsetToString(ss).c_str(), 0.0f);
			DLMAN->sessionRatings[Skill_Overall] = attr->value("playerRating", DLMAN->sessionRatings[Skill_Overall]);
		}
		catch (exception e) {
			FOREACH_ENUM(Skillset, ss)
				(DLMAN->sessionRatings)[ss] = 0.0f;
		}
		MESSAGEMAN->Broadcast("OnlineUpdate");
	};
	SendRequest("user/" + sessionUser, {}, done);
	return;
}

void DownloadManager::OnLogin()
{
	DLMAN->RefreshUserRank();
	DLMAN->RefreshUserData();
	FOREACH_ENUM(Skillset, ss)
		DLMAN->RefreshTop25(ss);
	if (DLMAN->ShouldUploadScores())
		DLMAN->UploadScores();
	if (GAMESTATE->m_pCurSteps[PLAYER_1] != nullptr)
		DLMAN->RequestChartLeaderBoard(GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey());
	MESSAGEMAN->Broadcast("Login");
	DLMAN->loggingIn = false;
}

void DownloadManager::StartSession(string user, string pass, function<void(bool loggedIn)> callback = [](bool) {return; })
{
	string url = serverURL.Get() + "/login";
	if (loggingIn || user == "") {
		return;
	}
	DLMAN->loggingIn = true;
	EndSessionIfExists();
	CURL *curlHandle = initCURLHandle(false);
	SetCURLPostToURL(curlHandle, url);
	curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, ""); /* start cookie engine */

	curl_httppost *form = nullptr;
	curl_httppost *lastPtr = nullptr;
	CURLFormPostField(curlHandle, form, lastPtr, "username", user.c_str());
	CURLFormPostField(curlHandle, form, lastPtr, "password", pass.c_str());
	CURLFormPostField(curlHandle, form, lastPtr, "clientData", CLIENT_DATA_KEY.c_str());
	curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form);

	auto done = [user, pass, callback](HTTPRequest& req, CURLMsg *) {
		json j;
		bool parsed = true;
		try {
			j = json::parse(req.result);
		}
		catch (exception e) {
			parsed = false;
		}
		try {
			DLMAN->authToken = j["data"]["attributes"].value("accessToken", "");
			DLMAN->sessionUser = user;
			DLMAN->sessionPass = pass;
		}
		catch (exception e) {
			DLMAN->authToken = DLMAN->sessionUser = DLMAN->sessionPass = "";
			MESSAGEMAN->Broadcast("LoginFailed");
			DLMAN->loggingIn = false;
			return;
		}
		DLMAN->OnLogin();
		callback(DLMAN->LoggedIn());
	};
	HTTPRequest* req = new HTTPRequest(curlHandle, done, form);
	req->Failed = [](HTTPRequest& req, CURLMsg *) {
		DLMAN->authToken = DLMAN->sessionUser = DLMAN->sessionPass = "";
		MESSAGEMAN->Broadcast("LoginFailed");
		DLMAN->loggingIn = false;
	};
	SetCURLResultsString(curlHandle, &(req->result));
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
	return static_cast<float>(sessionRatings[ss]);
}
void DownloadManager::RefreshPackList(string url)
{
	if (url == "") 
		return;
	auto done = [](HTTPRequest& req, CURLMsg *) {
		json j;
		bool parsed = true;
		try {
			j = json::parse(req.result);
		}
		catch (exception e) {
			parsed = false;
		}
		if (!parsed)
			return;
		auto& packlist = DLMAN->downloadablePacks;
		DLMAN->downloadablePacks.clear();
		try {
			nlohmann::basic_json<> packs;
			if (j.is_array())
				packs = j;
			else
				packs = *(j.find("data"));
			for (auto packJ : packs) {
				try {
					DownloadablePack tmp;
					if (packJ.find("id") != packJ.end())
						tmp.id = stoi(packJ.value("id", ""));
					auto attr = packJ.find("attributes");
					auto pack = attr != packJ.end() ? *attr : packJ;
					
					if (pack.find("pack") != pack.end())
						tmp.name = pack.value("pack", "");
					else if (pack.find("packname") != pack.end())
						tmp.name = pack.value("packname", "");
					else if (pack.find("name") != pack.end())
						tmp.name = pack.value("name", "");
					else
						continue;
					try {
						if (pack.find("download") != pack.end())
							tmp.url = pack.value("download", "");
						else if (pack.find("url") != pack.end())
							tmp.url = pack.value("url", "");
						else
							continue;
					}
					catch (exception e) {
						continue;
					}
					if (tmp.url.empty())
						continue;
					if (pack.find("average") != pack.end())
						tmp.avgDifficulty = static_cast<float>(pack.value("average", 0.0));
					else
						tmp.avgDifficulty = 0.f;
					if (pack.find("size") != pack.end())
						tmp.size = pack.value("size", 0);
					else
						tmp.size = 0;
					packlist.push_back(tmp);
				} catch (exception e) {}
			}
		} catch (exception e) { }
		DLMAN->RefreshCoreBundles();
	};
	SendRequestToURL(url, {}, done, false, false, true, false);
	return;
}

Download::Download(string url, string filename, function<void(Download*)> done)
{
	Done = done;
	m_Url = url;
	handle = initBasicCURLHandle();
	m_TempFileName = filename != "" ? filename : MakeTempFileName(url);
	auto opened = p_RFWrapper.file.Open(m_TempFileName, 2);
	ASSERT(opened);
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
	static int GetPacklist(T* p, lua_State* L) {
		DLMAN->pl.PushSelf(L);
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
		return 0;
	}
	static int LoginWithToken(T* p, lua_State* L)
	{
		string user = SArg(1);
		string token = SArg(2);
		DLMAN->EndSessionIfExists();
		DLMAN->authToken = token;
		DLMAN->sessionUser = user;
		DLMAN->sessionPass = "";
		DLMAN->OnLogin();
		return 0;
	}
	static int Logout(T* p, lua_State* L)
	{
		DLMAN->EndSessionIfExists();
		return 0;
	}
	static int GetLastVersion(T* p, lua_State* L)
	{
		lua_pushstring(L, DLMAN->lastVersion.c_str());
		return 1;
	}
	static int GetRegisterPage(T* p, lua_State* L)
	{
		lua_pushstring(L, DLMAN->registerPage.c_str());
		return 1;
	}
	static int GetTopSkillsetScore(T* p, lua_State* L)
	{
		int rank = IArg(1);
		auto ss = Enum::Check<Skillset>(L, 2);
		bool result;
		auto onlineScore = DLMAN->GetTopSkillsetScore(rank, ss, result);
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
		LuaHelpers::Push(L,PlayerStageStats::GetGrade(onlineScore.wifeScore));
		lua_setfield(L, -2, "grade");
		return 1;
	}
	static int GetTopChartScoreCount(T* p, lua_State* L)
	{
		string ck = SArg(1);
		if (DLMAN->chartLeaderboards.count(ck))
			lua_pushnumber(L, DLMAN->chartLeaderboards[ck].size());
		else
			lua_pushnumber(L, 0); 
		return 1;
	}
	static int GetTopChartScore(T* p, lua_State* L)
	{
		string chartkey = SArg(1);
		int rank = IArg(2);
		int index = rank - 1;
		if (index < 0 || !DLMAN->chartLeaderboards.count(chartkey) || index >= static_cast<int>(DLMAN->chartLeaderboards[chartkey].size())) {
			lua_pushnil(L);
			return 1;
		}
		auto& score = DLMAN->chartLeaderboards[chartkey][index];
		lua_createtable(L, 0, 17 + NUM_Skillset + (score.replayData.empty() ? 0 : 1));
		FOREACH_ENUM(Skillset, ss) {
			lua_pushnumber(L, score.SSRs[ss]);
			lua_setfield(L, -2, SkillsetToString(ss).c_str());
		}
		lua_pushboolean(L, score.valid);
		lua_setfield(L, -2, "valid");
		lua_pushnumber(L, score.rate);
		lua_setfield(L, -2, "rate");
		lua_pushnumber(L, score.wife);
		lua_setfield(L, -2, "wife");
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
		auto bundle = DLMAN->GetCoreBundle(SArg(1));
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

		if(!bundle.empty())
			avgpackdiff /= bundle.size();
		
		// this may be kind of unintuitive but lets roll with it for now -mina
		lua_pushnumber(L, totalsize);
		lua_setfield(L, -2, "TotalSize");
		lua_pushnumber(L, avgpackdiff);
		lua_setfield(L, -2, "AveragePackDifficulty");

		return 1;
	}
	static int DownloadCoreBundle(T* p, lua_State* L)
	{
		DLMAN->DownloadCoreBundle(SArg(1));
		return 0;
	}
	static int GetToken(T* p, lua_State* L)
	{
		lua_pushstring(L, DLMAN->authToken.c_str());
		return 1;
	}

	static int RequestChartLeaderBoard(T* p, lua_State* L) {
		//p->RequestChartLeaderBoard(SArg(1));
		p->MakeAThing(SArg(1));
		vector<HighScore*> wot;
		unordered_set<string> userswithscores;
		float currentrate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		for (auto& zoop : p->athing) {
			if (lround(zoop.GetMusicRate() * 10000.f) != lround(currentrate * 10000.f) && p->currentrateonly)
				continue;
			if (userswithscores.count(zoop.GetName()) == 1 && p->topscoresonly)
				continue;
			wot.push_back(&zoop);
			userswithscores.emplace(zoop.GetName());
		}
		LuaHelpers::CreateTableFromArray(wot, L);
		return 1;
	}

	static int ToggleRateFilter(T* p, lua_State* L) {
		p->currentrateonly = !p->currentrateonly;
		return 0;
	}
	static int GetCurrentRateFilter(T* p, lua_State* L) {
		lua_pushboolean(L, p->currentrateonly);
		return 1;
	}
	static int ToggleTopScoresOnlyFilter(T* p, lua_State* L) {
		p->topscoresonly = !p->topscoresonly;
		return 0;
	}
	static int GetTopScoresOnlyFilter(T* p, lua_State* L) {
		lua_pushboolean(L, p->topscoresonly);
		return 1;
	}

	LunaDownloadManager()
	{
		ADD_METHOD(DownloadCoreBundle);
		ADD_METHOD(GetCoreBundle);
		ADD_METHOD(GetPacklist);
		ADD_METHOD(GetDownloadingPacks);
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
		ADD_METHOD(RequestChartLeaderBoard);
		ADD_METHOD(ToggleRateFilter);
		ADD_METHOD(GetCurrentRateFilter);
		ADD_METHOD(ToggleTopScoresOnlyFilter);
		ADD_METHOD(GetTopScoresOnlyFilter);
		ADD_METHOD(Logout);
	}
};
LUA_REGISTER_CLASS(DownloadManager) 

class LunaPacklist: public Luna<Packlist>
{
public:
	static int GetPackTable(T* p, lua_State* L) {
		LuaHelpers::CreateTableFromArray(p->packs, L);
		return 1;
	}
	static int SetFromCoreBundle(T* p, lua_State* L) {
		p->packs.clear();
		p->packs = DLMAN->GetCoreBundle(SArg(1));
		MESSAGEMAN->Broadcast("RefreshPacklist");
		return 0;
	}
	static int FilterAndSearch(T* p, lua_State* L) {
		if (lua_gettop(L) < 5) {
			return luaL_error(L, "GetFilteredAndSearchedPackList expects exactly 5 arguments(packname, lower diff, upper diff, lower size, upper size)");
		}
		string name = SArg(1);
		double avgLower = max(luaL_checknumber(L, 2), 0.0);
		double avgUpper = max(luaL_checknumber(L, 3), 0.0);
		size_t sizeLower = static_cast<size_t>(luaL_checknumber(L, 4));
		size_t sizeUpper = static_cast<size_t>(luaL_checknumber(L, 5));
		
		p->packs.clear();
		auto &packs = DLMAN->downloadablePacks;
		for (unsigned i = 0; i < packs.size(); ++i) {
			if (packs[i].avgDifficulty >= avgLower &&
				findStringIC(packs[i].name, name) &&
				(avgUpper <= 0 || packs[i].avgDifficulty < avgUpper) &&
				packs[i].size >= sizeLower &&
				(sizeUpper <= 0 || packs[i].size < sizeUpper))
				p->packs.push_back(&packs[i]);
		}
		MESSAGEMAN->Broadcast("RefreshPacklist");
		return 0;
	}
	static int GetTotalSize(T* p, lua_State* L) {
		size_t totalsize = 0;
		for (auto n : p->packs)
			totalsize += n->size;
		lua_pushnumber(L, totalsize / 1024 / 1024);
		return 1;
	}
	static int GetAvgDiff(T* p, lua_State* L) {
		float avgpackdiff = 0.f;
		for (auto n : p->packs)
			avgpackdiff += n->avgDifficulty;
		if (!p->packs.empty())
			avgpackdiff /= p->packs.size();
		lua_pushnumber(L, avgpackdiff);
		return 1;
	}
	// i guess these should be internal functions and lua just calls them huh -mina
	static int SortByName(T* p, lua_State* L) {
		if(p->sortmode == 1)
			if (p->asc) {
				auto comp = [](DownloadablePack* a, DownloadablePack* b) { return Rage::make_lower(a->name) > Rage::make_lower(b->name); };	// custom operators?
				sort(p->packs.begin(), p->packs.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](DownloadablePack* a, DownloadablePack* b) { return Rage::make_lower(a->name) < Rage::make_lower(b->name); };
		sort(p->packs.begin(), p->packs.end(), comp);
		p->sortmode = 1;
		p->asc = true;
		MESSAGEMAN->Broadcast("RefreshPacklist");
		return 0;
	}
	static int SortByDiff(T* p, lua_State* L) {
		auto& packs = p->packs;
		if (p->sortmode == 2)
			if (p->asc) {
				auto comp = [](DownloadablePack* a, DownloadablePack* b) { return (a->avgDifficulty < b->avgDifficulty); };
				sort(packs.begin(), packs.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](DownloadablePack* a, DownloadablePack* b) { return (a->avgDifficulty > b->avgDifficulty); };
		sort(packs.begin(), packs.end(), comp);
		p->sortmode = 2;
		p->asc = true;
		MESSAGEMAN->Broadcast("RefreshPacklist");
		return 0;
	}
	static int SortBySize(T* p, lua_State* L) {
		auto& packs = p->packs;
		if (p->sortmode == 3)
			if (p->asc) {
				auto comp = [](DownloadablePack* a, DownloadablePack* b) { return (a->size < b->size); };
				sort(packs.begin(), packs.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](DownloadablePack* a, DownloadablePack* b) { return (a->size > b->size); };
		sort(packs.begin(), packs.end(), comp);
		p->sortmode = 3;
		p->asc = true;
		MESSAGEMAN->Broadcast("RefreshPacklist");
		return 0;
	}
	static int SetFromAll(T* p, lua_State* L) {
		auto& packs = DLMAN->downloadablePacks;
		p->packs.clear();
		for (auto& n : packs)
			p->packs.emplace_back(&n);
		MESSAGEMAN->Broadcast("RefreshPacklist");
		return 0;
	}
	LunaPacklist()
	{
		ADD_METHOD(GetPackTable);
		ADD_METHOD(GetTotalSize);
		ADD_METHOD(GetAvgDiff);
		ADD_METHOD(SetFromCoreBundle);
		ADD_METHOD(SortByName);
		ADD_METHOD(SortByDiff);
		ADD_METHOD(SortBySize);
		ADD_METHOD(FilterAndSearch);
		ADD_METHOD(SetFromAll);
	}
};

LUA_REGISTER_CLASS(Packlist)


class LunaDownloadablePack : public Luna<DownloadablePack>
{
public:
	static int DownloadAndInstall(T* p, lua_State* L)
	{
		if (p->downloading)
			return 1;
		Download * dl = DLMAN->DownloadAndInstallPack(p);
		if (dl) {
			dl->PushSelf(L);
			p->downloading = true;
		}
		else
			lua_pushnil(L);
		IsQueued(p, L);
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
	static int IsQueued(T* p, lua_State* L)
	{
		auto it = std::find(DLMAN->DownloadQueue.begin(), DLMAN->DownloadQueue.end(), p);
		lua_pushboolean(L, it != DLMAN->DownloadQueue.end());
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
	static int GetID(T* p, lua_State* L) {
		lua_pushnumber(L, p->id);
		return 1;
	}
	LunaDownloadablePack()
	{
		ADD_METHOD(DownloadAndInstall);
		ADD_METHOD(IsDownloading);
		ADD_METHOD(IsQueued);
		ADD_METHOD(GetAvgDifficulty);
		ADD_METHOD(GetName);
		ADD_METHOD(GetSize);
		ADD_METHOD(GetDownload);
		ADD_METHOD(GetID);
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



