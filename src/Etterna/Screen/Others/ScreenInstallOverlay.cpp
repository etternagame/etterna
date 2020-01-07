#include "Etterna/Globals/global.h"
#include "ScreenInstallOverlay.h"
#include "RageUtil/File/RageFileManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Models/Misc/Preference.h"
#include "RageUtil/Misc/RageLog.h"
#include "Etterna/Models/Misc/Preference.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "ScreenInstallOverlay.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Globals/SpecialFiles.h"
class Song;
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Screen/Others/ScreenSelectMusic.h"
#include "Etterna/Models/NoteWriters/NotesWriterSSC.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Singletons/CommandLineActions.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Globals/StepMania.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include <fstream>
#include "Etterna/Singletons/DownloadManager.h"

const RString TEMP_OS_MOUNT_POINT = "/@temp-os/";

struct FileCopyResult
{
	FileCopyResult(RString _sFile, RString _sComment)
	  : sFile(_sFile)
	  , sComment(_sComment)
	{
	}
	RString sFile, sComment;
};

void
InstallSmzipOsArg(const std::string& sOsZipFile)
{
	SCREENMAN->SystemMessage("Installing " + sOsZipFile);

	RString sOsDir, sFilename, sExt;
	splitpath(sOsZipFile, sOsDir, sFilename, sExt);

	if (!FILEMAN->Mount("dir", sOsDir, TEMP_OS_MOUNT_POINT))
		FAIL_M("Failed to mount " + sOsDir);
	DLMAN->InstallSmzip(TEMP_OS_MOUNT_POINT + sFilename + sExt);

	FILEMAN->Unmount("dir", sOsDir, TEMP_OS_MOUNT_POINT);
}
static bool
IsHTTPProtocol(const RString& arg)
{
	return BeginsWith(arg, "http://") || BeginsWith(arg, "https://");
}

static bool
IsPackageFile(const RString& arg)
{
	RString ext = GetExtension(arg);
	return ext.EqualsNoCase("smzip") || ext.EqualsNoCase("zip");
}

void
EnsureSlashEnding(RString& path)
{
	if (path.back() != '/' && path.back() != '\\')
		path.append("/");
}

