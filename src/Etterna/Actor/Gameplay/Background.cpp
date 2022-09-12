#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Background.h"
#include "Etterna/Models/Misc/BackgroundUtil.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/NoteTypes.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Actor/Base/Quad.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Graphics/RageTextureManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/FileTypes/XmlFileUtil.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Globals/rngthing.h"

#include <deque>
#include <algorithm>

using std::deque;
using std::map;

static ThemeMetric<float> LEFT_EDGE("Background", "LeftEdge");
static ThemeMetric<float> TOP_EDGE("Background", "TopEdge");
static ThemeMetric<float> RIGHT_EDGE("Background", "RightEdge");
static ThemeMetric<float> BOTTOM_EDGE("Background", "BottomEdge");
static ThemeMetric<float> CLAMP_OUTPUT_PERCENT("Background",
											   "ClampOutputPercent");
static ThemeMetric<bool> USE_STATIC_BG("Background", "UseStaticBackground");
static ThemeMetric<float> RAND_BG_START_BEAT("Background", "RandomBGStartBeat");
static ThemeMetric<float> RAND_BG_CHANGE_MEASURES("Background",
												  "RandomBGChangeMeasures");
static ThemeMetric<bool> RAND_BG_CHANGES_WHEN_BPM_CHANGES(
  "Background",
  "RandomBGChangesWhenBPMChangesAtMeasureStart");
static ThemeMetric<bool> RAND_BG_ENDS_AT_LAST_BEAT("Background",
												   "RandomBGEndsAtLastBeat");

static Preference<bool> g_bShowDanger("ShowDanger", false);
static Preference<RandomBackgroundMode> g_RandomBackgroundMode(
  "RandomBackgroundMode",
  BGMODE_OFF);
static Preference<int> g_iNumBackgrounds("NumBackgrounds", 10);
static Preference<bool> g_bSongBackgrounds("SongBackgrounds", true);

class BrightnessOverlay : public ActorFrame
{
  public:
	BrightnessOverlay();
	void Update(float fDeltaTime) override;

	void FadeToActualBrightness();
	void SetActualBrightness();
	void Set(float fBrightness);

  private:
	Quad m_quadBGBrightness;
	Quad m_quadBGBrightnessFade;
};

struct BackgroundTransition
{
	apActorCommands cmdLeaves;
	apActorCommands cmdRoot;
};

class BackgroundImpl : public ActorFrame
{
  public:
	BackgroundImpl();
	~BackgroundImpl() override;
	void Init();

	virtual void LoadFromSong(const Song* pSong);
	virtual void Unload();

	void Update(float fDeltaTime) override;
	void DrawPrimitives() override;

	void FadeToActualBrightness() { m_Brightness.FadeToActualBrightness(); }
	void SetBrightness(float fBrightness)
	{
		m_Brightness.Set(fBrightness);
	} /* overrides pref and Cover */

	void GetLoadedBackgroundChanges(
	  std::vector<BackgroundChange>* pBackgroundChangesOut[NUM_BackgroundLayer]);

  protected:
	bool m_bInitted;
	const Song* m_pSong;
	map<std::string, BackgroundTransition> m_mapNameToTransition;
	deque<BackgroundDef> m_RandomBGAnimations; // random background to choose
											   // from.  These may or may not be
											   // loaded into m_BGAnimations.

	void LoadFromRandom(float fFirstBeat,
						float fEndBeat,
						const BackgroundChange& change);
	bool IsDangerAllVisible();

	class Layer
	{
	  public:
		Layer();
		void Unload();

		// return true if created and added to m_BGAnimations
		bool CreateBackground(const Song* pSong,
							  const BackgroundDef& bd,
							  Actor* pParent);
		// return def of the background that was created and added to
		// m_BGAnimations. calls CreateBackground
		BackgroundDef CreateRandomBGA(const Song* pSong,
									  const std::string& sEffect,
									  deque<BackgroundDef>& RandomBGAnimations,
									  Actor* pParent);

		int FindBGSegmentForBeat(float fBeat) const;
		void UpdateCurBGChange(
		  const Song* pSong,
		  float fLastMusicSeconds,
		  float fCurrentTime,
		  const map<std::string, BackgroundTransition>& mapNameToTransition);

		map<BackgroundDef, Actor*> m_BGAnimations;
		std::vector<BackgroundChange> m_aBGChanges;
		int m_iCurBGChangeIndex;
		Actor* m_pCurrentBGA;
		Actor* m_pFadingBGA;
	};
	Layer m_Layer[NUM_BackgroundLayer];

	float m_fLastMusicSeconds;
	bool m_bDangerAllWasVisible;

