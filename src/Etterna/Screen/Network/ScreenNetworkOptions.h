#ifndef SCREEN_NETWORK_OPTIONS_H
#define SCREEN_NETWORK_OPTIONS_H

#include "Etterna/Screen/Options/ScreenOptions.h"

class ScreenNetworkOptions : public ScreenOptions
{
  public:
	void Init() override;

	void HandleScreenMessage(const ScreenMessage& SM) override;

	bool MenuStart(const InputEventPlus& input) override;

  private:
	void ImportOptions(int iRow, const PlayerNumber& vpns) override;
	void ExportOptions(int iRow, const PlayerNumber& vpns) override;
	// vector<NetServerInfo> AllServers;

	void UpdateConnectStatus();
};

#endif
