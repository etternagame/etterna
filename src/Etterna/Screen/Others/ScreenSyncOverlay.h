#ifndef ScreenSyncOverlay_H
#define ScreenSyncOverlay_H

#include "Etterna/Actor/Base/AutoActor.h"
#include "Screen.h"
/** @brief Credits and statistics drawn on top of everything else. */
class ScreenSyncOverlay : public Screen
{
  public:
	void Init() override;

	bool Input(const InputEventPlus& input) override;

	void Update(float fDeltaTime) override;

	static void SetShowAutoplay(bool b);

  private:
	void UpdateText(bool forcedChange = false);
	void ShowHelp();
	void HideHelp();

	AutoActor m_overlay;
};

#endif