	// cover up the edge of animations that might hang outside of the background
	// rectangle
	Quad m_quadBorderLeft, m_quadBorderTop, m_quadBorderRight,
	  m_quadBorderBottom;

	BrightnessOverlay m_Brightness;

	BackgroundDef m_StaticBackgroundDef;
};

static RageColor
GetBrightnessColor(float fBrightnessPercent)
{
	auto cBrightness = RageColor(0, 0, 0, 1 - fBrightnessPercent);
	const auto cClamp = RageColor(0.5f, 0.5f, 0.5f, CLAMP_OUTPUT_PERCENT);

	// blend the two colors above as if cBrightness is drawn, then cClamp drawn
	// on top
	cBrightness.a *= (1 - cClamp.a); // premultiply alpha

	RageColor ret;
	ret.a = cBrightness.a + cClamp.a;
	ret.r = (cBrightness.r * cBrightness.a + cClamp.r * cClamp.a) / ret.a;
	ret.g = (cBrightness.g * cBrightness.a + cClamp.g * cClamp.a) / ret.a;
	ret.b = (cBrightness.b * cBrightness.a + cClamp.b * cClamp.a) / ret.a;
	return ret;
}

BackgroundImpl::BackgroundImpl()
{
	m_bInitted = false;
	m_pSong = nullptr;
	m_fLastMusicSeconds = 0.f;
	m_bDangerAllWasVisible = false;
}

BackgroundImpl::Layer::Layer()
{
	m_iCurBGChangeIndex = -1;
	m_pCurrentBGA = nullptr;
	m_pFadingBGA = nullptr;
}

void
BackgroundImpl::Init()
{
	if (m_bInitted)
		return;
	m_bInitted = true;
	m_bDangerAllWasVisible = false;
	m_StaticBackgroundDef = BackgroundDef();

	if (!USE_STATIC_BG) {
		m_StaticBackgroundDef.m_sColor1 = "#00000000";
		m_StaticBackgroundDef.m_sColor2 = "#00000000";
	}

	// load transitions
	{
		ASSERT(m_mapNameToTransition.empty());
		std::vector<std::string> vsPaths, vsNames;
		BackgroundUtil::GetBackgroundTransitions("", vsPaths, vsNames);
		for (unsigned i = 0; i < vsPaths.size(); i++) {
			const auto& sPath = vsPaths[i];
			const auto& sName = vsNames[i];

			XNode xml;
			XmlFileUtil::LoadFromFileShowErrors(xml, sPath);
			ASSERT(xml.GetName() == "BackgroundTransition");
			auto& bgt = m_mapNameToTransition[sName];

			std::string sCmdLeaves;
			auto bSuccess = xml.GetAttrValue("LeavesCommand", sCmdLeaves);
			ASSERT(bSuccess);
			bgt.cmdLeaves = ActorUtil::ParseActorCommands(sCmdLeaves);

			std::string sCmdRoot;
			bSuccess = xml.GetAttrValue("RootCommand", sCmdRoot);
			ASSERT(bSuccess);
			bgt.cmdRoot = ActorUtil::ParseActorCommands(sCmdRoot);
		}
	}

	const auto c = GetBrightnessColor(0);

	m_quadBorderLeft.StretchTo(
	  RectF(SCREEN_LEFT, SCREEN_TOP, LEFT_EDGE, SCREEN_BOTTOM));
	m_quadBorderLeft.SetDiffuse(c);
	m_quadBorderTop.StretchTo(
	  RectF(LEFT_EDGE, SCREEN_TOP, RIGHT_EDGE, TOP_EDGE));
	m_quadBorderTop.SetDiffuse(c);
	m_quadBorderRight.StretchTo(
	  RectF(RIGHT_EDGE, SCREEN_TOP, SCREEN_RIGHT, SCREEN_BOTTOM));
	m_quadBorderRight.SetDiffuse(c);
	m_quadBorderBottom.StretchTo(
	  RectF(LEFT_EDGE, BOTTOM_EDGE, RIGHT_EDGE, SCREEN_BOTTOM));
	m_quadBorderBottom.SetDiffuse(c);

	this->AddChild(&m_quadBorderLeft);
	this->AddChild(&m_quadBorderTop);
	this->AddChild(&m_quadBorderRight);
	this->AddChild(&m_quadBorderBottom);

	this->AddChild(&m_Brightness);
}

BackgroundImpl::~BackgroundImpl()
{
	Unload();
}

void
BackgroundImpl::Unload()
{
	FOREACH_BackgroundLayer(i) m_Layer[i].Unload();
	m_pSong = nullptr;
	m_fLastMusicSeconds = -9999;
	m_RandomBGAnimations.clear();
}

