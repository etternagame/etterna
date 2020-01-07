/* ScreenSaveSync -  */

#ifndef ScreenSaveSync_H
#define ScreenSaveSync_H

#include "ScreenPrompt.h"

class ScreenSaveSync : public ScreenPrompt
{
  public:
	void Init() override;

	static void PromptSaveSync(ScreenMessage sm = SM_None);
};

#endif
