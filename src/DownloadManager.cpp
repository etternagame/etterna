#pragma once

#include "global.h"
#if !defined(WITHOUT_NETWORKING)
#include "RageFileManager.h"
#include "ScreenManager.h"
#include "Preference.h"
#include "RageLog.h"
#include "RageFile.h"
#include "DownloadManager.h"
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

shared_ptr<DownloadManager> DLMAN = nullptr;

static Preference<unsigned int> maxDLPerSecond("maximumBytesDownloadedPerSecond", 0);
static Preference<unsigned int> maxDLPerSecondGameplay("maximumBytesDownloadedPerSecondDuringGameplay", 300000);
static Preference<RString> packListURL("packListURL", "https://etternaonline.com/api/pack_list");
static Preference<RString> serverURL("UploadServerURL", "https://etternaonline.com/api");
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

void DownloadManager::InstallSmzip(const string &sZipFile)
{
	if (!FILEMAN->Mount("zip", sZipFile, TEMP_ZIP_MOUNT_POINT))
		FAIL_M(static_cast<string>("Failed to mount " + sZipFile).c_str());

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
inline void DownloadManager::SetCURLPostToURL(CURL *curlHandle, string url)
{
	checkProtocol(url);
	EncodeSpaces(url);
	curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curlHandle, CURLOPT_POST, 1L);
}
inline void SetCURLFormPostField(CURL* curlHandle, curl_httppost *&form, curl_httppost *&lastPtr, char* field, string value)
{
	curl_formadd(&form,
		&lastPtr,
		CURLFORM_COPYNAME, field,
		CURLFORM_COPYCONTENTS, curlHandle, value.c_str(),
		CURLFORM_END);
}
inline void SetCURLFormPostField(CURL* curlHandle, curl_httppost *&form, curl_httppost *&lastPtr, string field, string value)
{
	curl_formadd(&form,
		&lastPtr,
		CURLFORM_COPYNAME, field.c_str(),
		CURLFORM_COPYCONTENTS, value.c_str(),
		CURLFORM_END);
}
template<typename T>
inline void SetCURLFormPostField(CURL* curlHandle, curl_httppost *&form, curl_httppost *&lastPtr, string field, T value)
{
	SetCURLFormPostField(curlHandle, form, lastPtr, field, to_string(value));
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
	if (mHandle != nullptr)
		curl_multi_cleanup(mHandle);
	mHandle = nullptr;
	for (auto &dl : downloads) {
		if (dl.second->handle != nullptr) {
			curl_easy_cleanup(dl.second->handle);
			dl.second->handle = nullptr;
		}
		delete dl.second;
	}
	curl_global_cleanup();
}

