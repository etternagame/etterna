/* This stores a single note pattern for a song.
 *
 * We can have too much data to keep everything decompressed as NoteData, so
 * most songs are kept in memory compressed as SMData until requested.  NoteData
 * is normally not requested casually during gameplay; we can move through
 * screens, the music wheel, etc. without touching any NoteData.
 *
 * To save more memory, if data is cached on disk, read it from disk on demand.
 * Not all Steps will have an associated file for this purpose.  (Profile edits
 * don't do this yet.)
 *
 * Data can be on disk (always compressed), compressed in memory, and
 * uncompressed in memory. */
#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Globals/MinaCalc.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/NoteData/NoteDataUtil.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderBMS.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderDWI.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderKSF.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderOSU.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSM.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSMA.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSSC.h"
#include "Etterna/Models/NoteWriters/NotesWriterETT.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include <algorithm>
#include <thread>
#include "Etterna/Models/NoteData/NoteDataStructures.h"

/* register DisplayBPM with StringConversion */
#include "Etterna/Models/Misc/EnumHelper.h"

// For hashing wife chart keys - Mina
#include "Etterna/Singletons/CryptManager.h"

static const char* DisplayBPMNames[] = {
	"Actual",
	"Specified",
	"Random",
};
XToString(DisplayBPM);
LuaXType(DisplayBPM);

Steps::Steps(Song* song)
  : m_StepsType(StepsType_Invalid)
  , m_pSong(song)
  , m_pNoteData(new NoteData)
  , m_bNoteDataIsFilled(false)
  , m_sNoteDataCompressed("")
  , m_sFilename("")
  , m_bSavedToDisk(false)
  , m_LoadedFromProfile(ProfileSlot_Invalid)
  , m_iHash(0)
  , m_sDescription("")
  , m_sChartStyle("")
  , m_Difficulty(Difficulty_Invalid)
  , m_iMeter(0)
  , m_bAreCachedRadarValuesJustLoaded(false)
  , m_sCredit("")
  , displayBPMType(DISPLAY_BPM_ACTUAL)
  , specifiedBPMMin(0)
  , specifiedBPMMax(0)
{
}

Steps::~Steps() = default;

void
Steps::GetDisplayBpms(DisplayBpms& AddTo) const
{
	if (this->GetDisplayBPM() == DISPLAY_BPM_SPECIFIED) {
		AddTo.Add(this->GetMinBPM());
		AddTo.Add(this->GetMaxBPM());
	} else {
		float fMinBPM;
		float fMaxBPM;
		this->GetTimingData()->GetActualBPM(fMinBPM, fMaxBPM);
		AddTo.Add(fMinBPM);
		AddTo.Add(fMaxBPM);
	}
}

auto
Steps::GetHash() const -> unsigned
{
	if (m_iHash != 0u) {
		return m_iHash;
	}
	if (m_sNoteDataCompressed.empty()) {
		if (!m_bNoteDataIsFilled) {
			return 0; // No data, no hash.
		}
		NoteDataUtil::GetSMNoteDataString(*m_pNoteData, m_sNoteDataCompressed);
	}
	m_iHash = GetHashForString(m_sNoteDataCompressed);
	return m_iHash;
}

auto
Steps::IsNoteDataEmpty() const -> bool
{
	return this->m_sNoteDataCompressed.empty() && !m_bNoteDataIsFilled;
}

auto
Steps::GetNoteDataFromSimfile() -> bool
{
	// Replace the line below with the Steps' cache file.
	RString stepFile = this->GetFilename();
	RString extension = GetExtension(stepFile);
	extension
	  .MakeLower(); // must do this because the code is expecting lowercase

	if (extension.empty() || extension == "ssc" ||
		extension == "ats") // remember cache files.
	{
		SSCLoader loader;
		if (!loader.LoadNoteDataFromSimfile(stepFile, *this)) {
			/*
			HACK: 7/20/12 -- see bugzilla #740
			users who edit songs using the ever popular .sm file
			that remove or tamper with the .ssc file later on
			complain of blank steps in the editor after reloading.
			Despite the blank steps being well justified since
			the cache files contain only the SSC step file,
			give the user some leeway and search for a .sm replacement
			*/
			SMLoader backup_loader;
			RString transformedStepFile = stepFile;
			transformedStepFile.Replace(".ssc", ".sm");

			return backup_loader.LoadNoteDataFromSimfile(transformedStepFile,
														 *this);
		}
		return true;
	}
	if (extension == "sm") {
		SMLoader loader;
		return loader.LoadNoteDataFromSimfile(stepFile, *this);
	} else if (extension == "sma") {
		SMALoader loader;
		return loader.LoadNoteDataFromSimfile(stepFile, *this);
	} else if (extension == "dwi") {
		return DWILoader::LoadNoteDataFromSimfile(stepFile, *this);
	} else if (extension == "ksf") {
		return KSFLoader::LoadNoteDataFromSimfile(stepFile, *this);
	} else if (extension == "bms" || extension == "bml" || extension == "bme" ||
			   extension == "pms") {
		return BMSLoader::LoadNoteDataFromSimfile(stepFile, *this);
	} else if (extension == "osu") {
		return OsuLoader::LoadNoteDataFromSimfile(stepFile, *this);
	} else if (extension == "edit") {
		// Try SSC, then fallback to SM.
		SSCLoader ldSSC;
		if (!ldSSC.LoadNoteDataFromSimfile(stepFile, *this)) {
			SMLoader ldSM;
			return ldSM.LoadNoteDataFromSimfile(stepFile, *this);
		}
		{
			return true;
		}
	}
	return false;
}

