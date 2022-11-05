#include "Etterna/Globals/global.h"
#include "Replay.h"
#include "HighScore.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/NoteData/NoteDataUtil.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/Misc/PlayerOptions.h"

// Singletons
#include "RageUtil/File/RageFileManager.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/ReplayManager.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Singletons/SongManager.h"

// STL
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <utility>

// for replay compression
// why does this have to be complicated
#ifdef _WIN32
#include "zlib.h"
#if defined(_MSC_VER)
#if defined(BINARY_ZDL)
#pragma comment(lib, "zdll.lib")
#endif
#endif
#elif defined(__APPLE__)
#include "zlib.h"
#else
#include <zlib.h>
#endif

static void
MaintainReplayDirectory()
{
	if (!FILEMAN->IsADirectory(FULL_REPLAY_DIR)) {
		FILEMAN->CreateDir(FULL_REPLAY_DIR);
	}
}

static void
MaintainInputDirectory()
{
	if (!FILEMAN->IsADirectory(INPUT_DATA_DIR)) {
		FILEMAN->CreateDir(INPUT_DATA_DIR);
	}
}

Replay::Replay() {

}

Replay::Replay(HighScore* hs)
  : scoreKey(hs->GetScoreKey())
  , chartKey(hs->GetChartKey())
  , fMusicRate(hs->GetMusicRate())
  , fSongOffset(hs->GetSongOffset())
  , rngSeed(hs->GetStageSeed())
{
	// dont set mods here because it is slow.
	// load from disk or when highscore is saving
}

Replay::~Replay() {
	Unload();
}

auto
Replay::HasReplayData() -> bool
{
	return DoesFileExist(GetFullPath()) || DoesFileExist(GetBasicPath());
}

auto
Replay::GetHighScore() -> HighScore*
{
	HighScore* o = nullptr;

	// for local scores
	auto scoresByKey = SCOREMAN->GetScoresByKey();
	if (!scoresByKey.empty()) {
		auto it = scoresByKey.find(scoreKey);
		if (it != scoresByKey.end()) {
			o = it->second;
		}
	}

	// for online scores
	if (o == nullptr) {
		if (DLMAN->chartLeaderboards.count(chartKey) != 0u) {
			auto& leaderboard = DLMAN->chartLeaderboards.at(chartKey);
			auto it = find_if(
			  leaderboard.begin(), leaderboard.end(), [this](OnlineScore& hs) {
				  return hs.hs.GetScoreKey() == scoreKey;
			  });
			if (it != leaderboard.end()) {
				o = &it->hs;
			}
		}
	}

	return o;
}

auto
Replay::GetSteps() -> Steps*
{
	return SONGMAN->GetStepsByChartkey(chartKey);
}

auto
Replay::GetStyle() -> const Style*
{
	auto* steps = GetSteps();
	if (steps == nullptr) {
		return nullptr;
	}
	auto st = steps->m_StepsType;
	return GAMEMAN->GetStyleForStepsType(st);
}

auto
Replay::GetNoteData(Steps* pSteps, bool bTransform) -> NoteData
{
	if (pSteps == nullptr) {
		pSteps = GetSteps();	
	}
	if (pSteps != nullptr) {
		auto noteData = pSteps->GetNoteData();

		if (bTransform) {
			auto* style = GetStyle();
			auto* td = GetTimingData();

			// transform the notedata by style if necessary
			if (style != nullptr) {
				NoteData ndo;
				style->GetTransformedNoteDataForStyle(PLAYER_1, noteData, ndo);
				noteData = ndo;
			}

			// Have to account for mirror and shuffle
			if (style != nullptr && td != nullptr) {
				PlayerOptions po;
				po.Init();
				po.SetForReplay(true);
				po.FromString(mods);
				auto tmpSeed = GAMESTATE->m_iStageSeed;

				// if rng was not saved, only apply non shuffle mods
				if (rngSeed == 0) {
					po.m_bTurns[PlayerOptions::TURN_SHUFFLE] = false;
					po.m_bTurns[PlayerOptions::TURN_SOFT_SHUFFLE] = false;
					po.m_bTurns[PlayerOptions::TURN_SUPER_SHUFFLE] = false;
					po.m_bTurns[PlayerOptions::TURN_HRAN_SHUFFLE] = false;
				} else {
					GAMESTATE->m_iStageSeed = rngSeed;
				}

				NoteDataUtil::TransformNoteData(
				  noteData, *td, po, style->m_StepsType);
				GAMESTATE->m_iStageSeed = tmpSeed;
			}
		}

		return noteData;
	}

	// empty notedata
	NoteData tmp;
	return tmp;
}

auto
Replay::GetTimingData() -> TimingData*
{
	auto* steps = GetSteps();
	if (steps == nullptr) {
		return nullptr;
	}
	return steps->GetTimingData();
}

auto
Replay::SetHighScoreMods() -> void
{
	auto* hs = GetHighScore();
	if (hs != nullptr) {
		auto ms = hs->GetModifiers();
		ms.erase(
		  std::remove_if(ms.begin(),
						 ms.end(),
						 [](unsigned char x) { return std::isspace(x); }),
		  ms.end());
		mods = ms;
	} else {
		mods = NO_MODS;
	}
}

auto
Replay::GetReplaySnapshotForNoterow(int row) -> std::shared_ptr<ReplaySnapshot>
{
	if (m_ReplaySnapshotMap.empty()) {
		return std::shared_ptr<ReplaySnapshot>{ new ReplaySnapshot };
	}

	// The row doesn't necessarily have to exist in the Snapshot map.
	// Because after a Snapshot, we can try this again for a later row
	// And if there are no new snapshots (no events) nothing changes
	// So we return that earlier snapshot.

	// If the lowest value in the map is above the given row, return an empty
	// snapshot
	if (m_ReplaySnapshotMap.begin()->first > row) {
		return std::shared_ptr<ReplaySnapshot>{ new ReplaySnapshot };
	}

	// For some reason I don't feel like figuring out, if the largest value in
	// the map is below the given row, it returns 0 So we need to return the
	// last value
	if (m_ReplaySnapshotMap.rbegin()->first < row) {
		return std::shared_ptr<ReplaySnapshot>{
			&m_ReplaySnapshotMap.rbegin()->second, [](ReplaySnapshot*) {}
		};
	}

	// Otherwise just go ahead and return what we want
	auto lb = m_ReplaySnapshotMap.lower_bound(row);
	auto foundRow = lb->first;

	// lower_bound gets an iter to the next element >= the given key
	// and we have to decrement if it is greater than the key (because we want
	// that)
	if (foundRow > row) {
		if (lb != m_ReplaySnapshotMap.begin()) {
			--lb;
			foundRow = lb->first;
		} else {
			return std::shared_ptr<ReplaySnapshot>{ new ReplaySnapshot };
		}
	}
	return std::shared_ptr<ReplaySnapshot>{ &m_ReplaySnapshotMap[foundRow],
											[](ReplaySnapshot*) {} };
}

auto
Replay::LoadReplayData() -> bool
{
	return LoadReplayDataFull() || LoadReplayDataBasic();
}

auto
Replay::WriteReplayData() -> bool
{
	Locator::getLogger()->info("Writing out replay data to disk");
	std::string append;
	MaintainReplayDirectory();

	if (vNoteRowVector.empty()) {
		Locator::getLogger()->warn(
		  "Failed to write replay for {} - No data to write", scoreKey);
		return false;
	}

	const auto path = FULL_REPLAY_DIR + scoreKey;

	std::ofstream fileStream(path, std::ios::binary);
	if (!fileStream) {
		Locator::getLogger()->warn("Failed to create replay file at {}",
								   path.c_str());
		return false;
	}

	try {
		// output:
		// n n n	- noterow, offset, column
		// or
		// n n n n	- noterow, offset, column, special tap type
		const unsigned int sz = vNoteRowVector.size() - 1;
		for (unsigned int i = 0; i <= sz; i++) {
			append = std::to_string(vNoteRowVector.at(i)) + " " +
					 std::to_string(vOffsetVector.at(i)) + " " +
					 std::to_string(vTrackVector.at(i)) +
					 (vTapNoteTypeVector.at(i) != TapNoteType_Tap
						? " " + std::to_string(vTapNoteTypeVector.at(i))
						: "") +
					 "\n";
			fileStream.write(append.c_str(), append.size());
		}
		// output:
		// H n n	- noterow, column
		// or
		// H n n n	- noterow, column, hold type (roll)
		for (auto& hold : vHoldReplayDataVector) {
			append = "H " + std::to_string(hold.row) + " " +
					 std::to_string(hold.track) +
					 (hold.subType != TapNoteSubType_Hold
						? " " + std::to_string(hold.subType)
						: "") +
					 "\n";
			fileStream.write(append.c_str(), append.size());
		}
		fileStream.close();
	} catch (std::runtime_error& e) {
		Locator::getLogger()->warn(
		  "Failed to write replay data at {} due to runtime exception: {}",
		  path.c_str(),
		  e.what());
		fileStream.close();
		return false;
	}

	Locator::getLogger()->info("Created replay file at {}", path.c_str());
	return true;
}

