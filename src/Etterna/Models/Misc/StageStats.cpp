#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/MinaCalc/MinaCalc.h"
#include "PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Profile.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "StageStats.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "AdjustSync.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Core/Services/Locator.hpp"
#include "GamePreferences.h"
#include "Etterna/Singletons/ReplayManager.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Models/NoteData/NoteDataUtil.h"

#ifndef _WIN32
#include <cpuid.h>
#endif

#ifdef _WIN32
#include <intrin.h>
#include <iphlpapi.h>
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "IPHLPAPI.lib")

#include <algorithm>
#include <cassert>

// we just need this for purposes of unique machine id.
// So any one or two mac's is fine.
auto
hashMacAddress(PIP_ADAPTER_INFO info) -> uint16_t
{
	uint16_t hash = 0;
	for (uint32_t i = 0; i < info->AddressLength; i++) {
		hash += (info->Address[i] << ((i & 1) * 8));
	}
	return hash;
}

void
getMacHash(uint16_t& mac1, uint16_t& mac2)
{
	IP_ADAPTER_INFO AdapterInfo[32];
	DWORD dwBufLen = sizeof(AdapterInfo);

	const auto dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
	if (dwStatus != ERROR_SUCCESS) {
		return; // no adapters.
	}

	auto* const pAdapterInfo = AdapterInfo;
	mac1 = hashMacAddress(pAdapterInfo);
	if (pAdapterInfo->Next != nullptr) {
		mac2 = hashMacAddress(pAdapterInfo->Next);
	}

	// sort the mac addresses. We don't want to invalidate
	// both macs if they just change order.
	if (mac1 > mac2) {
		const auto tmp = mac2;
		mac2 = mac1;
		mac1 = tmp;
	}
}

auto
getCpuHash() -> uint16_t
{
	int cpuinfo[4] = { 0, 0, 0, 0 };
	__cpuid(cpuinfo, 0);
	uint16_t hash = 0;
	auto* ptr = reinterpret_cast<uint16_t*>(&cpuinfo[0]);
	for (uint32_t i = 0; i < 8; i++) {
		hash += ptr[i];
	}

	return hash;
}

auto
getMachineName() -> string
{
	static char computerName[128];
	DWORD size = 128;
	GetComputerName(computerName, &size);
	return string(computerName);
}

#else
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
#include <net/if_types.h>
#else //! DARWIN
#include <linux/if.h>
#include <linux/sockios.h>
#endif //! DARWIN

#include <sys/resource.h>
#include <sys/utsname.h>

//---------------------------------get MAC addresses
//---------------------------------
// we just need this for purposes of unique machine id. So any one or two
// mac's is fine.
uint16_t
hashMacAddress(uint8_t* mac)
{
	uint16_t hash = 0;

	for (uint32_t i = 0; i < 6; i++) {
		hash += (mac[i] << ((i & 1) * 8));
	}
	return hash;
}

void
getMacHash(uint16_t& mac1, uint16_t& mac2)
{
	mac1 = 0;
	mac2 = 0;

#ifdef __APPLE__

	struct ifaddrs* ifaphead;
	if (getifaddrs(&ifaphead) != 0)
		return;

	// iterate over the net interfaces
	bool foundMac1 = false;
	struct ifaddrs* ifap;
	for (ifap = ifaphead; ifap; ifap = ifap->ifa_next) {
		struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifap->ifa_addr;
		if (sdl && (sdl->sdl_family == AF_LINK) &&
			(sdl->sdl_type == IFT_ETHER)) {
			if (!foundMac1) {
				foundMac1 = true;
				mac1 = hashMacAddress((uint8_t*)(LLADDR(sdl)));
			} else {
				mac2 = hashMacAddress((uint8_t*)(LLADDR(sdl)));
				break;
			}
		}
	}

	freeifaddrs(ifaphead);

#else // !DARWIN

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock < 0)
		return;

	// enumerate all IP addresses of the system
	struct ifconf conf;
	char ifconfbuf[128 * sizeof(struct ifreq)];
	memset(ifconfbuf, 0, sizeof(ifconfbuf));
	conf.ifc_buf = ifconfbuf;
	conf.ifc_len = sizeof(ifconfbuf);
	if (ioctl(sock, SIOCGIFCONF, &conf)) {
		assert(0);
		close(sock);
		return;
	}

	// get MAC address
	bool foundMac1 = false;
	struct ifreq* ifr;
	for (ifr = conf.ifc_req;
		 (signed char*)ifr < (signed char*)conf.ifc_req + conf.ifc_len;
		 ifr++) {
		if (ifr->ifr_addr.sa_data == (ifr + 1)->ifr_addr.sa_data)
			continue; // duplicate, skip it

		if (ioctl(sock, SIOCGIFFLAGS, ifr))
			continue; // failed to get flags, skip it
		if (ioctl(sock, SIOCGIFHWADDR, ifr) == 0) {
			if (!foundMac1) {
				foundMac1 = true;
				mac1 = hashMacAddress((uint8_t*)&(ifr->ifr_addr.sa_data));
			} else {
				mac2 = hashMacAddress((uint8_t*)&(ifr->ifr_addr.sa_data));
				break;
			}
		}
	}

	close(sock);

