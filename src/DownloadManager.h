
#ifndef SM_DOWNMANAGER

#define SM_DOWNMANAGER

#if !defined(WITHOUT_NETWORKING)




#include "global.h"
#include "CommandLineActions.h"
#include "RageFile.h"
#include "ScreenManager.h"
#include "RageFileManager.h"
#include "curl/curl.h"





class DownloadablePack;

class ProgressData {
public:
	curl_off_t total = 0; //total bytes
	curl_off_t downloaded = 0; //bytes downloaded
	float time = 0;//seconds passed
};

class WriteThis {
public:
	shared_ptr<RageFile> file;
	bool stop = false;
};

class Download {
public:
	Download(string url);
	~Download();
	void Install();
	void Update(float fDeltaSeconds);
	void Failed();
	string StartMessage() { return "Downloading file " + m_TempFileName + " from " + m_Url; };
	string Status() { return m_TempFileName + "\n" + speed + " KB/s\n" +
		"Downloaded " + to_string(progress.downloaded / 1024) + "/" + to_string(progress.total / 1024) + " (KB)"; }
	CURL* handle;
	int running;
	ProgressData progress;
	string speed;
	curl_off_t downloadedAtLastUpdate = 0;
	curl_off_t lastUpdateDone = 0;
	RageFile m_TempFile;
	string m_Url;
	shared_ptr<WriteThis> wt;
	shared_ptr<DownloadablePack> pack;
protected:
	string MakeTempFileName(string s);
	string m_TempFileName;
};

class DownloadablePack {
public:
	string name = "";
	size_t size = 0;
	int id = 0;
	float avgDifficulty = 0;
	string url = "";
	bool downloading = false;
	shared_ptr<Download> download;
	// Lua
	void PushSelf(lua_State *L);
};

class DownloadManager
{
public:
	DownloadManager();
	~DownloadManager();
	vector<shared_ptr<Download>> downloads;
	CURLM* mHandle;
	string aux;
	int running;
	bool gameplay;
	string error;
	int lastid;

	vector<DownloadablePack> downloadablePacks;

	shared_ptr<Download> DownloadAndInstallPack(const string &url);
	shared_ptr<Download>  DownloadAndInstallPack(shared_ptr<DownloadablePack> pack);

	bool GetAndCachePackList(string url);

	vector<DownloadablePack>* GetPackList(string url, bool &result);

	void UpdateDLSpeed();
	void UpdateDLSpeed(bool gameplay);

	bool EncodeSpaces(string& str);

	void InstallSmzip(const string &sZipFile);
	
	string GetError() { return error; }
	bool Error() { return error == ""; }
	bool UpdateAndIsFinished(float fDeltaSeconds);

	bool UploadProfile(string url, string file, string user, string pass);

	// Lua
	void PushSelf(lua_State *L);
};

extern shared_ptr<DownloadManager> DLMAN;

#endif

#endif