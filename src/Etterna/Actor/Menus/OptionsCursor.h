#ifndef OPTIONS_CURSOR_H
#define OPTIONS_CURSOR_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"

/** @brief A cursor for ScreenOptions. */
class OptionsCursor : public ActorFrame
{
  public:
	/** @brief Set up a default OptionsCursor. */
	OptionsCursor();
	/**
	 * @brief Set up an OptionsCursor based on an existing copy.
	 * @param cpy the OptionsCursor we are copying. */
	OptionsCursor(const OptionsCursor& cpy);

	void Load(const std::string& sMetricsGroup, bool bLoadCanGos);

	void StopTweening() override;
	void BeginTweening(float fSecs, TweenType tt = TWEEN_DECELERATE) override;
	void SetBarWidth(int iWidth);
	int GetBarWidth() const;
	void SetCanGo(bool bCanGoLeft, bool bCanGoRight);

  protected:
	AutoActor m_sprMiddle;
	AutoActor m_sprLeft;
	AutoActor m_sprRight;

	AutoActor m_sprCanGoLeft;
	AutoActor m_sprCanGoRight;

	// save the metrics-set X because it gets obliterated on a call to
	// SetBarWidth
	int m_iOriginalLeftX;
	int m_iOriginalRightX;
	int m_iOriginalCanGoLeftX;
	int m_iOriginalCanGoRightX;
};

#endif