#endif // !DARWIN

	// sort the mac addresses. We don't want to invalidate
	// both macs if they just change order.
	if (mac1 > mac2) {
		uint16_t tmp = mac2;
		mac2 = mac1;
		mac1 = tmp;
	}
}

#ifdef __APPLE__
#include <mach-o/arch.h>
uint16_t
getCpuHash()
{
	const NXArchInfo* info = NXGetLocalArchInfo();
	uint16_t val = 0;
	val += (uint16_t)info->cputype;
	val += (uint16_t)info->cpusubtype;
	return val;
}

#else  // !DARWIN

uint16_t
getCpuHash()
{
	uint32_t cpuinfo[4] = { 0, 0, 0, 0 };
	__get_cpuid(0, &cpuinfo[0], &cpuinfo[1], &cpuinfo[2], &cpuinfo[3]);
	uint16_t hash = 0;
	uint32_t* ptr = (&cpuinfo[0]);
	for (uint32_t i = 0; i < 4; i++)
		hash += (ptr[i] & 0xFFFF) + (ptr[i] >> 16);

	return hash;
}
#endif // !DARWIN

string
getMachineName()
{
	static struct utsname u;

	if (uname(&u) < 0) {
		assert(0);
		return "unknown";
	}

	return string(u.nodename);
}
#endif

static auto
computeSystemUniqueId() -> uint16_t*
{
	static uint16_t id[3];
	static auto computed = false;
	if (computed) {
		return id;
	}

	// produce a number that uniquely identifies this system.
	id[0] = getCpuHash();
	getMacHash(id[1], id[2]);
	computed = true;
	return id;
}
auto
getSystemUniqueId() -> string
{
	// get the name of the computer
	auto str = getMachineName();

	auto* id = computeSystemUniqueId();
	for (uint32_t i = 0; i < 3; i++) {
		str = str + "." + std::to_string(id[i]);
	}
	return str;
}
/* Arcade:	for the current stage (one song).
 * Nonstop/Oni/Endless:	 for current course (which usually contains multiple
 * songs)
 */

StageStats::StageStats()
{
	m_Stage = Stage_Invalid;
	m_iStageIndex = -1;
	m_vpPlayedSongs.clear();
	m_vpPossibleSongs.clear();
	m_bGaveUp = false;
	m_bUsedAutoplay = false;
	m_fMusicRate = 1.F;
	m_player.Init(PLAYER_1);
	FOREACH_MultiPlayer(pn) { m_multiPlayer[pn].Init(pn); }
}

void
StageStats::Init()
{
	*this = StageStats();
}