void
BackgroundImpl::Layer::Unload()
{
	for (auto& iter : m_BGAnimations)
		delete iter.second;
	m_BGAnimations.clear();
	m_aBGChanges.clear();

	m_pCurrentBGA = nullptr;
	m_pFadingBGA = nullptr;
	m_iCurBGChangeIndex = -1;
}

bool
BackgroundImpl::Layer::CreateBackground(const Song* pSong,
										const BackgroundDef& bd,
										Actor* pParent)
{
	ASSERT(m_BGAnimations.find(bd) == m_BGAnimations.end());

	// Resolve the background names
	std::vector<std::string> vsToResolve;
	vsToResolve.push_back(bd.m_sFile1);
	vsToResolve.push_back(bd.m_sFile2);

	std::vector<std::string> vsResolved;
	vsResolved.resize(vsToResolve.size());
	std::vector<LuaThreadVariable*> vsResolvedRef;
	vsResolvedRef.resize(vsToResolve.size());

	for (unsigned i = 0; i < vsToResolve.size(); i++) {
		const auto& sToResolve = vsToResolve[i];

		if (sToResolve.empty()) {
			if (i == 0)
				return false;
			continue;
		}

		/* Look for vsFileToResolve[i] in:
		 * song's dir
		 * RandomMovies dir
		 * BGAnimations dir.
		 */
		std::vector<std::string> vsPaths, vsThrowAway;

		// Look for BGAnims in the song dir
		if (sToResolve == SONG_BACKGROUND_FILE)
			vsPaths.push_back(
			  pSong->HasBackground()
				? pSong->GetBackgroundPath()
				: THEME->GetPathG("Common", "fallback background"));
		if (vsPaths.empty())
			BackgroundUtil::GetSongBGAnimations(
			  pSong, sToResolve, vsPaths, vsThrowAway);
		if (vsPaths.empty())
			BackgroundUtil::GetSongMovies(
			  pSong, sToResolve, vsPaths, vsThrowAway);
		if (vsPaths.empty())
			BackgroundUtil::GetSongBitmaps(
			  pSong, sToResolve, vsPaths, vsThrowAway);
		if (vsPaths.empty())
			BackgroundUtil::GetGlobalBGAnimations(
			  pSong, sToResolve, vsPaths, vsThrowAway);

		auto& sResolved = vsResolved[i];

		if (!vsPaths.empty()) {
			sResolved = vsPaths[0];
		} else {
			// If the main background file is missing, we failed.
			if (i == 0)
				return false;
			sResolved = "../" + ThemeManager::GetBlankGraphicPath();
		}

		ASSERT(!sResolved.empty());

		vsResolvedRef[i] =
		  new LuaThreadVariable(ssprintf("File%d", i + 1), sResolved);
	}

	auto sEffect = bd.m_sEffect;
	if (sEffect.empty()) {
		const auto ft = ActorUtil::GetFileType(vsResolved[0]);
		switch (ft) {
			default:
				LuaHelpers::ReportScriptErrorFmt(
				  "CreateBackground() Unknown file type '%s'",
				  vsResolved[0].c_str());
				// fall through
			case FT_Bitmap:
			case FT_Sprite:
			case FT_Movie:
				sEffect = SBE_StretchNormal;
				break;
			case FT_Lua:
			case FT_Model:
				sEffect = SBE_UpperLeft;
				break;
			case FT_Xml:
				sEffect = SBE_Centered;
				break;
			case FT_Directory:
				if (DoesFileExist(vsResolved[0] + "/default.lua"))
					sEffect = SBE_UpperLeft;
				else
					sEffect = SBE_Centered;
				break;
		}
	}
	ASSERT(!sEffect.empty());

	// Set Lua color globals
	LuaThreadVariable sColor1(
	  "Color1", bd.m_sColor1.empty() ? std::string("#FFFFFFFF") : bd.m_sColor1);
	LuaThreadVariable sColor2(
	  "Color2", bd.m_sColor2.empty() ? std::string("#FFFFFFFF") : bd.m_sColor2);

	// Resolve the effect file.
	std::string sEffectFile;
	for (auto i = 0; i < 2; i++) {
		std::vector<std::string> vsPaths, vsThrowAway;
		BackgroundUtil::GetBackgroundEffects(sEffect, vsPaths, vsThrowAway);
		if (vsPaths.empty()) {
			LuaHelpers::ReportScriptErrorFmt(
			  "BackgroundEffect '%s' is missing.", sEffect.c_str());
			sEffect = SBE_Centered;
		} else if (vsPaths.size() > 1) {
			LuaHelpers::ReportScriptErrorFmt(
			  "BackgroundEffect '%s' has more than one match.",
			  sEffect.c_str());
			sEffect = SBE_Centered;
		} else {
			sEffectFile = vsPaths[0];
			break;
		}
	}
	ASSERT(!sEffectFile.empty());

	auto* pActor = ActorUtil::MakeActor(sEffectFile);

	if (pActor == nullptr)
		pActor = new Actor;
	m_BGAnimations[bd] = pActor;

	for (auto& i : vsResolvedRef)
		delete i;

	return true;
}

