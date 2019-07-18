/* ScrollBar - A simple scrollbar. */

#ifndef SCROLL_BAR_H
#define SCROLL_BAR_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"

class ScrollBar : public ActorFrame
{
  public:
	ScrollBar();

	void SetBarHeight(int iHeight);
	void SetPercentage(float fCenterPercent, float fSizePercent);

  protected:
	int m_iBarHeight;

	AutoActor m_sprMiddle;
	AutoActor m_sprTop;
	AutoActor m_sprBottom;
	AutoActor m_sprScrollTickThumb;
	AutoActor m_sprScrollStretchThumb[2];
};

#endif
