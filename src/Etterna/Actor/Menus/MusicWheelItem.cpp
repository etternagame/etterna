#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Menus/MusicWheel.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "MusicWheelItem.h"
#include "RageUtil/Misc/RageTypes.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include <cstdio>

static const char* MusicWheelItemTypeNames[] = {
	"Song", "SectionExpanded", "SectionCollapsed", "Roulette", "Sort",
	"Mode", "Random",		   "Portal",		   "Custom",
};
XToString(MusicWheelItemType);

MusicWheelItemData::MusicWheelItemData(WheelItemDataType type,
									   Song* pSong,
									   const std::string& sSectionName,
									   const RageColor& color,
									   int iSectionCount)
  : WheelItemBaseData(type, sSectionName, color)
  , m_pSong(pSong)
  , m_iSectionCount(iSectionCount)
  , m_sLabel("")
  , m_pAction()
{
}

MusicWheelItem::MusicWheelItem(const std::string& sType)
  : WheelItemBase(sType)
{
	FOREACH_ENUM(MusicWheelItemType, i)
	{
		m_sprColorPart[i].Load(
		  THEME->GetPathG(sType, MusicWheelItemTypeToString(i) + " ColorPart"));
		m_sprColorPart[i]->SetName(MusicWheelItemTypeToString(i) + "ColorPart");
		ActorUtil::LoadAllCommands(m_sprColorPart[i], "MusicWheelItem");
		this->AddChild(m_sprColorPart[i]);

		m_sprNormalPart[i].Load(THEME->GetPathG(
		  sType, MusicWheelItemTypeToString(i) + " NormalPart"));
		m_sprNormalPart[i]->SetName(MusicWheelItemTypeToString(i) +
									"NormalPart");
		ActorUtil::LoadAllCommands(m_sprNormalPart[i], "MusicWheelItem");
		this->AddChild(m_sprNormalPart[i]);
	}

	m_TextBanner.SetName("SongName");
	ActorUtil::LoadAllCommands(m_TextBanner, "MusicWheelItem");
	m_TextBanner.Load("TextBanner");
	ActorUtil::SetXY(m_TextBanner, "MusicWheelItem");
	m_TextBanner.PlayCommand("On");
	this->AddChild(&m_TextBanner);

	FOREACH_ENUM(MusicWheelItemType, i)
	{
		m_sprOverPart[i].Load(
		  THEME->GetPathG(sType, MusicWheelItemTypeToString(i) + " OverPart"));
		m_sprOverPart[i]->SetName(MusicWheelItemTypeToString(i) + "OverPart");
		ActorUtil::LoadAllCommands(m_sprOverPart[i], "MusicWheelItem");
		this->AddChild(m_sprOverPart[i]);
	}

	FOREACH_ENUM(MusicWheelItemType, i)
	{
		m_pText[i] = nullptr;

		// Don't init text for Type_Song. It uses a TextBanner.
		if (i == MusicWheelItemType_Song)
			continue;

		m_pText[i] = new BitmapText;
		m_pText[i]->SetName(MusicWheelItemTypeToString(i));
		ActorUtil::LoadAllCommands(m_pText[i], "MusicWheelItem");
		m_pText[i]->LoadFromFont(
		  THEME->GetPathF(sType, MusicWheelItemTypeToString(i)));
		ActorUtil::SetXY(m_pText[i], "MusicWheelItem");
		m_pText[i]->PlayCommand("On");
		this->AddChild(m_pText[i]);
	}

	m_pTextSectionCount = new BitmapText;
	m_pTextSectionCount->SetName("SectionCount");
	ActorUtil::LoadAllCommands(m_pTextSectionCount, "MusicWheelItem");
	m_pTextSectionCount->LoadFromFont(THEME->GetPathF(sType, "SectionCount"));
	ActorUtil::SetXY(m_pTextSectionCount, "MusicWheelItem");
	m_pTextSectionCount->PlayCommand("On");
	this->AddChild(m_pTextSectionCount);

	m_pGradeDisplay.Load(THEME->GetPathG(sType, "grades"));
	m_pGradeDisplay->SetName(
	  ssprintf("GradeP%d", static_cast<int>(PLAYER_1 + 1)));
	this->AddChild(m_pGradeDisplay);
	LOAD_ALL_COMMANDS_AND_SET_XY(m_pGradeDisplay);

	this->SubscribeToMessage(Message_CurrentStepsChanged);
	this->SubscribeToMessage(Message_PreferredDifficultyP1Changed);
}

