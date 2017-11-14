#ifndef GROOVE_RADAR_H
#define GROOVE_RADAR_H

#include "ActorFrame.h"
#include "Sprite.h"
#include "AutoActor.h"
#include "PlayerNumber.h"
#include "GameConstantsAndTypes.h"
class Steps;
struct RadarValues;
/** @brief The song's GrooveRadar displayed in SelectMusic. */
class GrooveRadar : public ActorFrame
{
public:
	GrooveRadar();
	GrooveRadar *Copy() const override;
	void LoadFromNode( const XNode* pNode ) override;

	/** 
	 * @brief Give the Player an empty GrooveRadar.
	 * @param pn the Player to give an empty GrooveRadar. */
	void SetEmpty( PlayerNumber pn );
	void SetFromRadarValues( PlayerNumber pn, const RadarValues &rv );
	/**
	 * @brief Give the Player a GrooveRadar based on some Steps.
	 * @param pn the Player to give a GrooveRadar.
	 * @param pSteps the Steps to use to make the radar. If NULL, there are no Steps. */
	void SetFromSteps( PlayerNumber pn, Steps* pSteps );
	void SetFromValues( PlayerNumber pn, vector<float> vals );

	// Lua
	void PushSelf( lua_State *L ) override;

protected:
	/**
	 * @brief The companion ValueMap to the GrooveRadar.
	 *
	 * This must be a separate Actor so that it can be tweened separately from the labels. */
	class GrooveRadarValueMap : public ActorFrame
	{
	public:
		GrooveRadarValueMap();

		void Update( float fDeltaTime ) override;
		void DrawPrimitives() override;

		void SetEmpty();
		void SetFromSteps( const RadarValues &rv );
		void SetFromValues( vector<float> vals );

		void SetRadius( float f ) { m_size.x = f; m_size.y = f; }

		bool m_bValuesVisible;
		float m_PercentTowardNew;
		float m_fValuesNew[NUM_RadarCategory];
		float m_fValuesOld[NUM_RadarCategory];

		PlayerNumber m_PlayerNumber;
	};

	AutoActor m_sprRadarBase;
	GrooveRadarValueMap m_GrooveRadarValueMap[NUM_PLAYERS];
	// xxx: convert Sprite to AutoActor -aj
	Sprite m_sprRadarLabels[NUM_RadarCategory];
	ActorFrame m_Frame;
};

#endif

/**
 * @file
 * @author Chris Danford (c) 2001-2004
 * @section LICENSE
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
