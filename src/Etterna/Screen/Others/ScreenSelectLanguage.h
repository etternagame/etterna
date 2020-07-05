#ifndef ScreenSelectLanguage_H
#define ScreenSelectLanguage_H

#include "ScreenSelectMaster.h"

class ScreenSelectLanguage : public ScreenSelectMaster
{
  public:
	void Init() override;
	std::string GetDefaultChoice() override;
	void BeginScreen() override;
	bool MenuStart(const InputEventPlus& input) override;
	bool MenuBack(const InputEventPlus& input) override;
};

#endif
