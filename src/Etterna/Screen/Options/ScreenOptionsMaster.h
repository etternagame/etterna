#ifndef SCREEN_OPTIONS_MASTER_H
#define SCREEN_OPTIONS_MASTER_H

#include "ScreenOptions.h"

class OptionRowHandler;

class ScreenOptionsMaster : public ScreenOptions
{
  public:
	void Init() override;

  private:
	int m_iChangeMask;

  protected:
	void HandleScreenMessage(ScreenMessage SM) override;

	void ImportOptions(int iRow, const PlayerNumber& vpns) override;
	void ExportOptions(int iRow, const PlayerNumber& vpns) override;
};

#endif
