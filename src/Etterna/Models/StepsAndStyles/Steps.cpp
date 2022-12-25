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
#include "Etterna/MinaCalc/MinaCalc.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/NoteData/NoteDataUtil.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderBMS.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderDWI.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderKSF.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderOSU.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSM.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSMA.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSSC.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Singletons/FilterManager.h"

#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include "Etterna/Globals/SoloCalc.h"

/* register DisplayBPM with StringConversion */
#include "Etterna/Models/Misc/EnumHelper.h"

// For hashing wife chart keys - Mina
#include "Etterna/Singletons/CryptManager.h"

#include <algorithm>
#include <thread>

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
  , m_pNoteData()
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
auto Steps::operator=(const Steps &) -> Steps& = default;

void
Steps::GetDisplayBpms(DisplayBpms& AddTo, bool bIgnoreCurrentRate) const
{
	const auto demratesboiz =
	  bIgnoreCurrentRate ? 1.F
						 : GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	if (this->GetDisplayBPM() == DISPLAY_BPM_SPECIFIED) {
		AddTo.Add(this->GetMinBPM() * demratesboiz);
		AddTo.Add(this->GetMaxBPM() * demratesboiz);
	} else {
		float fMinBPM;
		float fMaxBPM;
		this->GetTimingData()->GetActualBPM(fMinBPM, fMaxBPM);
		AddTo.Add(fMinBPM * demratesboiz);
		AddTo.Add(fMaxBPM * demratesboiz);
	}
}

