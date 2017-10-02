#include "global.h"
#include "ScreenInstallOverlay.h"
#include "RageFileManager.h"
#include "ScreenManager.h"
#include "Preference.h"
#include "RageLog.h"
#include "FileDownload.h"
#include "json/value.h"
#include "JsonUtil.h"
#include "SpecialFiles.h"
class Song;
#include "SongManager.h"
#include "GameState.h"
#include "GameManager.h"
#include "ScreenSelectMusic.h"
#include "CommonMetrics.h"
#include "SongManager.h"
#include "CommandLineActions.h"
#include "ScreenDimensions.h"
#include "StepMania.h"
#include "ActorUtil.h"
#include "curl/curl.h"

struct ProgressData {
	curl_off_t total = 0; //total bytes
	curl_off_t downloaded = 0; //bytes downloaded
	float time = 0;//seconds passed
};

struct PlayAfterLaunchInfo
{
	RString sSongDir;
	RString sTheme;
	bool bAnySongChanged;
	bool bAnyThemeChanged;

	PlayAfterLaunchInfo()
	{
		bAnySongChanged = false;
		bAnyThemeChanged = false;
	}

	void OverlayWith( const PlayAfterLaunchInfo &other )
	{
		if( !other.sSongDir.empty() ) sSongDir = other.sSongDir;
		if( !other.sTheme.empty() ) sTheme = other.sTheme;
		bAnySongChanged |= other.bAnySongChanged;
		bAnyThemeChanged |= other.bAnyThemeChanged;
	}
};

void InstallSmzipOsArg( const RString &sOsZipFile, PlayAfterLaunchInfo &out );
PlayAfterLaunchInfo DoInstalls( CommandLineActions::CommandLineArgs args );

static void Parse( const RString &sDir, PlayAfterLaunchInfo &out )
{
	vector<RString> vsDirParts;
	split( sDir, "/", vsDirParts, true );
	if( vsDirParts.size() == 3 && vsDirParts[0].EqualsNoCase("Songs") )
		out.sSongDir = "/" + sDir;
	else if( vsDirParts.size() == 2 && vsDirParts[0].EqualsNoCase("Themes") )
		out.sTheme = vsDirParts[1];
}

static const RString TEMP_ZIP_MOUNT_POINT = "/@temp-zip/";
const RString TEMP_OS_MOUNT_POINT = "/@temp-os/";

static void InstallSmzip( const RString &sZipFile, PlayAfterLaunchInfo &out )
{
	if( !FILEMAN->Mount( "zip", sZipFile, TEMP_ZIP_MOUNT_POINT ) )
		FAIL_M("Failed to mount " + sZipFile );

	vector<RString> vsFiles;
	{
		vector<RString> vsRawFiles;
		GetDirListingRecursive( TEMP_ZIP_MOUNT_POINT, "*", vsRawFiles);

		vector<RString> vsPrettyFiles;
		FOREACH_CONST( RString, vsRawFiles, s )
		{
			if( GetExtension(*s).EqualsNoCase("ctl") )
				continue;

			vsFiles.push_back( *s);

			RString s2 = s->Right( s->length() - TEMP_ZIP_MOUNT_POINT.length() );
			vsPrettyFiles.push_back( s2 );
		}
		sort( vsPrettyFiles.begin(), vsPrettyFiles.end() );
	}

	RString sResult = "Success installing " + sZipFile;
	FOREACH_CONST( RString, vsFiles, sSrcFile )
	{
		RString sDestFile = *sSrcFile;
		sDestFile = sDestFile.Right( sDestFile.length() - TEMP_ZIP_MOUNT_POINT.length() );

		RString sDir, sThrowAway;
		splitpath( sDestFile, sDir, sThrowAway, sThrowAway );

		Parse( sDir, out );
		out.bAnySongChanged = true;

		if( !FileCopy( *sSrcFile, "Songs/" + sDestFile ) )
		{
			sResult = "Error extracting " + sDestFile;
			break;
		}
	}
	
	//FILEMAN->Unmount( "zip", sZipFile, TEMP_ZIP_MOUNT_POINT );


	SCREENMAN->SystemMessage( sResult );
}