auto
Replay::WriteInputData() -> bool
{
	Locator::getLogger()->info("Writing out input data to disk");
	std::string append;
	MaintainInputDirectory();

	if (InputData.empty()) {
		Locator::getLogger()->warn(
		  "Failed to write input data for {} - No data to write", scoreKey);
		return false;
	}

	if (mods.empty()) {
		SetHighScoreMods();
	}

	const auto path = INPUT_DATA_DIR + scoreKey;
	const auto path_z = path + "z";

	std::ofstream fileStream(path, std::ios::binary);
	if (!fileStream) {
		Locator::getLogger()->warn("Failed to create input data file at {}",
								   path.c_str());
		return false;
	}

	// for writing human readable text
	try {
		// it's bad to get this now, but it isn't saved anywhere else
		float fGlobalOffset = PREFSMAN->m_fGlobalOffsetSeconds.Get();
		// header:
		// chartkey scorekey rate offset globaloffset modstring rngseed
		auto modStr = mods.empty() ? NO_MODS : mods;
		auto headerLine1 =
		  chartKey + " " + scoreKey + " " + std::to_string(fMusicRate) + " " +
		  std::to_string(fSongOffset) + " " + std::to_string(fGlobalOffset) +
		  " " + modStr + " " + std::to_string(rngSeed) + "\n";
		fileStream.write(headerLine1.c_str(), headerLine1.size());

		// input data:
		// column press/lift time nearest_tap tap_offset
		const unsigned int sz = InputData.size() - 1;
		for (auto& data : InputData) {
			auto typestr =
			  data.nearestTapNoteType != TapNoteType_Tap
				? " " + std::to_string(data.nearestTapNoteType) + " "
				: "";
			// it would be unusual if typestr was blank and this wasnt.
			// it can only happen for TapNoteType_HoldHead
			auto subtypestr =
			  data.nearestTapNoteSubType != TapNoteSubType_Invalid
				? std::to_string(data.nearestTapNoteSubType)
				: "";

			append = std::to_string(data.column) + " " +
					 (data.is_press ? "1" : "0") + " " +
					 std::to_string(data.songPositionSeconds) + " " +
					 std::to_string(data.nearestTapNoterow) + " " +
					 std::to_string(data.offsetFromNearest) + typestr +
					 subtypestr + "\n";
			fileStream.write(append.c_str(), append.size());
		}

		// dropped hold data:
		// H n n	- noterow, column
		// or
		// H n n n	- noterow, column, hold type (roll)
		for (auto& hold : vHoldReplayDataVector) {
			append = "H " + std::to_string(hold.row) + " " +
					 std::to_string(hold.track) +
					 (hold.subType != TapNoteSubType_Hold
						? " " + std::to_string(hold.subType)
						: "") +
					 "\n";
			fileStream.write(append.c_str(), append.size());
		}

		// hit mine data:
		// M n n	- noterow, column
		for (auto& mine : vMineReplayDataVector) {
			append = "M " + std::to_string(mine.row) + " " +
					 std::to_string(mine.track) + "\n";
			fileStream.write(append.c_str(), append.size());
		}

		fileStream.close();

		/// compression
		FILE* infile = fopen(path.c_str(), "rb");
		if (infile == nullptr) {
			Locator::getLogger()->warn("Failed to compress new input data "
									   "because {} could not be opened",
									   path.c_str());
			return false;
		}
		gzFile outfile = gzopen(path_z.c_str(), "wb");
		if (outfile == Z_NULL) {
			Locator::getLogger()->warn("Failed to compress new input data "
									   "because {} could not be opened",
									   path_z.c_str());
			fclose(infile);
			return false;
		}

		char buf[128];
		unsigned int num_read = 0;
		unsigned long total_read = 0;
		while ((num_read = fread(buf, 1, sizeof(buf), infile)) > 0) {
			total_read += num_read;
			gzwrite(outfile, buf, num_read);
		}
		fclose(infile);
		gzclose(outfile);
		/////

		Locator::getLogger()->info("Created compressed input data file at {}",
								   path_z.c_str());

		if (FILEMAN->Remove(path))
			Locator::getLogger()->debug("Deleted uncompressed input data");
		else
			Locator::getLogger()->warn(
			  "Failed to delete uncompressed input data");
		return true;
	} catch (std::runtime_error& e) {
		Locator::getLogger()->warn(
		  "Failed to write input data at {} due to runtime exception: {}",
		  path.c_str(),
		  e.what());
		fileStream.close();
		return false;
	}

	// for writing binary output for "compression"
	// write vector size, then dump vector data
	/*
	try {
		size_t size = InputData.size();
		fileStream.write(reinterpret_cast<char*>(&size), sizeof size);
		fileStream.write(reinterpret_cast<char*>(&InputData[0]),
						 size * sizeof InputDataEvent);
		fileStream.close();
		Locator::getLogger()->trace("Created input data file at {}",
									path.c_str());
		return true;
	} catch (std::runtime_error& e) {
		Locator::getLogger()->warn(
		  "Failed to write input data at {} due to runtime exception: {}",
		  path.c_str(),
		  e.what());
		fileStream.close();
		return false;
	}
	*/
}

auto
Replay::LoadInputData(const std::string& replayDir) -> bool
{
	if (!InputData.empty())
		return true;

	const auto path = replayDir + scoreKey;
	const auto path_z = path + "z";
	std::vector<InputDataEvent> readInputs;
	std::vector<HoldReplayResult> vHoldReplayDataVector;
	std::vector<MineReplayResult> vMineReplayDataVector;

	/*
	std::ifstream inputStream(path, std::ios::binary);
	if (!inputStream) {
		Locator::getLogger()->trace("Failed to load input data at {}",
									path.c_str());
		return false;
	}
	*/

	// read vector size, then read vector data
	/*
	try {
		size_t size;
		inputStream.read(reinterpret_cast<char*>(&size), sizeof size);
		m_Impl->InputData.resize(size);
		inputStream.read(reinterpret_cast<char*>(&m_Impl->InputData[0]),
						 size * sizeof InputDataEvent);
		inputStream.close();
		Locator::getLogger()->trace("Loaded input data at {}", path.c_str());
		return true;
	}
	*/
	// human readable compression read-in
	try {
		gzFile infile = gzopen(path_z.c_str(), "rb");
		if (infile == Z_NULL) {
			Locator::getLogger()->warn("Failed to read input data at {}",
									   path_z.c_str());
			return false;
		}

		// hope nothing already exists here
		FILE* outfile = fopen(path.c_str(), "wb");
		if (outfile == nullptr) {
			Locator::getLogger()->warn(
			  "Failed to create tmp output file for input data at {}",
			  path.c_str());
			gzclose(infile);
			return false;
		}

		char buf[128];
		int num_read = 0;
		while ((num_read = gzread(infile, buf, sizeof(buf))) > 0) {
			fwrite(buf, 1, num_read, outfile);
		}
		gzclose(infile);
		fclose(outfile);

		std::ifstream inputStream(path, std::ios::binary);
		if (!inputStream) {
			Locator::getLogger()->debug("Failed to load input data at {}",
										path.c_str());
			return false;
		}

		std::string line;
		std::string buffer;
		std::vector<std::string> tokens;

		{
			// get the header line
			getline(inputStream, line);
			std::stringstream ss(line);
			while (ss >> buffer) {
				tokens.emplace_back(buffer);
			}
			if (tokens.size() != 5 && tokens.size() != 7) {
				Locator::getLogger()->warn("Bad input data header detected: {}",
										   path_z.c_str());
				return false;
			}

			this->chartKey = tokens[0];
			this->scoreKey = tokens[1];
			this->fMusicRate = std::stof(tokens[2]);
			this->fSongOffset = std::stof(tokens[3]);
			this->fGlobalOffset = std::stof(tokens[4]);

			if (tokens.size() == 7) {
				this->mods = tokens[5];
				this->rngSeed = std::stol(tokens[6]);
			}

			tokens.clear();
		}

		while (getline(inputStream, line)) {
			InputDataEvent ev;
			std::stringstream ss(line);
			// split line into tokens
			while (ss >> buffer) {
				tokens.emplace_back(buffer);
			}

			// hold data
			if (tokens[0] == "H") {
				HoldReplayResult hrr;
				hrr.row = std::stoi(tokens[1]);
				hrr.track = std::stoi(tokens[2]);
				auto tmp = 0;
				tmp = tokens.size() > 3 ? std::stoi(tokens[3])
										: TapNoteSubType_Hold;
				if (tmp < 0 || tmp >= NUM_TapNoteSubType ||
					!(typeid(tmp) == typeid(int))) {
					Locator::getLogger()->warn(
					  "Failed to load replay data at {} (\"Tapnotesubtype "
					  "value is not of type TapNoteSubType\")",
					  path.c_str());
				}
				hrr.subType = static_cast<TapNoteSubType>(tmp);
				vHoldReplayDataVector.emplace_back(hrr);
				tokens.clear();
				continue;
			}

			// mine data
			if (tokens[0] == "M") {
				MineReplayResult mrr;
				mrr.row = std::stoi(tokens[1]);
				mrr.track = std::stoi(tokens[2]);
				vMineReplayDataVector.emplace_back(mrr);
				tokens.clear();
				continue;
			}

			// everything else is input data
			if (tokens.size() < 5 || tokens.size() > 7) {
				Locator::getLogger()->warn(
				  "Bad input data detected: {} - Tokens size {}",
				  GetScoreKey().c_str(),
				  tokens.size());
				return false;
			}

			ev.column = std::stoi(tokens[0]);
			ev.is_press = (std::stoi(tokens[1]) != 0);
			ev.songPositionSeconds = std::stof(tokens[2]);
			ev.nearestTapNoterow = std::stoi(tokens[3]);
			ev.offsetFromNearest = std::stof(tokens[4]);

			// type data is present
			if (tokens.size() >= 6) {
				ev.nearestTapNoteType =
				  static_cast<TapNoteType>(std::stoi(tokens[5]));
			}
			if (tokens.size() >= 7) {
				ev.nearestTapNoteSubType =
				  static_cast<TapNoteSubType>(std::stoi(tokens[6]));
			}

			readInputs.push_back(ev);

			tokens.clear();
		}

		SetMineReplayDataVector(vMineReplayDataVector);
		SetHoldReplayDataVector(vHoldReplayDataVector);
		SetInputDataVector(readInputs);

		Locator::getLogger()->info("Loaded input data at {}", path.c_str());

		inputStream.close();
		if (FILEMAN->Remove(path))
			Locator::getLogger()->trace("Deleted uncompressed input data");
		else
			Locator::getLogger()->warn(
			  "Failed to delete uncompressed input data");
	} catch (std::runtime_error& e) {
		Locator::getLogger()->warn(
		  "Failed to read input data at {} due to runtime exception: {}",
		  path.c_str(),
		  e.what());
		return false;
	}
	return true;
}