BackgroundDef
BackgroundImpl::Layer::CreateRandomBGA(const Song* pSong,
									   const std::string& sEffect,
									   deque<BackgroundDef>& RandomBGAnimations,
									   Actor* pParent)
{
	if (g_RandomBackgroundMode == BGMODE_OFF)
		return BackgroundDef();

	// Set to not show any BGChanges, whether scripted or random
	if (GAMESTATE->m_SongOptions.GetCurrent().m_bStaticBackground)
		return BackgroundDef();

	if (RandomBGAnimations.empty())
		return BackgroundDef();

	/* XXX: every time we fully loop, shuffle, so we don't play the same
	 * sequence over and over; and nudge the shuffle so the next one won't be a
	 * repeat */
	auto bd = RandomBGAnimations.front();
	RandomBGAnimations.push_back(RandomBGAnimations.front());
	RandomBGAnimations.pop_front();

	if (!sEffect.empty())
		bd.m_sEffect = sEffect;

	const map<BackgroundDef, Actor*>::const_iterator iter =
	  m_BGAnimations.find(bd);

	// create the background if it's not already created
	if (iter == m_BGAnimations.end()) {
		const auto bSuccess = CreateBackground(pSong, bd, pParent);
		ASSERT(bSuccess); // we fed it valid files, so this shouldn't fail
	}
	return bd;
}

void
BackgroundImpl::LoadFromRandom(float fFirstBeat,
							   float fEndBeat,
							   const BackgroundChange& change)
{
	auto iStartRow = BeatToNoteRow(fFirstBeat);
	auto iEndRow = BeatToNoteRow(fEndBeat);

	const auto& timing = m_pSong->m_SongTiming;

	// change BG every time signature change or 4 measures
	const auto& tSigs = timing.GetTimingSegments(SEGMENT_TIME_SIG);

	for (unsigned i = 0; i < tSigs.size(); i++) {
		auto* ts = static_cast<TimeSignatureSegment*>(tSigs[i]);
		auto iSegmentEndRow =
		  (i + 1 == tSigs.size()) ? iEndRow : tSigs[i + 1]->GetRow();

		auto time_signature_start = std::max(ts->GetRow(), iStartRow);
		for (auto j = time_signature_start;
			 j < std::min(iEndRow, iSegmentEndRow);
			 j += static_cast<int>(RAND_BG_CHANGE_MEASURES *
								   ts->GetNoteRowsPerMeasure())) {
			// Don't fade. It causes frame rate dip, especially on slower
			// machines.
			auto bd = m_Layer[0].CreateRandomBGA(
			  m_pSong, change.m_def.m_sEffect, m_RandomBGAnimations, this);
			if (!bd.IsEmpty()) {
				auto c = change;
				c.m_def = bd;
				if (j == time_signature_start && i == 0) {
					c.m_fStartBeat = RAND_BG_START_BEAT;
				} else {
					c.m_fStartBeat = NoteRowToBeat(j);
				}
				m_Layer[0].m_aBGChanges.push_back(c);
			}
		}
	}

	if (RAND_BG_CHANGES_WHEN_BPM_CHANGES) {
		// change BG every BPM change that is at the beginning of a measure
		const auto& bpms = timing.GetTimingSegments(SEGMENT_BPM);
		for (auto* bpm : bpms) {
			auto bAtBeginningOfMeasure = false;
			for (auto* tSig : tSigs) {
				auto* ts = static_cast<TimeSignatureSegment*>(tSig);
				if ((bpm->GetRow() - ts->GetRow()) %
					  ts->GetNoteRowsPerMeasure() ==
					0) {
					bAtBeginningOfMeasure = true;
					break;
				}
			}

			if (!bAtBeginningOfMeasure)
				continue; // skip

			// start so that we don't create a BGChange right on top of fEndBeat
			auto bInRange =
			  bpm->GetRow() >= iStartRow && bpm->GetRow() < iEndRow;
			if (!bInRange)
				continue; // skip

			auto bd = m_Layer[0].CreateRandomBGA(
			  m_pSong, change.m_def.m_sEffect, m_RandomBGAnimations, this);
			if (!bd.IsEmpty()) {
				auto c = change;
				c.m_def.m_sFile1 = bd.m_sFile1;
				c.m_def.m_sFile2 = bd.m_sFile2;
				c.m_fStartBeat = bpm->GetBeat();
				m_Layer[0].m_aBGChanges.push_back(c);
			}
		}
	}
}

