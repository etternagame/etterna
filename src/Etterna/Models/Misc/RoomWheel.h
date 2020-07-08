/* RoomWheel - A wheel containing data about rooms. */

#ifndef ROOM_WHEEL_H
#define ROOM_WHEEL_H

#include "Etterna/Actor/Menus/WheelBase.h"
#include "Etterna/Actor/Menus/WheelItemBase.h"

class RoomData
{
  public:
	void SetName(const std::string& name) { m_name = name; }
	void SetDescription(const std::string& desc) { m_description = desc; }
	void SetState(unsigned int state) { m_state = state; }
	void SetFlags(unsigned int iFlags) { m_iFlags = iFlags; }
	void SetHasPassword(bool pass) { hasPassword = pass; }
	[[nodiscard]] std::string Name() const { return m_name; }
	[[nodiscard]] std::string Description() const { return m_description; }
	[[nodiscard]] unsigned int State() const { return m_state; }
	[[nodiscard]] bool HasPassword() const { return hasPassword; }
	[[nodiscard]] unsigned int GetFlags() const { return m_iFlags; }
	RoomData()
	{
		m_name = "";
		m_description = "";
		m_state = 0;
		m_iFlags = 0;
	}
	vector<std::string> players;

  private:
	std::string m_name;
	std::string m_description;
	unsigned int m_state;
	unsigned int m_iFlags;
	bool hasPassword{ false };
};

struct RoomWheelItemData : public WheelItemBaseData
{
	RoomWheelItemData() = default;
	RoomWheelItemData(WheelItemDataType type,
					  const std::string& sTitle,
					  const std::string& sDesc,
					  const RageColor& color,
					  const bool hasPass = false)
	  : WheelItemBaseData(type, sTitle, color)
	  , m_sDesc(sDesc)
	  , hasPassword(hasPass){};

	std::string m_sDesc;
	unsigned int m_iFlags{ 0 };
	bool hasPassword{ false };
};

class RoomWheelItem : public WheelItemBase
{
  public:
	RoomWheelItem(const std::string& sType = "RoomWheelItem");
	RoomWheelItem(const RoomWheelItem& cpy);

	void LoadFromWheelItemData(const WheelItemBaseData* pWID,
							   int iIndex,
							   bool bHasFocus,
							   int iDrawIndex) override;
	[[nodiscard]] RoomWheelItem* Copy() const override
	{
		return new RoomWheelItem(*this);
	}
	void Load(const std::string& sType);

  private:
	AutoActor m_sprNormalPart;
	AutoActor m_sprColorPart;
	AutoActor m_sprOverPart;
	BitmapText m_text;
	BitmapText m_Desc;
};

struct RoomInfo
{
	std::string songTitle;
	std::string songSubTitle;
	std::string songArtist;
	int numPlayers;
	int maxPlayers;
	vector<std::string> players;
};

struct RoomSearch
{
	std::string title;
	std::string desc;
	bool ingame;
	bool password;
	bool open;
	RoomSearch()
	{
		ingame = password = open = true;
		title = desc = "";
	}
};

class RoomWheel : public WheelBase
{
  public:
	~RoomWheel() override;
	void Load(const std::string& sType) override;
	virtual void BuildWheelItemsData(
	  vector<WheelItemBaseData*>& arrayWheelItemDatas);
	unsigned int GetNumItems() const override;
	bool Select() override;
	void Move(int n) override;

	RoomWheelItemData* GetItem(unsigned int i)
	{
		return dynamic_cast<RoomWheelItemData*>(
		  WheelBase::GetItem(i + m_offset));
	}
	void AddPermanentItem(RoomWheelItemData* itemdata);
	int GetCurrentIndex() const { return m_iSelection; }
	int GetPerminateOffset() const { return m_offset; }
	void AddItem(WheelItemBaseData* itemdata);
	void RemoveItem(int index);

	void StopSearch();
	void Search(RoomSearch findme);

	void BuildFromRoomDatas();
	void UpdateRoomsList(vector<RoomData>* m_Roomsptr);
	void FilterBySearch();

	// Lua
	void PushSelf(lua_State* L) override;

  private:
	WheelItemBase* MakeItem() override;
	int m_offset;

	vector<RoomData>* allRooms;
	vector<RoomData> roomsInWheel;

	RoomSearch currentSearch;
	bool searching;
};

#endif
