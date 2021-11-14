/* StepsDisplayList - Shows all available difficulties for a Song */
#ifndef DIFFICULTY_LIST_H
#define DIFFICULTY_LIST_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Misc/PlayerNumber.h"
#include "Etterna/Actor/GameplayAndMenus/StepsDisplay.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

class Song;
class Steps;

class StepsDisplayList final : public ActorFrame
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

	struct Lines
	{
		std::vector<std::unique_ptr<StepsDisplay>> lines;

		Lines() = default;
		Lines(const Lines& other) {
			for (auto& x : other.lines)
				this->lines.push_back(std::make_unique<StepsDisplay>(*x));
		}
		Lines& operator=(const Lines& other)
		{
			for (auto& x : other.lines)
				this->lines.push_back(std::make_unique<StepsDisplay>(*x));
		}
	};
	Lines m_Lines;

	const Song* m_CurSong;
	bool m_bShown;

	struct Row
	{
		Row()
		{
			m_Steps = nullptr;
			m_dc = Difficulty_Invalid;
			m_fY = 0;
			m_bHidden = false;
		}

		const Steps* m_Steps;
		Difficulty m_dc;
		float m_fY;
		bool m_bHidden; // currently off screen
	};

	std::vector<Row> m_Rows;
};

#endif