auto
Replay::LoadReplayDataBasic(const std::string& replayDir) -> bool
{
	// already exists
	if (vNoteRowVector.size() > 4 && vOffsetVector.size() > 4) {
		return true;
	}

	std::string profiledir;
	std::vector<int> vNoteRowVector;
	std::vector<float> vOffsetVector;
	const auto path = replayDir + scoreKey;

	std::ifstream fileStream(path, std::ios::binary);
	std::string line;
	std::string buffer;
	std::vector<std::string> tokens;
	int noteRow = 0;
	float offset = 0.f;

	// check file
	if (!fileStream) {
		Locator::getLogger()->warn("Failed to load replay data at {}",
								   path.c_str());
		return false;
	}

	// loop until eof
	try {

		while (getline(fileStream, line)) {
			std::stringstream ss(line);
			// split line into tokens
			while (ss >> buffer) {
				tokens.emplace_back(buffer);
			}

			if (tokens.size() > 2) {
				Locator::getLogger()->warn(
				  "looks like u got v2 replays in the v1 folder, move them "
				  "into Save/ReplaysV2 folder if you want them to load. If {} "
				  "is not a v2 replay that you placed into the Save/Replays "
				  "folder by accident, then it is probably corrupted and you "
				  "should delete it or move it out",
				  GetScoreKey().c_str());
				ASSERT(tokens.size() < 2);
			}

			noteRow = std::stoi(tokens[0]);
			if (!(typeid(noteRow) == typeid(int))) {
				throw std::runtime_error("NoteRow value is not of type: int");
			}
			vNoteRowVector.emplace_back(noteRow);

			offset = std::stof(tokens[1]);
			if (!(typeid(offset) == typeid(float))) {
				throw std::runtime_error("Offset value is not of type: float");
			}
			vOffsetVector.emplace_back(offset);
			tokens.clear();
		}
	} catch (std::runtime_error& e) {
		Locator::getLogger()->warn(
		  "Failed to load replay data at {} due to runtime exception: {}",
		  path.c_str(),
		  e.what());
		fileStream.close();
		return false;
	}
	fileStream.close();
	SetNoteRowVector(vNoteRowVector);
	SetOffsetVector(vOffsetVector);

	Locator::getLogger()->info("Loaded replay data type 1 at {}", path.c_str());
	return true;
}

auto
Replay::LoadReplayDataFull(const std::string& replayDir) -> bool
{
	if (vNoteRowVector.size() > 4 && vOffsetVector.size() > 4 &&
		vTrackVector.size() > 4) {
		return true;
	}

	std::string profiledir;
	std::vector<int> vNoteRowVector;
	std::vector<float> vOffsetVector;
	std::vector<int> vTrackVector;
	std::vector<TapNoteType> vTapNoteTypeVector;
	std::vector<HoldReplayResult> vHoldReplayDataVector;
	const auto path = replayDir + scoreKey;

	std::ifstream fileStream(path, std::ios::binary);
	std::string line;
	std::string buffer;
	std::vector<std::string> tokens;
	int noteRow = 0;
	float offset = 0.f;
	int track = 0;
	TapNoteType tnt = TapNoteType_Invalid;
	int tmp = 0;

	// check file
	if (!fileStream) {
		return false;
	}

	// loop until eof
	while (getline(fileStream, line)) {
		std::stringstream ss(line);
		// split line into tokens
		while (ss >> buffer) {
			tokens.emplace_back(buffer);
		}

		// probably replaydatav1 in the wrong folder, we could throw a trace or
		// a warn but i feel like nobody will care or do anything about it and
		// it will just pollute the log, nobody is going to parse the log and
		// properly split up their replays back into the respective folders
		// so...
		if (tokens.size() < 3) {
			return LoadReplayDataBasic(replayDir);
		}

		if (tokens[0] == "H") {
			HoldReplayResult hrr;
			hrr.row = std::stoi(tokens[1]);
			hrr.track = std::stoi(tokens[2]);
			tmp =
			  tokens.size() > 3 ? std::stoi(tokens[3]) : TapNoteSubType_Hold;
			if (tmp < 0 || tmp >= NUM_TapNoteSubType ||
				!(typeid(tmp) == typeid(int))) {
				Locator::getLogger()->warn(
				  "Failed to load replay data at {} (\"Tapnotesubtype value is "
				  "not of type TapNoteSubType\")",
				  path.c_str());
			}
			hrr.subType = static_cast<TapNoteSubType>(tmp);
			vHoldReplayDataVector.emplace_back(hrr);
			tokens.clear();
			continue;
		}

		auto a = buffer == "1";
		a = buffer == "2" || a;
		a = buffer == "3" || a;
		a = buffer == "4" || a;
		a = buffer == "5" || a;
		a = buffer == "6" || a;
		a = buffer == "7" || a;
		a = buffer == "8" || a;
		a = buffer == "9" || a;
		a = buffer == "0" || a;
		if (!a) {
			Locator::getLogger()->warn(
			  "Replay data at {} appears to be HOT BROKEN GARBAGE WTF",
			  path.c_str());
			return false;
		}

		noteRow = std::stoi(tokens[0]);
		if (!(typeid(noteRow) == typeid(int))) {
			Locator::getLogger()->warn(
			  "Failed to load replay data at {} (\"NoteRow value is "
			  "not of type: int\")",
			  path.c_str());
		}
		vNoteRowVector.emplace_back(noteRow);

		offset = std::stof(tokens[1]);
		if (!(typeid(offset) == typeid(float))) {
			Locator::getLogger()->warn(
			  "Failed to load replay data at {} (\"Offset value is not "
			  "of type: float\")",
			  path.c_str());
		}
		vOffsetVector.emplace_back(offset);

		track = std::stoi(tokens[2]);
		if (!(typeid(track) == typeid(int))) {
			Locator::getLogger()->warn(
			  "Failed to load replay data at {} (\"Track/Column value "
			  "is not of type: int\")",
			  path.c_str());
		}
		vTrackVector.emplace_back(track);

		tmp = tokens.size() >= 4 ? std::stoi(tokens[3]) : TapNoteType_Tap;
		if (tmp < 0 || tmp >= TapNoteType_Invalid ||
			!(typeid(tmp) == typeid(int))) {
			Locator::getLogger()->warn(
			  "Failed to load replay data at {} (\"Tapnotetype value "
			  "is not of type TapNoteType\")",
			  path.c_str());
		}
		tnt = static_cast<TapNoteType>(tmp);
		vTapNoteTypeVector.emplace_back(tnt);

		tokens.clear();
	}
	fileStream.close();
	SetNoteRowVector(vNoteRowVector);
	SetOffsetVector(vOffsetVector);
	SetTrackVector(vTrackVector);
	SetTapNoteTypeVector(vTapNoteTypeVector);
	SetHoldReplayDataVector(vHoldReplayDataVector);

	Locator::getLogger()->info("Loaded replay data type 2 at {}", path.c_str());
	return true;
}

auto
Replay::FillInBlanksForInputData() -> bool
{
	if (!LoadInputData()) {
		Locator::getLogger()->warn("Failed to correct InputData fields for "
								   "score {} because InputData was missing",
								   scoreKey);
		return false;
	}

	for (auto& d : InputData) {
		if (d.nearestTapNoteType != TapNoteType_Invalid) {
			// early exit because the data is already filled in
			return true;
		}
	}

	if (chartKey.empty()) {
		Locator::getLogger()->warn("Failed to correct InputData fields for "
								   "score {} because chartkey was blank",
								   scoreKey);
		return false;
	}

	auto chart = GetSteps();
	if (chart == nullptr) {
		Locator::getLogger()->warn("Failed to correct InputData fields for "
								   "score {} because chartkey {} is missing",
								   scoreKey,
								   chartKey);
		return false;
	}

	auto notedata = GetNoteData(chart);
	if (notedata.IsEmpty()) {
		Locator::getLogger()->warn("Failed to correct InputData fields for "
								   "score {} because chartkey [} is empty",
								   scoreKey,
								   chartKey);
		return false;
	}

	if (mods.empty()) {
		SetHighScoreMods();
	}

	for (auto& d : InputData) {
		auto& row = d.nearestTapNoterow;
		auto& col = d.column;

		const auto& tn = notedata.GetTapNote(col, row);
		if (tn == TAP_EMPTY) {
			// usually row -1 produces this
			// row -1 is the result of tap really far away
			// or a tap nearest to a note that is already judged
			// (releases near to already judged notes also)
			Locator::getLogger()->debug(
			  "WHAT??? row {} col {} time {}", row, col, d.songPositionSeconds);
		}
		d.nearestTapNoteType = tn.type;
		d.nearestTapNoteSubType = tn.subType;
	}

	// save changes
	WriteInputData();

	return true;
}