void
Steps::SetNoteData(const NoteData& noteDataNew)
{
	ASSERT(noteDataNew.GetNumTracks() ==
		   GAMEMAN->GetStepsTypeInfo(m_StepsType).iNumTracks);

	*m_pNoteData = noteDataNew;
	m_bNoteDataIsFilled = true;

	m_sNoteDataCompressed = RString();
	m_iHash = 0;
}

void
Steps::GetNoteData(NoteData& noteDataOut) const
{
	Decompress();

	if (m_bNoteDataIsFilled) {
		noteDataOut = *m_pNoteData;
	} else {
		noteDataOut.ClearAll();
		noteDataOut.SetNumTracks(
		  GAMEMAN->GetStepsTypeInfo(m_StepsType).iNumTracks);
	}
}

auto
Steps::GetNoteData() const -> NoteData
{
	NoteData tmp;
	this->GetNoteData(tmp);
	return tmp;
}

void
Steps::SetSMNoteData(const RString& notes_comp_)
{
	m_pNoteData->Init();
	m_bNoteDataIsFilled = false;

	m_sNoteDataCompressed = notes_comp_;
	m_iHash = 0;
}

/* XXX: this function should pull data from m_sFilename, like Decompress() */
void
Steps::GetSMNoteData(RString& notes_comp_out) const
{
	if (m_sNoteDataCompressed.empty()) {
		if (!m_bNoteDataIsFilled) {
			/* no data is no data */
			notes_comp_out = "";
			return;
		}

		NoteDataUtil::GetSMNoteDataString(*m_pNoteData, m_sNoteDataCompressed);
	}

	notes_comp_out = m_sNoteDataCompressed;
}

/* XXX: this function should pull data from m_sFilename, like Decompress() */
void
Steps::GetETTNoteData(RString& notes_comp_out) const
{
	if (m_sNoteDataCompressed.empty()) {
		if (!m_bNoteDataIsFilled) {
			/* no data is no data */
			notes_comp_out = "";
			return;
		}

		NoteDataUtil::GetETTNoteDataString(*m_pNoteData, m_sNoteDataCompressed);
	}
	notes_comp_out = m_sNoteDataCompressed;
}

void
Steps::TidyUpData()
{
	// Don't set the StepsType to dance single if it's invalid.  That just
	// causes unrecognized charts to end up where they don't belong.
	// Leave it as StepsType_Invalid so the Song can handle it specially.  This
	// is a forwards compatibility feature, so that if a future version adds a
	// new style, editing a simfile with unrecognized Steps won't silently
	// delete them. -Kyz
	if (m_StepsType == StepsType_Invalid) {
		LOG->Warn("Detected steps with unknown style '%s' in '%s'",
				  m_StepsTypeStr.c_str(),
				  m_pSong->m_sSongFileName.c_str());
	} else if (m_StepsTypeStr.empty()) {
		m_StepsTypeStr = GAMEMAN->GetStepsTypeInfo(m_StepsType).szName;
	}

	if (GetDifficulty() == Difficulty_Invalid) {
		SetDifficulty(StringToDifficulty(GetDescription()));
	}

	if (GetDifficulty() == Difficulty_Invalid) {
		if (GetMeter() == 1) {
			SetDifficulty(Difficulty_Beginner);
		} else if (GetMeter() <= 3) {
			SetDifficulty(Difficulty_Easy);
		} else if (GetMeter() <= 6) {
			SetDifficulty(Difficulty_Medium);
		} else {
			SetDifficulty(Difficulty_Hard);
		}
	}

	if (GetMeter() < 1) { // meter is invalid
		SetMeter(static_cast<int>(1));
	}
}

