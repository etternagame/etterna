#ifndef MUSIC_WHEEL_ITEM_H
#define MUSIC_WHEEL_ITEM_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/GameCommand.h"
#include "TextBanner.h"
#include "WheelItemBase.h"

class Song;

struct MusicWheelItemData;

enum MusicWheelItemType
{
	MusicWheelItemType_Song,
	MusicWheelItemType_SectionExpanded,
	MusicWheelItemType_SectionCollapsed,
	MusicWheelItemType_Roulette,
	MusicWheelItemType_Sort,
	MusicWheelItemType_Mode,
	MusicWheelItemType_Random,
	MusicWheelItemType_Portal,
	MusicWheelItemType_Custom,
	NUM_MusicWheelItemType,
	MusicWheelItemType_Invalid,
};
auto
MusicWheelItemTypeToString(MusicWheelItemType i) -> const std::string&;
/** @brief An item on the MusicWheel. */
class MusicWheelItem final : public WheelItemBase
{
  public:
	MusicWheelItem(const std::string& sType = "MusicWheelItem");
	MusicWheelItem(const MusicWheelItem& cpy);
	~MusicWheelItem() override;
	[[nodiscard]] auto Copy() const -> MusicWheelItem* override
	{
		return new MusicWheelItem(*this);
	}

	void LoadFromWheelItemData(const WheelItemBaseData* pWID,
							   int iIndex,
							   bool bHasFocus,
							   int iDrawIndex) override;
	void HandleMessage(const Message& msg) override;
	void RefreshGrades();

  private:
	AutoActor m_sprColorPart[NUM_MusicWheelItemType];
	AutoActor m_sprNormalPart[NUM_MusicWheelItemType];
	AutoActor m_sprOverPart[NUM_MusicWheelItemType];

	TextBanner m_TextBanner; // used by Type_Song instead of m_pText
	BitmapText* m_pText[NUM_MusicWheelItemType];
	BitmapText* m_pTextSectionCount;
	AutoActor m_pGradeDisplay;
};

struct MusicWheelItemData : WheelItemBaseData
{
	MusicWheelItemData()
	  : m_sLabel("")
	{
	}
	MusicWheelItemData(WheelItemDataType type,
					   Song* pSong,
					   const std::string& sSectionName,
					   const RageColor& color,
					   int iSectionCount);

	Song* m_pSong{ nullptr };

	// for TYPE_SECTION
	int m_iSectionCount{ 0 };

	// for TYPE_SORT
	std::string m_sLabel;
	std::unique_ptr<GameCommand> m_pAction;
};

#endif