auto
Replay::GenerateReplayV2DataPresumptively() -> bool
{
	if (!LoadReplayDataBasic()) {
		// shouldnt get here
		Locator::getLogger()->warn(
		  "Failed to generate V2 data for V1 replay for score {} because "
		  "it appears "
		  "there is no replay to load in the first place",
		  scoreKey);
		return false;
	}

	auto noteData = GetNoteData();
	if (noteData.IsEmpty()) {
		Locator::getLogger()->warn(
		  "Failed to generate V2 data for V1 replay for score {} because "
		  "the NoteData was empty",
		  scoreKey);
		return false;
	}

	if (vOffsetVector.size() != vNoteRowVector.size()) {
		Locator::getLogger()->warn(
		  "Failed to generate V2 data for V1 replay for score {} because "
		  "existing data was invalid - vector sizes are different {} - {}",
		  scoreKey,
		  vOffsetVector.size(),
		  vNoteRowVector.size());
		return false;
	}


	std::vector<int> vTrackVector;
	std::vector<TapNoteType> vTapNoteTypeVector;
	std::vector<HoldReplayResult> vHoldReplayDataVector;

	std::map<int, std::set<int>> usedRowsAndColumns{};

	const auto numCols = noteData.GetNumTracks();
	for (auto i = 0; i < vOffsetVector.size(); i++) {
		auto& offset = vOffsetVector.at(i);
		auto& row = vNoteRowVector.at(i);

		if (usedRowsAndColumns.count(row) == 0u) {
			std::set<int> v;
			usedRowsAndColumns.emplace(row, v);
		}

		auto& cols = usedRowsAndColumns.at(row);
		bool placed = false;
		for (auto c = 0; !placed && c < numCols; c++) {
			if (cols.count(c)) {
				continue;
			}
			const auto& tn = noteData.GetTapNote(c, row);
			if (tn != TAP_EMPTY) {
				cols.emplace(c);
				vTapNoteTypeVector.emplace_back(tn.type);
				vTrackVector.emplace_back(c);
				placed = true;
			}
		}
		if (!placed) {
			Locator::getLogger()->warn(
			  "Replay for score {} has too many notes for row {} to correctly "
			  "assume information",
			  scoreKey,
			  row);
		}
	}

	SetTrackVector(vTrackVector);
	SetTapNoteTypeVector(vTapNoteTypeVector);
	SetHoldReplayDataVector(vHoldReplayDataVector);

	return true;
}

auto
Replay::GeneratePrimitiveVectors() -> bool
{
	if (LoadReplayDataFull()) {
		// already done
		return true;
	}

	if (LoadReplayDataBasic()) {
		// we have replay data but not column data
		return GenerateReplayV2DataPresumptively();
	}
	
	if (!LoadInputData()) {
		Locator::getLogger()->warn("Failed to generate primitive vectors for "
								   "score {} because input data is not present",
								   scoreKey);
		return false;
	}

	if (!FillInBlanksForInputData()) {
		Locator::getLogger()->warn(
		  "Failed to generate primitive vectors for score {} because input "
		  "data is missing info that could not be recovered",
		  scoreKey);
		return false;
	}

	auto* chart = GetSteps();
	if (chart == nullptr) {
		Locator::getLogger()->warn("Failed to geneate primitive vectors for "
								   "score {} because chartkey {} is missing",
								   scoreKey,
								   chartKey);
		return false;
	}

	auto* td = chart->GetTimingData();

	// input data should be parsed in order of input
	// that means if a judged note is seen twice, ignore the second time
	// (if it is seen twice but not judged the first time, dont ignore)
	std::unordered_map<int, std::set<int>> judgedRowsToCols{};

	std::vector<float> vOffsetVector{};
	std::vector<int> vNoteRowVector{};
	std::vector<int> vTrackVector{};
	std::vector<TapNoteType> vTapNoteTypeVector{};

	for (auto& d : InputData) {
		auto time = d.songPositionSeconds - d.offsetFromNearest;

		auto noterow = BeatToNoteRow(td->GetBeatFromElapsedTime(time));
		auto track = d.column;
		auto tnt = d.nearestTapNoteType;
		auto offset = d.offsetFromNearest;

		bool canJudge = true;

		// dont judge a lift if it isnt a lift
		if (!d.is_press && d.nearestTapNoteType != TapNoteType_Lift) {
			canJudge = false;
		} else if (d.nearestTapNoteType == TapNoteType_Empty ||
				   d.nearestTapNoteType == TapNoteType_Invalid) {
			canJudge = false;
		}

		if (judgedRowsToCols.count(noterow) != 0u) {
			auto& s = judgedRowsToCols.at(noterow);
			if (s.count(track)) {
				// note already judged
				continue;
			} else if (canJudge) {
				s.insert(track);
			}
		}

		if (canJudge) {
			vOffsetVector.emplace_back(offset);
			vNoteRowVector.emplace_back(noterow);
			vTrackVector.emplace_back(track);
			vTapNoteTypeVector.emplace_back(tnt);
		}
	}

	SetOffsetVector(vOffsetVector);
	SetNoteRowVector(vNoteRowVector);
	SetTrackVector(vTrackVector);
	SetTapNoteTypeVector(vTapNoteTypeVector);
	return true;
}

auto
Replay::GenerateNoterowsFromTimestamps() -> bool
{
	if (!vNoteRowVector.empty()) {
		return true;
	}

	if (vOnlineReplayTimestampVector.empty()) {
		Locator::getLogger()->warn(
		  "Failed to generate noterows from timestamps because timestamps for "
		  "score {} were not present",
		  scoreKey);
		return false;
	}

	if (chartKey.empty() || SONGMAN->GetStepsByChartkey(chartKey) == nullptr) {
		Locator::getLogger()->warn(
		  "Failed to generate noterows from timestamps because chart {} is not "
		  "loaded for score {}",
		  chartKey,
		  scoreKey);
		return false;
	}

	auto* chart = SONGMAN->GetStepsByChartkey(chartKey);
	auto* td = chart->GetTimingData();

	// bad
	GAMESTATE->SetProcessedTimingData(td);

	auto nd = chart->GetNoteData();
	// non empty noterows
	auto nerv = nd.BuildAndGetNerv(td);
	// estimated time all non empty noterows
	auto etaner = td->BuildAndGetEtaner(nerv);

	if (nerv.empty() || etaner.empty()) {
		Locator::getLogger()->warn(
		  "Failed to generate noterows from timestamps because failed to "
		  "generate nerv or etaner for chart {}, score {}",
		  chartKey,
		  scoreKey);
		GAMESTATE->SetProcessedTimingData(nullptr);
		return false;
	}

	const auto fileOffsetError =
	  etaner.at(0) - (vOnlineReplayTimestampVector.at(0) * fMusicRate);
	std::vector<int> noterows;
	for (auto& t : vOnlineReplayTimestampVector) {
		const auto beat = td->GetBeatFromElapsedTime(t * fMusicRate) + fileOffsetError;
		const auto noterow = BeatToNoteRow(beat);
		noterows.emplace_back(noterow);
	}
	const auto amountToNormalizeNoterows = nerv.at(0) - noterows.at(0);
	for (auto& noterow : noterows) {
		noterow += amountToNormalizeNoterows;
	}
	SetNoteRowVector(noterows);
	ValidateOffsets();

	// bad 2
	GAMESTATE->SetProcessedTimingData(nullptr);

	return true;
}

void
Replay::ValidateOffsets()
{
	auto offsetIt = vOffsetVector.begin();
	auto noterowIt = vNoteRowVector.begin();
	auto trackIt = vTrackVector.begin();
	auto tntIt = vTapNoteTypeVector.begin();

	auto moveIts = [this, &offsetIt, &noterowIt, &trackIt, &tntIt]() {
		if (offsetIt != vOffsetVector.end())
			offsetIt++;
		if (noterowIt != vNoteRowVector.end())
			noterowIt++;
		if (trackIt != vTrackVector.end())
			trackIt++;
		if (tntIt != vTapNoteTypeVector.end())
			tntIt++;
	};
	auto removeIts = [this, &offsetIt, &noterowIt, &trackIt, &tntIt](){
		if (offsetIt != vOffsetVector.end())
			vOffsetVector.erase(offsetIt);
		if (noterowIt != vNoteRowVector.end())
			vNoteRowVector.erase(noterowIt);
		if (trackIt != vTrackVector.end())
			vTrackVector.erase(trackIt);
		if (tntIt != vTapNoteTypeVector.end())
			vTapNoteTypeVector.erase(tntIt);
	};

	// the point here is to erase offsets and the associated data
	// in parallel ... but sometimes the vectors are empty
	// they should NEVER be different length, but CAN be empty instead
	while (offsetIt != vOffsetVector.end()) {
		if (fabs(*offsetIt) >= MISS_WINDOW_BEGIN_SEC) {
			removeIts();
		} else {
			moveIts();
		}
	}
}

