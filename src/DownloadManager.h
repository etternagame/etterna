
#ifndef SM_DOWNMANAGER

#define SM_DOWNMANAGER





#include "global.h"
#include "CommandLineActions.h"
#include "RageFile.h"
#include "ScreenManager.h"
#include "RageFileManager.h"
#include "curl/curl.h"




#if !defined(WITHOUT_NETWORKING)

class DownloadablePack;

struct ProgressData {
	curl_off_t total = 0; //total bytes
	curl_off_t downloaded = 0; //bytes downloaded
	float time = 0;//seconds passed
};

struct WriteThis {
	RageFile* file;
	bool stop = false;
};

struct Download {
	CURL* handle;
	int running;
	ProgressData progress;
	string m_TempFileName;
	string status;
	string speed;
	curl_off_t downloadedAtLastUpdate = 0;
	curl_off_t lastUpdateDone = 0;
	RageFile m_TempFile;
	string m_Url;
	WriteThis* wt;
	DownloadablePack* pack;

};

class DownloadablePack {
public:
	string name = "";
	size_t size = 0;
	int id = 0;
	float avgDifficulty = 0;
	string url = "";
	bool downloading = false;
	Download* download;
	// Lua
	void PushSelf(lua_State *L);
};

class DownloadManager
{
public:
	DownloadManager();
	~DownloadManager();
	vector<Download*> downloads;
	CURLM* mHandle;
	string aux;
	int running;
	bool gameplay;
	string error;
	int lastid;

	vector<DownloadablePack> downloadablePacks;

	Download* DownloadAndInstallPack(const string &url);
	Download* DownloadManager::DownloadAndInstallPack(DownloadablePack* pack);

	bool GetAndCachePackList(string url);

	vector<DownloadablePack>* GetPackList(string url, bool &result);

	void UpdateDLSpeed();
	void UpdateDLSpeed(bool gameplay);

	bool DownloadManager::EncodeSpaces(string& str);

	void InstallSmzip(const string &sZipFile);
	
	string GetError() { return error; }
	bool Error() { return error == ""; }
	bool UpdateAndIsFinished(float fDeltaSeconds);
	string MakeTempFileName(string s);

	bool UploadProfile(string url, string file, string user, string pass);

	// Lua
	void PushSelf(lua_State *L);
};

extern DownloadManager *DLMAN;

#endif

#endif