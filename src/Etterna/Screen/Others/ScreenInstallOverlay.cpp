#include "Etterna/Globals/global.h"
#include "ScreenInstallOverlay.h"
#include "RageUtil/File/RageFileManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Screen/Others/ScreenSelectMusic.h"
#include "Etterna/Models/NoteWriters/NotesWriterSSC.h"
#include "Etterna/Singletons/CommandLineActions.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Singletons/DownloadManager.h"

#include <iostream>
#include <iterator>
#include <vector>
#include <fstream>

class Song;
const std::string TEMP_OS_MOUNT_POINT = "/@temp-os/";

struct FileCopyResult
{
	FileCopyResult(std::string _sFile, std::string _sComment)
	  : sFile(_sFile)
	  , sComment(_sComment)
	{
	}
	std::string sFile, sComment;
};

void
InstallSmzipOsArg(const string& sOsZipFile)
{
	SCREENMAN->SystemMessage("Installing " + sOsZipFile);

	std::string sOsDir, sFilename, sExt;
	splitpath(sOsZipFile, sOsDir, sFilename, sExt);

	if (!FILEMAN->Mount("dir", sOsDir, TEMP_OS_MOUNT_POINT))
		FAIL_M("Failed to mount " + sOsDir);
	DLMAN->InstallSmzip(TEMP_OS_MOUNT_POINT + sFilename + sExt);

	FILEMAN->Unmount("dir", sOsDir, TEMP_OS_MOUNT_POINT);
}
static bool
IsHTTPProtocol(const std::string& arg)
{
	return BeginsWith(arg, "http://") || BeginsWith(arg, "https://");
}

static bool
IsPackageFile(const std::string& arg)
{
	auto ext = GetExtension(arg);
	return EqualsNoCase(ext, "smzip") || EqualsNoCase(ext, "zip");
}

void
EnsureSlashEnding(std::string& path)
{
	if (path.back() != '/' && path.back() != '\\')
		path.append("/");
}

void
DoInstalls(CommandLineActions::CommandLineArgs args)
{
	auto reload = false;
	for (size_t i = 0; i < args.argv.size(); i++) {
		auto s = args.argv[i];
		if (s == "notedataCache") {
			// TODO: Create the directories if they dont exist
			string packFolder = "packbanner/";
			string cdtitleFolder = "cdtitle/";
			string bgFolder = "bg/";
			string bannerFolder = "banner/";

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
				if (!f.Open(path)) {
					RageException::Throw(
					  "ScreenInstallOverlay failed to open \"%s\": %s",
					  path.c_str(),
					  f.GetError().c_str());
				}
				auto p = f.GetPath();
				f.Close();
				std::ofstream dst(imgsOutputPath + packFolder + pack +
									"_packbanner." + GetExtension(path).c_str(),
								  std::ios::binary);
				std::ifstream src(p, std::ios::binary);
				dst << src.rdbuf();
				dst.close();
			}
			for (auto& pSong : SONGMAN->GetAllSongs()) {
				// Fill steps to save
				vector<Steps*> vpStepsToSave;
				for (auto& pSteps : pSong->m_vpSteps) {
					vpStepsToSave.push_back(pSteps);
				}

				for (auto& s : pSong->m_UnknownStyleSteps) {
					vpStepsToSave.push_back(s);
				}

				string songkey;
				for (auto& st : vpStepsToSave) {
					songkey += st->GetChartKey();
				}

				songkey = BinaryToHex(CryptManager::GetSHA1ForString(songkey));

				// Save ssc/sm5 cache file
				{
					// Hideous hack: Save to a tmp file and then copy its
					// contents to the file we want We use ofstream to save
					// files here, im not sure if pathing is compatible And SSC
					// write uses ragefile. So this way we dont have to mess
					// with ssc writer
					std::string tmpOutPutPath = "Cache/tmp.ssc";
					auto sscCacheFilePath = sscOutputPath + songkey + ".ssc";

					NotesWriterSSC::Write(
					  tmpOutPutPath, *pSong, vpStepsToSave, true);

					RageFile f;
					if (!f.Open(tmpOutPutPath)) {
						RageException::Throw(
						  "ScreenInstallOverlay failed to open \"%s\": %s",
						  tmpOutPutPath.c_str(),
						  f.GetError().c_str());
					}
					auto p = f.GetPath();
					f.Close();
					std::ofstream dst(sscCacheFilePath, std::ios::binary);
					std::ifstream src(p, std::ios::binary);
					dst << src.rdbuf();
					dst.close();
				}

				if (pSong->HasBanner()) {
					RageFile f;
					f.Open(pSong->GetBannerPath());
					auto p = f.GetPath();
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
					auto p = f.GetPath();
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
					auto p = f.GetPath();
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
				for (auto& steps : pSong->GetAllSteps()) {
					auto td = steps->GetTimingData();
					NoteData nd;
					steps->GetNoteData(nd);

					nd.LogNonEmptyRows(td);
					auto& nerv = nd.GetNonEmptyRowVector();
					auto& etaner = td->BuildAndGetEtaner(nerv);
					auto& serializednd = nd.SerializeNoteData(etaner);

					auto path = ndOutputPath + steps->GetChartKey() + ".cache";
					std::ofstream FILE(path,
									   std::ios::out | std::ofstream::binary);
					FILE.write((char*)&serializednd[0],
							   serializednd.size() * sizeof(NoteInfo));
					FILE.close();
					vector<NoteInfo> newVector;
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
	return Screen::Input(input);
}

void
ScreenInstallOverlay::Update(float fDeltaTime)
{
	Screen::Update(fDeltaTime);
	while (CommandLineActions::ToProcess.size() > 0) {
		auto args = CommandLineActions::ToProcess.back();
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

		vector<std::string> dls;
		for (auto& dl : DLMAN->downloads) {
			dls.push_back(dl.second->Status());
		}
		msg.SetParam("dlsize", static_cast<int>(DLMAN->downloads.size()));
		msg.SetParam("dlprogress", join("\n", dls));

		if (!DLMAN->DownloadQueue.empty()) {
			vector<std::string> cue;
			for (auto& q : DLMAN->DownloadQueue) {
				cue.push_back(q.first->name);
			}
			msg.SetParam("queuesize",
						 static_cast<int>(DLMAN->DownloadQueue.size()));
			msg.SetParam("queuedpacks", join("\n", cue));
		} else {
			msg.SetParam("queuesize", 0);
			msg.SetParam("queuedpacks", std::string(""));
		}
		MESSAGEMAN->Broadcast(msg);
	}
}
