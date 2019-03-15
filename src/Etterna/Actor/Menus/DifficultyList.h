/* StepsDisplayList - Shows all available difficulties for a Song */
#ifndef DIFFICULTY_LIST_H
#define DIFFICULTY_LIST_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Misc/PlayerNumber.h"
#include "Etterna/Actor/GameplayAndMenus/StepsDisplay.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

class Song;
class Steps;

class StepsDisplayList : public ActorFrame
{
  public:
	StepsDisplayList();
	~StepsDisplayList() override;
	StepsDisplayList* Copy() const override;
	void LoadFromNode(const XNode* pNode) override;

	void HandleMessage(const Message& msg) override;

	void SetFromGameState();
	void TweenOnScreen();
	void TweenOffScreen();
	void Hide();
	void Show();
	int GetCurrentRowIndex(PlayerNumber pn) const;
	// Lua
	void PushSelf(lua_State* L) override;

  private:
	void UpdatePositions();
	void PositionItems();
	void HideRows();

	ThemeMetric<float> ITEMS_SPACING_Y;
	ThemeMetric<int> NUM_SHOWN_ITEMS;
	ThemeMetric<bool> CAPITALIZE_DIFFICULTY_NAMES;
	ThemeMetric<apActorCommands> MOVE_COMMAND;

	AutoActor m_Cursors;
	ActorFrame m_CursorFrames; // contains Cursor so that color can
											// fade independent of other tweens

	struct Line
	{
		StepsDisplay m_Meter;
	};
	vector<Line> m_Lines;

	const Song* m_CurSong;
	bool m_bShown;

	struct Row
	{
		Row()
		{
			m_Steps = NULL;
			m_dc = Difficulty_Invalid;
			m_fY = 0;
			m_bHidden = false;
		}

		const Steps* m_Steps;
		Difficulty m_dc;
		float m_fY;
		bool m_bHidden; // currently off screen
	};

	vector<Row> m_Rows;
};

#endif

/*
 * (c) 2003-2004 Glenn Maynard
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
