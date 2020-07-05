/* StreamDisplay -  */
#ifndef StreamDisplay_H
#define StreamDisplay_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Lua/LuaExpressionTransform.h"
#include "Etterna/Actor/Base/Quad.h"
#include "Etterna/Actor/Base/Sprite.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

enum StreamType
{
	StreamType_Normal,
	StreamType_Passing,
	StreamType_Hot,
	NUM_StreamType,
};

class StreamDisplay : public ActorFrame
{
  public:
	StreamDisplay();

	void Update(float fDeltaSecs) override;

	void Load(const std::string& sMetricsGroup);

	void SetPercent(float fPercent);
	void SetPassingAlpha(float fPassingAlpha)
	{
		m_fPassingAlpha = fPassingAlpha;
	}
	void SetHotAlpha(float fHotAlpha) { m_fHotAlpha = fHotAlpha; }

	float GetPercent() { return m_fPercent; }

  private:
	vector<Sprite*> m_vpSprPill[NUM_StreamType];

	LuaExpressionTransform
	  m_transformPill; // params: self,offsetFromCenter,itemIndex,numItems
	ThemeMetric<float> VELOCITY_MULTIPLIER;
	ThemeMetric<float> VELOCITY_MIN;
	ThemeMetric<float> VELOCITY_MAX;
	ThemeMetric<float> SPRING_MULTIPLIER;
	ThemeMetric<float> VISCOSITY_MULTIPLIER;

	float m_fPercent; // percent filled
	float
	  m_fTrailingPercent; // this approaches m_fPercent, use this value to draw
	float m_fVelocity;	  // velocity of m_fTrailingPercent

	float m_fPassingAlpha;
	float m_fHotAlpha;

	bool m_bAlwaysBounce;
};

#endif