void
StageStats::AssertValid(PlayerNumber pn) const
{
	ASSERT(!m_vpPlayedSongs.empty());
	ASSERT(!m_vpPossibleSongs.empty());
	ASSERT(m_player.m_iStepsPlayed > 0);
	ASSERT(!m_player.m_vpPossibleSteps.empty());
	ASSERT(m_player.m_vpPossibleSteps[0] != nullptr);
	ASSERT_M(m_player.m_vpPossibleSteps[0]->GetDifficulty() < NUM_Difficulty,
			 ssprintf("Invalid Difficulty %i",
					  m_player.m_vpPossibleSteps[0]->GetDifficulty()));
	ASSERT_M(static_cast<int>(m_vpPlayedSongs.size()) ==
			   m_player.m_iStepsPlayed,
			 ssprintf("%i Songs Played != %i Steps Played for player %i",
					  (int)m_vpPlayedSongs.size(),
					  (int)m_player.m_iStepsPlayed,
					  pn));
	ASSERT_M(m_vpPossibleSongs.size() == m_player.m_vpPossibleSteps.size(),
			 ssprintf("%i Possible Songs != %i Possible Steps for player %i",
					  (int)m_vpPossibleSongs.size(),
					  (int)m_player.m_vpPossibleSteps.size(),
					  pn));
}

void
StageStats::AssertValid(MultiPlayer pn) const
{
	ASSERT(!m_vpPlayedSongs.empty());
	ASSERT(!m_vpPossibleSongs.empty());
	ASSERT(!m_multiPlayer[pn].m_vpPossibleSteps.empty());
	ASSERT(m_multiPlayer[pn].m_vpPossibleSteps[0] != nullptr);
	ASSERT_M(m_player.m_vpPossibleSteps[0]->GetDifficulty() < NUM_Difficulty,
			 ssprintf("difficulty %i",
					  m_player.m_vpPossibleSteps[0]->GetDifficulty()));
	ASSERT(static_cast<int>(m_vpPlayedSongs.size()) == m_player.m_iStepsPlayed);
	ASSERT(m_vpPossibleSongs.size() == m_player.m_vpPossibleSteps.size());
}

auto
StageStats::GetAverageMeter(PlayerNumber pn) const -> int
{
	AssertValid(pn);

	// TODO(Sam): This isn't correct for courses.
	auto iTotalMeter = 0;

	for (unsigned i = 0; i < m_vpPlayedSongs.size(); i++) {
		const Steps* pSteps = m_player.m_vpPossibleSteps[i];
		iTotalMeter += pSteps->GetMeter();
	}
	return iTotalMeter / m_vpPlayedSongs.size(); // round down
}

void
StageStats::AddStats(const StageStats& other)
{
	ASSERT(!other.m_vpPlayedSongs.empty());
	for (const auto& s : other.m_vpPlayedSongs) {
		m_vpPlayedSongs.push_back(s);
	}

	for (const auto& s : other.m_vpPossibleSongs) {
		m_vpPossibleSongs.push_back(s);
	}

	m_Stage = Stage_Invalid; // meaningless
	m_iStageIndex = -1;		 // meaningless

	m_bGaveUp |= static_cast<int>(other.m_bGaveUp);
	m_bUsedAutoplay |= static_cast<int>(other.m_bUsedAutoplay);

	m_player.AddStats(other.m_player);
}

auto
StageStats::Failed() const -> bool
{
	return m_player.m_bFailed;
}

auto
StageStats::GetTotalPossibleStepsSeconds() const -> float
{
	float fSecs = 0;
	for (const auto& s : m_vpPossibleSongs) {
		fSecs += s->GetStepsSeconds();
	}
	return fSecs / m_fMusicRate;
}

