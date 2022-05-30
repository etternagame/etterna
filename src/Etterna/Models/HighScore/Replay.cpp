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
Replay::WriteReplayData() -> bool
{
	Locator::getLogger()->info("Writing out replay data to disk");
	std::string append;
	std::string profiledir;
	// These two lines should probably be somewhere else
	if (!FILEMAN->IsADirectory(FULL_REPLAY_DIR)) {
		FILEMAN->CreateDir(FULL_REPLAY_DIR);
	}
	const auto path = FULL_REPLAY_DIR + scoreKey;
	std::ofstream fileStream(path, std::ios::binary);
	// check file

	ASSERT(!vNoteRowVector.empty());

	if (!fileStream) {
		Locator::getLogger()->warn("Failed to create replay file at {}",
								   path.c_str());
		return false;
	}

	const unsigned int idx = vNoteRowVector.size() - 1;
	// loop for writing both vectors side by side
	for (unsigned int i = 0; i <= idx; i++) {
		append = std::to_string(vNoteRowVector[i]) + " " +
				 std::to_string(vOffsetVector[i]) + " " +
				 std::to_string(vTrackVector[i]) +
				 (vTapNoteTypeVector[i] != TapNoteType_Tap
					? " " + std::to_string(vTapNoteTypeVector[i])
					: "") +
				 "\n";
		fileStream.write(append.c_str(), append.size());
	}
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
	Locator::getLogger()->info("Created replay file at {}", path.c_str());
	return true;
}

auto
Replay::WriteInputData() -> bool
{
	std::string append;
	Locator::getLogger()->info("Writing out Input Data to disk");
	// These two lines should probably be somewhere else
	if (!FILEMAN->IsADirectory(INPUT_DATA_DIR)) {
		FILEMAN->CreateDir(INPUT_DATA_DIR);
	}

	const auto path = INPUT_DATA_DIR + scoreKey;
	const auto path_z = path + "z";
	if (InputData.empty()) {
		Locator::getLogger()->warn(
		  "Attempted to write input data for {} but there was nothing to write",
		  path.c_str());
	}

	// check file
	std::ofstream fileStream(path, std::ios::binary);
	if (!fileStream) {
		Locator::getLogger()->warn("Failed to create input data file at {}",
								   path.c_str());
		return false;
	}

	// for writing human readable text
	try {
		float fGlobalOffset = PREFSMAN->m_fGlobalOffsetSeconds.Get();
		// header
		auto headerLine1 = chartKey + " " + scoreKey + " " +
						   std::to_string(fMusicRate) + " " +
						   std::to_string(fSongOffset) + " " +
						   std::to_string(fGlobalOffset) + "\n";
		fileStream.write(headerLine1.c_str(), headerLine1.size());

		// input data
		const unsigned int idx = InputData.size() - 1;
		for (unsigned int i = 0; i <= idx; i++) {
			append = std::to_string(InputData[i].column) + " " +
					 (InputData[i].is_press ? "1" : "0") + " " +
					 std::to_string(InputData[i].songPositionSeconds) + " " +
					 std::to_string(InputData[i].nearestTapNoterow) + " " +
					 std::to_string(InputData[i].offsetFromNearest) + "\n";
			fileStream.write(append.c_str(), append.size());
		}

		// dropped hold data
		for (auto& hold : vHoldReplayDataVector) {
			append = "H " + std::to_string(hold.row) + " " +
					 std::to_string(hold.track) +
					 (hold.subType != TapNoteSubType_Hold
						? " " + std::to_string(hold.subType)
						: "") +
					 "\n";
			fileStream.write(append.c_str(), append.size());
		}

		// hit mine data
		for (auto& mine : vMineReplayDataVector) {
			append = "M " + std::to_string(mine.row) + " " +
					 std::to_string(mine.track) + "\n";
			fileStream.write(append.c_str(), append.size());
		}

		fileStream.close();

		FILE* infile = fopen(path.c_str(), "rb");
		gzFile outfile = gzopen(path_z.c_str(), "wb");
		if ((infile == nullptr) || (outfile == nullptr)) {
			Locator::getLogger()->warn("Failed to compress new input data.");
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
Replay::LoadInputData() -> bool
{
	if (!InputData.empty())
		return true;

	auto path = INPUT_DATA_DIR + scoreKey;
	auto path_z = path + "z";
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
		// hope nothing already exists here
		FILE* outfile = fopen(path.c_str(), "wb");
		if ((infile == nullptr) || (outfile == nullptr)) {
			Locator::getLogger()->warn("Failed to read input data at {}",
									   path_z.c_str());
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
			if (tokens.size() != 3) {
				Locator::getLogger()->warn("Bad input data header detected: {}",
										   GetScoreKey().c_str());
				return false;
			}

			auto chartkey = tokens[0];
			auto scorekey = tokens[1];
			auto rate = std::stof(tokens[2]);
			auto songoffset = std::stof(tokens[3]);
			auto globaloffset = std::stof(tokens[4]);
			// ... for later

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
			InputData.push_back(ev);

			tokens.clear();
		}

		SetMineReplayDataVector(vMineReplayDataVector);
		SetHoldReplayDataVector(vHoldReplayDataVector);

		Locator::getLogger()->info("Loaded input data at {}", path.c_str());

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
Replay::LoadReplayData() -> bool
{
	if (LoadReplayDataFull(FULL_REPLAY_DIR)) {
		return true;
	}
	return LoadReplayDataBasic(BASIC_REPLAY_DIR);
}

auto
Replay::HasReplayData() -> bool
{
	return false;
}

auto
Replay::LoadReplayDataBasic(const std::string& dir) -> bool
{
	// already exists
	if (vNoteRowVector.size() > 4 && vOffsetVector.size() > 4) {
		return true;
	}

	std::string profiledir;
	std::vector<int> vNoteRowVector;
	std::vector<float> vOffsetVector;
	auto path = dir + scoreKey;

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
Replay::LoadReplayDataFull(const std::string& dir) -> bool
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
	auto path = dir + scoreKey;

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
			return LoadReplayDataBasic(dir);
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
	}
};

LUA_REGISTER_CLASS(Replay)