auto
Replay::GenerateInputData() -> bool
{
	if (LoadInputData()) {
		return true;
	}

	if (!LoadReplayData() && !GenerateNoterowsFromTimestamps()) {
		Locator::getLogger()->warn("Failed to generate input data because "
								   "replay for score {} could not be loaded",
								   scoreKey);
		return false;
	}

	if (chartKey.empty() || SONGMAN->GetStepsByChartkey(chartKey) == nullptr) {
		Locator::getLogger()->warn("Failed to generate input data because "
								   "chart {} is not loaded for score {}",
								   chartKey,
								   scoreKey);
		return false;
	}

	const auto replayType = GetReplayType();
	if (replayType == ReplayType_V2) {
		auto sz = vNoteRowVector.size();
		if (sz != vOffsetVector.size() || sz != vTrackVector.size() ||
			sz != vTapNoteTypeVector.size()) {
			Locator::getLogger()->warn(
			  "Replay V2 vectors not equal. Exited early - Replay {}",
			  scoreKey);
			return false;
		}

		const auto* td = SONGMAN->GetStepsByChartkey(chartKey)->GetTimingData();
		std::vector<InputDataEvent> readInputs;
		for (int i = 0; i < sz; i++) {
			const auto& noterow = vNoteRowVector.at(i);
			const auto& offset = vOffsetVector.at(i);
			if (offset > MISS_WINDOW_BEGIN_SEC) {
				// nah
				continue;
			}

			const auto positionSeconds =
			  td->GetElapsedTimeFromBeat(NoteRowToBeat(noterow)) +
			  offset * fMusicRate;

			InputDataEvent evt;
			evt.column = vTrackVector.at(i);
			evt.is_press =
			  vTapNoteTypeVector.at(i) == TapNoteType_Lift ? false : true;
			evt.nearestTapNoterow = noterow;
			evt.offsetFromNearest = offset;
			evt.songPositionSeconds = positionSeconds;
			
			InputDataEvent lift_evt(evt);
			lift_evt.is_press = !lift_evt.is_press;
			lift_evt.songPositionSeconds =
			  lift_evt.is_press ? lift_evt.songPositionSeconds - 0.001F
								: lift_evt.songPositionSeconds + 0.001F;
			readInputs.push_back(evt);
			readInputs.push_back(lift_evt);
		}
		SetInputDataVector(readInputs);
	} else if (replayType == ReplayType_V1) {
		auto sz = vNoteRowVector.size();
		if (sz != vOffsetVector.size()) {
			Locator::getLogger()->warn(
			  "Replay V1 vectors not equal. Exited early - Replay {}",
			  scoreKey);
			return false;
		}

		const auto* song = SONGMAN->GetStepsByChartkey(chartKey);
		auto nd = song->GetNoteData();
		const auto* td = song->GetTimingData();
		const auto columns = nd.GetNumTracks();
		std::map<int, std::set<int>> assignedColumns{};
		std::vector<InputDataEvent> readInputs;

		for (int i = 0; i < sz; i++) {
			const auto& noterow = vNoteRowVector.at(i);
			const auto& offset = vOffsetVector.at(i);
			TapNoteType tnt = TapNoteType_Invalid;
			auto columnToUse = -1;

			if (offset > MISS_WINDOW_BEGIN_SEC) {
				// nah
				continue;
			}

			for (int j = 0; j < columns; j++) {
				const auto& tn = nd.GetTapNote(j, noterow);
				if (tn == TAP_EMPTY) {
					// column cant be used
					continue;
				}

				if (assignedColumns.count(noterow) == 0) {
					assignedColumns.emplace(noterow, std::set<int>());
				}
				if (assignedColumns.at(noterow).count(j) == 0) {
					// column not yet used
					assignedColumns.at(noterow).insert(j);
					columnToUse = j;
					tnt = tn.type;
					break;
				}
			}

			if (columnToUse == -1) {
				Locator::getLogger()->warn("Replay V1 could not find a note to "
										   "use on row {} - Replay {}",
										   noterow,
										   scoreKey);
				return false;
			}

			const auto positionSeconds =
			  td->GetElapsedTimeFromBeat(NoteRowToBeat(noterow)) +
			  offset * fMusicRate;

			InputDataEvent evt;
			evt.column = columnToUse;
			evt.is_press = tnt == TapNoteType_Lift ? false : true;
			evt.nearestTapNoterow = noterow;
			evt.offsetFromNearest = offset;
			evt.songPositionSeconds = positionSeconds;

			InputDataEvent lift_evt(evt);
			lift_evt.is_press = !lift_evt.is_press;
			lift_evt.songPositionSeconds =
			  lift_evt.is_press ? lift_evt.songPositionSeconds - 0.001F
								: lift_evt.songPositionSeconds + 0.001F;
			readInputs.push_back(evt);
			readInputs.push_back(lift_evt);
		}
		SetInputDataVector(readInputs);
	} else {
		Locator::getLogger()->warn("not implemented for replay type {}",
								   ReplayTypeToString(GetReplayType()));
		return false;
	}

	return true;
}

auto
Replay::GeneratePlaybackEvents() -> std::map<int, std::vector<PlaybackEvent>>
{
	std::map<int, std::vector<PlaybackEvent>> out;

	// make sure we have input data loaded
	if (!GenerateInputData()) {
		Locator::getLogger()->warn(
		  "Failed to generate playback events because replay for score {} "
		  "could not generate input data",
		  scoreKey);
		return out;
	}

	if (chartKey.empty() || SONGMAN->GetStepsByChartkey(chartKey) == nullptr) {
		Locator::getLogger()->warn("Failed to generate playback events because "
								   "chart {} is not loaded for score {}",
								   chartKey,
								   scoreKey);
		return out;
	}

	const auto* td = SONGMAN->GetStepsByChartkey(chartKey)->GetTimingData();
	for (const InputDataEvent& evt : InputData) {
		const auto& evtPositionSeconds = evt.songPositionSeconds;
		const auto& column = evt.column;
		const auto& isPress = evt.is_press;
		const auto positionSeconds = evt.songPositionSeconds;

		const auto noterow =
		  BeatToNoteRow(td->GetBeatFromElapsedTimeNoOffset(positionSeconds));
		PlaybackEvent playback(noterow, positionSeconds, column, isPress);
		playback.noterowJudged = evt.nearestTapNoterow;
		if (!out.count(noterow)) {
			out.emplace(noterow, std::vector<PlaybackEvent>());
		}
		out.at(noterow).push_back(playback);
	}

	return out;
}

auto
Replay::GenerateDroppedHoldColumnsToRowsMap() -> std::map<int, std::set<int>>
{
	std::map<int, std::set<int>> mapping;

	for (auto& h : vHoldReplayDataVector) {
		if (mapping.count(h.track) == 0) {
			mapping.emplace(h.track, std::set<int>());
		}
		mapping.at(h.track).insert(h.row);
	}

	return mapping;
}

auto
Replay::GenerateDroppedHoldRowsToColumnsMap() -> std::map<int, std::set<int>>
{
	std::map<int, std::set<int>> mapping;

	for (auto& h : vHoldReplayDataVector) {
		if (mapping.count(h.row) == 0) {
			mapping.emplace(h.row, std::set<int>());
		}
		mapping.at(h.row).insert(h.track);
	}

	return mapping;
}

