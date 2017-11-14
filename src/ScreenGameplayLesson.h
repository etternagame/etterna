#ifndef ScreenGameplayLesson_H
#define ScreenGameplayLesson_H

#include "ScreenGameplayNormal.h"
class CourseEntry;

/** @brief Shows some explanation pages, then allows 3 tries to pass a song. */
class ScreenGameplayLesson : public ScreenGameplayNormal
{
public:
	ScreenGameplayLesson();
	void Init() override;

	bool Input( const InputEventPlus &input ) override;
	void HandleScreenMessage( const ScreenMessage SM ) override;

	bool MenuStart( const InputEventPlus &input ) override;
	bool MenuBack( const InputEventPlus &input ) override;

protected:
	void ChangeLessonPage( int iDir );
	void ResetAndRestartCurrentSong();

	vector<AutoActor> m_vPages;
	int m_iCurrentPageIndex;

	enum Try
	{
		Try_1,
		Try_2,
		Try_3,
		NUM_Try
	};
	Try m_Try;
};

#endif

/*
 * (c) 2003-2004 Chris Danford
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
