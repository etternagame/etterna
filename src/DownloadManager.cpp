#include "global.h"
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

DownloadManager *DLMAN = NULL;

static Preference<unsigned int> maxDLPerSecond("maximumBytesDownloadedPerSecond", 0);
static Preference<unsigned int> maxDLPerSecondGameplay("maximumBytesDownloadedPerSecondDuringGameplay", 0);
static Preference<RString> packListURL("packListURL", "");

static const RString TEMP_ZIP_MOUNT_POINT = "/@temp-zip/";

struct BufferStruct {
	string str;
};

size_t write_memory_buffer(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	string tmp(static_cast<char*>(contents), realsize);
	static_cast<BufferStruct*>(userp)->str.append(tmp);
	return realsize;

}

struct ReadThis {
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

	vector<RString> vsFiles;
	{
		vector<RString> vsRawFiles;
		GetDirListingRecursive(TEMP_ZIP_MOUNT_POINT, "*", vsRawFiles);

		vector<RString> vsPrettyFiles;
		FOREACH_CONST(RString, vsRawFiles, s)
		{
			if (GetExtension(*s).EqualsNoCase("ctl"))
				continue;

			vsFiles.push_back(*s);

			RString s2 = s->Right(s->length() - TEMP_ZIP_MOUNT_POINT.length());
			vsPrettyFiles.push_back(s2);
		}
		sort(vsPrettyFiles.begin(), vsPrettyFiles.end());
	}

