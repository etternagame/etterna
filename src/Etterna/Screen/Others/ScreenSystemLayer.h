/* ScreenSystemLayer - credits and statistics drawn on top of everything else.
 */

#ifndef ScreenSystemLayer_H
#define ScreenSystemLayer_H

#include "Etterna/Actor/Base/AutoActor.h"
#include "Screen.h"

class ScreenSystemLayer : public Screen
{
  public:
	void Init() override;

  private:
	AutoActor m_sprOverlay;
	AutoActor m_errLayer;
};

#endif
