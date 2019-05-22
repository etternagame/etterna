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
#include <MinaCalc/MinaCalc.h>
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
		float fMinBPM, fMaxBPM;
		this->GetTimingData()->GetActualBPM(fMinBPM, fMaxBPM);
		AddTo.Add(fMinBPM);
		AddTo.Add(fMaxBPM);
	}
}

unsigned
Steps::GetHash() const
{
	if (m_iHash != 0u)
		return m_iHash;
	if (m_sNoteDataCompressed.empty()) {
		if (!m_bNoteDataIsFilled)
			return 0; // No data, no hash.
		NoteDataUtil::GetSMNoteDataString(*m_pNoteData, m_sNoteDataCompressed);
	}
	m_iHash = GetHashForString(m_sNoteDataCompressed);
	return m_iHash;
}

bool
Steps::IsNoteDataEmpty() const
{
	return this->m_sNoteDataCompressed.empty() && !m_bNoteDataIsFilled;
}

bool
Steps::GetNoteDataFromSimfile()
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
		} else {
			return true;
		}
	} else if (extension == "sm") {
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
		if (ldSSC.LoadNoteDataFromSimfile(stepFile, *this) != true) {
			SMLoader ldSM;
			return ldSM.LoadNoteDataFromSimfile(stepFile, *this);
		} else
			return true;
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

NoteData
Steps::GetNoteData() const
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
	} else if (m_StepsTypeStr == "") {
		m_StepsTypeStr = GAMEMAN->GetStepsTypeInfo(m_StepsType).szName;
	}

	if (GetDifficulty() == Difficulty_Invalid)
		SetDifficulty(StringToDifficulty(GetDescription()));

	if (GetDifficulty() == Difficulty_Invalid) {
		if (GetMeter() == 1)
			SetDifficulty(Difficulty_Beginner);
		else if (GetMeter() <= 3)
			SetDifficulty(Difficulty_Easy);
		else if (GetMeter() <= 6)
			SetDifficulty(Difficulty_Medium);
		else
			SetDifficulty(Difficulty_Hard);
	}

	if (GetMeter() < 1) // meter is invalid
		SetMeter(static_cast<int>(1));
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
	if (m_pNoteData->GetNumTracks() == 0)
		return;

	// this is only ever called from copyfrom and recalculateradarvalues
	// the former is obsolete and in the case of the latter we know
	// the note data is decompressed already, so, we don't need to copy(?)
	// the decompressed note data, again, a decompress call can be placed here
	// instead of getnotedata if it turns out we need it -mina
	auto td = this->GetTimingData();
	GAMESTATE->SetProcessedTimingData(td);
	NoteDataUtil::CalculateRadarValues(
	  *m_pNoteData, fMusicLengthSeconds, m_CachedRadarValues, td);

	GAMESTATE->SetProcessedTimingData(NULL);
}

void
Steps::Decompress() const
{
	const_cast<Steps*>(this)->Decompress();
}

