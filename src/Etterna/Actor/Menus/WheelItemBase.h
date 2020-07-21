#ifndef WHEEL_ITEM_BASE_H
#define WHEEL_ITEM_BASE_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

#include <cassert>

struct WheelItemBaseData;
/** @brief The different types of Wheel Items. */
enum WheelItemDataType
{
	WheelItemDataType_Generic,	/**< A generic item on the Wheel. */
	WheelItemDataType_Section,	/**< A general section on the Wheel. */
	WheelItemDataType_Song,		/**< A Song on the Wheel. */
	WheelItemDataType_Roulette, /**< The roulette section on the Wheel. */
	WheelItemDataType_Random,	/**< The random section on the Wheel. */
	WheelItemDataType_Portal,	/**< The portal section on the Wheel. */
	WheelItemDataType_Course,	/**< A Course on the Wheel. */
	WheelItemDataType_Sort,		/**< A generic sorting item on the Wheel. */
	WheelItemDataType_Custom,	/**< A custom item on the Wheel. */
	NUM_WheelItemDataType,
	WheelItemDataType_Invalid
};
LuaDeclareType(WheelItemDataType);

struct WheelItemBaseData
{
	WheelItemBaseData() = default;
	WheelItemBaseData(WheelItemDataType type,
					  const std::string& sText,
					  const RageColor& color);
	virtual ~WheelItemBaseData() = default;
	WheelItemDataType m_Type;
	std::string m_sText;
	RageColor m_color; // either text color or section background color
};
/** @brief An item on the wheel. */
class WheelItemBase : public ActorFrame
{
  public:
	WheelItemBase(const std::string& sType);
	WheelItemBase(const WheelItemBase& cpy);
	void DrawPrimitives() override;
	[[nodiscard]] auto Copy() const -> WheelItemBase* override
	{
		return new WheelItemBase(*this);
	}

	void Load();
	void DrawGrayBar(Actor& bar);
	void SetExpanded(bool bExpanded) { m_bExpanded = bExpanded; }

	virtual void LoadFromWheelItemData(const WheelItemBaseData* pWID,
									   int iIndex,
									   bool bHasFocus,
									   int iDrawIndex);

	RageColor m_colorLocked;

	auto GetText() -> const std::string
	{
		assert(m_pData != nullptr);
		return m_pData->m_sText;
	}
	auto GetColor() -> const RageColor
	{
		assert(m_pData != nullptr);
		return m_pData->m_color;
	}
	auto GetType() -> WheelItemDataType
	{
		assert(m_pData != nullptr);
		return m_pData->m_Type;
	}
	auto IsLoaded() -> bool { return m_pData != nullptr; }

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	void SetGrayBar(Actor* pBar) { m_pGrayBar = pBar; }

	const WheelItemBaseData* m_pData;
	bool m_bExpanded; // if TYPE_SECTION whether this section is expanded

	Actor* m_pGrayBar;
};

#endif