void
BackgroundImpl::LoadFromSong(const Song* pSong)
{
	Init();
	Unload();
	m_pSong = pSong;
	m_StaticBackgroundDef.m_sFile1 = SONG_BACKGROUND_FILE;

	// do not load any background if it will never change and is 0 brightness
	// this allows something like lua to load or modify in the background layer
	if ((PREFSMAN->m_fBGBrightness == 0.f || !PREFSMAN->m_bShowBackgrounds) &&
		!m_pSong->HasBGChanges())
		return;

	// Choose a bunch of backgrounds that we'll use for the random file marker
	{
		std::vector<std::string> vsThrowAway, vsNames;
		switch (g_RandomBackgroundMode) {
			default:
				ASSERT_M(0,
						 ssprintf("Invalid RandomBackgroundMode: %i",
								  static_cast<int>(g_RandomBackgroundMode)));
				break;
			case BGMODE_OFF:
				break;
			case BGMODE_ANIMATIONS:
				BackgroundUtil::GetGlobalBGAnimations(
				  pSong, "", vsThrowAway, vsNames);
				break;
		}

		// Pick the same random items every time the song is played.
		RandomGen rnd(GetHashForString(pSong->GetSongDir()));
		std::shuffle(vsNames.begin(), vsNames.end(), rnd);
		auto iSize = std::min(static_cast<int>(g_iNumBackgrounds),
							  static_cast<int>(vsNames.size()));
		vsNames.resize(iSize);

		for (auto& s : vsNames) {
			BackgroundDef bd;
			bd.m_sFile1 = s;
			m_RandomBGAnimations.push_back(bd);
		}
	}

	/* Song backgrounds (even just background stills) can get very big; never
	 * keep them in memory. */
	auto OldPolicy = TEXTUREMAN->GetDefaultTexturePolicy();
	TEXTUREMAN->SetDefaultTexturePolicy(RageTextureID::TEX_VOLATILE);

	TEXTUREMAN->DisableOddDimensionWarning();

	// Set to not show any BGChanges, whether scripted or random if
	// m_bStaticBackground is on
	if (!g_bSongBackgrounds ||
		GAMESTATE->m_SongOptions.GetCurrent().m_bStaticBackground) {
		// Backgrounds are disabled; just display the song background.
		BackgroundChange change;
		change.m_def = m_StaticBackgroundDef;
		change.m_fStartBeat = 0;
		m_Layer[0].m_aBGChanges.push_back(change);
	}
	// If m_bRandomBGOnly is on, then we want to ignore the scripted BG in
	// favour of randomly loaded BGs
	else if (pSong->HasBGChanges() &&
			 !GAMESTATE->m_SongOptions.GetCurrent().m_bRandomBGOnly) {
		FOREACH_BackgroundLayer(i)
		{
			auto& layer = m_Layer[i];

			// Load all song-specified backgrounds
			FOREACH_CONST(BackgroundChange, pSong->GetBackgroundChanges(i), bgc)
			{
				auto change = *bgc;
				auto& bd = change.m_def;

				auto bIsAlreadyLoaded =
				  layer.m_BGAnimations.find(bd) != layer.m_BGAnimations.end();

				if (bd.m_sFile1 != RANDOM_BACKGROUND_FILE &&
					!bIsAlreadyLoaded) {
					if (layer.CreateBackground(m_pSong, bd, this)) {
					} else {
						if (i == BACKGROUND_LAYER_1) {
							// The background was not found. Try to use a random
							// one instead. Don't use the BackgroundDef's
							// effect, because it may be an effect that requires
							// 2 files, and random BGA will only supply one file
							bd = layer.CreateRandomBGA(
							  pSong, "", m_RandomBGAnimations, this);
							if (bd.IsEmpty())
								bd = m_StaticBackgroundDef;
						}
					}
				}

				if (!bd.IsEmpty())
					layer.m_aBGChanges.push_back(change);
			}
		}
	} else // pSong doesn't have an animation plan
	{
		auto& layer = m_Layer[0];
		auto firstBeat = pSong->GetFirstBeat();
		auto lastBeat = pSong->GetLastBeat();

		LoadFromRandom(firstBeat, lastBeat, BackgroundChange());

		if (RAND_BG_ENDS_AT_LAST_BEAT) {
			// end showing the static song background
			BackgroundChange change;
			change.m_def = m_StaticBackgroundDef;
			change.m_fStartBeat = lastBeat;
			layer.m_aBGChanges.push_back(change);
		}
	}

	// sort segments
	FOREACH_BackgroundLayer(i)
	{
		auto& layer = m_Layer[i];
		BackgroundUtil::SortBackgroundChangesArray(layer.m_aBGChanges);
	}

	auto& mainlayer = m_Layer[0];

	BackgroundChange change;
	change.m_def = m_StaticBackgroundDef;
	change.m_fStartBeat = -10000;
	mainlayer.m_aBGChanges.insert(mainlayer.m_aBGChanges.begin(), change);

	// If any BGChanges use the background image, load it.
	auto bStaticBackgroundUsed = false;
	FOREACH_BackgroundLayer(i)
	{
		auto& layer = m_Layer[i];
		FOREACH_CONST(BackgroundChange, layer.m_aBGChanges, bgc)
		{
			const auto& bd = bgc->m_def;
			if (bd == m_StaticBackgroundDef) {
				bStaticBackgroundUsed = true;
				break;
			}
		}
		if (bStaticBackgroundUsed)
			break;
	}

	if (bStaticBackgroundUsed) {
		auto bIsAlreadyLoaded =
		  mainlayer.m_BGAnimations.find(m_StaticBackgroundDef) !=
		  mainlayer.m_BGAnimations.end();
		if (!bIsAlreadyLoaded) {
			auto bSuccess =
			  mainlayer.CreateBackground(m_pSong, m_StaticBackgroundDef, this);
			ASSERT(bSuccess);
		}
	}

	// Look for the random file marker, and replace the segment with
	// LoadFromRandom.
	for (unsigned i = 0; i < mainlayer.m_aBGChanges.size(); i++) {
		const auto change = mainlayer.m_aBGChanges[i];
		if (change.m_def.m_sFile1 != RANDOM_BACKGROUND_FILE)
			continue;

		auto fStartBeat = change.m_fStartBeat;
		auto fEndBeat = pSong->GetLastBeat();
		if (i + 1 < mainlayer.m_aBGChanges.size())
			fEndBeat = mainlayer.m_aBGChanges[i + 1].m_fStartBeat;

		mainlayer.m_aBGChanges.erase(mainlayer.m_aBGChanges.begin() + i);
		--i;

		LoadFromRandom(fStartBeat, fEndBeat, change);
	}

	// At this point, we shouldn't have any BGChanges to "".  "" is an invalid
	// name.
	for (auto& m_aBGChange : mainlayer.m_aBGChanges)
		ASSERT(!m_aBGChange.m_def.m_sFile1.empty());

	// Re-sort.
	BackgroundUtil::SortBackgroundChangesArray(mainlayer.m_aBGChanges);

	TEXTUREMAN->EnableOddDimensionWarning();

	TEXTUREMAN->SetDefaultTexturePolicy(OldPolicy);
}