MusicWheelItem::MusicWheelItem(const MusicWheelItem& cpy)
  : WheelItemBase(cpy)
  , m_TextBanner(cpy.m_TextBanner)
{
	FOREACH_ENUM(MusicWheelItemType, i)
	{
		m_sprColorPart[i] = cpy.m_sprColorPart[i];
		this->AddChild(m_sprColorPart[i]);

		m_sprNormalPart[i] = cpy.m_sprNormalPart[i];
		this->AddChild(m_sprNormalPart[i]);
	}

	this->AddChild(&m_TextBanner);

	FOREACH_ENUM(MusicWheelItemType, i)
	{
		m_sprOverPart[i] = cpy.m_sprOverPart[i];
		this->AddChild(m_sprOverPart[i]);
	}

	FOREACH_ENUM(MusicWheelItemType, i)
	{
		if (cpy.m_pText[i] == nullptr) {
			m_pText[i] = nullptr;
		} else {
			m_pText[i] = new BitmapText(*cpy.m_pText[i]);
			this->AddChild(m_pText[i]);
		}
	}

	m_pTextSectionCount = new BitmapText(*cpy.m_pTextSectionCount);
	this->AddChild(m_pTextSectionCount);

	m_pGradeDisplay = cpy.m_pGradeDisplay;
	this->AddChild(m_pGradeDisplay);
}

MusicWheelItem::~MusicWheelItem()
{
	FOREACH_ENUM(MusicWheelItemType, i) { SAFE_DELETE(m_pText[i]); }
	delete m_pTextSectionCount;
}

void
MusicWheelItem::LoadFromWheelItemData(const WheelItemBaseData* pData,
									  int iIndex,
									  bool bHasFocus,
									  int iDrawIndex)
{
	WheelItemBase::LoadFromWheelItemData(pData, iIndex, bHasFocus, iDrawIndex);

	const auto* pWID = dynamic_cast<const MusicWheelItemData*>(pData);
	ASSERT_M(pWID != nullptr,
			 "Dynamic cast to load wheel item datas failed at runtime.");

	// hide all
	FOREACH_ENUM(MusicWheelItemType, i)
	{
		m_sprColorPart[i]->SetVisible(false);
		m_sprNormalPart[i]->SetVisible(false);
		m_sprOverPart[i]->SetVisible(false);
	}
	m_TextBanner.SetVisible(false);
	FOREACH_ENUM(MusicWheelItemType, i)
	if (m_pText[i])
		m_pText[i]->SetVisible(false);
	m_pTextSectionCount->SetVisible(false);
	m_pGradeDisplay->SetVisible(false);

	// Fill these in below
	std::string sDisplayName, sTranslitName;
	MusicWheelItemType type = MusicWheelItemType_Invalid;

	auto wheel = dynamic_cast<const MusicWheel*>(GetParent());
	ASSERT_M(wheel != nullptr,
			 "Parent(Wheel) dynamic cast to load wheel item datas failed at runtime.");

	switch (pWID->m_Type) {
		DEFAULT_FAIL(pWID->m_Type);
		case WheelItemDataType_Song:
			type = MusicWheelItemType_Song;

			m_TextBanner.SetFromSong(pWID->m_pSong);
			// We can do this manually if we wanted... maybe have a metric for
			// overrides? -aj
			m_TextBanner.SetDiffuse(pWID->m_color);
			m_TextBanner.SetVisible(true);

			RefreshGrades();
			break;
		case WheelItemDataType_Section: {
			sDisplayName = SONGMAN->ShortenGroupName(pWID->m_sText);
			if (GAMESTATE->sExpandedSectionName == pWID->m_sText) {
				type = MusicWheelItemType_SectionExpanded;
			} else {
				type = MusicWheelItemType_SectionCollapsed;
			}

			auto plays_by_group = wheel->packProgressByGroup.at(GAMESTATE->m_SortOrder);
			auto plays_in_group = plays_by_group.find(pWID->m_sText);
			if (PREFSMAN->m_bPackProgressInWheel && plays_in_group != plays_by_group.end()) {
				int num_played_songs = plays_in_group->second;

				RageColor color;
				if (num_played_songs == pWID->m_iSectionCount) {
					color = RageColor(1, 1, 1, 0);
				} else {
					auto hue = (float) num_played_songs / pWID->m_iSectionCount * 100 - 5;
					// Mix color with white, else the color would be too vibrant
					color = RageColor::Lerp(RageColor(1, 1, 1, 0), RageColor::FromHue(hue), 0.5);
				}

				m_pTextSectionCount->SetText(ssprintf("%d/%d", num_played_songs, pWID->m_iSectionCount));
				m_pTextSectionCount->SetDiffuseColor(color);
			} else {
				m_pTextSectionCount->SetText(ssprintf("%d", pWID->m_iSectionCount));
				// m_pTextSectionCount->SetDiffuseColor(???);
			}

			m_pTextSectionCount->SetVisible(true);
		} break;
		case WheelItemDataType_Sort:
			sDisplayName = pWID->m_sLabel;
			type = MusicWheelItemType_Sort;
			break;
		case WheelItemDataType_Roulette:
			sDisplayName = THEME->GetString("MusicWheel", "Roulette");
			type = MusicWheelItemType_Roulette;
			break;
		case WheelItemDataType_Random:
			sDisplayName = THEME->GetString("MusicWheel", "Random");
			type = MusicWheelItemType_Random;
			break;
		case WheelItemDataType_Portal:
			sDisplayName = THEME->GetString("MusicWheel", "Portal");
			type = MusicWheelItemType_Portal;
			break;
		case WheelItemDataType_Custom:
			sDisplayName = pWID->m_sLabel;
			type = MusicWheelItemType_Custom;
			break;
	}

	m_sprColorPart[type]->SetVisible(true);
	m_sprColorPart[type]->SetDiffuse(pWID->m_color);
	m_sprNormalPart[type]->SetVisible(true);
	m_sprOverPart[type]->SetVisible(true);
	BitmapText* bt = m_pText[type];
	if (bt != nullptr) {
		bt->SetText(sDisplayName, sTranslitName);
		bt->SetDiffuse(pWID->m_color);
		bt->SetVisible(true);
	}

	FOREACH_ENUM(MusicWheelItemType, i)
	{
		if (m_sprColorPart[i]->GetVisible()) {
			SetGrayBar(m_sprColorPart[i]);
			break;
		}
	}

	// Call "Set" so that elements like TextBanner react to the change in song.
	{
		Message msg("Set");
		msg.SetParam("Song", pWID->m_pSong);
		msg.SetParam("Index", iIndex);
		msg.SetParam("HasFocus", bHasFocus);
		msg.SetParam("Text", pWID->m_sText);
		msg.SetParam("DrawIndex", iDrawIndex);
		msg.SetParam("Type", MusicWheelItemTypeToString(type));
		msg.SetParam("Color", pWID->m_color);
		msg.SetParam("Label", pWID->m_sLabel);

		this->HandleMessage(msg);
	}
}

