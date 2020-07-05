#ifndef RollingNumbers_H
#define RollingNumbers_H

#include "BitmapText.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

/** @brief Animates from one number to another by scrolling its digits. */
class RollingNumbers : public BitmapText
{
  public:
	RollingNumbers();

	void Load(const std::string& sMetricsGroup);
	RollingNumbers* Copy() const override;

	void DrawPart(RageColor const* diffuse,
				  RageColor const& stroke,
				  float crop_left,
				  float crop_right);
	void DrawPrimitives() override;
	void Update(float fDeltaTime) override;

	/**
	 * @brief Set the new target number to be reached.
	 * @param fTargetNumber the new target number. */
	void SetTargetNumber(float fTargetNumber);

	void UpdateText();

	// Commands
	void PushSelf(lua_State* L) override;

  private:
	ThemeMetric<std::string> TEXT_FORMAT;
	ThemeMetric<float> APPROACH_SECONDS;
	ThemeMetric<bool> COMMIFY;
	ThemeMetric<RageColor> LEADING_ZERO_MULTIPLY_COLOR;

	/** @brief The currently showing number. */
	float m_fCurrentNumber;
	/** @brief The number we are trying to approach. */
	float m_fTargetNumber;
	/** @brief The speed we are trying to reach the target number. */
	float m_fScoreVelocity;
	bool m_metrics_loaded;
};

#endif
