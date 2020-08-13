#ifndef SCREENSMONLINELOGIN_H
#define SCREENSMONLINELOGIN_H

#include "Etterna/Screen/Options/ScreenOptions.h"

class ScreenSMOnlineLogin : public ScreenOptions
{
  public:
	void Init() override;
	void HandleScreenMessage(const ScreenMessage& SM) override;
	bool MenuStart(const InputEventPlus& input) override;
	void SendLogin(std::string sPassword);
	void SendLogin(std::string sPassword, std::string user);

  private:
	void ImportOptions(int iRow, const PlayerNumber& vpns) override;
	void ExportOptions(int iRow, const PlayerNumber& vpns) override;
	std::string GetSelectedProfileID();
	int m_iPlayer = 0;
	bool typeUsername{ false };
	string username;
};

#endif
