#include "global.h"
#include "RageFileManager.h"
#include "ScreenManager.h"
#include "Preference.h"
#include "RageLog.h"
#include "RageFile.h"
#include "DownloadManager.h"
#include "RageFileManager.h"
#include "SongManager.h"
#include "ScreenInstallOverlay.h"
#include "CommandLineActions.h"
#include "ScreenSelectMusic.h"
#include "SpecialFiles.h"
#include "curl/curl.h"

DownloadManager *DLMAN = NULL;

static Preference<unsigned int> maxDLPerSecond("maximumBytesDownloadedPerSecond", 0);
static Preference<unsigned int> maxDLPerSecondGameplay("maximumBytesDownloadedPerSecondDuringGameplay", 0);

static const RString TEMP_ZIP_MOUNT_POINT = "/@temp-zip/";
const RString TEMP_OS_MOUNT_POINT = "/@temp-os/";

void InstallSmzip(const RString &sZipFile)
{
	if (!FILEMAN->Mount("zip", sZipFile, TEMP_ZIP_MOUNT_POINT))
		FAIL_M("Failed to mount " + sZipFile);

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

void InstallSmzipOsArg(const RString &sOsZipFile)
{
	SCREENMAN->SystemMessage("Installing " + sOsZipFile);

	RString sOsDir, sFilename, sExt;
	splitpath(sOsZipFile, sOsDir, sFilename, sExt);

	if (!FILEMAN->Mount("dir", sOsDir, TEMP_OS_MOUNT_POINT))
		FAIL_M("Failed to mount " + sOsDir);
	InstallSmzip(TEMP_OS_MOUNT_POINT + sFilename + sExt);

	FILEMAN->Unmount("dir", sOsDir, TEMP_OS_MOUNT_POINT);

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
	auto file = static_cast<RageFile*>(pnf);
	int bytes = size*nmemb;
	return file->Write(dlBuffer, bytes);
}

DownloadManager::DownloadManager() {
	gameplay = false;
	mHandle = curl_multi_init();
	error = "";
}

void DownloadManager::Download(const RString &url)
{
	download* dl = new download();
	dl->m_Url = url;
	dl->handle = curl_easy_init();
	dl->m_TempFileName = MakeTempFileName(url);
	dl->m_TempFile.Open(dl->m_TempFileName, 2);

	//Parse spaces (curl doesnt parse them properly)
	size_t index = dl->m_Url.find(" ", 0);
	while (index != string::npos) {

		dl->m_Url.erase(index, 1);
		dl->m_Url.insert(index, "%20");
		index = dl->m_Url.find(" ", index);
	}

	curl_easy_setopt(dl->handle, CURLOPT_WRITEDATA, &(dl->m_TempFile));
	curl_easy_setopt(dl->handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(dl->handle, CURLOPT_URL, dl->m_Url);
	curl_easy_setopt(dl->handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(dl->handle, CURLOPT_XFERINFODATA, &(dl->progress));
	curl_easy_setopt(dl->handle, CURLOPT_XFERINFOFUNCTION, progressfunc);
	curl_easy_setopt(dl->handle, CURLOPT_NOPROGRESS, 0);
	dl->running = 1;
	dl->lastUpdateDone = 0;
	curl_multi_add_handle(mHandle, dl->handle);
	downloads.push_back(dl);

	curl_off_t maxDLSpeed = 0;
	auto screen = SCREENMAN->GetScreen(0);
	if (screen && screen->GetName() == "ScreenGamePlay") {
		maxDLSpeed = maxDLPerSecondGameplay;
	}
	else {
		maxDLSpeed = maxDLPerSecond;
	}
	for (auto x : downloads)
		curl_easy_setopt(x->handle, CURLOPT_MAX_RECV_SPEED_LARGE, static_cast<curl_off_t>(maxDLSpeed / downloads.size()));
	curl_multi_perform(mHandle, &running);
	SCREENMAN->SystemMessage("Downloading file " + dl->m_TempFileName + " from " + url);
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

			if (dl->m_TempFile.IsOpen()) {
				dl->m_TempFile.Close();
			}
			FILEMAN->Remove(dl->m_TempFileName);
		}
}
bool DownloadManager::UpdateAndIsFinished(float fDeltaSeconds)
{
	if (!running)
		return true;
	struct timeval timeout;
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
		struct timeval wait = { 0, 1 };
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
		if (msg->msg == CURLMSG_DONE) {
			addedPacks = true;
			download* finishedDl;
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

			//install smzip
			InstallSmzip(finishedDl->m_TempFileName);

			FILEMAN->Remove(finishedDl->m_TempFileName);

			Message msg("Download(" + finishedDl->m_Url + ") Finished");
			MESSAGEMAN->Broadcast(msg);
			delete finishedDl;
		}
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

RString DownloadManager::MakeTempFileName(RString s)
{
	return SpecialFiles::CACHE_DIR + "Downloads/" + Basename(s);
}




#endif