void InstallSmzipOsArg( const RString &sOsZipFile, PlayAfterLaunchInfo &out )
{
	SCREENMAN->SystemMessage("Installing " + sOsZipFile );

	RString sOsDir, sFilename, sExt;
	splitpath( sOsZipFile, sOsDir, sFilename, sExt );

	if( !FILEMAN->Mount( "dir", sOsDir, TEMP_OS_MOUNT_POINT ) )
		FAIL_M("Failed to mount " + sOsDir );
	InstallSmzip( TEMP_OS_MOUNT_POINT + sFilename + sExt, out );

	//FILEMAN->Unmount( "dir", sOsDir, TEMP_OS_MOUNT_POINT );

}
struct FileCopyResult
{
	FileCopyResult( RString _sFile, RString _sComment ) : sFile(_sFile), sComment(_sComment) {}
	RString sFile, sComment;
};

#if !defined(WITHOUT_NETWORKING)
Preference<RString> g_sCookie( "Cookie", "" );

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
class DownloadTask
{
	enum
	{
		control,
		packages
	} m_DownloadState;
	PlayAfterLaunchInfo m_playAfterLaunchInfo;
public:
	CURL* download;
	CURLM* mHandle;
	int running;
	ProgressData progress;
	string m_TempFileName;
	string status;
	string speed;
	curl_off_t downloadedAtLastUpdate=0;
	curl_off_t lastUpdateDone=0;
	string m_Url;
	RageFile m_TempFile;
	string aux;
	DownloadTask(const RString &url)
	{
		m_Url = url;
		download = curl_easy_init();
		m_TempFileName = MakeTempFileName(url);
		m_TempFile.Open(m_TempFileName, 2);

		curl_easy_setopt(download, CURLOPT_WRITEDATA, &m_TempFile);
		curl_easy_setopt(download, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(download, CURLOPT_URL, url);
		curl_easy_setopt(download, CURLOPT_USERAGENT, "libcurl-agent/1.0");	
		//curl_easy_setopt(download, CURLOPT_PROGRESSDATA, &progress);
		//curl_easy_setopt(download, CURLOPT_PROGRESSFUNCTION, progressfunc);
		curl_easy_setopt(download, CURLOPT_XFERINFODATA, &progress);
		curl_easy_setopt(download, CURLOPT_XFERINFOFUNCTION, progressfunc);
		curl_easy_setopt(download, CURLOPT_NOPROGRESS, 0);
		running = 1;
		lastUpdateDone = 0;
		mHandle = curl_multi_init();
		curl_multi_add_handle(mHandle, download);
		curl_multi_perform(mHandle, &running);
		SCREENMAN->SystemMessage( "Downloading file "+m_TempFileName+" from "+url);
		//m_pTransfer = new FileTransfer();
		//m_pTransfer->StartDownload( sControlFileUri, "" );
		m_DownloadState = control;
	}
	~DownloadTask()
	{
		//SAFE_DELETE(m_pTransfer);
		if(mHandle!=nullptr)
			curl_multi_cleanup(mHandle);
		mHandle = nullptr;
		if (download != nullptr)
			curl_easy_cleanup(download);
		download = nullptr;
		if(m_TempFile.IsOpen())
			m_TempFile.Close();
	}
	RString GetStatus()
	{
		return status;
	}
	bool UpdateAndIsFinished( float fDeltaSeconds, PlayAfterLaunchInfo &playAfterLaunchInfo )
	{
		if (!running)
		{
			if (mHandle != nullptr)
				curl_multi_cleanup(mHandle);
			mHandle = nullptr;
			if (download != nullptr)
				curl_easy_cleanup(download);
			download = nullptr;
			if (m_TempFile.IsOpen())
				m_TempFile.Close();
			//install smzip
			InstallSmzip(m_TempFileName, m_playAfterLaunchInfo);

			auto screen = SCREENMAN->GetScreen(0);
			if (screen && screen->GetName() == "ScreenSelectMusic") {
				static_cast<ScreenSelectMusic*>(screen)->DifferentialReload();
			}
			else
			{
				SONGMAN->DifferentialReload();
			}
			//SONGMAN->Reload(false, NULL);
			Message msg("Download("+m_Url+") Finished");
			MESSAGEMAN->Broadcast(msg);

			playAfterLaunchInfo = m_playAfterLaunchInfo;
			return true;
		}
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
			status = "curl_multi_fdset() failed, code " + mc;
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
			struct timeval wait = { 0, 1 * 1000 }; /* 100ms */
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
			status = "select error" + mc;
			break;
		case 0: /* timeout */
		default: /* action */
			curl_multi_perform(mHandle, &running);
			progress.time += fDeltaSeconds;
			if (progress.time > 1.0) {
				speed = to_string(progress.downloaded/1024 - downloadedAtLastUpdate);
				progress.time = 0;
				downloadedAtLastUpdate = progress.downloaded/1024;
			}
			status = m_TempFileName+"\n"+speed+" KB/s\n"+
				"Downloaded "+ to_string(progress.downloaded/1024) +"/"+to_string(progress.total / 1024)+" (KB)";
			
			break;
		}
		return false;

	}