int
BackgroundImpl::Layer::FindBGSegmentForBeat(float fBeat) const
{
	if (m_aBGChanges.empty())
		return -1;
	if (fBeat < m_aBGChanges[0].m_fStartBeat)
		return -1;

	// assumption: m_aBGChanges are sorted by m_fStartBeat
	int i;
	for (i = m_aBGChanges.size() - 1; i >= 0; i--) {
		if (fBeat >= m_aBGChanges[i].m_fStartBeat)
			return i;
	}

	return i;
}

/* If the BG segment has changed, move focus to it. Send Update() calls. */
void
BackgroundImpl::Layer::UpdateCurBGChange(
  const Song* pSong,
  float fLastMusicSeconds,
  float fCurrentTime,
  const map<std::string, BackgroundTransition>& mapNameToTransition)
{
	ASSERT(fCurrentTime != GameState::MUSIC_SECONDS_INVALID);

	if (m_aBGChanges.size() == 0)
		return;

	TimingData::GetBeatArgs beat_info;
	beat_info.elapsed_time = fCurrentTime;
	pSong->m_SongTiming.GetBeatAndBPSFromElapsedTime(beat_info);

	// Calls to Update() should *not* be scaled by music rate; fCurrentTime is.
	// Undo it.
	const auto fRate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;

	// Find the BGSegment we're in
	const auto i = FindBGSegmentForBeat(beat_info.beat);

	auto fDeltaTime = fCurrentTime - fLastMusicSeconds;
	if (i != -1 && i != m_iCurBGChangeIndex) // we're changing backgrounds
	{
		// LOG->Trace( "old bga %d -> new bga %d (%s), %f, %f",
		// m_iCurBGChangeIndex, i, m_aBGChanges[i].GetTextDescription().c_str(),
		// m_aBGChanges[i].m_fStartBeat, fBeat );

		if (m_iCurBGChangeIndex != -1)
			BackgroundChange oldChange = m_aBGChanges[m_iCurBGChangeIndex];

		m_iCurBGChangeIndex = i;

		const auto& change = m_aBGChanges[i];

		m_pFadingBGA = m_pCurrentBGA;

		const map<BackgroundDef, Actor*>::const_iterator iter =
		  m_BGAnimations.find(change.m_def);
		if (iter == m_BGAnimations.end()) {
			auto* pNode = change.m_def.CreateNode();
			auto xml = XmlFileUtil::GetXML(pNode);
			Trim(xml);
			LuaHelpers::ReportScriptErrorFmt(
			  "Tried to switch to a background that was never loaded:\n%s",
			  xml.c_str());
			SAFE_DELETE(pNode);
			return;
		}

		m_pCurrentBGA = iter->second;

		if (m_pFadingBGA == m_pCurrentBGA) {
			m_pFadingBGA = nullptr;
			// LOG->Trace( "bg didn't actually change.  Ignoring." );
		} else {
			if (m_pFadingBGA != nullptr) {
				m_pFadingBGA->PlayCommand("LoseFocus");

				if (!change.m_sTransition.empty()) {
					const auto lIter =
					  mapNameToTransition.find(change.m_sTransition);
					if (lIter == mapNameToTransition.end()) {
						LuaHelpers::ReportScriptErrorFmt(
						  "'%s' is not the name of a BackgroundTransition "
						  "file.",
						  change.m_sTransition.c_str());
					} else {
						const auto& bt = lIter->second;
						m_pFadingBGA->RunCommandsOnLeaves(*bt.cmdLeaves);
						m_pFadingBGA->RunCommands(*bt.cmdRoot);
					}
				}
			}
		}

		m_pCurrentBGA->SetUpdateRate(change.m_fRate);

		m_pCurrentBGA->InitState();
		m_pCurrentBGA->PlayCommand("On");
		m_pCurrentBGA->PlayCommand("GainFocus");

		/* How much time of this BGA have we skipped?  (This happens with
		 * SetSeconds.) */
		const auto fStartSecond =
		  pSong->m_SongTiming.WhereUAtBro(change.m_fStartBeat);

		/* This is affected by the music rate. */
		fDeltaTime = fCurrentTime - fStartSecond;
	}

	if (m_pFadingBGA != nullptr) {
		if (m_pFadingBGA->GetTweenTimeLeft() == 0)
			m_pFadingBGA = nullptr;
	}

	/* This is unaffected by the music rate. */
	const auto fDeltaTimeNoMusicRate = std::max(fDeltaTime / fRate, 0.F);

	if (m_pCurrentBGA != nullptr)
		m_pCurrentBGA->Update(fDeltaTimeNoMusicRate);
	if (m_pFadingBGA != nullptr)
		m_pFadingBGA->Update(fDeltaTimeNoMusicRate);
}

