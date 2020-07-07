/* ScreenMapControllers - Maps device input to instrument buttons. */

#ifndef SCREEN_MAP_CONTROLLERS_H
#define SCREEN_MAP_CONTROLLERS_H

#include "Etterna/Actor/Base/ActorScroller.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Singletons/InputMapper.h"
#include "RageUtil/Sound/RageSound.h"
#include "ScreenWithMenuElements.h"

#include <set>

class ScreenMapControllers : public ScreenWithMenuElements
{
  public:
	ScreenMapControllers();
	~ScreenMapControllers() override;
	void Init() override;
	void BeginScreen() override;

	void Update(float fDeltaTime) override;
	bool Input(const InputEventPlus& input) override;
	void HandleMessage(const Message& msg) override;
	void HandleScreenMessage(const ScreenMessage& SM) override;

  private:
	Actor* GetActorWithFocus();
	void BeforeChangeFocus();
	void AfterChangeFocus();
	void Refresh();
	void DismissWarning();
	bool CursorOnAction();
	bool CursorOnHeader();
	bool CursorOnKey();
	bool CursorCanGoUp();
	bool CursorCanGoDown();
	bool CursorCanGoLeft();
	bool CursorCanGoRight();
	int CurKeyIndex();
	int CurActionIndex();
	void SetCursorFromSetListCurrent();
	void StartWaitingForPress();

	unsigned int m_CurController = 0;
	unsigned int m_CurButton = 0;
	unsigned int m_CurSlot = 0;
	unsigned int m_MaxDestItem = 0;

	bool m_ChangeOccurred;

	RageTimer m_WaitingForPress;
	DeviceInput m_DeviceIToMap;

	struct KeyToMap
	{
		GameButton m_GameButton;

		// owned by m_Line
		BitmapText* m_textMappedTo[NUM_GameController]
								  [NUM_SHOWN_GAME_TO_DEVICE_SLOTS];
	};
	vector<KeyToMap> m_KeysToMap;

	BitmapText m_textDevices;

	BitmapText m_textLabel[NUM_GameController];
	BitmapText m_ListHeaderCenter;
	BitmapText m_ListHeaderLabels[NUM_GameController]
								 [NUM_SHOWN_GAME_TO_DEVICE_SLOTS];

	float m_AutoDismissWarningSecs = 2.5f;
	AutoActor m_Warning;

	float m_AutoDismissNoSetListPromptSecs = 5.f;
	AutoActor m_NoSetListPrompt;

	float m_AutoDismissSanitySecs = 5.f;
	AutoActor m_SanityMessage;

	struct SetListEntry
	{
		int m_button;
		int m_controller;
		int m_slot;
		SetListEntry(int b, int c, int s)
		  : m_button(b)
		  , m_controller(c)
		  , m_slot(s)
		{
		}
		bool operator<(SetListEntry const& rhs) const
		{
			if (m_controller != rhs.m_controller) {
				return m_controller < rhs.m_controller;
			}
			if (m_button != rhs.m_button) {
				return m_button < rhs.m_button;
			}
			return m_slot < rhs.m_slot;
		}
	};
	std::set<SetListEntry> m_SetList;
	std::set<SetListEntry>::iterator m_SetListCurrent;
	bool m_InSetListMode;

	using action_fun_t = void (ScreenMapControllers::*)();
	struct ActionRow
	{
		std::string m_name;
		AutoActor m_actor;
		action_fun_t m_action;
		void Load(std::string const& scr_name,
				  std::string const& name,
				  ScreenMapControllers::action_fun_t action,
				  ActorFrame* line,
				  ActorScroller* scroller);
	};
	void ClearToDefault();
	void ReloadFromDisk();
	void SaveToDisk();
	void SetListMode();
	void ExitAction();
	bool SanityCheckWrapper();

	vector<ActionRow> m_Actions;

	vector<ActorFrame*> m_Line;
	ActorScroller m_LineScroller;

	RageSound m_soundChange;
	RageSound m_soundDelete;
};

#endif