auto
Replay::GenerateJudgeInfoAndReplaySnapshots(int startingRow, float timingScale) -> bool
{
	{
		// force regenerate...
		JudgeInfo tmp;
		SetJudgeInfo(tmp);
		m_ReplaySnapshotMap.clear();
	}

	if (!GeneratePrimitiveVectors()) {
		Locator::getLogger()->warn(
		  "Failed to generate judgment info for score {} because primitive "
		  "vectors could not be generated",
		  scoreKey);
		return false;
	}

	if (mods.empty()) {
		SetHighScoreMods();
	}

	JudgeInfo& ji = judgeInfo;

	// map of rows to vectors of holdreplayresults
	auto& m_ReplayHoldMap = ji.hrrMap;
	// map of rows to vectors of tapreplayresults
	auto& m_ReplayTapMap = ji.trrMap;

	NoteData noteData = GetNoteData();
	auto* pReplayTiming = GetTimingData();

	// Record valid noterows so that we don't waste a lot of time to do the last
	// step here
	{
		std::set<int> tmp;
		significantNoterows = tmp;
	}
	auto& validNoterows = significantNoterows;

	// Generate TapReplayResults to put into a vector referenced by the song row
	// in a map
	for (size_t i = 0; i < vNoteRowVector.size(); i++) {
		if (fabsf(vOffsetVector.at(i)) > MISS_WINDOW_BEGIN_SEC)
			continue;
		if (vNoteRowVector.at(i) < startingRow)
			continue;

		bool dontMakeNewTRR = false;

		// ReplayData recording allows for multiple taps in 1 row
		// This should only occur for mines that were hit by tapping them
		// Check specifically for this happening
		if (HasColumnData()) {
			// skip out of bounds indices...
			// this happens in some very far edge cases
			// replays can be type 2 without tapnotetype vectors
			// (multiplayer related usually)
			if (i < vNoteRowVector.size() &&
				i < vTapNoteTypeVector.size() &&
				i < vOffsetVector.size() && i < vTrackVector.size()) {

				// if scoring issues continue to happen, finish this
				// right now, only checking for mines
				if (vTapNoteTypeVector.at(i) == TapNoteType_Mine) {
					if (m_ReplayTapMap.count(vNoteRowVector.at(i)) != 0) {
						// search for other mines in this column
						// if a relevant one exists, set it to this offset
						// skip iteration if a match is found
						for (auto& t : m_ReplayTapMap[vNoteRowVector.at(i)]) {
							if (t.track == vTrackVector.at(i) &&
								t.type == vTapNoteTypeVector.at(i)) {
								if (fabsf(vOffsetVector.at(i)) >
									fabsf(t.offset)) {
									t.offset = vOffsetVector.at(i);
								}
								dontMakeNewTRR = true;
								break;
							}
						}
					}
				}
			}
		}

		if (dontMakeNewTRR)
			continue;

		TapReplayResult trr;
		trr.row = vNoteRowVector.at(i);
		trr.offset = vOffsetVector.at(i);
		if (!vTrackVector.empty()) {
			trr.track = vTrackVector.at(i);

			// bad bandaid, the correct type could be resolved from notedata
			if (i < vTapNoteTypeVector.size())
				trr.type = vTapNoteTypeVector.at(i);
			else
				trr.type = TapNoteType_Tap;
		} else {
			// Anything else (and we got this far without crashing) means
			// it's not a Full Replay
			trr.track = 0;
			trr.type = TapNoteType_Empty;
		}

		// Create or append to the vector
		if (m_ReplayTapMap.count(vNoteRowVector.at(i)) != 0) {
			m_ReplayTapMap[vNoteRowVector.at(i)].push_back(trr);
		} else {
			std::vector<TapReplayResult> trrVector = { trr };
			m_ReplayTapMap[vNoteRowVector.at(i)] = trrVector;
			validNoterows.insert(vNoteRowVector.at(i));
		}
	}

	// Generate vectors made of pregenerated HoldReplayResults referenced by the
	// song row in a map
	// Only present in replays with column data.
	for (auto& i : vHoldReplayDataVector) {
		if (i.row < startingRow)
			continue;

		// Create or append to the vector
		if (m_ReplayHoldMap.count(i.row) != 0) {
			m_ReplayHoldMap[i.row].push_back(i);
		} else {
			std::vector<HoldReplayResult> hrrVector = { i };
			m_ReplayHoldMap[i.row] = hrrVector;
			validNoterows.insert(i.row);
		}
	}

	auto IsHoldDroppedInRowRangeForTrack =
	  [this, m_ReplayHoldMap](int firstRow, int endRow, int track) {
		  if (HasColumnData()) {
			  // Go over all holds in Replay Data
			  for (auto hiter = m_ReplayHoldMap.lower_bound(firstRow); hiter != m_ReplayHoldMap.end();
				   ++hiter) {
				  // If this row is before the start, skip it
				  if (hiter->first < firstRow)
					  continue;
				  // If this row is after the end, skip it
				  else if (hiter->first > endRow)
					  return -1;
				  // This row might work. Check what tracks might have dropped.
				  for (const auto hrr : hiter->second) {
					  if (hrr.track == track)
						  return hiter->first;
				  }
			  }
			  // Iteration finished without finding a drop.
			  return -1;
		  } else {
			  // Replay Data doesn't contain hold data.
			  return -1;
		  }
	  };

	// Don't continue if the replay used invalidating mods
	// (Particularly mods that make it difficult to match NoteData)
	{
		PlayerOptions potmp;
		potmp.FromString(mods);
		if (potmp.ContainsTransformOrTurn() && rngSeed == 0)
			return false;
	}

	// Fill out the Snapshot map now that the other maps are not so out of order
	// We leave out misses in this section because they aren't in the Replay
	// Data
	int tempJudgments[NUM_TapNoteScore] = { 0 };
	int tempHNS[NUM_HoldNoteScore] = { 0 };

	// Copy of the main hold map
	// Delete the values as we go on to make sure they get counted
	auto m_unjudgedholds(m_ReplayHoldMap);

	// If we don't have validnoterows, just do it the hard way
	if (validNoterows.empty()) {
		for (const auto& row : m_ReplayTapMap) {
			for (auto& trr : row.second) {
				if (trr.type == TapNoteType_Mine) {
					tempJudgments[TNS_HitMine]++;
				} else {
					auto tns = REPLAYS->CustomOffsetJudgingFunction(
					  trr.offset, timingScale);
					tempJudgments[tns]++;
				}
			}

			// Make the struct and cram it in there
			// ignore holds for now since the following block takes care of them
			ReplaySnapshot rs;
			FOREACH_ENUM(TapNoteScore, tns)
			rs.judgments[tns] = tempJudgments[tns];
			m_ReplaySnapshotMap[row.first] = rs;
		}
	} else {
		// Iterate over all the noterows we know are in the Replay Data
		for (auto validNoterow : validNoterows) {
			// Check for taps and mines
			if (m_ReplayTapMap.count(validNoterow) != 0) {
				for (auto instance = m_ReplayTapMap[validNoterow].begin();
					 instance != m_ReplayTapMap[validNoterow].end();
					 ++instance) {
					auto& trr = *instance;
					if (trr.type == TapNoteType_Mine) {
						tempJudgments[TNS_HitMine]++;
					} else {
						auto tns = REPLAYS->CustomOffsetJudgingFunction(
						  trr.offset, timingScale);
						tempJudgments[tns]++;
					}
				}
			}

			// Make the struct and cram it in there
			// ignore holds for now since the following block takes care of them
			ReplaySnapshot rs;
			FOREACH_ENUM(TapNoteScore, tns)
			rs.judgments[tns] = tempJudgments[tns];
			m_ReplaySnapshotMap[validNoterow] = rs;
		}
	}

	// Now handle misses and holds.
	// For every row in notedata...
	FOREACH_NONEMPTY_ROW_ALL_TRACKS(noteData, row)
	{
		auto tapsMissedInRow = 0;

		// some rows are not judgeable so should be ignored
		// (fake regions, warps)
		if (pReplayTiming != nullptr) {
			if (!pReplayTiming->IsJudgableAtRow(row))
				continue;
		}

		// For every track in the row...
		for (auto track = 0; track < noteData.GetNumTracks(); track++) {
			// Find the tapnote we are on
			const auto& tn = noteData.GetTapNote(track, row);

			if (tn != TAP_EMPTY) {
				// Deal with holds here
				if (tn.type == TapNoteType_HoldHead) {
					auto isDropped = IsHoldDroppedInRowRangeForTrack(
					  row, row + tn.iDuration, track);
					if (isDropped != -1) {
						if (m_unjudgedholds[isDropped].size() > 0) {
							tempHNS[HNS_LetGo]++;

							// erase the hold from the mapping of unjudged holds
							m_unjudgedholds[isDropped].erase(std::remove_if(
							  m_unjudgedholds[isDropped].begin(),
							  m_unjudgedholds[isDropped].end(),
							  [track](const HoldReplayResult& hrrr) {
								  return hrrr.track == track;
							  }));
							if (m_unjudgedholds[isDropped].empty())
								m_unjudgedholds.erase(isDropped);
						}
					} else {
						tempHNS[HNS_Held]++;
					}
				}

				// See if we passed the earliest dropped hold by now
				// This catches issues where a hold is missed completely
				// and the hold was short enough to bug out
				// and the reported row of the dropped hold is weirdly placed
				auto firstUnjudgedHold = m_unjudgedholds.begin();
				if (firstUnjudgedHold != m_unjudgedholds.end() &&
					row > firstUnjudgedHold->first) {
					auto hrrs = firstUnjudgedHold->second;
					for (size_t i = 0; i < hrrs.size(); i++) {
						if (hrrs.at(i).track == track) {
							m_unjudgedholds[firstUnjudgedHold->first].erase(
							  m_unjudgedholds[firstUnjudgedHold->first]
								.begin() +
							  i);
							tempHNS[HNS_LetGo]++;
						}
					}
					if (m_unjudgedholds[firstUnjudgedHold->first].empty())
						m_unjudgedholds.erase(firstUnjudgedHold->first);
				}

				// Deal with misses here
				// It is impossible to "miss" these notes
				// TapNoteType_HoldTail does not exist in NoteData
				if (tn.type == TapNoteType_Mine ||
					tn.type == TapNoteType_Fake ||
					tn.type == TapNoteType_AutoKeysound)
					continue;

				// If this tap is missing from the replay data, we count it as a
				// miss.
				if (HasColumnData()) {
					if (m_ReplayTapMap.count(row) != 0) {
						auto found = false;
						for (auto& trr : m_ReplayTapMap[row]) {
							if (trr.track == track)
								found = true;
						}
						if (!found) {
							tempJudgments[TNS_Miss]++;
							tapsMissedInRow++;
						}
					} else {
						tempJudgments[TNS_Miss]++;
						tapsMissedInRow++;
					}
				}
			}
		}

		// Count how many misses there are per row instead since we dont have
		// column data in type 1 replays
		if (!HasColumnData()) {
			unsigned notesOnRow = 0;
			unsigned notesInReplayData = 0;
			if (m_ReplayTapMap.count(row) != 0)
				notesInReplayData += m_ReplayTapMap[row].size();
			for (auto track = 0; track < noteData.GetNumTracks(); track++) {
				const auto& tn = noteData.GetTapNote(track, row);
				if (tn != TAP_EMPTY) {
					if (tn.type == TapNoteType_Fake ||
						tn.type == TapNoteType_Mine ||
						tn.type == TapNoteType_AutoKeysound)
						continue;
					notesOnRow++;
				}
			}
			tempJudgments[TNS_Miss] += (notesOnRow - notesInReplayData);
			tapsMissedInRow += (notesOnRow - notesInReplayData);
		}

		// We have to update every single row with the new miss & hns counts.
		// This unfortunately takes more time.
		// If current row is recorded in the snapshots, update the counts
		if (m_ReplaySnapshotMap.count(row) != 0) {
			m_ReplaySnapshotMap[row].judgments[TNS_Miss] =
			  tempJudgments[TNS_Miss];
			FOREACH_ENUM(HoldNoteScore, hns)
			m_ReplaySnapshotMap[row].hns[hns] = tempHNS[hns];

		} else {
			// If the current row is after the last recorded row, make a new one
			if (m_ReplaySnapshotMap.empty() ||
				m_ReplaySnapshotMap.rbegin()->first < row) {
				ReplaySnapshot rs;
				FOREACH_ENUM(TapNoteScore, tns)
				rs.judgments[tns] = tempJudgments[tns];
				FOREACH_ENUM(HoldNoteScore, hns)
				rs.hns[hns] = tempHNS[hns];
				m_ReplaySnapshotMap[row] = rs;
			}
			// If the current row is before the earliest recorded row, make a
			// new one
			else if (m_ReplaySnapshotMap.begin()->first > row) {
				ReplaySnapshot rs;
				rs.judgments[TNS_Miss] = tapsMissedInRow;
				m_ReplaySnapshotMap[row] = rs;
			} else // If the current row is in between recorded rows, copy an
				   // older one
			{
				ReplaySnapshot rs;
				auto prev = m_ReplaySnapshotMap.lower_bound(row);
				--prev;
				// it is expected at this point that prev is not somehow outside
				// the range if it is, we have bigger problems
				FOREACH_ENUM(TapNoteScore, tns)
				rs.judgments[tns] =
				  m_ReplaySnapshotMap[prev->first].judgments[tns];
				FOREACH_ENUM(HoldNoteScore, hns)
				rs.hns[hns] = m_ReplaySnapshotMap[prev->first].hns[hns];
				m_ReplaySnapshotMap[row] = rs;
			}
		}
	}
	/* The final output here has 2 minor issues:
	 * - Holds completely missed are not counted as HNS_Missed
	 * - Holds completed are not placed in Snapshot until after they are
	 * complete
	 * However, completely missed holds are present in replay data.
	 * The second issue does not cause miscounts, but does cause butchered holds
	 * to be missed (but not judged)
	 *
	 * Retrospective comment: im not sure if the above is true anymore
	 */

	// now update the wifescore values for each relevant snapshot.
	// some snapshots end up with 0 values due to being "missing" from the
	// replay data and we have to account for those
	std::vector<int> snapShotsUnused;
	snapShotsUnused.reserve(m_ReplaySnapshotMap.size());
	for (auto& it : m_ReplaySnapshotMap)
		snapShotsUnused.push_back(it.first);
	auto cws = 0.f; // curwifescore
	auto mws = 0.f; // maxwifescore
	auto taps = 0;	// tap count
	double runningmean = 0.0;
	double runningvariance = 0.0;
	for (auto it = m_ReplayTapMap.begin(); it != m_ReplayTapMap.end();) {
		const auto r = it->first;
		if (r > snapShotsUnused.front()) {
			// if we somehow skipped a snapshot, the only difference should be
			// in misses and non taps
			auto rs = &m_ReplaySnapshotMap[snapShotsUnused.front()];
			rs->curwifescore =
			  cws +
			  (rs->judgments[TNS_Miss] *
			   REPLAYS->CustomTapScoringFunction(1.F, TNS_Miss, timingScale)) +
			  (rs->hns[HNS_Missed] *
			   REPLAYS->CustomHoldNoteScoreScoringFunction(HNS_Missed)) +
			  (rs->hns[HNS_LetGo] *
			   REPLAYS->CustomHoldNoteScoreScoringFunction(HNS_LetGo)) +
			  (rs->hns[HNS_Held] *
			   REPLAYS->CustomHoldNoteScoreScoringFunction(HNS_Held));
			rs->maxwifescore =
			  mws +
			  (rs->judgments[TNS_Miss] *
			   REPLAYS->CustomTotalWifePointsCalculation(TapNoteType_Tap));
			rs->mean = runningmean;
			rs->standardDeviation =
			  taps > 1 ? std::sqrt(runningvariance / (taps - 1.0)) : 0.0;

			snapShotsUnused.erase(snapShotsUnused.begin());
			continue; // retry the iteration (it++ is moved to below)
		}
		auto rs = GetReplaySnapshotForNoterow(r);
		// set mean and sd initially
		rs->mean = runningmean;
		rs->standardDeviation =
		  taps > 1 ? std::sqrt(runningvariance / (taps - 1.0)) : 0.0;
		for (auto& trr : it->second) {
			if (trr.type == TapNoteType_Mine) {
				cws += REPLAYS->CustomMineScoringFunction();
			} else {
				auto tns =
				  REPLAYS->CustomOffsetJudgingFunction(trr.offset, timingScale);
				auto tapscore = REPLAYS->CustomTapScoringFunction(
				  trr.offset, tns, timingScale);
				cws += tapscore;
				mws +=
				  REPLAYS->CustomTotalWifePointsCalculation(trr.type);

				// do mean/sd for all non miss taps
				if (tns != TNS_Miss && tns != TNS_None) {
					taps++;

					// universe brain algorithm (Welford's)
					double delta = trr.offset * 1000.0 - runningmean;
					runningmean += delta / taps;
					double delta2 = trr.offset * 1000.0 - runningmean;
					runningvariance += delta * delta2;
				}

				// set mean and sd again to update it if necessary
				rs->mean = runningmean;
				rs->standardDeviation =
				  taps > 1 ? std::sqrt(runningvariance / (taps - 1.0)) : 0.0;
			}
		}
		rs->curwifescore =
		  cws +
		  (rs->judgments[TNS_Miss] *
		   REPLAYS->CustomTapScoringFunction(1.F, TNS_Miss, timingScale)) +
		  (rs->hns[HNS_Missed] *
		   REPLAYS->CustomHoldNoteScoreScoringFunction(HNS_Missed)) +
		  (rs->hns[HNS_LetGo] *
		   REPLAYS->CustomHoldNoteScoreScoringFunction(HNS_LetGo)) +
		  (rs->hns[HNS_Held] *
		   REPLAYS->CustomHoldNoteScoreScoringFunction(HNS_Held));
		rs->maxwifescore =
		  mws + (rs->judgments[TNS_Miss] *
				 REPLAYS->CustomTotalWifePointsCalculation(TapNoteType_Tap));

		snapShotsUnused.erase(snapShotsUnused.begin());
		++it;
	}
	if (!snapShotsUnused.empty()) {
		for (auto row : snapShotsUnused) {
			// This might not be technically correct
			// But my logic is this:
			// A snapshot without associated replaydata is one for a row
			// which has no stat-affecting changes made to it.
			// So this applies to rows with all Mines
			// or rows with all Fakes (in the latest version)
			auto prevrs = GetReplaySnapshotForNoterow(row - 1);
			auto rs = &m_ReplaySnapshotMap[row];
			rs->curwifescore = prevrs->curwifescore;
			rs->maxwifescore = prevrs->maxwifescore;
			rs->mean = prevrs->mean;
			rs->standardDeviation = prevrs->standardDeviation;
		}
	}

	// after snapshot generation is finished
	// generate the elapsed time maps
	// Don't continue if the replay doesn't have column data.
	// We can't be accurate without it.
	if (!HasColumnData()) {
		// true return here because we dont really need the rest
		return true;
	}

	auto& m_ReplayTapMapByElapsedTime = ji.trrMapByElapsedTime;
	auto& m_ReplayHoldMapByElapsedTime = ji.hrrMapByElapsedTime;

	// For every row in the replay data...
	for (auto& row : m_ReplayTapMap) {
		// Get the current time and go over all taps on this row...
		const auto rowTime = pReplayTiming->WhereUAtBro(row.first);
		for (auto& trr : row.second) {
			// Find the time adjusted for offset
			auto tapTime = rowTime + trr.offset;

			// Also put it into the elapsed time map :)
			if (m_ReplayTapMapByElapsedTime.count(tapTime) != 0) {
				m_ReplayTapMapByElapsedTime[tapTime].push_back(trr);
			} else {
				std::vector<TapReplayResult> trrVector = { trr };
				m_ReplayTapMapByElapsedTime[tapTime] = trrVector;
			}
		}
	}

	// Sneak in the HoldMapByElapsedTime construction for consistency
	// Go over all of the elements, you know the deal.
	// We can avoid getting offset rows here since drops don't do that
	for (auto& row : m_ReplayHoldMap) {
		auto dropTime = pReplayTiming->WhereUAtBro(row.first);
		for (auto& hrr : row.second) {
			if (m_ReplayHoldMapByElapsedTime.count(dropTime) != 0) {
				m_ReplayHoldMapByElapsedTime[dropTime].push_back(hrr);
			} else {
				std::vector<HoldReplayResult> hrrVector = { hrr };
				m_ReplayHoldMapByElapsedTime[dropTime] = hrrVector;
			}
		}
	}

	// If things were in the right order by this point
	// then we know SnapshotMap is filled out.
	// This is how we can find misses quickly without having to keep
	// track of them in some other special way.
	if (!m_ReplaySnapshotMap.empty()) {
		auto curSnap = m_ReplaySnapshotMap.begin();
		++curSnap;
		auto prevSnap = m_ReplaySnapshotMap.begin();
		while (curSnap != m_ReplaySnapshotMap.end()) {
			auto csn = curSnap->second;
			auto psn = prevSnap->second;
			const auto missDiff =
			  csn.judgments[TNS_Miss] - psn.judgments[TNS_Miss];
			if (missDiff > 0) {
				const auto row = curSnap->first;
				// the tap time is pushed back by the smallest normal boo
				// window. the reason for this is that's about the point where
				// the game should usually count something as a miss. we dont
				// use this time for anything other than chronologically parsing
				// replay data for combo/life stuff so this is okay (i hope)
				auto tapTime =
				  pReplayTiming->WhereUAtBro(row) + MISS_WINDOW_BEGIN_SEC;
				for (auto i = 0; i < missDiff; i++) {
					// we dont really care about anything other than the offset
					// because we have the estimate time at the row in the map
					// and then we just need to know what judgment to assign it
					TapReplayResult trr;
					trr.row = row;
					trr.offset = 1.f;
					if (m_ReplayTapMapByElapsedTime.count(tapTime) != 0) {
						m_ReplayTapMapByElapsedTime[tapTime].push_back(trr);
					} else {
						std::vector<TapReplayResult> trrVector = { trr };
						m_ReplayTapMapByElapsedTime[tapTime] = trrVector;
					}
				}
			}
			++curSnap;
			++prevSnap;
		}
	}

	return true;
}