// all the dumb reasons your score doesn't matter (or at least, most of them)
auto
DetermineScoreEligibility(const PlayerStageStats& pss, const PlayerState& ps)
  -> bool
{
	// chord cohesion is invalid
	if (!GAMESTATE->CountNotesSeparately()) {
		return false;
	}

	// you failed.
	if (pss.GetGrade() == Grade_Failed) {
		return false;
	}

	// just because you had failoff, doesn't mean you didn't fail.
	for (const auto& fail : pss.m_fLifeRecord) {
		if (fail.second == 0.F) {
			return false;
		}
	}

	// cut out stuff with under 200 notes to prevent super short vibro files
	// from being dumb
	if (pss.GetTotalTaps() < 200 && pss.GetTotalTaps() != 4) {
		return false;
	}

	// i'm not actually sure why this is here but if you activate this you don't
	// deserve points anyway
	if (pss.m_fWifeScore < 0.1F) {
		return false;
	}

	// no negative bpm garbage
	if (pss.filehadnegbpms) {
		return false;
	}

	// no lau script shenanigans
	if (pss.luascriptwasloaded) {
		return false;
	}

	// it would take some amount of effort to abuse this but hey, whatever
	if (pss.everusedautoplay) {
		return false;
	}

	auto& tfs = ps.m_PlayerOptions.GetStage().m_bTransforms;
	auto& turns = ps.m_PlayerOptions.GetStage().m_bTurns;

	// only do this if the file doesnt have mines
	if (tfs[PlayerOptions::TRANSFORM_NOMINES] && pss.filegotmines)
		return false;

	// only do this if the file has holds
	if ((tfs[PlayerOptions::TRANSFORM_NOHOLDS] ||
		 tfs[PlayerOptions::TRANSFORM_NOROLLS]) &&
		pss.filegotholds)
		return false;

	// invalidate if any transforms are on (Insert and Remove mods)
	// (this starts after nomines/noholds/norolls)
	for (int tfi = PlayerOptions::TRANSFORM_LITTLE;
		 tfi < PlayerOptions::NUM_TRANSFORMS;
		 tfi++) {
		PlayerOptions::Transform tf =
		  static_cast<PlayerOptions::Transform>(tfi);

		if (tfs[tf])
			return false;
	}

	// invalidate if any turns are on other than Mirror (shuffle)
	// (this starts after mirror)
	for (int ti = PlayerOptions::TURN_BACKWARDS;
		 ti < PlayerOptions::NUM_TURNS;
		 ti++) {
		PlayerOptions::Turn t =
		  static_cast<PlayerOptions::Turn>(ti);

		if (turns[t])
			return false;
	}

	// impossible for this to happen but just in case
	if (ps.m_PlayerOptions.GetStage().m_bPractice)
		return false;

	return true;
}

