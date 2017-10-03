
#ifndef SM_DOWNMANAGER

#define SM_DOWNMANAGER

#include "global.h"
#include "CommandLineActions.h"
#include "RageFile.h"
#include "ScreenManager.h"
#include "RageFileManager.h"
#include "curl/curl.h"



void InstallSmzipOsArg(const RString &sOsZipFile);
void InstallSmzip(const RString &sZipFile);

#if !defined(WITHOUT_NETWORKING)


struct ProgressData {
	curl_off_t total = 0; //total bytes
	curl_off_t downloaded = 0; //bytes downloaded
	float time = 0;//seconds passed
};

struct download {
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
};

class DownloadManager
{
public:
	vector<download*> downloads;
	CURLM* mHandle;
	string aux;
	int running;
	bool gameplay;
	string error;
	DownloadManager();
	void Download(const RString &url);
	~DownloadManager();
	
	RString GetError() { return error; }
	bool Error() { return error == ""; }
	bool UpdateAndIsFinished(float fDeltaSeconds);
	RString MakeTempFileName(RString s);
};

extern DownloadManager *DLMAN;

#endif

#endif