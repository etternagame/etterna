#ifndef SCREENSMONLINELOGIN_H
#define SCREENSMONLINELOGIN_H

#include "Etterna/Screen/Options/ScreenOptions.h"

class ScreenSMOnlineLogin : public ScreenOptions
{
  public:
	void Init() override;
	void HandleScreenMessage(ScreenMessage SM) override;
	bool MenuStart(const InputEventPlus& input) override;
	void SendLogin(RString sPassword);
	void SendLogin(RString sPassword, RString user);

  private:
	void ImportOptions(int iRow, const PlayerNumber& vpns) override;
	void ExportOptions(int iRow, const PlayerNumber& vpns) override;
	RString GetSelectedProfileID();
	int m_iPlayer;
	bool typeUsername{ false };
	string username;
};

#endif