void
Steps::CalculateRadarValues(float fMusicLengthSeconds)
{
	if (m_bAreCachedRadarValuesJustLoaded) {
		m_bAreCachedRadarValuesJustLoaded = false;
		return;
	}

	m_CachedRadarValues.Zero();

	// skip anything that uhh, doesn't have any notes to calculate radar values
	// from?
	if (m_pNoteData->GetNumTracks() == 0) {
		return;
	}

	// this is only ever called from copyfrom and recalculateradarvalues
	// the former is obsolete and in the case of the latter we know
	// the note data is decompressed already, so, we don't need to copy(?)
	// the decompressed note data, again, a decompress call can be placed here
	// instead of getnotedata if it turns out we need it -mina
	auto td = this->GetTimingData();
	GAMESTATE->SetProcessedTimingData(td);
	NoteDataUtil::CalculateRadarValues(
	  *m_pNoteData, fMusicLengthSeconds, m_CachedRadarValues, td);

	GAMESTATE->SetProcessedTimingData(nullptr);
}

void
Steps::Decompress() const
{
	const_cast<Steps*>(this)->Decompress();
}

void
Steps::Decompress()
{
	if (m_bNoteDataIsFilled) {
		return; // already decompressed
	}

	if (!m_sFilename.empty() && m_sNoteDataCompressed.empty()) {
		// We have NoteData on disk and not in memory. Load it.
		if (!this->GetNoteDataFromSimfile()) {
			LOG->Warn("Couldn't load the %s chart's NoteData from \"%s\"",
					  DifficultyToString(m_Difficulty).c_str(),
					  m_sFilename.c_str());
			return;
		}

		this->GetSMNoteData(m_sNoteDataCompressed);
	}

	if (m_sNoteDataCompressed.empty()) {
		/* there is no data, do nothing */
		return;
	}
	// load from compressed
	m_bNoteDataIsFilled = true;
	m_pNoteData->SetNumTracks(
	  GAMEMAN->GetStepsTypeInfo(m_StepsType).iNumTracks);
	NoteDataUtil::LoadFromSMNoteDataString(*m_pNoteData, m_sNoteDataCompressed);
}

auto
Steps::IsRecalcValid() -> bool
{
	if (m_StepsType != StepsType_dance_single &&
		m_StepsType != StepsType_dance_solo) {
		return false;
	}

	if (m_CachedRadarValues[RadarCategory_Notes] < 200 &&
		m_CachedRadarValues[RadarCategory_Notes] != 4) {
		return false;
	}

	return true;
}

auto
Steps::GetMSD(float rate, Skillset ss) const -> float
{
	if (rate > 2.f) // just extrapolate from 2x+
	{
		const float pDiff = diffByRate[13][ss];
		return pDiff + pDiff * ((rate - 2.f) * .5f);
	}

	int idx = static_cast<int>(rate * 10) - 7;
	float prop = fmod(rate * 10.f, 1.f);
	if (prop == 0 && rate <= 2.f) {
		return diffByRate[idx][ss];
	}

	const float pDiffL = diffByRate[idx][ss];
	const float pDiffH = diffByRate[idx + 1][ss];
	return lerp(prop, pDiffL, pDiffH);
}

auto
Steps::SortSkillsetsAtRate(float x, bool includeoverall)
  -> vector<pair<Skillset, float>>
{
	auto idx = static_cast<int>(x * 10) - 7;
	vector<float> tmp = diffByRate[idx];
	vector<pair<Skillset, float>> mort;
	FOREACH_ENUM(Skillset, ss)
	if (ss != Skill_Overall || includeoverall) {
		mort.emplace_back(ss, tmp[ss]);
	}
	std::sort(mort.begin(), mort.end(), [](auto& a, auto& b) -> bool {
		return a.second > b.second;
	});
	return mort;
}

void
Steps::CalcEtternaMetadata(Calc* calc)
{
	// keep nerv, it's needed for chartkey generation, etaner isn't
	const vector<NoteInfo>& cereal =
	  m_pNoteData->SerializeNoteData2(GetTimingData(), false);

	if (m_StepsType == StepsType_dance_solo) {
		diffByRate = SoloCalc(cereal);
	} else if (m_StepsType == StepsType_dance_single) {
		if (calc == nullptr) {
			// reloading at music select
			diffByRate = MinaSDCalc(cereal, SONGMAN->calc.get());
		} else {
			diffByRate = MinaSDCalc(cereal, calc);
		}
	}

	ChartKey = GenerateChartKey(*m_pNoteData, GetTimingData());

	// set first and last second for this steps object
	if (!cereal.empty()) {
		firstsecond = cereal[0].rowTime;
		lastsecond =
		  GetTimingData()->GetElapsedTimeFromBeat(m_pNoteData->GetLastBeat());
	}

	m_pNoteData->UnsetNerv();
	m_pNoteData->UnsetSerializedNoteData();
}

