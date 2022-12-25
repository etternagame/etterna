#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/HighScore/HighScore.h"
#include "Etterna/MinaCalc/MinaCalc.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/NoteData/NoteDataStructures.h"
#include "RageUtil/Misc/RageTimer.h"
#include "ScoreManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "RageUtil/Misc/RageThreads.h"
#include "Etterna/Globals/SoloCalc.h"

#include <cstdint>
#include <numeric>
#include <algorithm>

#include "GameManager.h"
#include "Etterna/MinaCalc/MinaCalcHelpers.h"

using std::lock_guard;
using std::mutex;

ScoreManager* SCOREMAN = nullptr;

ScoreManager::ScoreManager()
{
	tempscoreforonlinereplayviewing = nullptr;

	// Register with Lua.
	{
		auto* L = LUA->Get();
		lua_pushstring(L, "SCOREMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

ScoreManager::~ScoreManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal("SCOREMAN");
}

inline auto
RateKeyToDisplayString(float rate) -> string
{
	auto rs = ssprintf("%.2f", rate);
	auto j = 1;
	if (rs.find_last_not_of('0') == rs.find('.')) {
		j = 2;
	}

	rs.erase(rs.find_last_not_of('0') + j, std::string::npos);
	rs.append("x");

	return rs;
}

ScoresAtRate::ScoresAtRate()
{
	bestGrade = Grade_Invalid;
	scores.clear();
	PBptr = nullptr;
	noccPBptr = nullptr;
}

auto
ScoresAtRate::AddScore(HighScore& hs) -> HighScore*
{
	const auto& key = hs.GetScoreKey();

	bestGrade = std::min(hs.GetWifeGrade(), bestGrade);
	if (hs.GetWifeGrade() != Grade_Failed) {
		bestWifeScore = PREFSMAN->m_bSortBySSRNorm
						  ? std::max(hs.GetSSRNormPercent(), bestWifeScore)
						  : std::max(hs.GetWifeScore(), bestWifeScore);
	}

	scores.emplace(key, hs);
	if ((PBptr == nullptr) ||
		PBptr->GetSSRNormPercent() < hs.GetSSRNormPercent()) {
		PBptr = &scores.find(key)->second;
	}

	if (static_cast<int>(hs.GetChordCohesion()) == 0) {
		if ((noccPBptr == nullptr) ||
			noccPBptr->GetSSRNormPercent() < hs.GetSSRNormPercent()) {
			noccPBptr = &scores.find(key)->second;
		}
	}

	SCOREMAN->RegisterScore(&scores.find(key)->second);
	SCOREMAN->AddToKeyedIndex(&scores.find(key)->second);
	return &(scores.find(key)->second);
}

auto
ScoresAtRate::GetSortedKeys() const -> const std::vector<string>
{
	// it is technically possible for 2 scores to have the same percent
	// use a multimap to support this
	std::multimap<float, std::string, std::greater<>> tmp{};
	std::vector<std::string> o{};
	if (PREFSMAN->m_bSortBySSRNorm) {
		for (const auto& i : scores) {
			tmp.emplace(i.second.GetSSRNormPercent(), i.first);
		}
	} else {
		for (const auto& i : scores) {
			tmp.emplace(i.second.GetWifeScore(), i.first);
		}
	}

	o.reserve(tmp.size());
	for (const auto& j : tmp) {
		o.emplace_back(j.second);
	}

	return o;
}

auto
ScoresAtRate::GetAllScores() -> const std::vector<HighScore*>
{
	std::vector<HighScore*> o;
	for (auto& i : scores) {
		o.emplace_back(&i.second);
	}

	/* upload the worst scores first and the best scores last so we catch any
	 * de-facto pbs that are created by actual pbs failing the upload checks for
	 * w.e reason */
	auto ssrcomp = [](HighScore* a, HighScore* b) {
		return (a->GetSSRNormPercent() < b->GetSSRNormPercent());
	};

	sort(o.begin(), o.end(), ssrcomp);

	return o;
}

void
ScoreManager::PurgeScores()
{
	TopSSRs.clear();
	TopSSRs.shrink_to_fit();

	AllScores.clear();
	AllScores.shrink_to_fit();

	ScoresByKey.clear();

	pscores.clear();
}

void
ScoreManager::PurgeProfileScores(const string& profileID)
{
	TopSSRs.clear();
	TopSSRs.shrink_to_fit();
	for (auto& score : AllProfileScores[profileID]) {
		{
			auto it = find(AllScores.begin(), AllScores.end(), score);
			if (it != AllScores.end()) {
				AllScores.erase(it);
			}
		}
		auto it = ScoresByKey.find(score->GetChartKey());
		if (it != ScoresByKey.end()) {
			ScoresByKey.erase(it);
		}
	}

	AllScores.shrink_to_fit();
	AllProfileScores[profileID].clear();
	AllProfileScores[profileID].shrink_to_fit();

	pscores[profileID].clear();
}

ScoresForChart::ScoresForChart()
{
	bestGrade = Grade_Invalid;
	ScoresByRate.clear();
}

auto
ScoresForChart::GetPBAt(float rate) -> HighScore*
{
	const auto key = RateToKey(rate);
	if (ScoresByRate.count(key) != 0U) {
		return ScoresByRate.at(key).PBptr;
	}

	return nullptr;
}

auto
ScoresForChart::GetPBUpTo(float rate) -> HighScore*
{
	const auto key = RateToKey(rate);
	for (auto& i : ScoresByRate) {
		if (i.first <= key) {
			return i.second.PBptr;
		}
	}

	return nullptr;
}

auto
ScoresForChart::AddScore(HighScore& hs) -> HighScore*
{
	bestGrade = std::min(hs.GetWifeGrade(), bestGrade);
	if (hs.GetWifeGrade() != Grade_Failed) {
		bestWifeScore = PREFSMAN->m_bSortBySSRNorm
						  ? std::max(hs.GetSSRNormPercent(), bestWifeScore)
						  : std::max(hs.GetWifeScore(), bestWifeScore);
	}

	const auto rate = hs.GetMusicRate();
	const auto key = RateToKey(rate);
	auto* const hsPtr = ScoresByRate[key].AddScore(hs);
	// ok let's try this --lurker
	SetTopScores();
	hs.SetTopScore(hsPtr->GetTopScore());
	return hsPtr;
}

auto
ScoresForChart::GetPlayedRates() const -> const std::vector<float>
{
	std::vector<float> o;
	for (const auto& i : ScoresByRate) {
		o.emplace_back(KeyToRate(i.first));
	}

	return o;
}

auto
ScoresForChart::GetPlayedRateKeys() const -> const std::vector<int>
{
	std::vector<int> o;
	for (const auto& i : ScoresByRate) {
		o.emplace_back(i.first);
	}

	return o;
}

auto
ScoresForChart::GetPlayedRateDisplayStrings() const -> const std::vector<string>
{
	std::vector<string> o;
	for (const auto& rate : GetPlayedRates()) {
		o.emplace_back(RateKeyToDisplayString(rate));
	}

	return o;
}

// seems like this could be handled more elegantly by splitting operations up
// -mina
void
ScoresForChart::SetTopScores()
{
	std::vector<HighScore*> eligiblescores;
	for (auto& i : ScoresByRate) {
		auto& hs = i.second.noccPBptr;
		if ((hs != nullptr) && hs->GetSSRCalcVersion() == GetCalcVersion() &&
			hs->GetEtternaValid() &&
			static_cast<int>(hs->GetChordCohesion()) == 0 &&
			hs->GetGrade() != Grade_Failed) {
			eligiblescores.emplace_back(hs);
		}
	}

	// if there aren't 2 noccpbs in top scores we might as well use old cc
	// scores -mina
	if (eligiblescores.size() < 2) {
		for (auto& i : ScoresByRate) {

			auto& hs = i.second.PBptr;
			if ((hs != nullptr) &&
				hs->GetSSRCalcVersion() == GetCalcVersion() &&
				hs->GetEtternaValid() &&
				static_cast<int>(hs->GetChordCohesion()) != 0 &&
				hs->GetGrade() != Grade_Failed) {
				eligiblescores.emplace_back(hs);
			}
		}
	}

	if (eligiblescores.empty()) {
		return;
	}

	if (eligiblescores.size() == 1) {
		eligiblescores[0]->SetTopScore(1);
		return;
	}

	auto ssrcomp = [](HighScore* a, HighScore* b) {
		return (a->GetSkillsetSSR(Skill_Overall) >
				b->GetSkillsetSSR(Skill_Overall));
	};

	sort(eligiblescores.begin(), eligiblescores.end(), ssrcomp);

	for (auto& hs : eligiblescores) {
		hs->SetTopScore(0);
	}

	eligiblescores[0]->SetTopScore(1);
	eligiblescores[1]->SetTopScore(2);
}

auto
ScoresForChart::GetAllPBPtrs() -> const std::vector<HighScore*>
{
	std::vector<HighScore*> o;
	for (auto& i : ScoresByRate) {
		o.emplace_back(i.second.PBptr);
	}

	return o;
}

auto
ScoresForChart::GetAllScores() -> const std::vector<HighScore*>
{
	std::vector<HighScore*> o;
	for (auto& i : ScoresByRate) {
		for (const auto& s : i.second.GetAllScores()) {
			o.emplace_back(s);
		}
	}

	return o;
}

// is there any reason for this to be nested and not just a single vector?
auto
ScoreManager::GetAllPBPtrs(const string& profileID)
  -> const std::vector<vector<HighScore*>>
{
	std::vector<vector<HighScore*>> vec;
	for (auto& i : pscores.at(profileID)) {
		if (!SONGMAN->IsChartLoaded(i.first)) {
			continue;
		}
		vec.emplace_back(i.second.GetAllPBPtrs());
	}

	return vec;
}

auto
ScoreManager::GetChartPBAt(const string& ck,
						   float rate,
						   const string& profileID) -> HighScore*
{
	if (KeyHasScores(ck, profileID)) {
		return pscores.at(profileID).at(ck).GetPBAt(rate);
	}

	return nullptr;
}

auto
ScoreManager::GetChartPBUpTo(const string& ck,
							 float rate,
							 const string& profileID) -> HighScore*
{
	if (KeyHasScores(ck, profileID)) {
		return pscores.at(profileID).at(ck).GetPBUpTo(rate);
	}

	return nullptr;
}

void
ScoreManager::SetAllTopScores(const string& profileID)
{
	for (auto& i : pscores[profileID]) {
		if (!SONGMAN->IsChartLoaded(i.first)) {
			continue;
		}
		i.second.SetTopScores();
	}
}

auto
ScoresAtRate::HandleNoCCPB(HighScore& hs) -> bool
{
	// lurker says:
	// don't even TRY to fuck with nocc pb unless the score is nocc
	if (static_cast<int>(hs.GetChordCohesion()) == 0) {
		// Set any nocc pb
		if (noccPBptr == nullptr) {
			noccPBptr = &hs;
			return true;
		}
		// update nocc pb if a better score is found
		if (noccPBptr->GetSSRNormPercent() < hs.GetSSRNormPercent()) {
			noccPBptr = &hs;
			return true;
		}
	}

	return false;
}

void
ScoreManager::RecalculateSSRs(LoadingWindow* ld)
{
	RageTimer ld_timer;
	auto& scores = SCOREMAN->scorestorecalc;

	if (ld != nullptr) {
		ld->SetProgress(0);
		ld_timer.Touch();
		ld->SetIndeterminate(false);
		ld->SetTotalWork(scores.size());
		ld->SetText("\nUpdating Ratings for " + std::to_string(scores.size()) +
					" scores");
	}
	auto onePercent = std::max(static_cast<int>(scores.size() / 100 * 5), 1);

	mutex songVectorPtrMutex;
	std::vector<std::uintptr_t> currentlyLockedSongs;
	// This is meant to ensure mutual exclusion for a song
	class SongLock
	{
	  public:
		mutex& songVectorPtrMutex; // This mutex guards the vector
		std::vector<std::uintptr_t>&
		  currentlyLockedSongs; // Vector of currently locked songs
		std::uintptr_t song;	// The song for this lock
		SongLock(std::vector<std::uintptr_t>& vec, mutex& mut, std::uintptr_t k)
		  : songVectorPtrMutex(mut)
		  , currentlyLockedSongs(vec)
		  , song(k)
		{
			auto active = true;
			{
				lock_guard<mutex> lk(songVectorPtrMutex);
				active = find(currentlyLockedSongs.begin(),
							  currentlyLockedSongs.end(),
							  song) != currentlyLockedSongs.end();
				if (!active) {
					currentlyLockedSongs.emplace_back(song);
				}
			}
			while (active) {
				// TODO(Sam): Try to make this wake up from the destructor
				// (CondVar's maybe)
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				{
					lock_guard<mutex> lk(songVectorPtrMutex);
					active = find(currentlyLockedSongs.begin(),
								  currentlyLockedSongs.end(),
								  song) != currentlyLockedSongs.end();
					if (!active) {
						currentlyLockedSongs.emplace_back(song);
					}
				}
			}
		}
		~SongLock()
		{
			lock_guard<mutex> lk(songVectorPtrMutex);
			currentlyLockedSongs.erase(find(
			  currentlyLockedSongs.begin(), currentlyLockedSongs.end(), song));
		}
	};

	const std::function<void(
	  std::pair<vectorIt<HighScore*>, vectorIt<HighScore*>>, ThreadData*)>
	  callback =
		[&songVectorPtrMutex, &currentlyLockedSongs](
		  std::pair<vectorIt<HighScore*>, vectorIt<HighScore*>> workload,
		  ThreadData* data) {
			const auto per_thread_calc = std::make_unique<Calc>();
			per_thread_calc->loadparams = PREFSMAN->m_bAlwaysLoadCalcParams;

			auto* pair =
			  static_cast<std::pair<int, LoadingWindow*>*>(data->data);
			const auto onePercent = pair->first;
			auto* ld = pair->second;
			auto scoreIndex = 0;
			auto lastUpdate = 0;
			for (auto it = workload.first; it != workload.second; ++it) {
				auto* hs = *it;
				if ((ld != nullptr) && scoreIndex % onePercent == 0) {
					data->_progress += scoreIndex - lastUpdate;
					lastUpdate = scoreIndex;
					data->setUpdated(true);
				}
				++scoreIndex;

				auto ck = hs->GetChartKey();
				auto* steps = SONGMAN->GetStepsByChartkey(ck);

				// this _should_ be impossible since ischartloaded() checks
				// are required on all charts before getting here but just
				// in case...
				if (steps == nullptr) {
					continue;
				}

				SongLock lk(currentlyLockedSongs,
							songVectorPtrMutex,
							reinterpret_cast<std::uintptr_t>(steps->m_pSong));

				const auto ssrpercent = hs->GetSSRNormPercent();
				const auto musicrate = hs->GetMusicRate();

				// ghasgh we need to decompress to get maxpoints
				auto* td = steps->GetTimingData();
				NoteData nd;

				// try to normalize judgments
				// this function handles skip logic if not necessary
				hs->NormalizeJudgments();

				// rescore wife2 to wife3 for eligible scores
				auto remarried = false;
				if (hs->GetWifeVersion() != 3 && !hs->GetChordCohesion() &&
					hs->HasReplayData()) {
					steps->GetNoteData(nd);
					const auto maxpoints = nd.WifeTotalScoreCalc(td);
					if (maxpoints <= 0) {
						continue;
					}
					remarried =
					  hs->RescoreToWife3(static_cast<float>(maxpoints));
				}

				// don't waste time on <= 0%s or > 100%s
				if (ssrpercent <= 0.f || ssrpercent > 1.f || !steps->IsRecalcValid()) {
					if (ssrpercent > 1.f) {
						hs->SetSSRNormPercent(0.f);
						hs->SetWifeScore(0.f);
						hs->SetGrade(Grade_Failed);
					}
					hs->ResetSkillsets();
					continue;
				}

				// if this is not a rescore and has already been run on the
				// current calc vers, skip if it is a rescore, rerun it even
				// if the calc version is the same
				if (!remarried && hs->GetSSRCalcVersion() == GetCalcVersion()) {
					continue;
				}

				const std::vector<NoteInfo>* serializednd_ptr = nullptr;
				if (!steps->serializenotedatacache.empty()) {
					serializednd_ptr = &(steps->serializenotedatacache);
				} else {
					// notedata hasn't been loaded yet if we didn't rescore
					if (!remarried) {
						steps->GetNoteData(nd);
					}
					const auto& serializednd = nd.SerializeNoteData2(td);
					serializednd_ptr = &serializednd;
				}

				const auto& serializednd = *serializednd_ptr;
				std::vector<float> dakine;

				if (steps->m_StepsType == StepsType_dance_single) {
					// dakine = MinaSDCalc(serializednd, musicrate,
					// ssrpercent);
					dakine = MinaSDCalc(serializednd,
										musicrate,
										ssrpercent,
										per_thread_calc.get());
				} else {
					int columnCount =
					  GAMEMAN->GetStepsTypeInfo(steps->m_StepsType).iNumTracks;
					dakine = SoloCalc(serializednd, columnCount, musicrate, ssrpercent);
				}

				auto ssrVals = dakine;
				FOREACH_ENUM(Skillset, ss)
				{
					hs->SetSkillsetSSR(ss, ssrVals[ss]);
				}

				hs->SetSSRCalcVersion(GetCalcVersion());

				// we only want to upload scores that have been rescored to
				// wife3, not generic calc changes, since the site runs its
				// own calc anyway
				if (remarried) {

					// DISABLED CUZ WE DONT REALLY NEED AND MANUAL CAN SUFFICE
					// SCOREMAN->rescores.emplace(hs);
				}

				td->UnsetEtaner();
				nd.UnsetNerv();
				nd.UnsetSerializedNoteData();
				steps->Compress();

				/* Some scores were being incorrectly marked as ccon despite
				 * chord cohesion being disabled. Re-determine chord cohesion
				 * status from notecount, this should be robust as every score
				 * up to this point should be a fully completed pass. This will
				 * also allow us to mark files with 0 chords as being nocc
				 * (since it doesn't apply to them). */
				const auto totalstepsnotes =
				  steps->GetRadarValues()[RadarCategory_Notes];
				auto totalscorenotes = 0;
				totalscorenotes += hs->GetTapNoteScore(TNS_W1);
				totalscorenotes += hs->GetTapNoteScore(TNS_W2);
				totalscorenotes += hs->GetTapNoteScore(TNS_W3);
				totalscorenotes += hs->GetTapNoteScore(TNS_W4);
				totalscorenotes += hs->GetTapNoteScore(TNS_W5);
				totalscorenotes += hs->GetTapNoteScore(TNS_Miss);

				if (totalstepsnotes == totalscorenotes) {
					// the set function isn't inverted
					// but the get function is, this
					// sets bnochordcohesion to 1
					hs->SetChordCohesion(1);
				}
			}
		};
	auto onUpdate = [ld](int progress) {
		if (ld != nullptr) {
			ld->SetProgress(progress);
		}
	};

	parallelExecution<HighScore*>(
	  scores,
	  onUpdate,
	  callback,
	  static_cast<void*>(new std::pair<int, LoadingWindow*>(onePercent, ld)));

	SCOREMAN->scorestorecalc.clear();
	SCOREMAN->scorestorecalc.shrink_to_fit();
}

void
ScoreManager::RecalculateSSRs(const string& profileID)
{
	const auto& scores = SCOREMAN->GetAllProfileScores(profileID);

	mutex songVectorPtrMutex;
	std::vector<std::uintptr_t> currentlyLockedSongs;
	// This is meant to ensure mutual exclusion for a song
	class SongLock
	{
	  public:
		mutex& songVectorPtrMutex; // This mutex guards the vector
		std::vector<std::uintptr_t>&
		  currentlyLockedSongs; // Vector of currently locked songs
		std::uintptr_t song;	// The song for this lock
		SongLock(std::vector<std::uintptr_t>& vec, mutex& mut, std::uintptr_t k)
		  : songVectorPtrMutex(mut)
		  , currentlyLockedSongs(vec)
		  , song(k)
		{
			auto active = true;
			{
				lock_guard<mutex> lk(songVectorPtrMutex);
				active = find(currentlyLockedSongs.begin(),
							  currentlyLockedSongs.end(),
							  song) != currentlyLockedSongs.end();
				if (!active) {
					currentlyLockedSongs.push_back(song);
				}
			}
			while (active) {
				// TODO(Sam): Try to make this wake up from the destructor
				// (CondVar's maybe)
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				{
					lock_guard<mutex> lk(songVectorPtrMutex);
					active = find(currentlyLockedSongs.begin(),
								  currentlyLockedSongs.end(),
								  song) != currentlyLockedSongs.end();
					if (!active) {
						currentlyLockedSongs.push_back(song);
					}
				}
			}
		}
		~SongLock()
		{
			lock_guard<mutex> lk(songVectorPtrMutex);
			currentlyLockedSongs.erase(find(
			  currentlyLockedSongs.begin(), currentlyLockedSongs.end(), song));
		}
	};

	const std::function<void(
	  std::pair<vectorIt<HighScore*>, vectorIt<HighScore*>>, ThreadData*)>
	  callback =
		[&songVectorPtrMutex, &currentlyLockedSongs](
		  std::pair<vectorIt<HighScore*>, vectorIt<HighScore*>> workload,
		  ThreadData* data) {
			const auto per_thread_calc = std::make_unique<Calc>();
			per_thread_calc->loadparams = PREFSMAN->m_bAlwaysLoadCalcParams;

			auto scoreIndex = 0;
			for (auto it = workload.first; it != workload.second; ++it) {
				auto* hs = *it;
				++scoreIndex;

				auto ck = hs->GetChartKey();
				auto* steps = SONGMAN->GetStepsByChartkey(ck);

				// check for unloaded steps
				if (steps == nullptr) {
					continue;
				}

				const auto ssrpercent = hs->GetSSRNormPercent();

				// don't waste time on <= 0%s
				if (ssrpercent <= 0.f || !steps->IsRecalcValid()) {
					hs->ResetSkillsets();
					continue;
				}
				SongLock lk(currentlyLockedSongs,
							songVectorPtrMutex,
							reinterpret_cast<std::uintptr_t>(steps->m_pSong));

				const auto musicrate = hs->GetMusicRate();

				auto* td = steps->GetTimingData();
				NoteData nd;
				steps->GetNoteData(nd);

				auto serializednd = nd.SerializeNoteData2(td);
				std::vector<float> dakine;

				if (steps->m_StepsType == StepsType_dance_single) {
					dakine = MinaSDCalc(serializednd,
										musicrate,
										ssrpercent,
										per_thread_calc.get());
				} else {
					int columnCount =
					  GAMEMAN->GetStepsTypeInfo(steps->m_StepsType).iNumTracks;
					dakine = SoloCalc(
					  serializednd, columnCount, musicrate, ssrpercent);
				}

				auto ssrVals = dakine;
				FOREACH_ENUM(Skillset, ss)
				{
					hs->SetSkillsetSSR(ss, ssrVals[ss]);
				}
				hs->SetSSRCalcVersion(GetCalcVersion());

				td->UnsetEtaner();
				nd.UnsetNerv();
				nd.UnsetSerializedNoteData();
				steps->Compress();
			}
		};

	parallelExecution<HighScore*>(scores, callback);
}

void
ScoreManager::UnInvalidateAllScores(const string& profileID)
{
	for (auto& i : pscores[profileID]) {
		for (const auto& s : i.second.GetAllScores()) {
			s->SetEtternaValid(true);
		}
	}
}

void
ScoreManager::CalcPlayerRating(float& prating,
							   float* pskillsets,
							   const string& profileID)
{
	SetAllTopScores(profileID);

	std::vector<float> skillz;
	FOREACH_ENUM(Skillset, ss)
	{
		// skip overall ss
		if (ss == Skill_Overall) {
			continue;
		}

		std::vector<float> ssrs = SortTopSSRPtrs(ss, profileID, true);
		pskillsets[ss] = aggregate_skill(ssrs, 0.1L, (float)1.05, 0.0, (float)10.24);
		CLAMP(pskillsets[ss], 0.F, 100.F);
		skillz.push_back(pskillsets[ss]);
	}

	prating = aggregate_skill(skillz, 0.1L, (float)1.125, 0.0, (float)10.24);
	pskillsets[Skill_Overall] = prating;
}

std::vector<float>
ScoreManager::SortTopSSRPtrs(Skillset ss, const string& profileID, bool getSSRs)
{
	std::vector<float> o;
	TopSSRs.clear();
	for (auto& i : pscores[profileID]) {
		if (!SONGMAN->IsChartLoaded(i.first)) {
			continue;
		}
		for (const auto& hs : i.second.GetAllPBPtrs()) {
			TopSSRs.emplace_back(hs);

			if (getSSRs) {
				if (hs->GetSSRCalcVersion() != GetCalcVersion())
					continue;
				if (!hs->GetEtternaValid())
					continue;
				if (static_cast<int>(hs->GetChordCohesion()) == 1)
					continue;
				if (hs->GetTopScore() == 0)
					continue;
				if (SONGMAN->GetStepsByChartkey(hs->GetChartKey())
					  ->m_StepsType != StepsType_dance_single)
					continue;
				o.emplace_back(hs->GetSkillsetSSR(ss));
			}
		}
	}

	auto ssrcomp = [&ss](HighScore* a, HighScore* b) {
		return (a->GetSkillsetSSR(ss) > b->GetSkillsetSSR(ss));
	};

	sort(TopSSRs.begin(), TopSSRs.end(), ssrcomp);
	if (getSSRs)
		sort(o.begin(), o.end());
	return o;
}

std::map<DateTime, std::vector<float>>
ScoreManager::GetPlayerRatingOverTime(const std::string& profileID) {
	// returns DateTimes mapped to skillset ratings
	std::map<DateTime, std::vector<float>> ssrsByDate;

	SortTopSSRPtrsForGame(Skill_Overall, profileID);
	std::vector<HighScore*> scoresByDate = TopSSRsForGame;
	auto datecomp = [](HighScore* a, HighScore* b) {
		return a->GetDateTime() > b->GetDateTime();
	};
	sort(scoresByDate.begin(), scoresByDate.end(), datecomp);
	std::vector<DateTime> dates;
	std::set<DateTime> dates_used;
	for (auto& s : scoresByDate) {
		auto d = s->GetDateTime();
		d.StripTime();
		if (dates_used.count(d) == 0u) {
			dates_used.insert(d);
			dates.push_back(d);
		}
	}
	auto sortdates = [](DateTime a, DateTime b) { return a < b; };
	sort(dates.begin(), dates.end(), sortdates);

	const std::function<void(std::pair<vectorIt<DateTime>, vectorIt<DateTime>>,
							 ThreadData*)>
	  callback = [&ssrsByDate, &scoresByDate](
				   std::pair<vectorIt<DateTime>, vectorIt<DateTime>> workload,
				   ThreadData* data) {
		  for (auto it = workload.first; it != workload.second; it++) {
			  auto date = *it;

			  // ssrsInUse[x] = <all ssrs for this skillset>
			  std::vector<std::vector<float>> ssrsInUse;
			  ssrsInUse.resize(NUM_Skillset);
			  for (auto& s : scoresByDate) {
				  if (s->GetDateTime() <= date) {
					  FOREACH_ENUM(Skillset, ss)
					  {
						  ssrsInUse[ss].push_back(s->GetSkillsetSSR(ss));
					  }
				  }
			  }

			  std::vector<float> skillz;
			  skillz.resize(NUM_Skillset);
			  FOREACH_ENUM(Skillset, ss)
			  {
				  // skip overall ss
				  if (ss == Skill_Overall) {
					  continue;
				  }

				  std::vector<float> ssrs = ssrsInUse[ss];
				  skillz[ss] = aggregate_skill(ssrs, 0.1L, (float)1.05, 0.0, (float)10.24);
				  CLAMP(skillz[ss], 0.F, 100.F);
			  }
			  skillz[Skill_Overall] =
				aggregate_skill(skillz, 0.1L, (float)1.125, 0.0, (float)10.24);
			  ssrsByDate.emplace(date, skillz);
		  }
	  };
	parallelExecution<DateTime>(dates, callback);
	return ssrsByDate;
}

void
ScoreManager::SortTopSSRPtrsForGame(Skillset ss, const string& profileID)
{
	TopSSRsForGame.clear();
	for (auto& i : pscores[profileID]) {
		if (!SONGMAN->IsChartLoaded(i.first) ||
			!SONGMAN->GetStepsByChartkey(i.first)->IsPlayableForCurrentGame()) {
			continue;
		}
		for (const auto& hs : i.second.GetAllPBPtrs()) {
			TopSSRsForGame.emplace_back(hs);
		}
	}

	auto ssrcomp = [&ss](HighScore* a, HighScore* b) {
		return (a->GetSkillsetSSR(ss) > b->GetSkillsetSSR(ss));
	};

	sort(TopSSRsForGame.begin(), TopSSRsForGame.end(), ssrcomp);
}

auto
ScoreManager::GetTopSSRHighScore(unsigned int rank, int ss) -> HighScore*
{
	if (ss >= 0 && ss < NUM_Skillset && rank < TopSSRs.size()) {
		return TopSSRs[rank];
	}

	return nullptr;
}

auto
ScoreManager::GetTopSSRHighScoreForGame(unsigned int rank, int ss) -> HighScore*
{
	if (ss >= 0 && ss < NUM_Skillset && rank < TopSSRsForGame.size()) {
		return TopSSRsForGame[rank];
	}

	return nullptr;
}

void
ScoreManager::SortRecentScores(const string& profileID)
{
	TopSSRs.clear();
	for (auto& i : pscores[profileID]) {
		if (!SONGMAN->IsChartLoaded(i.first)) {
			continue;
		}
		for (const auto& hs : i.second.GetAllScores()) {
			TopSSRs.emplace_back(hs);
		}
	}

	auto datecomp = [](HighScore* a, HighScore* b) {
		return (a->GetDateTime() > b->GetDateTime());
	};

	sort(TopSSRs.begin(), TopSSRs.end(), datecomp);
}

void
ScoreManager::SortRecentScoresForGame(const string& profileID)
{
	TopSSRsForGame.clear();
	for (auto& i : pscores[profileID]) {
		if (!SONGMAN->IsChartLoaded(i.first) ||
			!SONGMAN->GetStepsByChartkey(i.first)->IsPlayableForCurrentGame()) {
			continue;
		}
		for (const auto& hs : i.second.GetAllScores()) {
			TopSSRsForGame.emplace_back(hs);
		}
	}

	auto datecomp = [](HighScore* a, HighScore* b) {
		return (a->GetDateTime() > b->GetDateTime());
	};

	sort(TopSSRsForGame.begin(), TopSSRsForGame.end(), datecomp);
}

auto
ScoreManager::GetRecentScore(const int rank) -> HighScore*
{
	if (rank >= 0 && rank < static_cast<int>(TopSSRs.size())) {
		return TopSSRs[rank];
	}

	return nullptr;
}

auto
ScoreManager::GetRecentScoreForGame(const int rank) -> HighScore*
{
	if (rank >= 0 && rank < static_cast<int>(TopSSRsForGame.size())) {
		return TopSSRsForGame[rank];
	}

	return nullptr;
}

void
ScoreManager::ImportScore(const HighScore& hs_, const string& profileID)
{
	auto hs = hs_;
	RegisterScoreInProfile(pscores[profileID][hs.GetChartKey()].AddScore(hs),
						   profileID);
}

void
ScoreManager::RegisterScoreInProfile(HighScore* hs_, const string& profileID)
{
	AllProfileScores[profileID].emplace_back(hs_);
}

std::vector<Skillset>
ScoreManager::GetTopPlayedSkillsets(const string& profileID)
{
	std::vector<Skillset> output;
	auto playcounts = GetPlaycountPerSkillset(profileID);
	std::vector<std::pair<Skillset, int>> sortedcounts;
	for (size_t i = 0; i < playcounts.size(); i++) {
		// i+1 because skipping Overall which should be 0
		sortedcounts.emplace_back(static_cast<Skillset>(i + 1), playcounts[i]);
	}
	std::sort(sortedcounts.begin(),
			  sortedcounts.end(),
			  [](auto& a, auto& b) -> bool { return a.second > b.second; });
	for (auto& x : sortedcounts) {
		output.push_back(x.first);
	}
	return output;
}

std::vector<int>
ScoreManager::GetPlaycountPerSkillset(const string& profileID)
{
	std::vector<int> counts;
	counts.resize(NUM_Skillset);

	// get relevant msd for each score and count the highest relevant
	for (auto& s : AllProfileScores[profileID]) {
		auto* c = SONGMAN->GetStepsByChartkey(s->GetChartKey());
		if (c == nullptr)
			continue;

		auto ss = c->SortSkillsetsAtRate(
		  std::clamp(s->GetMusicRate(), 0.7F, 2.F), false);
		counts[ss[0].first]++;
	}
	return counts;
}


// Write scores to xml
auto
ScoresAtRate::CreateNode(const int& rate) const -> XNode*
{
	auto* o = new XNode("ScoresAt");
	auto saved = 0;

	// prune out sufficiently low scores
	for (const auto& i : scores) {
		if (i.second.GetWifeScore() > SCOREMAN->minpercent) {
			o->AppendChild(i.second.CreateEttNode());
			saved++;
		}
	}

	if (o->ChildrenEmpty()) {
		return o;
	}

	const auto rs = ssprintf("%.3f", static_cast<float>(rate) / 10000.F);
	// should be safe as this is only called if there is at least 1 score
	// (which would be the pb)
	o->AppendAttr("PBKey", PBptr->GetScoreKey());
	if (noccPBptr != nullptr) {
		if (PBptr->GetScoreKey() != noccPBptr->GetScoreKey()) {
			o->AppendAttr("noccPBKey", noccPBptr->GetScoreKey());
		}
	}

	o->AppendAttr("BestGrade", GradeToString(bestGrade));
	o->AppendAttr("Rate", rs);

	return o;
}

auto
ScoresForChart::CreateNode(const string& ck) const -> XNode*
{
	auto loot = ch;
	loot.FromKey(ck); // needs to be here (or somewhere along the line,
					  // maybe not exactly here) -mina
	auto* o = loot.CreateNode(false);

	for (const auto& i : ScoresByRate) {
		auto* const node = i.second.CreateNode(i.first);
		if (!node->ChildrenEmpty()) {
			o->AppendChild(node);
		} else {
			delete node;
		}
	}
	return o;
}

auto
ScoreManager::CreateNode(const string& profileID) const -> XNode*
{
	auto* o = new XNode("PlayerScores");
	for (const auto& ch : pscores.find(profileID)->second) {
		auto* const node = ch.second.CreateNode(ch.first);
		if (!node->ChildrenEmpty()) {
			o->AppendChild(node);
		} else {
			delete node;
		}
	}

	return o;
}

// Read scores from xml
void
ScoresAtRate::LoadFromNode(const XNode* node,
						   const string& ck,
						   const float& rate,
						   const string& profileID)
{
	string sk;
	FOREACH_CONST_Child(node, p)
	{
		p->GetAttrValue("Key", sk);

		// Fill in stuff for the highscores
		scores[sk].SetChartKey(ck);
		scores[sk].SetScoreKey(sk);
		scores[sk].SetMusicRate(rate);
		scores[sk].LoadFromEttNode(p);

		// Set any pb
		if (PBptr == nullptr) {
			PBptr = &scores.find(sk)->second;
		} else {
			// update pb if a better score is found
			if (PBptr->GetSSRNormPercent() < scores[sk].GetSSRNormPercent()) {
				PBptr = &scores.find(sk)->second;
			}
		}

		HandleNoCCPB(scores[sk]);

		bestGrade = std::min(scores[sk].GetWifeGrade(), bestGrade);
		if (scores[sk].GetWifeGrade() != Grade_Failed) {
			bestWifeScore =
			  PREFSMAN->m_bSortBySSRNorm
				? std::max(scores[sk].GetSSRNormPercent(), bestWifeScore)
				: std::max(scores[sk].GetWifeScore(), bestWifeScore);
		}

		// Very awkward, need to figure this out better so there isn't
		// unnecessary redundancy between loading and adding
		SCOREMAN->RegisterScore(&scores.find(sk)->second);
		SCOREMAN->AddToKeyedIndex(&scores.find(sk)->second);
		SCOREMAN->RegisterScoreInProfile(&scores.find(sk)->second, profileID);

		/*  technically there is one scenario in which we can rescore to a
		 * new wifeversion but not need to recalculate the ssr, which is if
		 * the rescored wife% is identical to the previous one, this is
		 * unlikely enough that we can just run them in the same loop. we
		 * also must rescore all scores rather than just pbs because top
		 * score rankings may in fact shift post-rescore, this will be taken
		 * care of by calcplayerrating which will be called after
		 * recalculatessrs */

		const auto oldcalc = scores[sk].GetSSRCalcVersion() != GetCalcVersion();
		// don't include cc check here, we want cc scores to filter into the
		// recalc, just not the rescore
		const auto getremarried =
		  scores[sk].GetWifeVersion() != 3 && scores[sk].HasReplayData();

		// check to see if a file does not have normalized judgments
		// this will pile up but only for cases of large amounts of scores
		// missing replay data
		const auto notnormalized =
		  scores[sk].IsEmptyNormalized() &&
		  (scores[sk].HasReplayData() || scores[sk].GetJudgeScale() == 1.F);

		// oops (adding this to the recalc list will reset the score to 0)
		const auto broke = scores[sk].GetSSRNormPercent() > 1.F ||
						   scores[sk].GetWifeScore() > 1.F;

		// for debugging replay stuff. probably never uncomment this.
		/*
		if (SONGMAN->IsChartLoaded(ck) && scores[sk].HasReplayData()) {
			if (scores[sk].GetWifeGrade() != Grade_Failed) {
				scores[sk].replay->VerifyInputDataAndReplayData();
				scores[sk].replay->VerifyGeneratedInputDataMatchesReplayData();
			}
		}
		*/

		/* technically we don't need to have charts loaded to rescore to
		 * wife3, however trying to do this might be quite a bit of work (it
		 * would require making a new lambda loop) and while it would be
		 * nice to have at some point it's not worth it just at this moment,
		 * and while it sort of makes sense from a user convenience aspect
		 * to allow this, it definitely does not make sense from a clarity
		 * or consistency perspective */
		if ((oldcalc || getremarried || notnormalized || broke) && SONGMAN->IsChartLoaded(ck)) {
			SCOREMAN->scorestorecalc.emplace_back(&scores[sk]);
		}
	}
}

void
ScoresForChart::LoadFromNode(const XNode* node,
							 const string& ck,
							 const string& profileID)
{
	string rs;

	if (node->GetName() == "Chart") {
		ch.LoadFromNode(node);
	}

	if (node->GetName() == "ChartScores") {
		node->GetAttrValue("Key", rs);
		ch.key = rs;
		node->GetAttrValue("Song", ch.lastsong);
		node->GetAttrValue("Pack", ch.lastpack);
	}

	FOREACH_CONST_Child(node, p)
	{
		ASSERT(p->GetName() == "ScoresAt");
		p->GetAttrValue("Rate", rs);
		auto rate = 10 * StringToInt(rs.substr(0, 1) + rs.substr(2, 4));
		ScoresByRate[rate].LoadFromNode(p, ck, KeyToRate(rate), profileID);
		bestGrade = std::min(ScoresByRate[rate].bestGrade, bestGrade);

		bestWifeScore =
		  std::max(ScoresByRate[rate].bestWifeScore, bestWifeScore);
	}
}

void
ScoreManager::LoadFromNode(const XNode* node, const string& profileID)
{
	FOREACH_CONST_Child(node, p)
	{
		// ASSERT(p->GetName() == "Chart");
		string tmp;
		p->GetAttrValue("Key", tmp);
		const auto ck = tmp;
		pscores[profileID][ck].LoadFromNode(p, ck, profileID);
	}
}

auto
ScoresForChart::GetScoresAtRate(const int& rate) -> ScoresAtRate*
{
	auto it = ScoresByRate.find(rate);
	if (it != ScoresByRate.end()) {
		return &it->second;
	}
	return nullptr;
}

auto
ScoreManager::GetScoresForChart(const string& ck, const string& profileID)
  -> ScoresForChart*
{
	auto it = (pscores[profileID]).find(ck);
	if (it != (pscores[profileID]).end()) {
		return &it->second;
	}
	return nullptr;
}

#include "Etterna/Models/Lua/LuaBinding.h"

class LunaScoresAtRate : public Luna<ScoresAtRate>
{
  public:
	static auto GetScores(T* p, lua_State* L) -> int
	{
		lua_newtable(L);
		auto keys = p->GetSortedKeys();
		for (size_t i = 0; i < keys.size(); ++i) {
			auto& wot = p->scores[keys[i]];
			wot.PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}

		return 1;
	}

	LunaScoresAtRate() { ADD_METHOD(GetScores); }
};

LUA_REGISTER_CLASS(ScoresAtRate)

class LunaScoresForChart : public Luna<ScoresForChart>
{
  public:
	LunaScoresForChart() = default;
};

LUA_REGISTER_CLASS(ScoresForChart)

class LunaScoreManager : public Luna<ScoreManager>
{
  public:
	static auto GetScoresByKey(T* p, lua_State* L) -> int
	{
		const string& ck = SArg(1);
		auto* scores = p->GetScoresForChart(ck);

		if (scores != nullptr) {
			lua_newtable(L);
			auto ratekeys = scores->GetPlayedRateKeys();
			auto ratedisplay = scores->GetPlayedRateDisplayStrings();
			for (size_t i = 0; i < ratekeys.size(); ++i) {
				LuaHelpers::Push(L, ratedisplay[i]);
				scores->GetScoresAtRate(ratekeys[i])->PushSelf(L);
				lua_rawset(L, -3);
			}

			return 1;
		}

		lua_pushnil(L);
		return 1;
	}

	static auto SortSSRs(T* p, lua_State* L) -> int
	{
		p->SortTopSSRPtrs(Enum::Check<Skillset>(L, 1));
		return 1;
	}

	static auto SortSSRsForGame(T* p, lua_State* L) -> int
	{
		p->SortTopSSRPtrsForGame(Enum::Check<Skillset>(L, 1));
		return 1;
	}

	static auto GetTopSSRHighScore(T* p, lua_State* L) -> int
	{
		auto* ths =
		  p->GetTopSSRHighScore(IArg(1) - 1, Enum::Check<Skillset>(L, 2));
		if (ths != nullptr) {
			ths->PushSelf(L);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	static auto GetTopSSRHighScoreForGame(T* p, lua_State* L) -> int
	{
		auto* ths = p->GetTopSSRHighScoreForGame(IArg(1) - 1,
												 Enum::Check<Skillset>(L, 2));
		if (ths != nullptr) {
			ths->PushSelf(L);
		} else {
			lua_pushnil(L);
		}
		return 1;
	}

	static auto SortRecentScores(T* p, lua_State* L) -> int
	{
		p->SortRecentScores();
		return 0;
	}

	static auto SortRecentScoresForGame(T* p, lua_State* L) -> int
	{
		p->SortRecentScoresForGame();
		return 0;
	}

	static auto GetRecentScore(T* p, lua_State* L) -> int
	{
		auto* ths = p->GetRecentScore(IArg(1) - 1);
		if (ths != nullptr) {
			ths->PushSelf(L);
		} else {
			lua_pushnil(L);
		}

		return 1;
	}

	static auto GetRecentScoreForGame(T* p, lua_State* L) -> int
	{
		auto* ths = p->GetRecentScoreForGame(IArg(1) - 1);
		if (ths != nullptr) {
			ths->PushSelf(L);
		} else {
			lua_pushnil(L);
		}

		return 1;
	}

	static auto GetMostRecentScore(T* p, lua_State* L) -> int
	{
		// this _should_ always be viable if only called from eval
		auto* last = p->GetMostRecentScore();
		if (last == nullptr)
			lua_pushnil(L);
		else
			last->PushSelf(L);
		return 1;
	}

	static auto GetTotalNumberOfScores(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetAllProfileScores().size());
		return 1;
	}

	static auto GetTopPlayedSkillsets(T* p, lua_State* L) -> int
	{
		auto* profile = Luna<Profile>::check(L, 1);
		auto& s = profile->m_sProfileID;
		// expecting a profile ID here
		auto v = p->GetTopPlayedSkillsets(s);
		LuaHelpers::CreateTableFromArray(v, L);

		return 1;
	}

	static auto GetPlaycountPerSkillset(T* p, lua_State* L) -> int
	{
		auto* profile = Luna<Profile>::check(L, 1);
		auto& s = profile->m_sProfileID;
		// each index in the resulting vector here is essentially equal to Skillset
		// except it is offset by 1: position 0 is Stream (at the time of writing)
		// Overall is nowhere to be found because that doesn't make sense.
		// expecting a profile ID here
		auto v = p->GetPlaycountPerSkillset(s);
		LuaHelpers::CreateTableFromArray(v, L);

		return 1;
	}

	static auto GetScoresThisSession(T* p, lua_State* L) -> int
	{
		auto v = p->GetScoresThisSession();
		LuaHelpers::CreateTableFromArray(v, L);
		return 1;
	}

	static auto GetNumScoresThisSession(T* p, lua_State* L) -> int
	{
		lua_pushnumber(L, p->GetNumScoresThisSession());
		return 1;
	}

	static auto GetPlayerRatingOverTime(T* p, lua_State* L) -> int
	{
		auto ssrsByDate = p->GetPlayerRatingOverTime();
		lua_newtable(L);
		for (auto ssrAtDate : ssrsByDate) {
			lua_pushstring(L, ssrAtDate.first.GetString().c_str());
			LuaHelpers::CreateTableFromArray(ssrAtDate.second, L);
			lua_rawset(L, -3);
		}

		return 1;
	}

	DEFINE_METHOD(GetTempReplayScore, tempscoreforonlinereplayviewing);
	LunaScoreManager()
	{
		ADD_METHOD(GetScoresByKey);
		ADD_METHOD(SortSSRs);
		ADD_METHOD(SortSSRsForGame);
		ADD_METHOD(GetTopSSRHighScore);
		ADD_METHOD(GetTopSSRHighScoreForGame);
		ADD_METHOD(SortRecentScores);
		ADD_METHOD(SortRecentScoresForGame);
		ADD_METHOD(GetRecentScore);
		ADD_METHOD(GetRecentScoreForGame);
		ADD_METHOD(GetMostRecentScore);
		ADD_METHOD(GetTempReplayScore);
		ADD_METHOD(GetTotalNumberOfScores);
		ADD_METHOD(GetTopPlayedSkillsets);
		ADD_METHOD(GetPlaycountPerSkillset);
		ADD_METHOD(GetScoresThisSession);
		ADD_METHOD(GetNumScoresThisSession);
		ADD_METHOD(GetPlayerRatingOverTime);
	}
};

// put this here because didnt want to include lua in MinaCalc
LuaFunction(GetCalcVersion, GetCalcVersion())
LUA_REGISTER_CLASS(ScoreManager)
