#ifndef ScreenInstallOverlay_H
#define ScreenInstallOverlay_H

#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Screen.h"

/** @brief Package installation processing and messaging. */
class ScreenInstallOverlay : public Screen
{
  public:
	~ScreenInstallOverlay() override;
	void Init() override;

	void Update(float fDeltaTime) override;
	bool Input(const InputEventPlus& input) override;

  private:
	void UpdateText();

	BitmapText m_textStatus;
};
#endif