auto
Steps::DoATestThing(float ev, Skillset ss, float rate, Calc* calc) -> float
{
	// This is 4k only
	if (m_StepsType != StepsType_dance_single) {
		return 0.f;
	}
	auto& vh =
	  SONGMAN->testChartList[ss].filemapping.at(ChartKey).version_history;

	Decompress();
	const vector<int>& nerv = m_pNoteData->BuildAndGetNerv(GetTimingData());
	const vector<float>& etaner = GetTimingData()->BuildAndGetEtaner(nerv);
	const vector<NoteInfo>& cereal = m_pNoteData->SerializeNoteData(etaner);

	auto newcalc = MinaSDCalc(cereal, rate, 0.93f, calc);
	float last_msd = newcalc[ss];
	int prev_vers = GetCalcVersion() - 1;
	if (vh.count(prev_vers) != 0u) {
		last_msd = vh.at(prev_vers);
	}
	LOG->Trace("%0.2f : %0.2fx : %+0.2f : (%+06.2f%%) : %+0.2f : %s",
			   newcalc[ss],
			   rate,
			   newcalc[ss] - ev,
			   (newcalc[ss] - ev) / ev * 100.f,
			   newcalc[ss] - last_msd,
			   m_pSong->GetMainTitle().c_str());

	vh.emplace(pair<int, float>(GetCalcVersion(), newcalc[ss]));
	m_pNoteData->UnsetNerv();
	m_pNoteData->UnsetSerializedNoteData();
	GetTimingData()->UnsetEtaner();
	Compress();
	return newcalc[ss] - ev;
}

void
Steps::GetCalcDebugOutput()
{
	// makes calc display not update with rate changes
	// don't feel like making this fancy and it's fast
	// enough now i guess
	// if (!calcdebugoutput.empty())
	//	return;
	calcdebugoutput.clear();
	// function is responsible for producing debug output

	// This is 4k only
	if (m_StepsType != StepsType_dance_single) {
		return;
	}

	Decompress();
	const vector<NoteInfo>& cereal =
	  m_pNoteData->SerializeNoteData2(GetTimingData());

	MinaSDCalcDebug(cereal,
					GAMESTATE->m_SongOptions.GetSong().m_fMusicRate,
					0.93f,
					calcdebugoutput,
					debugstrings,
					*SONGMAN->calc.get());

	m_pNoteData->UnsetNerv();
	m_pNoteData->UnsetSerializedNoteData();
	GetTimingData()->UnsetEtaner();
	Compress();
}

void
Steps::UnloadCalcDebugOutput()
{
	calcdebugoutput.clear();
	calcdebugoutput.shrink_to_fit();
	debugstrings.clear();
	debugstrings.shrink_to_fit();
}

void
FillStringWithBPMs(size_t startRow,
				   size_t endRow,
				   const vector<int>& nerv,
				   const NoteData& nd,
				   TimingData* td,
				   std::string& inOut)
{
	float bpm = 0.f;
	for (size_t r = startRow; r < endRow; r++) {
		int row = nerv[r];
		for (int t = 0; t < nd.GetNumTracks(); ++t) {
			const TapNote& tn = nd.GetTapNote(t, row);
			inOut.append(to_string(tn.type));
		}
		bpm = td->GetBPMAtRow(row);
		inOut.append(to_string(static_cast<int>(bpm + 0.374643f)));
	}
}

auto
Steps::GenerateChartKey(NoteData& nd, TimingData* td) -> std::string
{
	std::string k = "";
	vector<int>& nerv = nd.GetNonEmptyRowVector();

	FillStringWithBPMs(0, nerv.size(), nerv, nd, td, k);

	return "X" + BinaryToHex(CryptManager::GetSHA1ForString(k));
}

void
Steps::Compress() const
{
	if (!m_sFilename.empty()) {
		/* We have a file on disk; clear all data in memory.
		 * Data on profiles can't be accessed normally (need to mount and
		 * time-out the device), and when we start a game and load edits, we
		 * want to be sure that it'll be available if the user picks it and
		 * pulls the device. Also, Decompress() doesn't know how to load .edits.
		 */
		m_pNoteData->Init();
		m_bNoteDataIsFilled = false;

		m_sNoteDataCompressed = "";
		m_sNoteDataCompressed.shrink_to_fit();
		return;
	}

	// We have no file on disk. Compress the data, if necessary.
	if (m_sNoteDataCompressed.empty()) {
		if (!m_bNoteDataIsFilled) {
			return; /* no data is no data */
		}
		NoteDataUtil::GetSMNoteDataString(*m_pNoteData, m_sNoteDataCompressed);
	}

	m_pNoteData->Init();
	m_bNoteDataIsFilled = false;
}

