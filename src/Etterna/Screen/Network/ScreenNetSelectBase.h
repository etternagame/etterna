/* ScreenNetSelectBase - Base screen containing chat room & user list */

#ifndef SCREEN_NET_SELECT_BASE_H
#define SCREEN_NET_SELECT_BASE_H

#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Actor/Base/Quad.h"
#include "Etterna/Screen/Others/ScreenWithMenuElements.h"
#include "Etterna/Actor/Base/Sprite.h"

class ScreenNetSelectBase : public ScreenWithMenuElements
{
  public:
	void Init() override;

	bool Input(const InputEventPlus& input) override;
	void HandleScreenMessage(ScreenMessage SM) override;
	void TweenOffScreen() override;

	void UpdateUsers();
	void UpdateTextInput();

	bool usersVisible = true;
	bool enableChatboxInput = true;
	void SetChatboxVisible(bool visibility);
	void SetUsersVisible(bool visibility);
	vector<BitmapText>* ToUsers();
	void Scroll(unsigned int movescroll);
	RString GetPreviousMsg();
	RString GetNextMsg();
	void SetInputText(RString text);
	void ShowPreviousMsg();
	void ShowNextMsg();
	unsigned int GetScroll() { return scroll; }
	unsigned int GetLines() { return m_textChatOutput.lines; }
	void PasteClipboard();
	// Lua
	void PushSelf(lua_State* L) override;

  private:
	// Chatting
	ColorBitmapText m_textChatInput;
	ColorBitmapText m_textChatOutput;
	AutoActor m_sprChatInputBox;
	AutoActor m_sprChatOutputBox;
	RString m_sTextInput;
	unsigned int m_sTextLastestInputsIndex = 0;
	vector<RString> m_sTextLastestInputs;
	unsigned int scroll = 0;
	RString m_actualText;

	vector<BitmapText> m_textUsers;
};

#endif