	RString sResult = "Success installing " + sZipFile;
	FOREACH_CONST(RString, vsFiles, sSrcFile)
	{
		RString sDestFile = *sSrcFile;
		sDestFile = sDestFile.Right(sDestFile.length() - TEMP_ZIP_MOUNT_POINT.length());

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


#if !defined(WITHOUT_NETWORKING)

int progressfunc(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	auto ptr = static_cast<ProgressData*>(clientp);
	ptr->total = dltotal;
	ptr->downloaded = dlnow;
	return 0;

}
size_t write_data(void *dlBuffer, size_t size, size_t nmemb, void *pnf)
{
	auto wt = static_cast<WriteThis*>(pnf);
	int bytes = size*nmemb;
	return wt->stop ? 0 : wt->file->Write(dlBuffer, bytes);
}

DownloadManager::DownloadManager() {
	curl_global_init(CURL_GLOBAL_ALL);
	gameplay = false;
	mHandle = curl_multi_init();
	error = "";
	lastid = 0;
	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring(L, "DLMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
	GetAndCachePackList(packListURL);
	vector<RString> vsRawFiles;
	GetDirListingRecursive(TEMP_ZIP_MOUNT_POINT, "*", vsRawFiles);
}

DownloadManager::~DownloadManager()
{
	if (mHandle != nullptr)
		curl_multi_cleanup(mHandle);
	mHandle = nullptr;
	for (auto dl : downloads)
		if (dl->handle != nullptr) {
			curl_easy_cleanup(dl);
			dl->handle = nullptr;



			FILEMAN->Remove(dl->m_TempFileName);
		}
	curl_global_cleanup();
}

void DownloadManager::UpdateDLSpeed()
{

	auto screen = SCREENMAN->GetScreen(0);
	UpdateDLSpeed(screen && screen->GetName() == "ScreenGamePlay");
}

void DownloadManager::UpdateDLSpeed(bool gameplay)
{
	size_t maxDLSpeed;
	if (gameplay) {
		maxDLSpeed = maxDLPerSecondGameplay;
	}
	else {
		maxDLSpeed = maxDLPerSecond;
	}
	for (auto x : downloads)
		curl_easy_setopt(x->handle, CURLOPT_MAX_RECV_SPEED_LARGE, static_cast<curl_off_t>(maxDLSpeed / downloads.size()));

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

Download* DownloadManager::DownloadAndInstallPack(const string &url)
{
	Download* dl = new Download();
	dl->m_Url = url;
	dl->handle = curl_easy_init();
	dl->m_TempFileName = MakeTempFileName(url);
	dl->m_TempFile.Open(dl->m_TempFileName, 2);
	WriteThis *wt=new WriteThis;
	wt->file = &(dl->m_TempFile);
	wt->stop = NULL;
	EncodeSpaces(dl->m_Url);

	curl_easy_setopt(dl->handle, CURLOPT_WRITEDATA, wt);
	curl_easy_setopt(dl->handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(dl->handle, CURLOPT_URL, dl->m_Url);
	curl_easy_setopt(dl->handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(dl->handle, CURLOPT_XFERINFODATA, &(dl->progress));
	curl_easy_setopt(dl->handle, CURLOPT_XFERINFOFUNCTION, progressfunc);
	curl_easy_setopt(dl->handle, CURLOPT_NOPROGRESS, 0);
	dl->running = 1;
	dl->lastUpdateDone = 0;
	dl->wt = wt;
	curl_multi_add_handle(mHandle, dl->handle);
	downloads.push_back(dl);

	UpdateDLSpeed();

	curl_multi_perform(mHandle, &running);
	SCREENMAN->SystemMessage("Downloading file " + dl->m_TempFileName + " from " + url);

	return dl;
}

Download* DownloadManager::DownloadAndInstallPack(DownloadablePack* pack)
{
	Download* dl = DownloadAndInstallPack(pack->url);
	dl->pack = pack;
	return dl;
}

bool DownloadManager::UpdateAndIsFinished(float fDeltaSeconds)
{
	if (!running)
		return true;
	timeval timeout;
	int rc; /* select() return code */
	CURLMcode mc; /* curl_multi_fdset() return code */

	fd_set fdread;
	fd_set fdwrite;
	fd_set fdexcep;
	int maxfd = -1;

	long curl_timeo = -1;

	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdexcep);

	/* set a suitable timeout to play around with */
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

	/* get file descriptors from the transfers */
	mc = curl_multi_fdset(mHandle, &fdread, &fdwrite, &fdexcep, &maxfd);

	if (mc != CURLM_OK) {
		error = "curl_multi_fdset() failed, code " + mc;
		return false;
	}

	/* On success the value of maxfd is guaranteed to be >= -1. We call
	select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
	no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
	to sleep 100ms, which is the minimum suggested value in the
	curl_multi_fdset() doc. */

	if (maxfd == -1) {
#ifdef _WIN32
		Sleep(1);
		rc = 0;
#else
		/* Portable sleep for platforms other than Windows. */
		timeval wait = { 0, 1 };
		rc = select(0, NULL, NULL, NULL, &wait);
#endif
	}
	else {
		/* Note that on some platforms 'timeout' may be modified by select().
		If you need access to the original value save a copy beforehand. */
		rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
	}

	switch (rc) {
	case -1:
		error = "select error" + mc;
		break;
	case 0: /* timeout */
	default: /* action */
		curl_multi_perform(mHandle, &running);
		for (auto dl : downloads) {
			dl->progress.time += fDeltaSeconds;
			if (dl->progress.time > 1.0) {
				dl->speed = to_string(dl->progress.downloaded / 1024 - dl->downloadedAtLastUpdate);
				dl->progress.time = 0;
				dl->downloadedAtLastUpdate = dl->progress.downloaded / 1024;
			}
			dl->status = dl->m_TempFileName + "\n" + dl->speed + " KB/s\n" +
				"Downloaded " + to_string(dl->progress.downloaded / 1024) + "/" + to_string(dl->progress.total / 1024) + " (KB)";
		}

		break;
	}
	//Check for finished downloads
	CURLMsg *msg;
	int msgs_left;
	bool addedPacks = false;

	while (msg = curl_multi_info_read(mHandle, &msgs_left)) {
		Download* finishedDl;
		/* Find out which handle this message is about */
		for (auto i = downloads.begin(); i < downloads.end(); i++) {
			if (msg->easy_handle == (*i)->handle) {
				finishedDl = *i;
				downloads.erase(i);
				break;
			}
		}
		if (finishedDl->handle != nullptr)
			curl_easy_cleanup(finishedDl->handle);
		finishedDl->handle = nullptr;
		if (finishedDl->m_TempFile.IsOpen())
			finishedDl->m_TempFile.Close();
		if (msg->msg == CURLMSG_DONE) {
			addedPacks = true;

			//install smzip
			InstallSmzip(finishedDl->m_TempFileName);

			if (finishedDl->pack != nullptr) {
				Message msg("PackDownloaded");
				msg.SetParam("pack", LuaReference::CreateFromPush(*(finishedDl->pack)));
				MESSAGEMAN->Broadcast(msg);
			}
		} 
		else {
			if (finishedDl->pack != nullptr) {
				Message msg("DownloadFailed");
				msg.SetParam("pack", LuaReference::CreateFromPush(*(finishedDl->pack)));
				MESSAGEMAN->Broadcast(msg);
			}
		}
		FILEMAN->Remove(finishedDl->m_TempFileName);
		delete finishedDl;
	}
	if (addedPacks) {
		curl_off_t maxDLSpeed = 0;
		auto screen = SCREENMAN->GetScreen(0);
		if (screen && screen->GetName() == "ScreenSelectMusic") {
			static_cast<ScreenSelectMusic*>(screen)->DifferentialReload();
		}
		else
		{
			SONGMAN->DifferentialReload();
			if (screen && screen->GetName() == "ScreenGamePlay" && !gameplay) {
				gameplay = true;
				maxDLSpeed = maxDLPerSecondGameplay;
			}
			else {
				maxDLSpeed = maxDLPerSecond;
				gameplay = false;
			}
		}
		for (auto x : downloads)
			curl_easy_setopt(x->handle, CURLOPT_MAX_RECV_SPEED_LARGE, static_cast<curl_off_t>(maxDLSpeed / downloads.size()));

	}

	return false;

}

string DownloadManager::MakeTempFileName(string s)
{
	return SpecialFiles::CACHE_DIR + "Downloads/" + Basename(s);
}

bool DownloadManager::UploadProfile(string url, string file, string user, string pass)
{

	CURL *curlForm = curl_easy_init();

	ReadThis w;
	//d = PROFILEMAN->GetProfileDir(ProfileSlot_Player1)+"etterna.xml";
	w.file.Open(file);

	curl_httppost *formpost = NULL;
	curl_httppost *lastptr = NULL;
	curl_slist *headerlist = NULL;

	/* Fill in the file upload field *//*
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "xml",
		//CURLFORM_FILE, "postit2.c",
		CURLFORM_STREAM, &w, //Use this instead to use the read_function
		CURLFORM_FILENAME, "etterna.xml",
		CURLFORM_CONTENTLEN, static_cast<long>(w.file.GetFileSize()),
		CURLFORM_CONTENTSLENGTH, static_cast<long>(w.file.GetFileSize()),
		CURLFORM_END);
	w.file.Seek(SEEK_SET);
	*/
	RString text;
	w.file.Read(text, w.file.GetFileSize());
	w.file.Close();
	//w.file.Seek(SEEK_SET);
	//text = curl_easy_escape(curlForm, text, 0);
	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "xml",
		//CURLFORM_STREAM, &w, //Use this instead to use the read_function
		//CURLFORM_FILENAME, "etterna.xml",
		//CURLFORM_CONTENTLEN, static_cast<long>(w.file.GetFileSize()),
		//CURLFORM_CONTENTSLENGTH, static_cast<long>(w.file.GetFileSize()),
		//CURLFORM_FILENAME, "etterna.xml",
		//CURLFORM_COPYCONTENTS, text,
		CURLFORM_BUFFER, "etterna.xml",
		//CURLFORM_BUFFERPTR, curl_easy_escape(curlForm, text.c_str(), 0),
		CURLFORM_BUFFERPTR, text.c_str(),
		CURLFORM_BUFFERLENGTH, 0,
		CURLFORM_END);

	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "user",
		CURLFORM_COPYCONTENTS, curl_easy_escape(curlForm, user.c_str(), 0), 
		CURLFORM_END);


	curl_formadd(&formpost,
		&lastptr,
		CURLFORM_COPYNAME, "pass",
		CURLFORM_COPYCONTENTS, curl_easy_escape(curlForm, pass.c_str(), 0) ,
		CURLFORM_END);



	//headerlist = curl_slist_append(headerlist, "Content - Type: multipart / form - data");

	EncodeSpaces(url);
	curl_easy_setopt(curlForm, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curlForm, CURLOPT_POST, 1L);
	//curl_easy_setopt(curlForm, CURLOPT_HTTPHEADER, headerlist);
	curl_easy_setopt(curlForm, CURLOPT_HTTPPOST, formpost);

	//curl_easy_setopt(curlForm, CURLOPT_READFUNCTION, ReadThisReadCallback);
	//curl_easy_setopt(curlForm, CURLOPT_SEEKFUNCTION, ReadThisSeekCallback);

	BufferStruct bs;
	curl_easy_setopt(curlForm, CURLOPT_WRITEDATA, &bs);
	curl_easy_setopt(curlForm, CURLOPT_WRITEFUNCTION, write_memory_buffer);
	CURLcode ret = curl_easy_perform(curlForm);

	curl_easy_cleanup(curlForm);
	return ret==0;
}



bool DownloadManager::GetAndCachePackList(string url)
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
		return NULL;
	}
	CURL *curl = curl_easy_init();

	BufferStruct bs;

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	curl_easy_setopt(curl, CURLOPT_POST, 1L);

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &bs);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_buffer);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "key=nick");

	int res = curl_easy_perform(curl);
	
	curl_easy_cleanup(curl);
	if (res != CURLE_OK) {
		result = false;
	}
	Json::Value packs;
	RString error;
	vector<DownloadablePack>* packlist = new vector<DownloadablePack>;
	bool parsed = JsonUtil::LoadFromString(packs, bs.str, error);

	if (!parsed) {
		result = false;
		return packlist;
	}

	string baseUrl = "http://simfiles.stepmania-online.com/";

	for (int index = 0; index < packs.size(); ++index) {
		DownloadablePack tmp;
		tmp.name = packs[index].get("packname", "").asString();
		if (packs[index].isMember("url"))
			tmp.url = packs[index].get("url", baseUrl + tmp.name + ".zip").asString();
		else
			tmp.url = baseUrl + tmp.name + ".zip";
		tmp.avgDifficulty = static_cast<float>(packs[index].get("average", 0).asDouble());
		tmp.id = ++lastid;
		packlist->push_back(tmp); 
	}
	result = true;
	return packlist;
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
		vector<DownloadablePack>& packs = p->downloadablePacks;
		lua_createtable(L, packs.size(), 0);
		for (unsigned i = 0; i < packs.size(); ++i) {
			packs[i].PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		/*
		for (unsigned i = 0; i < packs.size(); ++i)
		{

			lua_createtable(L, 0, 3);

			LuaHelpers::Push(L, packs[i].name);
			lua_setfield(L, -2, "name");

			LuaHelpers::Push(L, packs[i].avgDifficulty);
			lua_setfield(L, -2, "avgDifficulty");

			LuaHelpers::Push(L, packs[i].id);
			lua_setfield(L, -2, "id");

			lua_rawseti(L, -2, i + 1);
		}
		*/
		return 1;
	}
	LunaDownloadManager()
	{
		ADD_METHOD(GetPackList);
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
		p->download = DLMAN->DownloadAndInstallPack(p);
		lua_pushboolean(L, 0);
		return 1;
	}
	static int GetName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->name.c_str());
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
	static int GetKBDownloaded(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->download != nullptr ? p->download->progress.downloaded : 0);
		return 1;
	}
	static int GetTotalKB(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->download != nullptr ? p->download->progress.total : 0);
		return 1;
	}
	static int Stop(T* p, lua_State* L)
	{
		if(p->download && p->download->wt)
			p->download->wt->stop = true;
		lua_pushnumber(L, p->download != nullptr ? p->download->progress.total : 0);
		return 1;
	}
	LunaDownloadablePack()
	{
		ADD_METHOD(DownloadAndInstall);
		ADD_METHOD(GetTotalKB);
		ADD_METHOD(GetKBDownloaded);
		ADD_METHOD(IsDownloading);
		ADD_METHOD(GetAvgDifficulty);
		ADD_METHOD(GetName);
		ADD_METHOD(Stop);
	}
};

LUA_REGISTER_CLASS(DownloadablePack) 


#endif