void
Steps::CopyFrom(Steps* pSource,
				StepsType ntTo,
				float fMusicLengthSeconds) // pSource does not have to be of the
										   // same StepsType
{
	m_StepsType = ntTo;
	m_StepsTypeStr = GAMEMAN->GetStepsTypeInfo(ntTo).szName;
	NoteData noteData;
	pSource->GetNoteData(noteData);
	noteData.SetNumTracks(GAMEMAN->GetStepsTypeInfo(ntTo).iNumTracks);
	m_Timing = pSource->m_Timing;
	this->m_pSong = pSource->m_pSong;
	this->SetNoteData(noteData);
	this->SetDescription(pSource->GetDescription());
	this->SetDifficulty(pSource->GetDifficulty());
	this->SetMeter(pSource->GetMeter());
	this->CalculateRadarValues(fMusicLengthSeconds);
}

void
Steps::CreateBlank(StepsType ntTo)
{
	m_StepsType = ntTo;
	m_StepsTypeStr = GAMEMAN->GetStepsTypeInfo(ntTo).szName;
	NoteData noteData;
	noteData.SetNumTracks(GAMEMAN->GetStepsTypeInfo(ntTo).iNumTracks);
	this->SetNoteData(noteData);
}

void
Steps::SetDifficultyAndDescription(Difficulty dc,
								   const std::string& sDescription)
{
	m_Difficulty = dc;
	m_sDescription = sDescription;
	if (GetDifficulty() == Difficulty_Edit) {
		MakeValidEditDescription(m_sDescription);
	}
}

void
Steps::SetCredit(const std::string& sCredit)
{
	m_sCredit = sCredit;
}

void
Steps::SetChartStyle(const std::string& sChartStyle)
{
	m_sChartStyle = sChartStyle;
}

auto
Steps::MakeValidEditDescription(std::string& sPreferredDescription) -> bool
{
	if (static_cast<int>(sPreferredDescription.size()) >
		MAX_STEPS_DESCRIPTION_LENGTH) {
		sPreferredDescription =
		  sPreferredDescription.substr(0, MAX_STEPS_DESCRIPTION_LENGTH);
		return true;
	}
	return false;
}

void
Steps::SetMeter(int meter)
{
	m_iMeter = meter;
}

auto
Steps::GetTimingData() const -> const TimingData*
{
	return m_Timing.empty() ? &m_pSong->m_SongTiming : &m_Timing;
}

auto
Steps::HasSignificantTimingChanges() const -> bool
{
	const TimingData* timing = GetTimingData();
	if (timing->HasStops() || timing->HasDelays() || timing->HasWarps() ||
		timing->HasSpeedChanges() || timing->HasScrollChanges()) {
		return true;
	}

	if (timing->HasBpmChanges()) {
		// check to see if these changes are significant.
		if ((GetMaxBPM() - GetMinBPM()) > 3.000f) {
			return true;
		}
	}

	return false;
}

auto
Steps::GetMusicPath() const -> const std::string
{
	return Song::GetSongAssetPath(m_MusicFile.empty() ? m_pSong->m_sMusicFile
													  : m_MusicFile,
								  m_pSong->GetSongDir());
}

auto
Steps::GetMusicFile() const -> const std::string&
{
	return m_MusicFile;
}

void
Steps::SetMusicFile(const std::string& file)
{
	m_MusicFile = file;
}

void
Steps::SetCachedRadarValues(const RadarValues& rv)
{
	m_CachedRadarValues = rv;
	m_bAreCachedRadarValuesJustLoaded = true;
}

auto
Steps::GetNPSVector(const NoteData& nd,
					const vector<int>& nerv,
					const vector<float>& etaner,
					float rate) -> vector<int>
{
	vector<int> doot(static_cast<int>(etaner.back()));
	int notecounter = 0;
	int lastinterval = 0;
	int curinterval = 0;

	for (size_t i = 0; i < nerv.size(); ++i) {
		curinterval = static_cast<int>(etaner[i] / rate);
		if (curinterval > lastinterval) {
			doot[lastinterval] = notecounter;
			notecounter = 0;
			lastinterval = static_cast<int>(curinterval);
		}

		for (int t = 0; t < nd.GetNumTracks(); ++t) {
			const TapNote& tn = nd.GetTapNote(t, nerv[i]);
			if (tn.type == TapNoteType_Tap || tn.type == TapNoteType_HoldHead) {
				++notecounter;
			}
		}
	}
	return doot;
}