static auto
FillInHighScore(const PlayerStageStats& pss,
				const PlayerState& ps,
				const std::string& sRankingToFillInMarker,
				const std::string& sPlayerGuid) -> HighScore
{
	Locator::getLogger()->info("Filling Highscore");
	HighScore hs;
	hs.SetName(sRankingToFillInMarker);

	auto chartKey = GAMESTATE->m_pCurSteps->GetChartKey();
	hs.SetChartKey(chartKey);
	hs.SetGrade(pss.GetGrade());
	hs.SetMachineGuid(getSystemUniqueId());
	hs.SetScore(pss.m_iScore);
	hs.SetPercentDP(pss.GetPercentDancePoints());
	hs.SetWifeScore(pss.GetWifeScore());
	hs.SetWifePoints(pss.GetCurWifeScore());
	hs.SetMusicRate(GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate);
	hs.SetJudgeScale(pss.GetTimingScale());
	hs.SetChordCohesion(GAMESTATE->CountNotesSeparately());
	hs.SetMaxCombo(pss.GetMaxCombo().m_cnt);
	hs.SetPlayedSeconds(pss.m_fPlayedSeconds);

	std::vector<std::string> asModifiers;
	{
		auto sPlayerOptions = ps.m_PlayerOptions.GetStage().GetString();
		if (!sPlayerOptions.empty()) {
			asModifiers.push_back(sPlayerOptions);
		}
		auto sSongOptions = GAMESTATE->m_SongOptions.GetStage().GetString();
		if (!sSongOptions.empty()) {
			asModifiers.push_back(sSongOptions);
		}
	}
	const auto modstr = join(", ", asModifiers);
	hs.SetModifiers(modstr);

	hs.SetDateTime(DateTime::GetNowDateTime());
	hs.SetPlayerGuid(sPlayerGuid);
	FOREACH_ENUM(TapNoteScore, tns)
	hs.SetTapNoteScore(tns, pss.m_iTapNoteScores[tns]);
	FOREACH_ENUM(HoldNoteScore, hns)
	hs.SetHoldNoteScore(hns, pss.m_iHoldNoteScores[hns]);
	hs.SetRadarValues(pss.m_radarActual);
	hs.SetLifeRemainingSeconds(pss.m_fLifeRemainingSeconds);
	hs.SetDisqualified(pss.IsDisqualified());
	hs.SetDSFlag(pss.usedDoubleSetup);
	hs.SetStageSeed(GAMESTATE->m_iStageSeed);
	hs.SetSongOffset(ps.GetDisplayedTiming().m_fBeat0OffsetInSeconds);

	// Etterna validity check, used for ssr/eo eligibility -mina
	hs.SetEtternaValid(DetermineScoreEligibility(pss, ps));

	// force fail grade if player 'gave up', autoplay was used, or lua scripts
	// were loaded (this is sorta redundant with the above but ehh) -mina
	if (pss.gaveuplikeadumbass || pss.luascriptwasloaded ||
		pss.everusedautoplay) {
		hs.SetGrade(Grade_Failed);
	}

	// should maybe just make the setscorekey function do this internally rather
	// than recalling the datetime object -mina
	auto ScoreKey =
	  "S" +
	  BinaryToHex(CryptManager::GetSHA1ForString(hs.GetDateTime().GetString()));
	hs.SetScoreKey(ScoreKey);

	// DOES NOT WORK NEEDS FIX -mina
	// the vectors stored in pss are what are accessed by evaluation so we can
	// write them to the replay file instead of the highscore object (if
	// successful) -mina this is kinda messy meh -mina

	if (pss.m_fWifeScore > 0.F) {
		hs.SetOffsetVector(pss.GetOffsetVector());
		hs.SetNoteRowVector(pss.GetNoteRowVector());
		hs.SetTrackVector(pss.GetTrackVector());
		hs.SetTapNoteTypeVector(pss.GetTapNoteTypeVector());
		hs.SetHoldReplayDataVector(pss.GetHoldReplayDataVector());
		hs.SetMineReplayDataVector(pss.GetMineReplayDataVector());

		// ok this is a little jank but there's a few things going on here,
		// first we can't trust that scores getting here are necessarily either
		// fully completed or fails, so we can't trust that the wifescore in pss
		// is necessarily the correct wifescore, or that rescoring using the
		// offsetvectors will produce it
		// second, since there is no setwifeversion function, newly minted
		// scores won't have wifevers set, will be uploaded, then rescored on
		// next load, and uploaded again, not a huge deal, but still undesirable
		// thankfully we can fix both problems by just rescoring to wife3, this
		// will properly set the wifescore as well as wife vers flags

		// for this we need the actual totalpoints values, so we need steps data
		auto steps = GAMESTATE->m_pCurSteps;
		auto st = steps->m_StepsType;
		auto* style = GAMEMAN->GetStyleForStepsType(st);
		auto nd = steps->GetNoteData();
		auto* td = steps->GetTimingData();

		// transform the notedata by style if necessary
		if (style != nullptr) {
			NoteData ndo;
			style->GetTransformedNoteDataForStyle(PLAYER_1, nd, ndo);
			nd = ndo;
		}

		// Have to account for mirror and shuffle
		if (style != nullptr && td != nullptr) {
			PlayerOptions po;
			po.Init();
			po.SetForReplay(true);
			po.FromString(modstr);
			auto tmpSeed = GAMESTATE->m_iStageSeed;

			// if rng was not saved, only apply non shuffle mods
			if (hs.GetStageSeed() == 0) {
				po.m_bTurns[PlayerOptions::TURN_SHUFFLE] = false;
				po.m_bTurns[PlayerOptions::TURN_SOFT_SHUFFLE] = false;
				po.m_bTurns[PlayerOptions::TURN_SUPER_SHUFFLE] = false;
				po.m_bTurns[PlayerOptions::TURN_HRAN_SHUFFLE] = false;
			} else {
				GAMESTATE->m_iStageSeed = hs.GetStageSeed();
			}

			NoteDataUtil::TransformNoteData(nd, *td, po, style->m_StepsType);
			GAMESTATE->m_iStageSeed = tmpSeed;
		}
		auto maxpoints = static_cast<float>(nd.WifeTotalScoreCalc(td));

		// i _think_ an assert is ok here.. if this can happen we probably want
		// to know about it
		ASSERT(maxpoints > 0);

		if (pss.GetGrade() == Grade_Failed) {
			hs.SetSSRNormPercent(0.F);
		} else {
			hs.RescoreToWife3(maxpoints);
		}

		if (hs.GetEtternaValid()) {
			auto dakine = pss.CalcSSR(hs.GetSSRNormPercent());
			FOREACH_ENUM(Skillset, ss)
			hs.SetSkillsetSSR(ss, dakine[ss]);

			hs.SetSSRCalcVersion(GetCalcVersion());
		} else {
			FOREACH_ENUM(Skillset, ss)
			hs.SetSkillsetSSR(ss, 0.F);
		}
	}

	// Input data
	hs.SetInputDataVector(pss.GetInputDataVector());
	hs.SetMissDataVector(pss.GetMissDataVector());

	// Normalize Judgments to J4 (regardless of wifepercent)
	// If it fails, reset the replay data from pss and try one more time
	if (!hs.NormalizeJudgments()) {
		hs.SetOffsetVector(pss.GetOffsetVector());
		hs.SetNoteRowVector(pss.GetNoteRowVector());
		hs.SetTapNoteTypeVector(pss.GetTapNoteTypeVector());
		// potentially empty, that's fine
		hs.SetTrackVector(pss.GetTrackVector());

		if (!hs.NormalizeJudgments())
			Locator::getLogger()->warn(
			  "Failed to normalize judgments for HighScore Key {}",
			  hs.GetScoreKey());
	}

	hs.GenerateValidationKeys();

	return hs;
}

