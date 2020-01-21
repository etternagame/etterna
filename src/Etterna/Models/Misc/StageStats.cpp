#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Foreach.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Globals/MinaCalc.h"
#include "PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Profile.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "StageStats.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "PlayerAI.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "AdjustSync.h"
#include <fstream>
#include <sstream>
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Globals/MinaCalc.h"
#include "Etterna/Models/Songs/Song.h"
#include "GamePreferences.h"

#ifndef _WIN32
#include <cpuid.h>
#endif

#ifdef _WIN32
#include <intrin.h>
#include <iphlpapi.h>
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "IPHLPAPI.lib")

// we just need this for purposes of unique machine id.
// So any one or two mac's is fine.
uint16_t
hashMacAddress(PIP_ADAPTER_INFO info)
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

	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
	if (dwStatus != ERROR_SUCCESS)
		return; // no adapters.

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
	mac1 = hashMacAddress(pAdapterInfo);
	if (pAdapterInfo->Next)
		mac2 = hashMacAddress(pAdapterInfo->Next);

	// sort the mac addresses. We don't want to invalidate
	// both macs if they just change order.
	if (mac1 > mac2) {
		uint16_t tmp = mac2;
		mac2 = mac1;
		mac1 = tmp;
	}
}

uint16_t
getCpuHash()
{
	int cpuinfo[4] = { 0, 0, 0, 0 };
	__cpuid(cpuinfo, 0);
	uint16_t hash = 0;
	uint16_t* ptr = (uint16_t*)(&cpuinfo[0]);
	for (uint32_t i = 0; i < 8; i++)
		hash += ptr[i];

	return hash;
}

string
getMachineName()
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

static uint16_t*
computeSystemUniqueId()
{
	static uint16_t id[3];
	static bool computed = false;
	if (computed)
		return id;

	// produce a number that uniquely identifies this system.
	id[0] = getCpuHash();
	getMacHash(id[1], id[2]);
	computed = true;
	return id;
}
string
getSystemUniqueId()
{
	// get the name of the computer
	string str = getMachineName();

	uint16_t* id = computeSystemUniqueId();
	for (uint32_t i = 0; i < 3; i++)
		str = str + "." + to_string(id[i]);
	return str;
}
/* Arcade:	for the current stage (one song).
 * Nonstop/Oni/Endless:	 for current course (which usually contains multiple
 * songs)
 */

