/* ScreenDebugOverlay - credits and statistics drawn on top of everything else.
 */

#ifndef ScreenDebugOverlay_H
#define ScreenDebugOverlay_H

#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Actor/Base/Quad.h"
#include "Screen.h"

void
ChangeVolume(float fDelta);
void
ChangeVisualDelay(float fDelta);

class ScreenDebugOverlay : public Screen
{
  public:
	~ScreenDebugOverlay() override;
	void Init() override;

	bool Input(const InputEventPlus& input) override;

	void Update(float fDeltaTime) override;

  private:
	void UpdateText();

	RString GetCurrentPageName() const { return m_asPages[m_iCurrentPage]; }
	vector<RString> m_asPages;
	int m_iCurrentPage = 0;
	bool m_bForcedHidden = false;

	Quad m_Quad;
	BitmapText m_textHeader;
	vector<BitmapText*> m_vptextPages;
	vector<BitmapText*> m_vptextButton;
	vector<BitmapText*> m_vptextFunction;
};

#endif