void
StageStats::FinalizeScores()
{
	Locator::getLogger()->info("Finalizing Score");
	// if we're viewing an online replay this gets set to true -mina
	SCOREMAN->camefromreplay = false;

	// don't save scores if the player chose not to
	if (!GAMESTATE->m_SongOptions.GetCurrent().m_bSaveScore) {
		Locator::getLogger()->info("nevermind");
		return;
	}

	auto* const profile = PROFILEMAN->GetProfile(PLAYER_1);
	const auto sPlayerGuid = profile->m_sGuid;

	// make the highscore
	m_player.m_HighScore = FillInHighScore(m_player,
										   *GAMESTATE->m_pPlayerState,
										   RANKING_TO_FILL_IN_MARKER,
										   sPlayerGuid);

	auto& hs = m_player.m_HighScore;
	auto* pSteps = GAMESTATE->m_pCurSteps.Get();
	assert(pSteps != nullptr);

	// cope with autoplay and replays
	if (GamePreferences::m_AutoPlay != PC_HUMAN) {
		if (REPLAYS->GetActiveReplayScore() != nullptr) {
			if (!REPLAYS->GetActiveReplayScore()
				   ->GetCopyOfSetOnlineReplayTimestampVector()
				   .empty()) {
				// it's an online replay
				Locator::getLogger()->info(
				  "Online Replay detected - saved nothing");
				SCOREMAN->tempscoreforonlinereplayviewing =
				  REPLAYS->GetActiveReplayScore();
				SCOREMAN->tempscoreforonlinereplayviewing->SetRadarValues(
				  hs.GetRadarValues());
				SCOREMAN->camefromreplay = true;
			} else {
				// it's a local replay
				Locator::getLogger()->info(
				  "Local Replay detected - saved nothing");
				mostrecentscorekey = REPLAYS->GetActiveReplay()->GetScoreKey();
				SCOREMAN->PutScoreAtTheTop(mostrecentscorekey);
				if (SCOREMAN->GetMostRecentScore() == nullptr)
					Locator::getLogger()->warn("MOST RECENT SCORE WAS EMPTY.");
				else
					SCOREMAN->GetMostRecentScore()->SetRadarValues(
					  hs.GetRadarValues());
			}
		} else {
			// autoplay
			Locator::getLogger()->info("Autoplay detected - saved nothing");
		}
		profile->m_lastSong.FromSong(GAMESTATE->m_pCurSong);
		return;
	}

	// practice mode
	if (GAMESTATE->IsPracticeMode()) {
		Locator::getLogger()->info("Practice detected - saved nothing");
		SCOREMAN->camefromreplay = true;
		SCOREMAN->tempscoreforonlinereplayviewing = &hs;
		profile->m_lastSong.FromSong(GAMESTATE->m_pCurSong);
		return;
	}

	// determine topscore flag, add score to profile
	const auto topScore = SCOREMAN->AddScore(hs);

	// upload score
	if (DLMAN->ShouldUploadScores() && !AdjustSync::IsSyncDataChanged()) {
		Locator::getLogger()->info("Uploading score with replaydata");

		// topscore is only set for a score eligible to be uploaded
		hs.SetTopScore(topScore);

		// initial score upload
		auto* td = pSteps->GetTimingData();
		hs.timeStamps = td->ConvertReplayNoteRowsToTimestamps(
		  m_player.GetNoteRowVector(), hs.GetMusicRate());
		DLMAN->UploadScoreWithReplayData(&hs);
		hs.timeStamps.clear();
		hs.timeStamps.shrink_to_fit();

		// mega hack to stop non-pbs from overwriting pbs on eo (it happens rate
		// specific), we're just going to also upload whatever the pb for the
		// rate is now, since the site only tracks the best score per rate.
		// If there's no more replaydata on disk for the old pb this could maybe
		// be a problem and perhaps the better solution would be to check what
		// is listed on the site for this rate before uploading the score just
		// achieved but idk someone else can look into that

		// this _should_ be sound since addscore handles all re-evaluation of
		// top score flags and the setting of pbptrs
		auto* pbhs = SCOREMAN->GetChartPBAt(
		  pSteps->GetChartKey(),
		  GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate);
		if (pbhs->GetScoreKey() != hs.GetScoreKey()) {
			DLMAN->UploadScoreWithReplayDataFromDisk(pbhs);
		}
	}

	// tell multiplayer a score was set
	if (NSMAN->loggedIn) {
		NSMAN->ReportHighScore(&hs, m_player);
	}

	// write replays
	{
		// replay data (deprecated)
		hs.WriteReplayData();

		// compressed input data
		hs.WriteInputData();
	}

	profile->SetAnyAchievedGoals(GAMESTATE->m_pCurSteps->GetChartKey(),
							 GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate,
							 hs);
	mostrecentscorekey = hs.GetScoreKey();
	profile->m_lastSong.FromSong(GAMESTATE->m_pCurSong);

	// save for real
	if (m_bLivePlay) {
		GAMESTATE->SavePlayerProfile();
	}

	Locator::getLogger()->info("Done saving stats and high scores");
}

