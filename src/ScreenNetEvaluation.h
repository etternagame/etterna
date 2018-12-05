#include "BitmapText.h"
#include "DifficultyIcon.h"
#include "Quad.h"
#include "ScreenEvaluation.h"
#include "ScreenMessage.h"
#include "StepsDisplay.h"

class ScreenNetEvaluation : public ScreenEvaluation
{
  public:
	void Init() override;

	// sm-ssc:
	int GetNumActivePlayers() { return m_iActivePlayers; }
	bool Input(const InputEventPlus& input) override;
	// Lua
	void PushSelf(lua_State* L) override;

	int m_iCurrentPlayer;
	void UpdateStats();
  protected:
	void HandleScreenMessage(ScreenMessage SM) override;
	void TweenOffScreen() override;
  private:
	int m_iActivePlayers;

	PlayerNumber m_pActivePlayer;

	bool m_bHasStats;
};

/*
 * (c) 2004-2005 Charles Lohr, Joshua Allen
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
