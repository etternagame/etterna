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
	std::vector<BitmapText>* ToUsers();
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
	unsigned int m_sTextLastestInputsIndex;
	std::vector<RString> m_sTextLastestInputs;
	unsigned int scroll;
	RString m_actualText;

	std::vector<BitmapText> m_textUsers;
};

#endif

/*
 * (c) 2004 Charles Lohr
 * All rights reserved.
 *
 *     based off of ScreenEz2SelectMusic by "Frieza"
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