auto
StageStats::GetMinimumMissCombo() const -> unsigned int
{
	unsigned int iMin = INT_MAX;
	iMin = std::min(iMin, m_player.m_iCurMissCombo);
	return iMin;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the StageStats. */
class LunaStageStats : public Luna<StageStats>
{
  public:
	static auto GetPlayerStageStats(T* p, lua_State* L) -> int
	{
		p->m_player.PushSelf(L);
		return 1;
	}
	static auto GetMultiPlayerStageStats(T* p, lua_State* L) -> int
	{
		p->m_multiPlayer[Enum::Check<MultiPlayer>(L, 1)].PushSelf(L);
		return 1;
	}
	static auto GetPlayedSongs(T* p, lua_State* L) -> int
	{
		lua_newtable(L);
		for (auto i = 0; i < static_cast<int>(p->m_vpPlayedSongs.size()); ++i) {
			p->m_vpPlayedSongs[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	static auto GetPossibleSongs(T* p, lua_State* L) -> int
	{
		lua_newtable(L);
		for (auto i = 0; i < static_cast<int>(p->m_vpPossibleSongs.size());
			 ++i) {
			p->m_vpPossibleSongs[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	static auto Failed(T* p, lua_State* L) -> int
	{
		lua_pushboolean(L, p->Failed());
		return 1;
	}
	static auto GetStage(T* p, lua_State* L) -> int
	{
		LuaHelpers::Push(L, p->m_Stage);
		return 1;
	}
	DEFINE_METHOD(GetStageIndex, m_iStageIndex)
	static auto GetLivePlay(T* p, lua_State* L) -> int
	{
		lua_pushboolean(L, static_cast<int>(p->m_bLivePlay));
		return 1;
	}

	LunaStageStats()
	{
		ADD_METHOD(GetPlayerStageStats);
		ADD_METHOD(GetMultiPlayerStageStats);
		ADD_METHOD(GetPlayedSongs);
		ADD_METHOD(GetPossibleSongs);
		ADD_METHOD(Failed);
		ADD_METHOD(GetStage);
		ADD_METHOD(GetStageIndex);
		ADD_METHOD(GetLivePlay);
	}
};

LUA_REGISTER_CLASS(StageStats)
// lua end
