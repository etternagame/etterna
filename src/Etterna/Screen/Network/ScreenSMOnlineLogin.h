#ifndef SCREENSMONLINELOGIN_H
#define SCREENSMONLINELOGIN_H

#include "Etterna/Screen/Options/ScreenOptions.h"

class ScreenSMOnlineLogin : public ScreenWithMenuElements
{
  public:
	void Init() override;
	void HandleScreenMessage(const ScreenMessage& SM) override;
	void SendLogin(std::string sPassword);
	void SendLogin(std::string sPassword, std::string user);

  private:
	int m_iPlayer = 0;
	string username;
};

#endif