	static RString MakeTempFileName( RString s )
	{
		return SpecialFiles::CACHE_DIR + "Downloads/" + Basename(s);
	}
};
static vector<DownloadTask*> g_pDownloadTasks;
#endif

static bool IsStepManiaProtocol(const RString &arg)
{
	// for now, only load from the StepMania domain until the security implications of this feature are better understood.
	//return BeginsWith(arg,"stepmania://beta.stepmania.com/");
	return BeginsWith(arg,"http://");
}

static bool IsPackageFile(const RString &arg)
{
	RString ext = GetExtension(arg);
	return ext.EqualsNoCase("smzip") || ext.EqualsNoCase("zip");
}

PlayAfterLaunchInfo DoInstalls( CommandLineActions::CommandLineArgs args )
{
	PlayAfterLaunchInfo ret;
	bool reload = false;
	for( int i = 0; i<(int)args.argv.size(); i++ )
	{
		RString s = args.argv[i];
		if( IsStepManiaProtocol(s) )
    {

			curl_global_init(CURL_GLOBAL_ALL);
#if !defined(WITHOUT_NETWORKING)
			g_pDownloadTasks.push_back( new DownloadTask(s) );
#else
      // TODO: Figure out a meaningful log message.
#endif
    }
		else if (IsPackageFile(s)) {
			InstallSmzipOsArg(s, ret);
			reload = true;
		}
	}
	if (reload) {
		auto screen = SCREENMAN->GetScreen(0);
		if (screen && screen->GetName() == "ScreenSelectMusic") {
			static_cast<ScreenSelectMusic*>(screen)->DifferentialReload();
		}
		else
		{
			SONGMAN->DifferentialReload();
		}
	}
	return ret;
}

REGISTER_SCREEN_CLASS( ScreenInstallOverlay );

ScreenInstallOverlay::~ScreenInstallOverlay()
= default;
void ScreenInstallOverlay::Init()
{
	Screen::Init();

	m_textStatus.LoadFromFont( THEME->GetPathF("ScreenInstallOverlay", "status") );
	m_textStatus.SetName("Status");
	ActorUtil::LoadAllCommandsAndSetXY(m_textStatus,"ScreenInstallOverlay");
	this->AddChild( &m_textStatus );
}

bool ScreenInstallOverlay::Input( const InputEventPlus &input )
{
	/*
	if( input.DeviceI.button == g_buttonLogin && input.type == IET_FIRST_PRESS )
	{
		HOOKS->GoToURL("http://www.stepmania.com/launch.php");
		return true;
	}
	*/

	return Screen::Input(input);
}

void ScreenInstallOverlay::Update( float fDeltaTime )
{
	Screen::Update(fDeltaTime);
	PlayAfterLaunchInfo playAfterLaunchInfo;
	while( CommandLineActions::ToProcess.size() > 0 )
	{
		CommandLineActions::CommandLineArgs args = CommandLineActions::ToProcess.back();
		CommandLineActions::ToProcess.pop_back();
 		PlayAfterLaunchInfo pali2 = DoInstalls( args );
	}
#if !defined(WITHOUT_NETWORKING)
	for(int i=g_pDownloadTasks.size()-1; i>=0; --i)
	{
		DownloadTask *p = g_pDownloadTasks[i];
		PlayAfterLaunchInfo pali;
		if( p->UpdateAndIsFinished( fDeltaTime, pali) )
		{
			playAfterLaunchInfo.OverlayWith(pali);
			SAFE_DELETE(p);
			g_pDownloadTasks.erase( g_pDownloadTasks.begin()+i );
		}
	}

	{
		vector<RString> vsMessages;
		FOREACH_CONST( DownloadTask*, g_pDownloadTasks, pDT )
		{
			vsMessages.push_back( (*pDT)->GetStatus() );
		}
		m_textStatus.SetText( join("\n", vsMessages) );
	}
#endif
}

/*
 * (c) 2001-2005 Chris Danford, Glenn Maynard
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
