#ifndef ScreenOptionsEditProfile_H
#define ScreenOptionsEditProfile_H

#include "Etterna/Models/Misc/Profile.h"
#include "ScreenOptions.h"

class ScreenOptionsEditProfile : public ScreenOptions
{
  public:
	~ScreenOptionsEditProfile() override;

	void Init() override;
	void BeginScreen() override;

  private:
	void ImportOptions(int row, const PlayerNumber& vpns) override;
	void ExportOptions(int row, const PlayerNumber& vpns) override;

	virtual void GoToNextScreen();
	virtual void GoToPrevScreen();

	void HandleScreenMessage(const ScreenMessage& SM) override;
	void AfterChangeValueInRow(int iRow, PlayerNumber pn) override;
	void ProcessMenuStart(const InputEventPlus& input) override;

	Profile m_Original; // restore this on revert
};

#endif