void
DoInstalls(CommandLineActions::CommandLineArgs args)
{
	bool reload = false;
	for (int i = 0; i < (int)args.argv.size(); i++) {
		RString s = args.argv[i];
		if (s == "notedataCache") {
			// TODO: Create the directories if they dont exist
			std::string packFolder = "packbanner/";
			std::string cdtitleFolder = "cdtitle/";
			std::string bgFolder = "bg/";
			std::string bannerFolder = "banner/";

			auto ndOutputPath = args.argv[i + 1]; // notedata
			EnsureSlashEnding(ndOutputPath);
			auto sscOutputPath = args.argv.size() > i + 2
								   ? args.argv[i + 2]
								   : ndOutputPath + "ssc/";
			auto imgsOutputPath = args.argv.size() > i + 3
									? args.argv[i + 3]
									: ndOutputPath + "imgs/";
			EnsureSlashEnding(sscOutputPath);
			EnsureSlashEnding(imgsOutputPath);

			// Save pack banners
			auto packs = SONGMAN->GetSongGroupNames();
			for (auto& pack : packs) {
				auto path = SONGMAN->GetSongGroupBannerPath(pack);
				if (path == "" || !FILEMAN->IsAFile(path))
					continue;
				RageFile f;
				f.Open(path);
				std::string p = f.GetPath();
				f.Close();
				std::ofstream dst(imgsOutputPath + packFolder + pack +
									"_packbanner." + GetExtension(path).c_str(),
								  std::ios::binary);
				std::ifstream src(p, std::ios::binary);
				dst << src.rdbuf();
				dst.close();
			}
			FOREACH_CONST(Song*, SONGMAN->GetAllSongs(), iSong)
			{
				Song* pSong = (*iSong);

				// Fill steps to save
				std::vector<Steps*> vpStepsToSave;
				FOREACH_CONST(Steps*, pSong->m_vpSteps, s)
				{
					Steps* pSteps = *s;

					// Only save steps that weren't loaded from a profile.
					if (pSteps->WasLoadedFromProfile())
						continue;
					vpStepsToSave.push_back(pSteps);
				}
				FOREACH_CONST(Steps*, pSong->m_UnknownStyleSteps, s)
				{
					vpStepsToSave.push_back(*s);
				}
				std::string songkey;
				for (auto& st : vpStepsToSave)
					songkey += st->GetChartKey();

				songkey = BinaryToHex(CRYPTMAN->GetSHA1ForString(songkey));

				// Save ssc/sm5 cache file
				{
					// Hideous hack: Save to a tmp file and then copy its
					// contents to the file we want We use ofstream to save
					// files here, im not sure if pathing is compatible And SSC
					// write uses ragefile. So this way we dont have to mess
					// with ssc writer
					RString tmpOutPutPath = "Cache/tmp.ssc";
					RString sscCacheFilePath = sscOutputPath + songkey + ".ssc";

					NotesWriterSSC::Write(
					  tmpOutPutPath, *pSong, vpStepsToSave, true);

					RageFile f;
					f.Open(tmpOutPutPath);
					std::string p = f.GetPath();
					f.Close();
					std::ofstream dst(sscCacheFilePath, std::ios::binary);
					std::ifstream src(p, std::ios::binary);
					dst << src.rdbuf();
					dst.close();
				}

				if (pSong->HasBanner()) {
					RageFile f;
					f.Open(pSong->GetBannerPath());
					std::string p = f.GetPath();
					f.Close();
					std::ofstream dst(
					  imgsOutputPath + bannerFolder + songkey + "_banner." +
						GetExtension(pSong->m_sBannerFile).c_str(),
					  std::ios::binary);
					std::ifstream src(p, std::ios::binary);
					dst << src.rdbuf();
					dst.close();
				}

				if (pSong->HasCDTitle()) {
					RageFile f;
					f.Open(pSong->GetCDTitlePath());
					std::string p = f.GetPath();
					f.Close();
					std::ofstream dst(
					  imgsOutputPath + cdtitleFolder + songkey + "_cd." +
						GetExtension(pSong->m_sCDTitleFile).c_str(),
					  std::ios::binary);
					std::ifstream src(p, std::ios::binary);
					dst << src.rdbuf();
					dst.close();
				}

				if (pSong->HasBackground()) {
					RageFile f;
					f.Open(pSong->GetBackgroundPath());
					std::string p = f.GetPath();
					f.Close();
					std::ofstream dst(
					  imgsOutputPath + bgFolder + songkey + "_bg." +
						GetExtension(pSong->m_sBackgroundFile).c_str(),
					  std::ios::binary);
					std::ifstream src(p, std::ios::binary);
					dst << src.rdbuf();
					dst.close();
				}

				// Save notedata
				FOREACH_CONST(Steps*, pSong->GetAllSteps(), iSteps)
				{
					Steps* steps = (*iSteps);
					TimingData* td = steps->GetTimingData();
					NoteData nd;
					steps->GetNoteData(nd);

					nd.LogNonEmptyRows();
					auto& nerv = nd.GetNonEmptyRowVector();
					auto& etaner = td->BuildAndGetEtaner(nerv);
					auto& serializednd = nd.SerializeNoteData(etaner);

					std::string path =
						ndOutputPath + steps->GetChartKey() + ".cache";
					std::ofstream FILE(path, std::ios::out | std::ofstream::binary);
					FILE.write((char*)&serializednd[0],
							   serializednd.size() * sizeof(NoteInfo));
					FILE.close();
					std::vector<NoteInfo> newVector;
					std::ifstream INFILE(path,
										 std::ios::in | std::ifstream::binary);
					INFILE.seekg(0, std::ios::end);
					newVector.resize(u_int(INFILE.tellg() / sizeof(NoteInfo)));
					INFILE.seekg(0, std::ios::beg);
					INFILE.read((char*)&newVector[0],
								newVector.capacity() * sizeof(NoteData));
					INFILE.close();

					td->UnsetEtaner();
					nd.UnsetNerv();
					nd.UnsetSerializedNoteData();
					steps->Compress();
				}
			}
		}
		if (IsHTTPProtocol(s)) {
			DLMAN->DownloadAndInstallPack(s);
		} else if (IsPackageFile(s)) {
			InstallSmzipOsArg(s);
			reload = true;
		}
	}
	if (reload) {
		auto screen = SCREENMAN->GetScreen(0);
		if (screen && screen->GetName() == "ScreenSelectMusic") {
			static_cast<ScreenSelectMusic*>(screen)->DifferentialReload();
		} else {
			SCREENMAN->SetNewScreen("ScreenReloadSongs");
		}
	}
	return;
}

REGISTER_SCREEN_CLASS(ScreenInstallOverlay);

ScreenInstallOverlay::~ScreenInstallOverlay() = default;
void
ScreenInstallOverlay::Init()
{
	Screen::Init();

	m_textStatus.LoadFromFont(
	  THEME->GetPathF("ScreenInstallOverlay", "status"));
	m_textStatus.SetName("Status");
	LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_textStatus);
	this->AddChild(&m_textStatus);
}

bool
ScreenInstallOverlay::Input(const InputEventPlus& input)
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

void
ScreenInstallOverlay::Update(float fDeltaTime)
{
	Screen::Update(fDeltaTime);
	while (CommandLineActions::ToProcess.size() > 0) {
		CommandLineActions::CommandLineArgs args =
		  CommandLineActions::ToProcess.back();
		CommandLineActions::ToProcess.pop_back();
		DoInstalls(args);
	}

	if (!DLMAN->gameplay) {
		static float lastDLProgressUpdate = 0;
		lastDLProgressUpdate += fDeltaTime;
		if (DLMAN->downloads.empty() || lastDLProgressUpdate < 0.5)
			return;
		lastDLProgressUpdate = 0;
		Message msg("DLProgressAndQueueUpdate");

		std::vector<RString> dls;
		for (auto& dl : DLMAN->downloads) {
			dls.push_back(dl.second->Status());
		}
		msg.SetParam("dlsize", static_cast<int>(DLMAN->downloads.size()));
		msg.SetParam("dlprogress", join("\n", dls));

		if (!DLMAN->DownloadQueue.empty()) {
			std::vector<RString> cue;
			for (auto& q : DLMAN->DownloadQueue) {
				cue.push_back(q.first->name);
			}
			msg.SetParam("queuesize",
						 static_cast<int>(DLMAN->DownloadQueue.size()));
			msg.SetParam("queuedpacks", join("\n", cue));
		} else {
			msg.SetParam("queuesize", 0);
			msg.SetParam("queuedpacks", RString(""));
		}
		MESSAGEMAN->Broadcast(msg);
	}
}
