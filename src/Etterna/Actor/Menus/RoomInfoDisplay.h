/* RoomInfoDisplay: Shows information about an online game room. */

#ifndef ROOM_INFO_DISPLAY_H
#define ROOM_INFO_DISPLAY_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Misc/RoomWheel.h"

class RoomInfoDisplay : public ActorFrame
{
  public:
	RoomInfoDisplay();
	~RoomInfoDisplay() override;
	virtual void Load(const std::string& sType);
	void Update(float fDeltaTime) override;
	void SetRoom(const RoomWheelItemData* roomData);
	void SetRoomInfo(const RoomInfo& info);
	void DeployInfoBox();
	void RetractInfoBox();

  private:
	void RequestRoomInfo(const std::string& name);
	enum RoomInfoDisplayState
	{
		OPEN = 0,
		CLOSED,
		LOCKED
	};

	RoomInfoDisplayState m_state{ OPEN };
	AutoActor m_bg;
	BitmapText m_Title;
	BitmapText m_Desc;

	BitmapText m_lastRound;
	BitmapText m_songTitle;
	BitmapText m_songSub;
	BitmapText m_songArtist;

	BitmapText m_players;
	vector<BitmapText*> m_playerList;

	RageTimer m_deployDelay;

	ThemeMetric<float> X;
	ThemeMetric<float> Y;
	ThemeMetric<float> DEPLOY_DELAY;
	ThemeMetric<float> RETRACT_DELAY;
	ThemeMetric<float> PLAYERLISTX;
	ThemeMetric<float> PLAYERLISTY;
	ThemeMetric<float> PLAYERLISTOFFSETX;
	ThemeMetric<float> PLAYERLISTOFFSETY;
};

#endif