void
MusicWheelItem::RefreshGrades()
{
	const auto* pWID = dynamic_cast<const MusicWheelItemData*>(m_pData);

	if (pWID == nullptr)
		return; // LoadFromWheelItemData() hasn't been called yet.
	m_pGradeDisplay->SetVisible(false);

	if (pWID->m_pSong == nullptr)
		return;

	StepsType st;
	if (GAMESTATE->m_pCurSteps)
		st = GAMESTATE->m_pCurSteps->m_StepsType;
	else
		st = GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StepsType;

	m_pGradeDisplay->SetVisible(true);

	Grade gradeBest = Grade_Invalid;
	Difficulty dcBest = Difficulty_Invalid;
	if (pWID->m_pSong != nullptr) {
		bool hasCurrentStyleSteps = false;
		auto allSteps = pWID->m_pSong->GetChartsMatchingFilter();
		std::unordered_map<Difficulty, std::vector<Steps*>> difficultyToSteps{
			NUM_Difficulty
		};
		for (auto& s : allSteps) {
			auto d = s->GetDifficulty();
			if (difficultyToSteps.count(d) == 0u) {
				std::vector<Steps*> v;
				difficultyToSteps[d] = v;
			}
			difficultyToSteps[d].push_back(s);
		}
		FOREACH_ENUM_N(Difficulty, 6, i)
		{
			if (difficultyToSteps.count(i) == 0u)
				continue;
			auto& stepslist = difficultyToSteps[i];
			for (auto& s : stepslist) {
				if (s->m_StepsType == st) {
					hasCurrentStyleSteps = true;
					Grade dcg =
					  SCOREMAN->GetBestGradeFor(s->GetChartKey());
					if (gradeBest >= dcg) {
						dcBest = i;
						gradeBest = dcg;
					}
				}
			}
		}
		// If no grade was found for the current style/stepstype
		if (!hasCurrentStyleSteps) {
			// Get the best grade among all steps
			for (auto& stepsPtr : allSteps) {
				if (stepsPtr->m_StepsType ==
					st) // Skip already checked steps of type st
					continue;
				Grade dcg = SCOREMAN->GetBestGradeFor(stepsPtr->GetChartKey());
				if (gradeBest >= dcg) {
					dcBest = stepsPtr->GetDifficulty();
					gradeBest = dcg;
				}
			}
		}
	}

	// still needs cleaning up -mina
	Message msg("SetGrade");
	msg.SetParam("PlayerNumber", PLAYER_1);
	if (pWID->m_pSong->IsFavorited())
		msg.SetParam("Favorited", 1);
	if (pWID->m_pSong->IsPermaMirror())
		msg.SetParam("PermaMirror", 1);
	if (pWID->m_pSong->HasGoal())
		msg.SetParam("HasGoal", 1);
	if (gradeBest != Grade_Invalid) {
		msg.SetParam("Grade", gradeBest);
		msg.SetParam("Difficulty", DifficultyToString(dcBest));
		msg.SetParam("NumTimesPlayed", 0);
	}
	m_pGradeDisplay->HandleMessage(msg);
}

void
MusicWheelItem::HandleMessage(const Message& msg)
{
	WheelItemBase::HandleMessage(msg);
}