StageStats::StageStats()
{
	m_playMode = PlayMode_Invalid;
	m_Stage = Stage_Invalid;
	m_iStageIndex = -1;
	m_vpPlayedSongs.clear();
	m_vpPossibleSongs.clear();
	m_bGaveUp = false;
	m_bUsedAutoplay = false;
	m_fGameplaySeconds = 0;
	m_fStepsSeconds = 0;
	m_fMusicRate = 1;
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
	ASSERT(m_vpPlayedSongs.size() != 0);
	ASSERT(m_vpPossibleSongs.size() != 0);
	ASSERT(m_player.m_iStepsPlayed > 0);
	ASSERT(m_player.m_vpPossibleSteps.size() != 0);
	ASSERT(m_player.m_vpPossibleSteps[0] != NULL);
	ASSERT_M(m_playMode < NUM_PlayMode, ssprintf("playmode %i", m_playMode));
	ASSERT_M(m_player.m_vpPossibleSteps[0]->GetDifficulty() < NUM_Difficulty,
			 ssprintf("Invalid Difficulty %i",
					  m_player.m_vpPossibleSteps[0]->GetDifficulty()));
	ASSERT_M((int)m_vpPlayedSongs.size() == m_player.m_iStepsPlayed,
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
	ASSERT(m_vpPlayedSongs.size() != 0);
	ASSERT(m_vpPossibleSongs.size() != 0);
	ASSERT(m_multiPlayer[pn].m_vpPossibleSteps.size() != 0);
	ASSERT(m_multiPlayer[pn].m_vpPossibleSteps[0] != NULL);
	ASSERT_M(m_playMode < NUM_PlayMode, ssprintf("playmode %i", m_playMode));
	ASSERT_M(m_player.m_vpPossibleSteps[0]->GetDifficulty() < NUM_Difficulty,
			 ssprintf("difficulty %i",
					  m_player.m_vpPossibleSteps[0]->GetDifficulty()));
	ASSERT((int)m_vpPlayedSongs.size() == m_player.m_iStepsPlayed);
	ASSERT(m_vpPossibleSongs.size() == m_player.m_vpPossibleSteps.size());
}

int
StageStats::GetAverageMeter(PlayerNumber pn) const
{
	AssertValid(pn);

	// TODO: This isn't correct for courses.
	int iTotalMeter = 0;

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
	FOREACH_CONST(Song*, other.m_vpPlayedSongs, s)
	m_vpPlayedSongs.push_back(*s);
	FOREACH_CONST(Song*, other.m_vpPossibleSongs, s)
	m_vpPossibleSongs.push_back(*s);
	m_Stage = Stage_Invalid; // meaningless
	m_iStageIndex = -1;		 // meaningless

	m_bGaveUp |= static_cast<int>(other.m_bGaveUp);
	m_bUsedAutoplay |= static_cast<int>(other.m_bUsedAutoplay);

	m_fGameplaySeconds += other.m_fGameplaySeconds;
	m_fStepsSeconds += other.m_fStepsSeconds;

	m_player.AddStats(other.m_player);
}

bool
StageStats::OnePassed() const
{
	if (!m_player.m_bFailed)
		return true;
	return false;
}

bool
StageStats::AllFailed() const
{
	if (!m_player.m_bFailed)
		return false;
	return true;
}

float
StageStats::GetTotalPossibleStepsSeconds() const
{
	float fSecs = 0;
	FOREACH_CONST(Song*, m_vpPossibleSongs, s)
	fSecs += (*s)->GetStepsSeconds();
	return fSecs / m_fMusicRate;
}

// all the dumb reasons your score doesnt matter (or at least, most of them)
// -mina
bool
DetermineScoreEligibility(const PlayerStageStats& pss, const PlayerState& ps)
{

	// 4k only
	if (GAMESTATE->m_pCurSteps->m_StepsType != StepsType_dance_single)
		return false;

	// chord cohesion is invalid
	if (!GAMESTATE->CountNotesSeparately())
		return false;

	// you failed.
	if (pss.GetGrade() == Grade_Failed)
		return false;

	// just because you had failoff, doesn't mean you didn't fail.
	FOREACHM_CONST(float, float, pss.m_fLifeRecord, fail)
	if (fail->second == 0.f)
		return false;

	// cut out stuff with under 200 notes to prevent super short vibro files
	// from being dumb
	if (pss.GetTotalTaps() < 200 && pss.GetTotalTaps() != 4)
		return false;

	// i'm not actually sure why this is here but if you activate this you don't
	// deserve points anyway
	if (pss.m_fWifeScore < 0.1f)
		return false;

	// no negative bpm garbage
	if (pss.filehadnegbpms)
		return false;

	// no lau script shenanigans
	if (pss.luascriptwasloaded)
		return false;

	// it would take some amount of effort to abuse this but hey, whatever
	if (pss.everusedautoplay)
		return false;

	// mods that modify notedata other than mirror (too lazy to figure out how
	// to check for these in po)
	string mods = ps.m_PlayerOptions.GetStage().GetString();

	// should take care of all 3 shuffle mods
	if (mods.find("Shuffle") != mods.npos)
		return false;

	// only do this if the file doesnt have mines
	if (mods.find("NoMines") != mods.npos && pss.filegotmines)
		return false;

	// This is a mod which adds mines, replacing existing notes and making files
	// easier
	if (ps.m_PlayerOptions.GetStage()
		  .m_bTransforms[PlayerOptions::TRANSFORM_MINES])
		return false;

	// this would be difficult to accomplish but for parity's sake we should
	if (mods.find("NoHolds") != mods.npos && pss.filegotholds)
		return false;

	if (mods.find("Left") != mods.npos)
		return false;

	if (mods.find("Right") != mods.npos)
		return false;

	if (mods.find("Backwards") != mods.npos)
		return false;

	if (mods.find("Little") != mods.npos)
		return false;

	if (mods.find("NoJumps") != mods.npos)
		return false;

	if (mods.find("NoHands") != mods.npos)
		return false;

	if (mods.find("NoQuads") != mods.npos)
		return false;

	if (mods.find("NoStretch") != mods.npos)
		return false;

	return true;
}

static HighScore
FillInHighScore(const PlayerStageStats& pss,
				const PlayerState& ps,
				RString sRankingToFillInMarker,
				RString sPlayerGuid)
{
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
	hs.SetAliveSeconds(pss.m_fAliveSeconds);
	hs.SetMaxCombo(pss.GetMaxCombo().m_cnt);

	vector<RString> asModifiers;
	{
		RString sPlayerOptions = ps.m_PlayerOptions.GetStage().GetString();
		if (!sPlayerOptions.empty())
			asModifiers.push_back(sPlayerOptions);
		RString sSongOptions = GAMESTATE->m_SongOptions.GetStage().GetString();
		if (!sSongOptions.empty())
			asModifiers.push_back(sSongOptions);
	}
	hs.SetModifiers(join(", ", asModifiers));

	hs.SetDateTime(DateTime::GetNowDateTime());
	hs.SetPlayerGuid(sPlayerGuid);
	FOREACH_ENUM(TapNoteScore, tns)
	hs.SetTapNoteScore(tns, pss.m_iTapNoteScores[tns]);
	FOREACH_ENUM(HoldNoteScore, hns)
	hs.SetHoldNoteScore(hns, pss.m_iHoldNoteScores[hns]);
	hs.SetRadarValues(pss.m_radarActual);
	hs.SetLifeRemainingSeconds(pss.m_fLifeRemainingSeconds);
	hs.SetDisqualified(pss.IsDisqualified());

	// Etterna validity check, used for ssr/eo eligibility -mina
	hs.SetEtternaValid(DetermineScoreEligibility(pss, ps));

	// force fail grade if player 'gave up', autoplay was used, or lua scripts
	// were loaded (this is sorta redundant with the above but ehh) -mina
	if (pss.gaveuplikeadumbass || pss.luascriptwasloaded ||
		pss.everusedautoplay)
		hs.SetGrade(Grade_Failed);

	// should maybe just make the setscorekey function do this internally rather
	// than recalling the datetime object -mina
	RString ScoreKey =
	  "S" +
	  BinaryToHex(CryptManager::GetSHA1ForString(hs.GetDateTime().GetString()));
	hs.SetScoreKey(ScoreKey);

	// DOES NOT WORK NEEDS FIX -mina
	// the vectors stored in pss are what are accessed by evaluation so we can
	// write them to the replay file instead of the highscore object (if
	// successful) -mina this is kinda messy meh -mina

	if (pss.m_fWifeScore > 0.f) {
		hs.SetOffsetVector(pss.GetOffsetVector());
		hs.SetNoteRowVector(pss.GetNoteRowVector());
		hs.SetTrackVector(pss.GetTrackVector());
		hs.SetTapNoteTypeVector(pss.GetTapNoteTypeVector());
		hs.SetHoldReplayDataVector(pss.GetHoldReplayDataVector());
		hs.SetReplayType(
		  2); // flag this before rescore so it knows we're LEGGIT

		if (pss.GetGrade() == Grade_Failed)
			hs.SetSSRNormPercent(0.f);
		else
			hs.SetSSRNormPercent(hs.RescoreToWifeJudge(4));

		if (hs.GetEtternaValid()) {
			vector<float> dakine = pss.CalcSSR(hs.GetSSRNormPercent());
			FOREACH_ENUM(Skillset, ss)
			hs.SetSkillsetSSR(ss, dakine[ss]);

			hs.SetSSRCalcVersion(GetCalcVersion());
		} else {
			FOREACH_ENUM(Skillset, ss)
			hs.SetSkillsetSSR(ss, 0.f);
		}
	}

	hs.GenerateValidationKeys();

	if (!pss.InputData.empty())
		hs.WriteInputData(pss.InputData);
	return hs;
}

void
StageStats::FinalizeScores(bool bSummary)
{
	SCOREMAN->camefromreplay =
	  false; // if we're viewing an online replay this gets set to true -mina
	if (PREFSMAN->m_sTestInitialScreen.Get() != "") {
		m_player.m_iPersonalHighScoreIndex = 0;
		m_player.m_iMachineHighScoreIndex = 0;
	}

	// don't save scores if the player chose not to
	if (!GAMESTATE->m_SongOptions.GetCurrent().m_bSaveScore)
		return;

	LOG->Trace("saving stats and high scores");

	// generate a HighScore for each player

	// whether or not to save scores when the stage was failed depends on if
	// this is a course or not... it's handled below in the switch.
	RString sPlayerGuid = PROFILEMAN->IsPersistentProfile(PLAYER_1)
							? PROFILEMAN->GetProfile(PLAYER_1)->m_sGuid
							: RString("");
	m_player.m_HighScore = FillInHighScore(m_player,
										   *GAMESTATE->m_pPlayerState,
										   RANKING_TO_FILL_IN_MARKER,
										   sPlayerGuid);

	HighScore& hs = m_player.m_HighScore;

	const Steps* pSteps = GAMESTATE->m_pCurSteps;

	ASSERT(pSteps != NULL);
	Profile* zzz = PROFILEMAN->GetProfile(PLAYER_1);
	if (GamePreferences::m_AutoPlay != PC_HUMAN) {
		if (PlayerAI::pScoreData) {
			if (!PlayerAI::pScoreData->GetCopyOfSetOnlineReplayTimestampVector()
				   .empty()) {
				SCOREMAN->tempscoreforonlinereplayviewing =
				  PlayerAI::pScoreData;
				SCOREMAN->tempscoreforonlinereplayviewing->SetRadarValues(
				  hs.GetRadarValues());
				SCOREMAN->camefromreplay = true;
			} else { // dont do this if the replay was from online or bad stuff
					 // happens -mina
				mostrecentscorekey = PlayerAI::pScoreData->GetScoreKey();
				SCOREMAN->PutScoreAtTheTop(mostrecentscorekey);
				SCOREMAN->GetMostRecentScore()->SetRadarValues(
				  hs.GetRadarValues());
			}
		}
		zzz->m_lastSong.FromSong(GAMESTATE->m_pCurSong);
		return;
	}

	if (GAMESTATE->IsPracticeMode()) {
		SCOREMAN->camefromreplay = true;
		SCOREMAN->tempscoreforonlinereplayviewing = &hs;
		zzz->m_lastSong.FromSong(GAMESTATE->m_pCurSong);
		return;
	}
	// new score structure -mina
	int istop2 = SCOREMAN->AddScore(hs);
	if (DLMAN->ShouldUploadScores() && !AdjustSync::IsSyncDataChanged()) {
		CHECKPOINT_M("Uploading score with replaydata.");
		hs.SetTopScore(istop2); // ayy i did it --lurker
		auto steps = SONGMAN->GetStepsByChartkey(hs.GetChartKey());
		auto td = steps->GetTimingData();
		hs.timeStamps = td->ConvertReplayNoteRowsToTimestamps(
		  m_player.GetNoteRowVector(), hs.GetMusicRate());
		DLMAN->UploadScoreWithReplayData(&hs);
		hs.timeStamps.clear();
		hs.timeStamps.shrink_to_fit();
	}
	if (NSMAN->loggedIn)
		NSMAN->ReportHighScore(&hs, m_player);
	if (m_player.m_fWifeScore > 0.f) {

		bool writesuccess = hs.WriteReplayData();
		if (writesuccess)
			hs.UnloadReplayData();
	}
	zzz->SetAnyAchievedGoals(GAMESTATE->m_pCurSteps->GetChartKey(),
							 GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate,
							 hs);
	mostrecentscorekey = hs.GetScoreKey();
	zzz->m_lastSong.FromSong(GAMESTATE->m_pCurSong);

	LOG->Trace("done saving stats and high scores");
}

// all scores are saved so all scores are highscores, remove this later -mina
bool
StageStats::PlayerHasHighScore(PlayerNumber pn) const
{
	return true;
}

unsigned int
StageStats::GetMinimumMissCombo() const
{
	unsigned int iMin = INT_MAX;
	iMin = min(iMin, m_player.m_iCurMissCombo);
	return iMin;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the StageStats. */
class LunaStageStats : public Luna<StageStats>
{
  public:
	static int GetPlayerStageStats(T* p, lua_State* L)
	{
		p->m_player.PushSelf(L);
		return 1;
	}
	static int GetMultiPlayerStageStats(T* p, lua_State* L)
	{
		p->m_multiPlayer[Enum::Check<MultiPlayer>(L, 1)].PushSelf(L);
		return 1;
	}
	static int GetPlayedSongs(T* p, lua_State* L)
	{
		lua_newtable(L);
		for (int i = 0; i < (int)p->m_vpPlayedSongs.size(); ++i) {
			p->m_vpPlayedSongs[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	static int GetPossibleSongs(T* p, lua_State* L)
	{
		lua_newtable(L);
		for (int i = 0; i < (int)p->m_vpPossibleSongs.size(); ++i) {
			p->m_vpPossibleSongs[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	static int GetGameplaySeconds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_fGameplaySeconds);
		return 1;
	}
	static int OnePassed(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->OnePassed());
		return 1;
	}
	static int AllFailed(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->AllFailed());
		return 1;
	}
	static int GetStage(T* p, lua_State* L)
	{
		LuaHelpers::Push(L, p->m_Stage);
		return 1;
	}
	DEFINE_METHOD(GetStageIndex, m_iStageIndex)
	DEFINE_METHOD(GetStepsSeconds, m_fStepsSeconds)
	static int PlayerHasHighScore(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->PlayerHasHighScore(PLAYER_1));
		return 1;
	}
	static int GetLivePlay(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->m_bLivePlay);
		return 1;
	}

	LunaStageStats()
	{
		ADD_METHOD(GetPlayerStageStats);
		ADD_METHOD(GetMultiPlayerStageStats);
		ADD_METHOD(GetPlayedSongs);
		ADD_METHOD(GetPossibleSongs);
		ADD_METHOD(GetGameplaySeconds);
		ADD_METHOD(OnePassed);
		ADD_METHOD(AllFailed);
		ADD_METHOD(GetStage);
		ADD_METHOD(GetStageIndex);
		ADD_METHOD(GetStepsSeconds);
		ADD_METHOD(PlayerHasHighScore);
		ADD_METHOD(GetLivePlay);
	}
};

LUA_REGISTER_CLASS(StageStats)
// lua end
