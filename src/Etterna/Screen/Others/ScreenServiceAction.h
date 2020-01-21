/* ScreenServiceAction -  */

#ifndef ScreenServiceAction_H
#define ScreenServiceAction_H

#include "ScreenPrompt.h"

class ScreenServiceAction : public ScreenPrompt
{
  public:
	void BeginScreen() override;
};

#endif
