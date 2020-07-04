#ifndef ScreenOptionsManageProfiles_H
#define ScreenOptionsManageProfiles_H

#include "Etterna/Screen/Others/ScreenMiniMenu.h"
#include "ScreenOptions.h"

class ScreenOptionsManageProfiles : public ScreenOptions
{
  public:
	void Init() override;
	void BeginScreen() override;

	void HandleScreenMessage(ScreenMessage SM) override;

  protected:
	void ImportOptions(int iRow, const PlayerNumber& vpns) override;
	void ExportOptions(int iRow, const PlayerNumber& vpns) override;

	void AfterChangeRow(PlayerNumber pn) override;
	void ProcessMenuStart(const InputEventPlus& input) override;

	int GetLocalProfileIndexWithFocus() const;
	RString GetLocalProfileIDWithFocus() const;

	vector<std::string> m_vsLocalProfileID;
};

#endif