void
BackgroundImpl::Update(float fDeltaTime)
{
	ActorFrame::Update(fDeltaTime);

	{
		const auto bVisible = IsDangerAllVisible();
		if (m_bDangerAllWasVisible != bVisible)
			MESSAGEMAN->Broadcast(bVisible ? "ShowDangerAll" : "HideDangerAll");
		m_bDangerAllWasVisible = bVisible;
	}

	FOREACH_BackgroundLayer(i)
	{
		auto& layer = m_Layer[i];
		layer.UpdateCurBGChange(m_pSong,
								m_fLastMusicSeconds,
								GAMESTATE->m_Position.m_fMusicSeconds,
								m_mapNameToTransition);
	}
	m_fLastMusicSeconds = GAMESTATE->m_Position.m_fMusicSeconds;
}

void
BackgroundImpl::DrawPrimitives()
{
	if (PREFSMAN->m_fBGBrightness == 0.0f)
		return;

	{
		FOREACH_BackgroundLayer(i)
		{
			auto& layer = m_Layer[i];
			if (layer.m_pCurrentBGA != nullptr)
				layer.m_pCurrentBGA->Draw();
			if (layer.m_pFadingBGA != nullptr)
				layer.m_pFadingBGA->Draw();
		}
	}

	ActorFrame::DrawPrimitives();
}

void
BackgroundImpl::GetLoadedBackgroundChanges(
  std::vector<BackgroundChange>* pBackgroundChangesOut[NUM_BackgroundLayer])
{
	FOREACH_BackgroundLayer(i) * pBackgroundChangesOut[i] =
	  m_Layer[i].m_aBGChanges;
}