void
Steps::Decompress()
{
	if (m_bNoteDataIsFilled)
		return; // already decompressed

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

bool
Steps::IsRecalcValid()
{
	if (m_StepsType != StepsType_dance_single)
		return false;

	if (m_CachedRadarValues[RadarCategory_Notes] < 200 &&
		m_CachedRadarValues[RadarCategory_Notes] != 4)
		return false;

	TimingData* td = GetTimingData();
	if (td->HasWarps())
		return false;

	return true;
}

float
Steps::GetMSD(float x, int i) const
{
	if (x > 2.f) // just extrapolate from 2x+
		return stuffnthings[13][i] + stuffnthings[13][i] * ((x - 2.f) * .5f);

	int idx = static_cast<int>(x * 10) - 7;
	float prop = fmod(x * 10.f, 1.f);
	if (prop == 0 && x <= 2.f)
		return stuffnthings[idx][i];
	return lerp(prop, stuffnthings[idx][i], stuffnthings[idx + 1][i]);
}

map<float, Skillset>
Steps::SortSkillsetsAtRate(float x, bool includeoverall)
{
	int idx = static_cast<int>(x * 10) - 7;
	map<float, Skillset> why;
	SDiffs tmp = stuffnthings[idx];
	FOREACH_ENUM(Skillset, ss)
	if (ss != Skill_Overall || includeoverall)
		why.emplace(tmp[ss], ss);
	return why;
}

void
Steps::CalcEtternaMetadata()
{
	const std::vector<int>& nerv = m_pNoteData->BuildAndGetNerv();
	const std::vector<float>& etaner = GetTimingData()->BuildAndGetEtaner(nerv);
	const std::vector<NoteInfo>& cereal = m_pNoteData->SerializeNoteData(etaner);

	stuffnthings = MinaSDCalc(cereal,
							  m_pNoteData->GetNumTracks(),
							  0.93f,
							  1.f,
							  GetTimingData()->HasWarps());

	// if (GetNoteData().GetNumTracks() == 4 && GetTimingData()->HasWarps() ==
	// false)  MinaCalc2(stuffnthings,
	// GetNoteData().SerializeNoteData2(etaner), 1.f, 0.93f);

	ChartKey = GenerateChartKey(*m_pNoteData, GetTimingData());

	// replace the old sm notedata string with the new ett notedata string
	// compressed format for internal use
	/*	Not yet though
	if (m_pNoteData->GetNumTracks() == 4 && m_StepsType ==
	StepsType_dance_single) NoteDataUtil::GetETTNoteDataString(*m_pNoteData,
	m_sNoteDataCompressed); else { m_sNoteDataCompressed = "";
	m_sNoteDataCompressed.shrink_to_fit();
	}
	*/

	// set first and last second for this steps object
	if (!etaner.empty()) {
		firstsecond = etaner.front();
		lastsecond =
		  GetTimingData()->GetElapsedTimeFromBeat(m_pNoteData->GetLastBeat());
	}

	m_pNoteData->UnsetNerv();
	m_pNoteData->UnsetSerializedNoteData();
	// m_pNoteData->UnsetSerializedNoteData2();
	GetTimingData()->UnsetEtaner();
}

RString
Steps::GenerateChartKey(NoteData& nd, TimingData* td)
{
	RString o = "X"; // I was thinking of using "C" to indicate chart..
					 // however.. X is cooler... - Mina
	RString k = "";
	std::vector<int>& nerv = nd.GetNonEmptyRowVector();

	unsigned int numThreads = max(std::thread::hardware_concurrency(), 1u);
	std::vector<RString> keyParts;
	keyParts.reserve(numThreads);

	size_t segmentSize = nerv.size() / numThreads;
	std::vector<std::thread> threads;
	threads.reserve(numThreads);

	for (unsigned int curThread = 0; curThread < numThreads; curThread++) {
		keyParts.push_back("");
		size_t start = segmentSize * curThread;
		size_t end = start + segmentSize;
		if (curThread + 1 == numThreads)
			end = nerv.size();

		threads.push_back(std::thread(&Steps::FillStringWithBPMs,
									  this,
									  start,
									  end,
									  std::ref(nerv),
									  std::ref(nd),
									  td,
									  std::ref(keyParts[curThread])));
	}

	for (auto& t : threads) {
		if (t.joinable())
			t.join();
	}

	for (size_t i = 0; i < numThreads; i++)
		k += keyParts[i];

	o.append(BinaryToHex(CryptManager::GetSHA1ForString(k)));
	return o;
}

void
Steps::FillStringWithBPMs(size_t startRow,
						  size_t endRow,
						  std::vector<int>& nerv,
						  NoteData& nd,
						  TimingData* td,
						  RString& inOut)
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
		if (!m_bNoteDataIsFilled)
			return; /* no data is no data */
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
Steps::SetDifficultyAndDescription(Difficulty dc, const RString& sDescription)
{
	m_Difficulty = dc;
	m_sDescription = sDescription;
	if (GetDifficulty() == Difficulty_Edit)
		MakeValidEditDescription(m_sDescription);
}

void
Steps::SetCredit(const RString& sCredit)
{
	m_sCredit = sCredit;
}

void
Steps::SetChartStyle(const RString& sChartStyle)
{
	m_sChartStyle = sChartStyle;
}

bool
Steps::MakeValidEditDescription(RString& sPreferredDescription)
{
	if (static_cast<int>(sPreferredDescription.size()) >
		MAX_STEPS_DESCRIPTION_LENGTH) {
		sPreferredDescription =
		  sPreferredDescription.Left(MAX_STEPS_DESCRIPTION_LENGTH);
		return true;
	}
	return false;
}

void
Steps::SetMeter(int meter)
{
	m_iMeter = meter;
}

const TimingData*
Steps::GetTimingData() const
{
	return m_Timing.empty() ? &m_pSong->m_SongTiming : &m_Timing;
}

bool
Steps::HasSignificantTimingChanges() const
{
	const TimingData* timing = GetTimingData();
	if (timing->HasStops() || timing->HasDelays() || timing->HasWarps() ||
		timing->HasSpeedChanges() || timing->HasScrollChanges())
		return true;

	if (timing->HasBpmChanges()) {
		// check to see if these changes are significant.
		if ((GetMaxBPM() - GetMinBPM()) > 3.000f)
			return true;
	}

	return false;
}

const RString
Steps::GetMusicPath() const
{
	return Song::GetSongAssetPath(m_MusicFile.empty() ? m_pSong->m_sMusicFile
													  : m_MusicFile,
								  m_pSong->GetSongDir());
}

const RString&
Steps::GetMusicFile() const
{
	return m_MusicFile;
}

void
Steps::SetMusicFile(const RString& file)
{
	m_MusicFile = file;
}

void
Steps::SetCachedRadarValues(const RadarValues& rv)
{
	m_CachedRadarValues = rv;
	m_bAreCachedRadarValuesJustLoaded = true;
}

std::vector<int>
Steps::GetNPSVector(NoteData& nd,
					std::vector<int> nerv,
					std::vector<float> etaner,
					float rate)
{
	std::vector<int> doot(static_cast<int>(etaner.back()));
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

std::vector<int>
Steps::GetCNPSVector(NoteData& nd,
					 std::vector<int> nerv,
					 std::vector<float> etaner,
					 int chordsize,
					 float rate)
{
	std::vector<int> doot(static_cast<int>(etaner.back()));
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
		if (notesinchord == chordsize)
			chordnotecounter += notesinchord;
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

	static int HasSignificantTimingChanges(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasSignificantTimingChanges());
		return 1;
	}
	static int HasAttacks(T* p, lua_State* L)
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	static int GetRadarValues(T* p, lua_State* L)
	{
		PlayerNumber pn = PLAYER_1;
		if (!lua_isnil(L, 1)) {
			pn = PLAYER_1;
		}

		RadarValues& rv = const_cast<RadarValues&>(p->GetRadarValues());
		rv.PushSelf(L);
		return 1;
	}
	// Sigh -Mina
	static int GetRelevantRadars(T* p, lua_State* L)
	{
		std::vector<int> relevants;
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
	static int GetTimingData(T* p, lua_State* L)
	{
		p->GetTimingData()->PushSelf(L);
		return 1;
	}
	static int GetHash(T* p, lua_State* L)
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
	static int GetChartName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetChartName());
		return 1;
	}
	static int GetDisplayBpms(T* p, lua_State* L)
	{
		DisplayBpms temp;
		p->GetDisplayBpms(temp);
		float fMin = temp.GetMin();
		float fMax = temp.GetMax();
		std::vector<float> fBPMs;
		fBPMs.push_back(fMin);
		fBPMs.push_back(fMax);
		LuaHelpers::CreateTableFromArray(fBPMs, L);
		return 1;
	}
	static int IsDisplayBpmSecret(T* p, lua_State* L)
	{
		DisplayBpms temp;
		p->GetDisplayBpms(temp);
		lua_pushboolean(L, static_cast<int>(temp.IsSecret()));
		return 1;
	}
	static int IsDisplayBpmConstant(T* p, lua_State* L)
	{
		DisplayBpms temp;
		p->GetDisplayBpms(temp);
		lua_pushboolean(L, static_cast<int>(temp.BpmIsConstant()));
		return 1;
	}
	static int IsDisplayBpmRandom(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->GetDisplayBPM() == DISPLAY_BPM_RANDOM);
		return 1;
	}
	DEFINE_METHOD(PredictMeter, PredictMeter())
	static int GetDisplayBPMType(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, p->GetDisplayBPM());
		return 1;
	}

	static int GetChartKey(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetChartKey().c_str());
		return 1;
	}

	static int GetMSD(T* p, lua_State* L)
	{
		float rate = FArg(1);
		int index = IArg(2) - 1;
		CLAMP(rate, 0.7f, 3.f);
		lua_pushnumber(L, p->GetMSD(rate, index));
		return 1;
	}
	// ok really is this how i have to do this - mina
	static int GetRelevantSkillsetsByMSDRank(T* p, lua_State* L)
	{
		float rate = FArg(1);
		CLAMP(rate, 0.7f, 2.f);
		auto sortedskillsets = p->SortSkillsetsAtRate(rate, false);
		int rank = IArg(2);
		int i = NUM_Skillset - 1; // exclude Overall from this... need to handle
								  // overall better - mina
		Skillset o = Skillset_Invalid;
		float rval = 0.f;
		float highval = 0.f;
		FOREACHM(float, Skillset, sortedskillsets, thingy)
		{
			if (i == rank) {
				rval = thingy->first;
				o = thingy->second;
			}
			if (i == 1)
				highval = thingy->first;
			--i;
		}
		if (rval > highval * 0.9f)
			lua_pushstring(L, SkillsetToString(o));
		else
			lua_pushstring(L, "");
		return 1;
	}
	static int GetNonEmptyNoteData(T* p, lua_State* L)
	{
		lua_newtable(L);
		auto nd = p->GetNoteData();
		auto loot = nd.BuildAndGetNerv();

		LuaHelpers::CreateTableFromArray(
		  loot, L); // row (we need timestamps technically)
		lua_rawseti(L, -2, 1);

		for (int i = 0; i < nd.GetNumTracks(); ++i) { // tap or not
			std::vector<int> doot;
			for (auto r : loot) {
				auto tn = nd.GetTapNote(i, r);
				if (tn.type == TapNoteType_Empty)
					doot.push_back(0);
				else if (tn.type == TapNoteType_Tap)
					doot.push_back(1);
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
	static int GetCDGraphVectors(T* p, lua_State* L)
	{
		float rate = FArg(1);
		CLAMP(rate, 1.f, 3.f);
		auto nd = p->GetNoteData();
		if (nd.IsEmpty())
			return 0;
		const std::vector<int>& nerv = nd.BuildAndGetNerv();
		const std::vector<float>& etaner =
		  p->GetTimingData()->BuildAndGetEtaner(nerv);

		// directly using CreateTableFromArray(p->GetNPSVector(nd, nerv,
		// etaner), L) produced tables full of 0 values for ???? reason -mina
		std::vector<int> scroot = p->GetNPSVector(nd, nerv, etaner, rate);
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
	static int GetNumColumns(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetNoteData().GetNumTracks());
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
		ADD_METHOD(IsAnEdit);
		ADD_METHOD(IsAPlayerEdit);
		ADD_METHOD(GetDisplayBpms);
		ADD_METHOD(IsDisplayBpmSecret);
		ADD_METHOD(IsDisplayBpmConstant);
		ADD_METHOD(IsDisplayBpmRandom);
		ADD_METHOD(PredictMeter);
		ADD_METHOD(GetDisplayBPMType);
		ADD_METHOD(GetRelevantSkillsetsByMSDRank);
		ADD_METHOD(GetCDGraphVectors);
		ADD_METHOD(GetNumColumns);
		ADD_METHOD(GetNonEmptyNoteData);
	}
};

LUA_REGISTER_CLASS(Steps)
// lua end

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard, David Wilson
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