auto
Steps::GetCNPSVector(const NoteData& nd,
					 const vector<int>& nerv,
					 const vector<float>& etaner,
					 int chordsize,
					 float rate) -> vector<int>
{
	vector<int> doot(static_cast<int>(etaner.back()));
	int chordnotecounter = 0; // number of NOTES inside chords of this size, so
							  // 5 jumps = 10 notes, 3 hands = 9 notes, etc
	int lastinterval = 0;
	int curinterval = 0;

	for (size_t i = 0; i < nerv.size(); ++i) {
		curinterval = static_cast<int>(etaner[i] / rate);
		if (curinterval > lastinterval) {
			doot[lastinterval] = chordnotecounter;
			chordnotecounter = 0;
			lastinterval = static_cast<int>(curinterval);
		}
		int notesinchord = 0;
		for (int t = 0; t < nd.GetNumTracks(); ++t) {
			const TapNote& tn = nd.GetTapNote(t, nerv[i]);
			if (tn.type == TapNoteType_Tap || tn.type == TapNoteType_HoldHead) {
				++notesinchord;
			}
		}
		if (notesinchord == chordsize) {
			chordnotecounter += notesinchord;
		}
	}
	return doot;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"
/** @brief Allow Lua to have access to the Steps. */
class LunaSteps : public Luna<Steps>
{
  public:
	DEFINE_METHOD(GetStepsType, m_StepsType)
	DEFINE_METHOD(GetDifficulty, GetDifficulty())
	DEFINE_METHOD(GetDescription, GetDescription())
	DEFINE_METHOD(GetChartStyle, GetChartStyle())
	DEFINE_METHOD(GetAuthorCredit, GetCredit())
	DEFINE_METHOD(GetMeter, GetMeter())
	DEFINE_METHOD(GetFilename, GetFilename())
	DEFINE_METHOD(IsAnEdit, IsAnEdit())
	DEFINE_METHOD(IsAPlayerEdit, IsAPlayerEdit())

	static auto HasSignificantTimingChanges(T* p, lua_State* L) -> int
	{
		lua_pushboolean(L, static_cast<int>(p->HasSignificantTimingChanges()));
		return 1;
	}
	static auto HasAttacks(T* /*p*/, lua_State* L) -> int
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	static auto GetRadarValues(T* p, lua_State* L) -> int
	{
		PlayerNumber pn = PLAYER_1;
		if (!lua_isnil(L, 1)) {
			pn = PLAYER_1;
		}

		auto& rv = const_cast<RadarValues&>(p->GetRadarValues());
		rv.PushSelf(L);
		return 1;
	}
	// Sigh -Mina
	static auto GetRelevantRadars(T* p, lua_State* L) -> int
	{
		vector<int> relevants;
		const RadarValues& rv = p->GetRadarValues();
		relevants.emplace_back(rv[0]); // notes
		relevants.emplace_back(rv[2]); // jumps
		relevants.emplace_back(rv[5]); // hands
		relevants.emplace_back(rv[3]); // holds
		relevants.emplace_back(rv[4]); // mines
		relevants.emplace_back(rv[6]); // rolls
		relevants.emplace_back(rv[7]); // lifts
		relevants.emplace_back(rv[8]); // fakes

		LuaHelpers::CreateTableFromArray(relevants, L);
		return 1;
	}
	static auto GetTimingData(T* p, lua_State* L) -> int
	{
		p->GetTimingData()->PushSelf(L);
		return 1;
	}
	static auto GetHash(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetHash());
		return 1;
	}
	// untested
	/*
	static int GetSMNoteData( T* p, lua_State *L )
	{
		RString out;
		p->GetSMNoteData( out );
		lua_pushstring( L, out );
		return 1;
	}
	*/
	static auto GetChartName(T* p, lua_State* L) -> int
	{
		lua_pushstring(L, p->GetChartName().c_str());
		return 1;
	}
	static auto GetDisplayBpms(T* p, lua_State* L) -> int
	{
		DisplayBpms temp;
		p->GetDisplayBpms(temp);
		float fMin = temp.GetMin();
		float fMax = temp.GetMax();
		vector<float> fBPMs;
		fBPMs.push_back(fMin);
		fBPMs.push_back(fMax);
		LuaHelpers::CreateTableFromArray(fBPMs, L);
		return 1;
	}
	static auto IsDisplayBpmSecret(T* p, lua_State* L) -> int
	{
		DisplayBpms temp;
		p->GetDisplayBpms(temp);
		lua_pushboolean(L, static_cast<int>(temp.IsSecret()));
		return 1;
	}
	static auto IsDisplayBpmConstant(T* p, lua_State* L) -> int
	{
		DisplayBpms temp;
		p->GetDisplayBpms(temp);
		lua_pushboolean(L, static_cast<int>(temp.BpmIsConstant()));
		return 1;
	}
	static auto IsDisplayBpmRandom(T* p, lua_State* L) -> int
	{
		lua_pushboolean(
		  L, static_cast<int>(p->GetDisplayBPM() == DISPLAY_BPM_RANDOM));
		return 1;
	}

	static auto GetDisplayBPMType(T* p, lua_State* L) -> int
	{
		LuaHelpers::Push(L, p->GetDisplayBPM());
		return 1;
	}

	static auto GetChartKey(T* p, lua_State* L) -> int
	{
		lua_pushstring(L, p->GetChartKey().c_str());
		return 1;
	}

	static auto GetMSD(T* p, lua_State* L) -> int
	{
		float rate = FArg(1);
		auto index = static_cast<Skillset>(IArg(2) - 1);
		CLAMP(rate, 0.7f, 3.f);
		lua_pushnumber(L, p->GetMSD(rate, index));
		return 1;
	}

	static auto GetSSRs(T* p, lua_State* L) -> int
	{
		float rate = FArg(1);
		float goal = FArg(2);
		CLAMP(rate, 0.7f, 3.f);
		auto nd = p->GetNoteData();
		auto loot = nd.BuildAndGetNerv(p->GetTimingData());
		const vector<float>& etaner =
		  p->GetTimingData()->BuildAndGetEtaner(loot);
		auto& ni = nd.SerializeNoteData(etaner);
		if (ni.empty()) {
			return 0;
		}
		std::vector<float> d;

		if (p->m_StepsType == StepsType_dance_solo) {
			d = SoloCalc(ni, rate, goal);
		} else {
			d = MinaSDCalc(ni, rate, goal, SONGMAN->calc.get());
		}

		auto ssrs = d;
		LuaHelpers::CreateTableFromArray(ssrs, L);
		return 1;
	}
	static auto GetRelevantSkillsetsByMSDRank(T* p, lua_State* L) -> int
	{
		float rate = FArg(1);
		CLAMP(rate, 0.7f, 2.f);
		int rank = IArg(2) - 1; // indexing
		auto sortedskillsets = p->SortSkillsetsAtRate(rate, false);
		float relevance_cutoff = 0.9f;
		float rval = sortedskillsets[rank].second;
		float highval = sortedskillsets[0].second;
		if (rank == 0) {
			lua_pushstring(L, SkillsetToString(sortedskillsets[0].first));
		} else if (rval > highval * relevance_cutoff) {
			lua_pushstring(L, SkillsetToString(sortedskillsets[rank].first));
		} else {
			lua_pushstring(L, "");
		}
		return 1;
	}
	static auto GetNonEmptyNoteData(T* p, lua_State* L) -> int
	{
		lua_newtable(L);
		auto nd = p->GetNoteData();
		auto loot = nd.BuildAndGetNerv(p->GetTimingData());

		LuaHelpers::CreateTableFromArray(
		  loot, L); // row (we need timestamps technically)
		lua_rawseti(L, -2, 1);

		for (int i = 0; i < nd.GetNumTracks(); ++i) { // tap or not
			vector<int> doot;
			for (auto r : loot) {
				auto tn = nd.GetTapNote(i, r);
				if (tn.type == TapNoteType_Empty) {
					doot.push_back(0);
				} else if (tn.type == TapNoteType_Tap) {
					doot.push_back(1);
				}
			}
			LuaHelpers::CreateTableFromArray(doot, L);
			lua_rawseti(L, -2, i + 2);
		}

		vector<int> doot;
		for (auto r : loot) {
			doot.push_back(static_cast<int>(GetNoteType(r)) + 1); // note denom
			LuaHelpers::CreateTableFromArray(doot, L);
			lua_rawseti(L, -2, 6);
		}

		nd.UnsetNerv();
		return 1;
	}
	static auto GetCDGraphVectors(T* p, lua_State* L) -> int
	{
		float rate = FArg(1);
		CLAMP(rate, 1.f, 3.f);
		auto nd = p->GetNoteData();
		if (nd.IsEmpty()) {
			return 0;
		}
		vector<int> nerv = nd.BuildAndGetNerv(p->GetTimingData());
		if (nerv.back() != nd.GetLastRow()) {
			nerv.emplace_back(nd.GetLastRow());
		}
		const vector<float>& etaner =
		  p->GetTimingData()->BuildAndGetEtaner(nerv);

		// directly using CreateTableFromArray(p->GetNPSVector(nd, nerv,
		// etaner), L) produced tables full of 0 values for ???? reason -mina
		vector<int> scroot = p->GetNPSVector(nd, nerv, etaner, rate);
		lua_newtable(L);
		LuaHelpers::CreateTableFromArray(scroot, L);
		lua_rawseti(L, -2, 1);

		for (int i = 1; i < nd.GetNumTracks(); ++i) {
			scroot = p->GetCNPSVector(
			  nd,
			  nerv,
			  etaner,
			  i + 1,
			  rate); // sort of confusing: the luatable pos/chordsize are i + 1
			LuaHelpers::CreateTableFromArray(
			  scroot, L); // but we're iterating over tracks which are 0 indexed
			lua_rawseti(
			  L,
			  -2,
			  i +
				1); // so jumps are position 2 and 2 notes each when i = 1 -mina
		}
		nd.UnsetNerv();
		p->GetTimingData()->UnsetEtaner();
		return 1;
	}
	static auto GetNumColumns(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetNoteData().GetNumTracks());
		return 1;
	}
	static auto GetCalcDebugOutput(T* p, lua_State* L) -> int
	{
		p->GetCalcDebugOutput();
		lua_newtable(L);
		lua_pushstring(L, RString("CalcPatternMod"));
		lua_createtable(L, 0, NUM_CalcPatternMod);
		for (int i = 0; i < NUM_CalcPatternMod; ++i) {
			lua_pushstring(
			  L, CalcPatternModToString(static_cast<CalcPatternMod>(i)));
			lua_createtable(L, 0, 2);
			for (int j = 0; j < 2; ++j) {
				vector<float> poop;
				if (!p->calcdebugoutput.empty()) { // empty for non 4k
					if (!p->calcdebugoutput[j]
						   .empty()) { // empty for "garbage files"
						poop = p->calcdebugoutput[j][0][i];
					}
				}
				LuaHelpers::CreateTableFromArray(poop, L);
				lua_rawseti(L, -2, j + 1);
			}
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);

		lua_pushstring(L, RString("CalcDiffValue"));
		lua_createtable(L, 0, NUM_CalcDiffValue);
		for (int i = 0; i < NUM_CalcDiffValue; ++i) {
			lua_pushstring(
			  L, CalcDiffValueToString(static_cast<CalcDiffValue>(i)));
			lua_createtable(L, 0, 2);
			for (int j = 0; j < 2; ++j) {
				vector<float> poop;
				if (!p->calcdebugoutput.empty()) { // empty for non 4k
					if (!p->calcdebugoutput[j]
						   .empty()) { // empty for "garbage files"
						poop = p->calcdebugoutput[j][1][i];
					}
				}
				LuaHelpers::CreateTableFromArray(poop, L);
				lua_rawseti(L, -2, j + 1);
			}
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);

		lua_pushstring(L, RString("CalcDebugMisc"));
		lua_createtable(L, 0, NUM_CalcDebugMisc);
		for (int i = 0; i < NUM_CalcDebugMisc; ++i) {
			lua_pushstring(
			  L, CalcDebugMiscToString(static_cast<CalcDebugMisc>(i)));
			lua_createtable(L, 0, 2);
			for (int j = 0; j < 2; ++j) {
				vector<float> poop;
				if (!p->calcdebugoutput.empty()) { // empty for non 4k
					if (!p->calcdebugoutput[j]
						   .empty()) { // empty for "garbage files"
						poop = p->calcdebugoutput[j][2][i];
					}
				}
				LuaHelpers::CreateTableFromArray(poop, L);
				lua_rawseti(L, -2, j + 1);
			}
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
		return 1;
	}
	static auto GetDebugStrings(T* p, lua_State* L) -> int
	{
		LuaHelpers::CreateTableFromArray(p->Getdebugstrings(), L);
		return 1;
	}
	LunaSteps()
	{
		ADD_METHOD(GetAuthorCredit);
		ADD_METHOD(GetChartStyle);
		ADD_METHOD(GetDescription);
		ADD_METHOD(GetDifficulty);
		ADD_METHOD(GetFilename);
		ADD_METHOD(GetHash);
		ADD_METHOD(GetMeter);
		ADD_METHOD(HasSignificantTimingChanges);
		ADD_METHOD(HasAttacks);
		ADD_METHOD(GetRadarValues);
		ADD_METHOD(GetRelevantRadars);
		ADD_METHOD(GetTimingData);
		ADD_METHOD(GetChartName);
		// ADD_METHOD( GetSMNoteData );
		ADD_METHOD(GetStepsType);
		ADD_METHOD(GetChartKey);
		ADD_METHOD(GetMSD);
		ADD_METHOD(GetSSRs);
		ADD_METHOD(IsAnEdit);
		ADD_METHOD(IsAPlayerEdit);
		ADD_METHOD(GetDisplayBpms);
		ADD_METHOD(IsDisplayBpmSecret);
		ADD_METHOD(IsDisplayBpmConstant);
		ADD_METHOD(IsDisplayBpmRandom);
		ADD_METHOD(GetDisplayBPMType);
		ADD_METHOD(GetRelevantSkillsetsByMSDRank);
		ADD_METHOD(GetCDGraphVectors);
		ADD_METHOD(GetNumColumns);
		ADD_METHOD(GetNonEmptyNoteData);
		ADD_METHOD(GetCalcDebugOutput);
		ADD_METHOD(GetDebugStrings);
	}
};

LUA_REGISTER_CLASS(Steps)
// lua end