Download* DownloadManager::DownloadAndInstallPack(const string &url)
{	
	Download* dl = new Download(url);

	if (mHandle == nullptr)
		mHandle = curl_multi_init();
	curl_multi_add_handle(mHandle, dl->handle);
	downloads[url] = dl;

	UpdateDLSpeed();

	ret = curl_multi_perform(mHandle, &running);
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
	if (reloadPending && !gameplay) {
		auto screen = SCREENMAN->GetScreen(0);
		if (screen && screen->GetName() == "ScreenSelectMusic")
			static_cast<ScreenSelectMusic*>(screen)->DifferentialReload();
		else
			SONGMAN->DifferentialReload();
		reloadPending = false;
	}
	if (!running)
		return true;
	timeval timeout;
	int rc, maxfd = -1;
	CURLMcode mc;
	fd_set fdread, fdwrite, fdexcep;
	long curl_timeo = -1;
	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdexcep);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	curl_multi_timeout(mHandle, &curl_timeo);
	if (curl_timeo >= 0) {
		timeout.tv_sec = curl_timeo / 1000;
		if (timeout.tv_sec > 1)
			timeout.tv_sec = 1;
		else
			timeout.tv_usec = (curl_timeo % 1000) * 1000;
	}

	mc = curl_multi_fdset(mHandle, &fdread, &fdwrite, &fdexcep, &maxfd);
	if (mc != CURLM_OK) {
		error = "curl_multi_fdset() failed, code " + mc;
		return false;
	}
	if (maxfd == -1) {
#ifdef _WIN32
		Sleep(1);
		rc = 0;
#else
		/* Portable sleep for platforms other than Windows. */
		timeval wait = { 0, 1 };
		rc = select(0, nullptr, nullptr, nullptr, &wait);
#endif
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
		curl_multi_perform(mHandle, &running);
		for (auto &dl : downloads) 
			dl.second->Update(fDeltaSeconds);
		break;
	}

	//Check for finished downloads
	CURLMsg *msg;
	int msgs_left;
	bool addedPacks = false;	
	while (msg = curl_multi_info_read(mHandle, &msgs_left)) {
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
bool DownloadManager::UploadProfile(string file, string user, string pass)
{
	if (user != sessionUser || pass != sessionPass)
		if (!StartSession(user, pass))
			return false;
	return UploadProfile(file);
}

bool DownloadManager::UploadProfile(string file)
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
	SetCURLPostToURL(curlHandle, url);
	AddSessionCookieToCURL(curlHandle);
	string result;
	SetCURLResultsString(curlHandle, result);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form);
	CURLcode ret = curl_easy_perform(curlHandle);
	curl_easy_cleanup(curlHandle);
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
bool DownloadManager::UploadScore(HighScore* hs) 
{
	if (!LoggedIn())
		return false;
	CURL *curlHandle = initCURLHandle();
	string url = serverURL.Get() + "/upload_score";
	curl_httppost *form = nullptr;
	curl_httppost *lastPtr = nullptr;
	curl_slist *headerlist = nullptr;
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
	string replayString = "[";
	vector<float> timestamps = hs->timeStamps;
	vector<float> offsets = hs->GetOffsetVector();
	for (int i = 0; i < offsets.size(); i++) {
		replayString += "[" + to_string(timestamps[i]) + "," + to_string(offsets[i]) + "],";
	}
	replayString = replayString.substr(0, replayString.size() - 1); //remove ","
	replayString += "]";
	SetCURLFormPostField(curlHandle, form, lastPtr, "replay_data", replayString);
	SetCURLPostToURL(curlHandle, url);
	AddSessionCookieToCURL(curlHandle);
	string result;
	SetCURLResultsString(curlHandle, result);
	curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form);
	CURLcode ret = curl_easy_perform(curlHandle);
	curl_easy_cleanup(curlHandle);
	if (result != "\"Success\"") {
		LOG->Trace(result.c_str());
		return false;
	}

	mostrecentresult = result;

	return ret == 0;
}
void DownloadManager::EndSessionIfExists()
{
	if (!LoggedIn())
		return;
	string url = serverURL.Get() +"/destroy";
	CURL *curlHandle = initCURLHandle();

	SetCURLPostToURL(curlHandle, url);

	AddSessionCookieToCURL(curlHandle);

	CURLcode ret = curl_easy_perform(curlHandle);

	curl_easy_cleanup(curlHandle);
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

bool DownloadManager::StartSession(string user, string pass)
{
	string url = serverURL.Get() + "/login";
	if (user == sessionUser && pass == sessionPass) {
		return true;
	}
	EndSessionIfExists();
	CURL *curlHandle = initCURLHandle();
	curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, ""); /* start cookie engine */


	curl_httppost *form = nullptr;
	curl_httppost *lastPtr = nullptr;

	SetCURLFormPostField(curlHandle, form, lastPtr, "username", user);
	SetCURLFormPostField(curlHandle, form, lastPtr, "password", pass);

	SetCURLPostToURL(curlHandle, url);

	curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, form);

	string result;
	SetCURLResultsString(curlHandle, result);

	CURLcode ret = curl_easy_perform(curlHandle);

	vector<string> v_cookies;
	if (result == "\"Success\"") {
		struct curl_slist *cookies;
		struct curl_slist *cookieIterator;

		printf("Cookies, curl knows:\n");
		curl_easy_getinfo(curlHandle, CURLINFO_COOKIELIST, &cookies);

		cookieIterator = cookies;
		while (cookieIterator) {
			v_cookies.push_back(cookieIterator->data);
			cookieIterator = cookieIterator->next;
		}
		curl_slist_free_all(cookies);
		curl_easy_cleanup(curlHandle);
		for (auto& cook : v_cookies) {
			vector<string> parts = split(cook, '\t');
			for (auto x = parts.begin(); x != parts.end(); x++) {
				if (*x == "ci_session") {
					session = *(x + 1);
					sessionCookie = cook;
					break;
				}
			}
			if (!session.empty())
				break;
		}
		sessionUser = user;
		sessionPass = pass;
	}
	else {
		session = sessionUser = sessionPass = sessionCookie = "";
	}
	return !session.empty();
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
	DLMAN->InstallSmzip(m_TempFileName);
	Message msg("PackDownloaded");
	msg.SetParam("pack", LuaReference::CreateFromPush(*p_Pack));
	MESSAGEMAN->Broadcast(msg);
	return;
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
		lua_pushboolean(L, DLMAN->StartSession(user, pass));
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
	static int GetMostRecentUploadResult(T* p, lua_State* L)
	{
		lua_pushstring(L, p->mostrecentresult);
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
		ADD_METHOD(GetMostRecentUploadResult);
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
		ADD_METHOD(Stop);
	}
};

LUA_REGISTER_CLASS(Download)

#endif



