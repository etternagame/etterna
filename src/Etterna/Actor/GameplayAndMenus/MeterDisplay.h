﻿/* MeterDisplay - Display position in a song. */

#ifndef METER_DISPLAY_H
#define METER_DISPLAY_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"

class MeterDisplay : public ActorFrame
{
  public:
	MeterDisplay();
	void Load(const RString& sStreamPath,
			  float fStreamWidth,
			  const RString& sTipPath);
	void LoadFromNode(const XNode* pNode) override;
	MeterDisplay* Copy() const override;

	void SetPercent(float fPercent);
	void SetStreamWidth(float fStreamWidth);

	// Lua
	void PushSelf(lua_State* L) override;

  private:
	float m_fStreamWidth;
	float m_fPercent;
	AutoActor m_sprStream;
	AutoActor m_sprTip;
};

class SongMeterDisplay : public MeterDisplay
{
  public:
	void Update(float fDeltaTime) override;
	SongMeterDisplay* Copy() const override;
};

#endif