// Lua
#include "Etterna/Models/Lua/LuaBinding.h"

class LunaReplay : public Luna<Replay>
{
  public:
	static auto GetOffsetVector(T* p, lua_State* L) -> int
	{
		auto v = p->GetOffsetVector();
		const auto loaded = !v.empty();
		if (loaded || p->LoadReplayData()) {
			if (!loaded) {
				v = p->GetOffsetVector();
			}
			for (auto& i : v) {
				i = i * 1000;
			}
			LuaHelpers::CreateTableFromArray(v, L);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	static auto GetNoteRowVector(T* p, lua_State* L) -> int
	{
		const auto* v = &(p->GetNoteRowVector());
		const auto loaded = !v->empty();

		auto timestamps = p->GetCopyOfOnlineReplayTimestampVector();

		if (loaded || p->LoadReplayData()) {
			// this is a local highscore with a local replay
			// easy to just output the noterows loaded
			LuaHelpers::CreateTableFromArray((*v), L);
		} else if (!timestamps.empty() && v->empty()) {
			// this is a legacy online replay
			// missing rows but with timestamps instead
			// we can try our best to show the noterows by approximating
			auto* chart = SONGMAN->GetStepsByChartkey(p->GetChartKey());
			auto* td = chart->GetTimingData();
			GAMESTATE->SetProcessedTimingData(td);
			auto nd = chart->GetNoteData();
			auto nerv = nd.BuildAndGetNerv(td);
			auto sdifs = td->BuildAndGetEtaner(nerv);
			std::vector<int> noterows;
			for (auto t : timestamps) {
				auto timestamptobeat =
				  td->GetBeatFromElapsedTime(t * p->GetMusicRate());
				const auto somenumberscaledbyoffsets =
				  sdifs[0] - (timestamps[0] * p->GetMusicRate());
				timestamptobeat += somenumberscaledbyoffsets;
				auto noterowfrombeat = BeatToNoteRow(timestamptobeat);
				noterows.emplace_back(noterowfrombeat);
			}
			const auto noterowoffsetter = nerv[0] - noterows[0];
			for (auto& noterowwithoffset : noterows) {
				noterowwithoffset += noterowoffsetter;
			}
			GAMESTATE->SetProcessedTimingData(nullptr);
			p->SetNoteRowVector(noterows);

			v = &(p->GetNoteRowVector()); // uhh

			LuaHelpers::CreateTableFromArray((*v), L);
		} else {
			// ok we got nothing, just throw null
			lua_pushnil(L);
		}
		return 1;
	}

	static auto GetTrackVector(T* p, lua_State* L) -> int
	{
		const auto* v = &(p->GetTrackVector());
		const auto loaded = !v->empty();
		if (loaded || p->LoadReplayData()) {
			if (!loaded) {
				v = &(p->GetTrackVector());
			}
			LuaHelpers::CreateTableFromArray((*v), L);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	static auto GetTapNoteTypeVector(T* p, lua_State* L) -> int
	{
		const auto* v = &(p->GetTapNoteTypeVector());
		const auto loaded = !v->empty();
		if (loaded || p->LoadReplayData()) {
			if (!loaded) {
				v = &(p->GetTapNoteTypeVector());
			}
			LuaHelpers::CreateTableFromArray((*v), L);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	static auto GetHoldNoteVector(T* p, lua_State* L) -> int
	{
		auto v = p->GetHoldReplayDataVector();
		const auto loaded = !v.empty();
		if (loaded || p->LoadReplayData()) {
			if (!loaded) {
				v = p->GetHoldReplayDataVector();
			}
			// make containing table
			lua_newtable(L);
			for (size_t i = 0; i < v.size(); i++) {
				// make table for each item
				lua_createtable(L, 0, 3);

				lua_pushnumber(L, v.at(i).row);
				lua_setfield(L, -2, "row");
				lua_pushnumber(L, v.at(i).track);
				lua_setfield(L, -2, "track");
				LuaHelpers::Push<TapNoteSubType>(L, v.at(i).subType);
				lua_setfield(L, -2, "TapNoteSubType");

				lua_rawseti(L, -2, i + 1);
			}
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	static auto GetMineHitVector(T* p, lua_State* L) -> int
	{
		auto v = p->GetMineReplayDataVector();
		const auto loaded = !v.empty();
		if (loaded || p->LoadReplayData()) {
			if (!loaded) {
				v = p->GetMineReplayDataVector();
			}
			// make containing table
			lua_newtable(L);
			for (size_t i = 0; i < v.size(); i++) {
				// make table for each item
				lua_createtable(L, 0, 3);

				lua_pushnumber(L, v.at(i).row);
				lua_setfield(L, -2, "row");
				lua_pushnumber(L, v.at(i).track);
				lua_setfield(L, -2, "track");

				lua_rawseti(L, -2, i + 1);
			}
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	static auto GetReplaySnapshotForNoterow(T* p, lua_State* L) -> int
	{
		auto row = IArg(1);
		p->GetReplaySnapshotForNoterow(row)->PushSelf(L);
		return 1;
	}

	static auto GetInputData(T* p, lua_State* L) -> int
	{
		if (!p->LoadReplayData() && p->GetReplayType() != ReplayType_Input) {
			lua_pushnil(L);
			return 1;
		}

		auto idv = p->GetCopyOfInputDataVector();

		lua_createtable(L, 0, idv.size());
		int i = 0;
		for (auto& id : idv) {
			id.PushSelf(L);
			lua_rawseti(L, -2, ++i);
		}

		return 1;
	}

	DEFINE_METHOD(HasReplayData, HasReplayData())
	DEFINE_METHOD(GetChartKey, GetChartKey())
	DEFINE_METHOD(GetScoreKey, GetScoreKey())
	DEFINE_METHOD(GetReplayType, GetReplayType())
	DEFINE_METHOD(GetRngSeed, GetRngSeed())
	DEFINE_METHOD(GetModifiers, GetModifiers());

	LunaReplay() {
		ADD_METHOD(HasReplayData);
		ADD_METHOD(GetChartKey);
		ADD_METHOD(GetScoreKey);
		ADD_METHOD(GetReplayType);
		ADD_METHOD(GetRngSeed);
		ADD_METHOD(GetModifiers);

		ADD_METHOD(GetOffsetVector);
		ADD_METHOD(GetNoteRowVector);
		ADD_METHOD(GetTrackVector);
		ADD_METHOD(GetTapNoteTypeVector);
		ADD_METHOD(GetHoldNoteVector);
		ADD_METHOD(GetMineHitVector);
		ADD_METHOD(GetInputData);
		ADD_METHOD(GetReplaySnapshotForNoterow);
	}
};
LUA_REGISTER_CLASS(Replay)

class LunaReplaySnapshot : public Luna<ReplaySnapshot>
{
  public:

	static int GetJudgments(T* p, lua_State* L)
	{
		lua_createtable(L, 0, NUM_TapNoteScore);
		FOREACH_ENUM(TapNoteScore, tns) {
			auto str = TapNoteScoreToString(tns);
			lua_pushnumber(L, p->judgments[tns]);
			lua_setfield(L, -2, str.c_str());
		}

		return 1;
	}
	static int GetHoldNoteScores(T* p, lua_State* L)
	{
		lua_createtable(L, 0, NUM_HoldNoteScore);
		FOREACH_ENUM(HoldNoteScore, hns) {
			auto str = HoldNoteScoreToString(hns);
			lua_pushnumber(L, p->hns[hns]);
			lua_setfield(L, -2, str.c_str());
		}

		return 1;
	}
	static int GetWifePercent(T* p, lua_State* L)
	{
		auto& c = p->curwifescore;
		auto& m = p->maxwifescore;

		lua_pushnumber(L, c / m);
		return 1;
	}

	DEFINE_METHOD(GetCurWifeScore, curwifescore);
	DEFINE_METHOD(GetMaxWifeScore, maxwifescore);
	DEFINE_METHOD(GetStandardDeviation, standardDeviation);
	DEFINE_METHOD(GetMean, mean);

	LunaReplaySnapshot() {
		ADD_METHOD(GetJudgments);
		ADD_METHOD(GetHoldNoteScores);
		ADD_METHOD(GetCurWifeScore);
		ADD_METHOD(GetMaxWifeScore);
		ADD_METHOD(GetStandardDeviation);
		ADD_METHOD(GetMean);
		ADD_METHOD(GetWifePercent);
	}
};
LUA_REGISTER_CLASS(ReplaySnapshot)

class LunaInputDataEvent : public Luna<InputDataEvent>
{
  public:

	DEFINE_METHOD(IsPress, is_press);
	DEFINE_METHOD(GetColumn, column);
	DEFINE_METHOD(GetSongPositionSeconds, songPositionSeconds);
	DEFINE_METHOD(GetNearestTapNoterow, nearestTapNoterow);
	DEFINE_METHOD(GetOffsetFromNearest, offsetFromNearest);
	DEFINE_METHOD(GetNearestTapNoteType, nearestTapNoteType);
	DEFINE_METHOD(GetNearestTapNoteSubType, nearestTapNoteSubType);

	LunaInputDataEvent() {
		ADD_METHOD(IsPress);
		ADD_METHOD(GetColumn);
		ADD_METHOD(GetSongPositionSeconds);
		ADD_METHOD(GetNearestTapNoterow);
		ADD_METHOD(GetOffsetFromNearest);
		ADD_METHOD(GetNearestTapNoteType);
		ADD_METHOD(GetNearestTapNoteSubType);
	}
};
LUA_REGISTER_CLASS(InputDataEvent)