auto
Steps::GetHash() const -> unsigned
{
	if (m_iHash != 0U) {
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
	auto stepFile = this->GetFilename();
	auto extension = make_lower(GetExtension(stepFile));

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
			auto transformedStepFile = stepFile;
			s_replace(transformedStepFile, ".ssc", ".sm");

			return backup_loader.LoadNoteDataFromSimfile(transformedStepFile,
														 *this);
		}
		return true;
	}
	if (extension == "sm") {
		SMLoader loader;
		return loader.LoadNoteDataFromSimfile(stepFile, *this);
	}
	if (extension == "sma") {
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
Steps::SetNoteData(const NoteData& noteDataNew) const
{
	ASSERT(noteDataNew.GetNumTracks() ==
		   GAMEMAN->GetStepsTypeInfo(m_StepsType).iNumTracks);

	*m_pNoteData = noteDataNew;
	m_bNoteDataIsFilled = true;

	m_sNoteDataCompressed = std::string();
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
Steps::SetSMNoteData(const std::string& notes_comp_)
{
	m_pNoteData->Init();
	m_bNoteDataIsFilled = false;

	m_sNoteDataCompressed = notes_comp_;
	m_iHash = 0;
}

/* XXX: this function should pull data from m_sFilename, like Decompress() */
void
Steps::GetSMNoteData(std::string& notes_comp_out) const
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
Steps::GetETTNoteData(std::string& notes_comp_out) const
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
        Locator::getLogger()->debug("Detected steps with unknown style '{}' in '{}'",
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
Steps::CalculateRadarValues()
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
	const auto td = this->GetTimingData();
	GAMESTATE->SetProcessedTimingData(td);
	NoteDataUtil::CalculateRadarValues(*m_pNoteData, m_CachedRadarValues, td);

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
            Locator::getLogger()->warn("Couldn't load the {} chart's NoteData from \"{}\"",
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
	if (m_CachedRadarValues[RadarCategory_Notes] < 200 &&
		m_CachedRadarValues[RadarCategory_Notes] != 4) {
		return false;
	}

	return true;
}

auto
Steps::IsSkillsetHighestOfChart(Skillset skill, float rate) -> bool
{
	auto sorted_skills = SortSkillsetsAtRate(rate, false);
	return (sorted_skills[0].first == skill);
}

auto
Steps::GetMSD(float rate, Skillset ss) const -> float
{
	if (rate > 2.F) // extrapolate from 2x+
	{
		const auto pDiff = diffByRate[13][ss];
		return pDiff + pDiff * ((rate - 2.F) * .5F);
	}
	if (rate < .7F) { // extrapolate from .7x- steeper
		const auto pDiff = diffByRate[0][ss];
		return std::max(pDiff - pDiff * ((.7F - rate)), 0.F);
	}

	// return whole rates by the cached value
	const auto idx = static_cast<int>(rate * 10) - 7;
	const auto prop = fmod(rate * 10.F, 1.F);
	if (prop == 0 && rate <= 2.F) {
		return diffByRate[idx][ss];
	}

	// linear interpolate half rates using surrounding cached values
	const auto pDiffL = diffByRate[idx][ss];
	const auto pDiffH = diffByRate[idx + 1][ss];
	return lerp(prop, pDiffL, pDiffH);
}

auto
Steps::SortSkillsetsAtRate(float x, bool includeoverall)
  -> std::vector<std::pair<Skillset, float>>
{
	const auto idx = static_cast<int>(x * 10) - 7;
	auto tmp = diffByRate[idx];
	std::vector<std::pair<Skillset, float>> mort;
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
	const auto& cereal =
	  m_pNoteData->SerializeNoteData2(GetTimingData(), false);

	if (m_StepsType != StepsType_dance_single) {
		int columnCount =
		  GAMEMAN->GetStepsTypeInfo(m_StepsType).iNumTracks;
		diffByRate = SoloCalc(cereal, columnCount);
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
	if (!cereal.empty() || !m_pNoteData->IsEmpty()) {
		firstsecond =
		  GetTimingData()->GetElapsedTimeFromBeat(m_pNoteData->GetFirstBeat());
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
		return 0.F;
	}
	auto& vh =
	  SONGMAN->testChartList[ss].filemapping.at(ChartKey).version_history;

	Decompress();
	const auto& nerv = m_pNoteData->BuildAndGetNerv(GetTimingData());
	const auto& etaner = GetTimingData()->BuildAndGetEtaner(nerv);
	const auto& cereal = m_pNoteData->SerializeNoteData(etaner);

	auto newcalc = MinaSDCalc(cereal, rate, 0.93F, calc);
	auto last_msd = newcalc[ss];
	const auto prev_vers = GetCalcVersion() - 1;
	if (vh.count(prev_vers) != 0U) {
		last_msd = vh.at(prev_vers);
	}
	Locator::getLogger()->info("{:+.2f} : {:+.2f} : {:+.2f} : ({:+06.2f}%) : {:+.2f} : {}",
			   newcalc[ss],
			   rate,
			   newcalc[ss] - ev,
			   (newcalc[ss] - ev) / ev * 100.F,
			   newcalc[ss] - last_msd,
			   m_pSong->GetMainTitle().c_str());

	vh.emplace(std::pair<int, float>(GetCalcVersion(), newcalc[ss]));
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
	const auto& cereal = m_pNoteData->SerializeNoteData2(GetTimingData());

	MinaSDCalcDebug(cereal,
					GAMESTATE->m_SongOptions.GetSong().m_fMusicRate,
					0.93F,
					calcdebugoutput,
					debugstrings,
					*SONGMAN->calc);

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
FillStringWithBPMs(const int startRow,
				   const int endRow,
				   const std::vector<int>& nerv,
				   const NoteData& nd,
				   TimingData* td,
				   std::string& inOut)
{
	for (auto r = startRow; r < endRow; r++) {
		auto row = nerv[r];
		for (auto t = 0; t < nd.GetNumTracks(); ++t) {
			const auto& tn = nd.GetTapNote(t, row);
			inOut.append(std::to_string(tn.type));
		}
		const auto bpm = td->GetBPMAtRow(row);
		inOut.append(std::to_string(static_cast<int>(bpm + 0.374643F)));
	}
}

auto
Steps::GenerateChartKey(NoteData& nd, TimingData* td) -> std::string
{
	std::string k;
	auto& nerv = nd.GetNonEmptyRowVector();

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
				StepsType ntTo) // pSource does not have to be of the
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
	this->CalculateRadarValues();
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
	const auto timing = GetTimingData();
	if (timing->HasStops() || timing->HasDelays() || timing->HasWarps() ||
		timing->HasSpeedChanges() || timing->HasScrollChanges()) {
		return true;
	}

	if (timing->HasBpmChanges()) {
		// check to see if these changes are significant.
		if ((GetMaxBPM() - GetMinBPM()) > 3.000F) {
			return true;
		}
	}

	return false;
}

auto
Steps::IsPlayableForCurrentGame() const -> bool
{
	std::vector<StepsType> types;
	GAMEMAN->GetStepsTypesForGame(GAMESTATE->m_pCurGame, types);
	return find(types.begin(), types.end(), m_StepsType) != types.end();
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
					const std::vector<float>& etaner,
					const std::vector<int>& nerv,
					const float rate) -> std::vector<int>
{
	std::vector<int> doot(static_cast<int>(etaner.back()));
	auto notecounter = 0;
	auto lastinterval = 0;

	for (auto i = 0; i < static_cast<int>(nerv.size()); ++i) {
		const auto curinterval = static_cast<int>(etaner[i] / rate);
		if (curinterval > lastinterval) {
			doot[lastinterval] = notecounter;
			notecounter = 0;
			lastinterval = static_cast<int>(curinterval);
		}

		for (auto t = 0; t < nd.GetNumTracks(); ++t) {
			const auto& tn = nd.GetTapNote(t, nerv[i]);
			if (tn.type == TapNoteType_Tap || tn.type == TapNoteType_HoldHead) {
				++notecounter;
			}
		}
	}
	return doot;
}

// YEAH THIS IS LIKE, REALLY INEFFICIENT
auto
Steps::GetNPSPerMeasure(const NoteData& nd,
						const std::vector<float>& etaner,
						const std::vector<int>& nerv,
						const float rate) -> std::vector<float>
{
	std::vector<float> doot;

	auto* td = GetTimingData();
	const auto lastbeat = td->GetBeatFromElapsedTime(lastsecond);
	const auto lastmeasure = std::ceil(lastbeat / 4.F);

	for (auto i = 0; i < lastmeasure; ++i) {
		const auto m_start = td->GetElapsedTimeFromBeat(i * 4.F);
		const auto m_end =
		  td->GetElapsedTimeFromBeat(static_cast<float>(i + 1) * 4.F);
		const auto m_time = m_end - m_start;

		auto m_counter = 0;
		for (auto j = 0; j < static_cast<int>(nerv.size()); ++j) {
			if (etaner[j] > m_end) {
				continue;
			}

			if (etaner[j] > m_start) {
				for (auto t = 0; t < nd.GetNumTracks(); ++t) {
					const auto& tn = nd.GetTapNote(t, nerv[j]);
					if (tn.type == TapNoteType_Tap ||
						tn.type == TapNoteType_HoldHead) {
						++m_counter;
					}
				}
			}
		}

		doot.emplace_back(static_cast<float>(m_counter) / m_time);
	}

	return doot;
}

auto
Steps::GetCNPSVector(const NoteData& nd,
					 const std::vector<int>& nerv,
					 const std::vector<float>& etaner,
					 const int chordsize,
					 const float rate) -> std::vector<int>
{
	std::vector<int> doot(static_cast<int>(etaner.back()));
	auto chordnotecounter = 0; // number of NOTES inside chords of this size, so
							   // 5 jumps = 10 notes, 3 hands = 9 notes, etc
	auto lastinterval = 0;

	for (auto i = 0; i < static_cast<int>(nerv.size()); ++i) {
		const auto curinterval = static_cast<int>(etaner[i] / rate);
		if (curinterval > lastinterval) {
			doot[lastinterval] = chordnotecounter;
			chordnotecounter = 0;
			lastinterval = static_cast<int>(curinterval);
		}
		auto notesinchord = 0;
		for (auto t = 0; t < nd.GetNumTracks(); ++t) {
			const auto& tn = nd.GetTapNote(t, nerv[i]);
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

	static auto HasSignificantTimingChanges(T* p, lua_State* L) -> int
	{
		lua_pushboolean(L, static_cast<int>(p->HasSignificantTimingChanges()));
		return 1;
	}
	static auto GetRadarValues(T* p, lua_State* L) -> int
	{
		auto& rv = const_cast<RadarValues&>(p->GetRadarValues());
		rv.PushSelf(L);
		return 1;
	}

	// Convenience reorder so lua can use a simple loop
	static auto GetRelevantRadars(T* p, lua_State* L) -> int
	{
		std::vector<int> relevants;
		const auto& rv = p->GetRadarValues();
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
	static auto GetChartName(T* p, lua_State* L) -> int
	{
		lua_pushstring(L, p->GetChartName().c_str());
		return 1;
	}
	static auto GetDisplayBpms(T* p, lua_State* L) -> int
	{
		DisplayBpms temp;
		bool bIgnore = false;
		if (!lua_isnoneornil(L, 1)) {
			bIgnore = BArg(1);
		}
		p->GetDisplayBpms(temp, bIgnore);
		const auto fMin = temp.GetMin();
		const auto fMax = temp.GetMax();
		std::vector<float> fBPMs;
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
		const auto rate = FArg(1);
		const auto index = static_cast<Skillset>(IArg(2) - 1);
		lua_pushnumber(L, p->GetMSD(rate, index));
		return 1;
	}

	static auto GetSSRs(T* p, lua_State* L) -> int
	{
		const auto rate = std::clamp(FArg(1), 0.7F, 3.F);
		const auto goal = FArg(2);
		auto nd = p->GetNoteData();
		const auto loot = nd.BuildAndGetNerv(p->GetTimingData());
		const auto& etaner = p->GetTimingData()->BuildAndGetEtaner(loot);
		const auto& ni = nd.SerializeNoteData(etaner);
		if (ni.empty()) {
			return 0;
		}
		std::vector<float> d;

		if (p->m_StepsType != StepsType_dance_single) {
			int columnCount =
			  GAMEMAN->GetStepsTypeInfo(p->m_StepsType).iNumTracks;
			d = SoloCalc(ni, columnCount, rate, goal);
		} else {
			d = MinaSDCalc(ni, rate, goal, SONGMAN->calc.get());
		}

		const auto ssrs = d;
		LuaHelpers::CreateTableFromArray(ssrs, L);
		return 1;
	}
	static auto GetRelevantSkillsetsByMSDRank(T* p, lua_State* L) -> int
	{
		const auto rate = std::clamp(FArg(1), 0.7F, 2.F);
		const auto rank = IArg(2) - 1; // indexing
		auto sortedskillsets = p->SortSkillsetsAtRate(rate, false);
		const auto relevance_cutoff = 0.9F;
		const auto rval = sortedskillsets[rank].second;
		const auto highval = sortedskillsets[0].second;
		if (rank == 0) {
			lua_pushstring(L,
						   SkillsetToString(sortedskillsets[0].first).c_str());
		} else if (rval > highval * relevance_cutoff) {
			lua_pushstring(
			  L, SkillsetToString(sortedskillsets[rank].first).c_str());
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

		for (auto i = 0; i < nd.GetNumTracks(); ++i) { // tap or not
			std::vector<int> doot;
			for (auto r : loot) {
				const auto tn = nd.GetTapNote(i, r);
				if (tn.type == TapNoteType_Empty) {
					doot.push_back(0);
				} else if (tn.type == TapNoteType_Tap) {
					doot.push_back(1);
				}
			}
			LuaHelpers::CreateTableFromArray(doot, L);
			lua_rawseti(L, -2, i + 2);
		}

		std::vector<int> doot;
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
		const auto rate = std::clamp(FArg(1), 0.7F, 3.F);
		auto nd = p->GetNoteData();
		if (nd.IsEmpty()) {
			return 0;
		}
		auto nerv = nd.BuildAndGetNerv(p->GetTimingData());
		if (nerv.back() != nd.GetLastRow()) {
			nerv.emplace_back(nd.GetLastRow());
		}
		const auto& etaner = p->GetTimingData()->BuildAndGetEtaner(nerv);

		// directly using CreateTableFromArray(p->GetNPSVector(nd, nerv,
		// etaner), L) produced tables full of 0 values for ???? reason -mina
		auto scroot = p->GetNPSVector(nd, etaner, nerv, rate);
		lua_newtable(L);
		LuaHelpers::CreateTableFromArray(scroot, L);
		lua_rawseti(L, -2, 1);

		for (auto i = 1; i < nd.GetNumTracks(); ++i) {
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
	static auto GetCalcDebugJack(T* p, lua_State* L) -> int
	{
		lua_newtable(L);
		lua_pushstring(L, "JackHand");
		lua_createtable(L, 0, 2);
		if (p->calcdebugoutput.empty()) {
			for (auto hand = 0; hand < 2; hand++) {
				lua_pushstring(L, hand != 0 ? "Right" : "Left");
				std::vector<float> nothing;
				LuaHelpers::CreateTableFromArray(nothing, L);
				lua_rawset(L, -3);
			}
			return 1;
		}
		for (auto hand = 0; hand < 2; hand++) {
			lua_pushstring(L, hand != 0 ? "Right" : "Left");
			lua_createtable(L, 0, SONGMAN->calc->jack_diff.at(hand).size());
			auto vals = SONGMAN->calc->jack_diff.at(hand);
			auto stam_vals = SONGMAN->calc->jack_stam_stuff.at(hand);
			auto loss_vals = SONGMAN->calc->jack_loss.at(hand);
			for (size_t i = 0; i < vals.size() && i < stam_vals.size() &&
							   i < loss_vals.size();
				 i++) {
				auto v1 = vals[i].first;
				auto v2 = vals[i].second;
				auto v3 = 0.F;
				auto v4 = loss_vals[i];
				// this is required because stam_vals is not guaranteed the same size
				// also due to a calc bug
				if (i < stam_vals.size())
					v3 = stam_vals[i];
				std::vector<float> stuff{ v1, v2, v3, v4 };
				LuaHelpers::CreateTableFromArray(stuff, L);
				lua_rawseti(L, -2, i + 1);
			}
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
		return 1;
	}
	static auto GetCalcDebugExt(T* p, lua_State* L) -> int {
		lua_newtable(L);
		lua_pushstring(L, "DebugValues");
		if (p->calcdebugoutput.empty()) {
			lua_pushnil(L);
			return 1;
		}

		auto ff = [&](std::array<std::array<std::vector<float>, NUM_Skillset>,
								 num_hands>& debugArr) {
			lua_createtable(L, 0, 2);
			for (auto hand = 0; hand < num_hands; hand++) {
				lua_pushstring(L, hand != 0 ? "Right" : "Left");
				lua_createtable(L, 0, NUM_Skillset);
				FOREACH_ENUM(Skillset, ss)
				{
					auto vals = debugArr.at(hand).at(ss);
					LuaHelpers::CreateTableFromArray(vals, L);
					lua_rawseti(L, -2, ss + 1);
				}
				lua_rawset(L, -3);
			}
			lua_rawset(L, -3);
		};

		auto ff2 =
		  [&](std::array<std::array<std::vector<std::pair<float, float>>, 2>,
						 num_hands>& debugArr) {
			  lua_createtable(L, 0, 2);
			  for (auto hand = 0; hand < num_hands; hand++) {
				  lua_pushstring(L, hand != 0 ? "Right" : "Left");
				  lua_createtable(L, 0, 2);
				  for (auto col = 0; col < 2; col++) {
					  lua_pushstring(L, col != 0 ? "Right" : "Left");
					  lua_createtable(L, 0, debugArr.at(hand).at(col).size());
					  int i = 1;
					  for (auto& x : debugArr.at(hand).at(col)) {
						  std::vector<float> stuff{ x.first, x.second };
						  LuaHelpers::CreateTableFromArray(stuff, L);
						  lua_rawseti(L, -2, i++);
					  }
					  lua_rawset(L, -3);
				  }
				  lua_rawset(L, -3);
			  }
			  lua_rawset(L, -3);
		  };

		auto ff3 = [&](std::array<std::vector<std::array<float, 4>>, num_hands>&
						 debugArr) {
			lua_createtable(L, 0, 2);
			for (auto hand = 0; hand < num_hands; hand++) {
				lua_pushstring(L, hand != 0 ? "Right" : "Left");
				lua_createtable(L, 0, 2);
				int i = 1;
				for (auto& x : debugArr.at(hand)) {
					std::vector<float> stuff{ x[0], x[1], x[2], x[3] };
					LuaHelpers::CreateTableFromArray(stuff, L);
					lua_rawseti(L, -2, i++);
				}
				lua_rawset(L, -3);
			}
			lua_rawset(L, -3);
		};

		// debugMSD, debugPtLoss, debugTotalPatternMod
		lua_createtable(L, 0, 4);
		lua_pushstring(L, "DebugMSD");
		ff(SONGMAN->calc->debugMSD);
		lua_pushstring(L, "DebugPtLoss");
		ff(SONGMAN->calc->debugPtLoss);
		lua_pushstring(L, "DebugTotalPatternMod");
		ff(SONGMAN->calc->debugTotalPatternMod);

		// debugMovingWindowCV
		lua_pushstring(L, "DebugMovingWindowCV");
		ff2(SONGMAN->calc->debugMovingWindowCV);

		// debugTechVals
		lua_pushstring(L, "DebugTechVals");
		ff3(SONGMAN->calc->debugTechVals);

		return 1;
	}
	static auto GetCalcDebugOutput(T* p, lua_State* L) -> int
	{
		p->GetCalcDebugOutput();
		lua_newtable(L);
		lua_pushstring(L, "CalcPatternMod");
		lua_createtable(L, 0, NUM_CalcPatternMod);
		for (auto i = 0; i < NUM_CalcPatternMod; ++i) {
			lua_pushstring(
			  L,
			  CalcPatternModToString(static_cast<CalcPatternMod>(i)).c_str());
			lua_createtable(L, 0, 2);
			for (auto j = 0; j < 2; ++j) {
				std::vector<float> poop;
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

		lua_pushstring(L, "CalcDiffValue");
		lua_createtable(L, 0, NUM_CalcDiffValue);
		for (auto i = 0; i < NUM_CalcDiffValue; ++i) {
			lua_pushstring(
			  L, CalcDiffValueToString(static_cast<CalcDiffValue>(i)).c_str());
			lua_createtable(L, 0, 2);
			for (auto j = 0; j < 2; ++j) {
				std::vector<float> poop;
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

		lua_pushstring(L, "CalcDebugMisc");
		lua_createtable(L, 0, NUM_CalcDebugMisc);
		for (auto i = 0; i < NUM_CalcDebugMisc; ++i) {
			lua_pushstring(
			  L, CalcDebugMiscToString(static_cast<CalcDebugMisc>(i)).c_str());
			lua_createtable(L, 0, 2);
			for (auto j = 0; j < 2; ++j) {
				std::vector<float> poop;
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

		lua_pushstring(L, "Grindscaler");
		lua_pushnumber(L, SONGMAN->calc->grindscaler);
		lua_rawset(L, -3);

		return 1;
	}
	static auto GetDebugStrings(T* p, lua_State* L) -> int
	{
		LuaHelpers::CreateTableFromArray(p->Getdebugstrings(), L);
		return 1;
	}
	static auto GetLengthSeconds(T* p, lua_State* L) -> int
	{
		const auto curr_rate =
		  GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		lua_pushnumber(L, p->GetLengthSeconds(curr_rate));
		return 1;
	}
	static auto GetFirstSecond(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->firstsecond);
		return 1;
	}
	static auto GetLastSecond(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->lastsecond);
		return 1;
	}
	static auto GetNPSPerMeasure(T* p, lua_State* L) -> int
	{
		const auto rate = std::clamp(FArg(1), 0.7F, 3.F);

		auto nd = p->GetNoteData();
		if (nd.IsEmpty()) {
			return 0;
		}
		auto nerv = nd.BuildAndGetNerv(p->GetTimingData());
		if (nerv.back() != nd.GetLastRow()) {
			nerv.emplace_back(nd.GetLastRow());
		}
		const auto& etaner = p->GetTimingData()->BuildAndGetEtaner(nerv);

		auto ee = p->GetNPSPerMeasure(nd, etaner, nerv, rate);
		LuaHelpers::CreateTableFromArray(ee, L);
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
		ADD_METHOD(GetRadarValues);
		ADD_METHOD(GetRelevantRadars);
		ADD_METHOD(GetTimingData);
		ADD_METHOD(GetChartName);
		// ADD_METHOD( GetSMNoteData );
		ADD_METHOD(GetStepsType);
		ADD_METHOD(GetChartKey);
		ADD_METHOD(GetMSD);
		ADD_METHOD(GetSSRs);
		ADD_METHOD(GetDisplayBpms);
		ADD_METHOD(IsDisplayBpmSecret);
		ADD_METHOD(IsDisplayBpmConstant);
		ADD_METHOD(IsDisplayBpmRandom);
		ADD_METHOD(GetDisplayBPMType);
		ADD_METHOD(GetRelevantSkillsetsByMSDRank);
		ADD_METHOD(GetCDGraphVectors);
		ADD_METHOD(GetNumColumns);
		ADD_METHOD(GetNonEmptyNoteData);
		ADD_METHOD(GetCalcDebugJack);
		ADD_METHOD(GetCalcDebugExt);
		ADD_METHOD(GetCalcDebugOutput);
		ADD_METHOD(GetDebugStrings);
		ADD_METHOD(GetLengthSeconds);
		ADD_METHOD(GetFirstSecond);
		ADD_METHOD(GetLastSecond);
		ADD_METHOD(GetNPSPerMeasure);
	}
};

LUA_REGISTER_CLASS(Steps)
// lua end