bool
BackgroundImpl::IsDangerAllVisible()
{
	// The players are never in danger in FAIL_OFF.
	if (GAMESTATE->GetPlayerFailType(GAMESTATE->m_pPlayerState) == FailType_Off)
		return false;
	if (!g_bShowDanger)
		return false;

	/* Don't show it if everyone is already failing: it's already too late and
	 * it's annoying for it to show for the entire duration of a song. */
	if (STATSMAN->m_CurStageStats.Failed())
		return false;

	return GAMESTATE->AllAreInDangerOrWorse();
}

BrightnessOverlay::BrightnessOverlay()
{
	const auto fQuadWidth = (RIGHT_EDGE - LEFT_EDGE);

	m_quadBGBrightness.StretchTo(
	  RectF(LEFT_EDGE, TOP_EDGE, LEFT_EDGE + fQuadWidth, BOTTOM_EDGE));
	m_quadBGBrightnessFade.StretchTo(RectF(
	  LEFT_EDGE + fQuadWidth, TOP_EDGE, RIGHT_EDGE - fQuadWidth, BOTTOM_EDGE));

	m_quadBGBrightness.SetName("BrightnessOverlay");
	ActorUtil::LoadAllCommands(m_quadBGBrightness, "Background");
	this->AddChild(&m_quadBGBrightness);

	m_quadBGBrightnessFade.SetName("BrightnessOverlay");
	ActorUtil::LoadAllCommands(m_quadBGBrightnessFade, "Background");
	this->AddChild(&m_quadBGBrightnessFade);

	SetActualBrightness();
}

void
BrightnessOverlay::Update(float fDeltaTime)
{
	ActorFrame::Update(fDeltaTime);
	/* If we're actually playing, then we're past fades, etc; update the
	 * background brightness to follow Cover. */
	if (!GAMESTATE->m_bGameplayLeadIn)
		SetActualBrightness();
}

void
BrightnessOverlay::SetActualBrightness()
{
	auto fLeftBrightness =
	  1 - GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_fCover;
	auto fRightBrightness =
	  1 - GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_fCover;

	const float fBaseBGBrightness = PREFSMAN->m_fBGBrightness;

	// Revision:  Themes that implement a training mode should handle the
	// brightness for it.  The engine should not force the brightness for
	// anything. -Kyz
	/*
	// HACK: Always show training in full brightness
	if( GAMESTATE->m_pCurSong && GAMESTATE->m_pCurSong->IsTutorial() )
		fBaseBGBrightness = 1.0f;
	*/

	fLeftBrightness *= fBaseBGBrightness;
	fRightBrightness *= fBaseBGBrightness;

	if (!GAMESTATE->IsHumanPlayer(PLAYER_1))
		fLeftBrightness = fRightBrightness;

	const auto LeftColor = GetBrightnessColor(fLeftBrightness);
	const auto RightColor = GetBrightnessColor(fRightBrightness);

	m_quadBGBrightness.SetDiffuse(LeftColor);
	m_quadBGBrightnessFade.SetDiffuseLeftEdge(LeftColor);
	m_quadBGBrightnessFade.SetDiffuseRightEdge(RightColor);
}

void
BrightnessOverlay::Set(float fBrightness)
{
	const auto c = GetBrightnessColor(fBrightness);

	m_quadBGBrightness.SetDiffuse(c);
	m_quadBGBrightnessFade.SetDiffuse(c);
}

void
BrightnessOverlay::FadeToActualBrightness()
{
	this->PlayCommand("Fade");
	SetActualBrightness();
}

Background::Background()
{
	m_disable_draw = false;
	m_pImpl = new BackgroundImpl;
	this->AddChild(m_pImpl);
}
Background::~Background()
{
	SAFE_DELETE(m_pImpl);
}
void
Background::Init()
{
	m_pImpl->Init();
}
void
Background::LoadFromSong(const Song* pSong)
{
	m_pImpl->LoadFromSong(pSong);
}
void
Background::Unload()
{
	m_pImpl->Unload();
}
void
Background::FadeToActualBrightness()
{
	m_pImpl->FadeToActualBrightness();
}
void
Background::SetBrightness(float fBrightness)
{
	m_pImpl->SetBrightness(fBrightness);
}
void
Background::GetLoadedBackgroundChanges(
  std::vector<BackgroundChange>* pBackgroundChangesOut[NUM_BackgroundLayer])
{
	m_pImpl->GetLoadedBackgroundChanges(pBackgroundChangesOut);
}
