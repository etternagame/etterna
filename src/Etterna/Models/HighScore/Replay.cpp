#include "Etterna/Globals/global.h"
#include "Replay.h"
#include "HighScore.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/NoteData/NoteData.h"

// Singletons
#include "RageUtil/File/RageFileManager.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/GameState.h"
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
{

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
			append = std::to_string(vNoteRowVector[i]) + " " +
					 std::to_string(vOffsetVector[i]) + " " +
					 std::to_string(vTrackVector[i]) +
					 (vTapNoteTypeVector[i] != TapNoteType_Tap
						? " " + std::to_string(vTapNoteTypeVector[i])
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
		// chartkey scorekey rate offset globaloffset
		auto headerLine1 = chartKey + " " + scoreKey + " " +
						   std::to_string(fMusicRate) + " " +
						   std::to_string(fSongOffset) + " " +
						   std::to_string(fGlobalOffset) + "\n";
		fileStream.write(headerLine1.c_str(), headerLine1.size());

		// input data:
		// column press/lift time nearest_tap tap_offset
		const unsigned int sz = InputData.size() - 1;
		for (unsigned int i = 0; i <= sz; i++) {
			append = std::to_string(InputData[i].column) + " " +
					 (InputData[i].is_press ? "1" : "0") + " " +
					 std::to_string(InputData[i].songPositionSeconds) + " " +
					 std::to_string(InputData[i].nearestTapNoterow) + " " +
					 std::to_string(InputData[i].offsetFromNearest) + "\n";
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
		if (outfile == nullptr) {
			Locator::getLogger()->warn("Failed to compress new input data "
									   "because {} could not be opened",
									   path_z.c_str());
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
		if (infile == nullptr) {
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
			if (tokens.size() != 5) {
				Locator::getLogger()->warn("Bad input data header detected: {}",
										   path_z.c_str());
				return false;
			}

			this->chartKey = tokens[0];
			this->scoreKey = tokens[1];
			this->fMusicRate = std::stof(tokens[2]);
			this->fSongOffset = std::stof(tokens[3]);
			this->fGlobalOffset = std::stof(tokens[4]);
			
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
			if (tokens.size() != 5) {
				Locator::getLogger()->warn("Bad input data detected: {}",
										   GetScoreKey().c_str());
				return false;
			}

			ev.column = std::stoi(tokens[0]);
			ev.is_press = (std::stoi(tokens[1]) != 0);
			ev.songPositionSeconds = std::stof(tokens[2]);
			ev.nearestTapNoterow = std::stoi(tokens[3]);
			ev.offsetFromNearest = std::stof(tokens[4]);
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
		if (fabs(*offsetIt) >= 0.18F) {
			removeIts();
		} else {
			moveIts();
		}
	}
}

auto
Replay::GenerateInputData() -> bool
{
	if (!LoadReplayData() && !LoadInputData() && !GenerateNoterowsFromTimestamps()) {
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
			if (offset > 0.180F) {
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

			if (offset > 0.180F) {
				// nah
				continue;
			}

			for (int j = 0; j < columns; j++) {
				const auto tn = nd.GetTapNote(j, noterow);
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

				lua_pushnumber(L, v[i].row);
				lua_setfield(L, -2, "row");
				lua_pushnumber(L, v[i].track);
				lua_setfield(L, -2, "track");
				LuaHelpers::Push<TapNoteSubType>(L, v[i].subType);
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

				lua_pushnumber(L, v[i].row);
				lua_setfield(L, -2, "row");
				lua_pushnumber(L, v[i].track);
				lua_setfield(L, -2, "track");

				lua_rawseti(L, -2, i + 1);
			}
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	DEFINE_METHOD(HasReplayData, HasReplayData())
	DEFINE_METHOD(GetChartKey, GetChartKey())
	DEFINE_METHOD(GetScoreKey, GetScoreKey())
	DEFINE_METHOD(GetReplayType, GetReplayType())

	LunaReplay() {
		ADD_METHOD(HasReplayData);
		ADD_METHOD(GetChartKey);
		ADD_METHOD(GetScoreKey);
		ADD_METHOD(GetReplayType);

		ADD_METHOD(GetOffsetVector);
		ADD_METHOD(GetNoteRowVector);
		ADD_METHOD(GetTrackVector);
		ADD_METHOD(GetTapNoteTypeVector);
		ADD_METHOD(GetHoldNoteVector);
		ADD_METHOD(GetMineHitVector);
	}
};

LUA_REGISTER_CLASS(Replay)
