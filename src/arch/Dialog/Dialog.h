#ifndef DIALOG_BOX_H
#define DIALOG_BOX_H

namespace Dialog {
/* ID can be used to identify a class of messages, for "don't display this
 * dialog"-type prompts. */
void
Init();
void
Shutdown();

void
SetWindowed(bool bWindowed);

enum Result
{
	ok,
	cancel,
	abort,
	retry,
	ignore,
	yes,
	no
};
void
Error(const RString& sError, const RString& sID = "");
void
OK(const RString& sMessage, const RString& sID = "");
Result
OKCancel(const RString& sMessage, const RString& sID = "");
Result
AbortRetryIgnore(const RString& sMessage, const RString& sID = "");
Result
AbortRetry(const RString& sMessage, const RString& sID = "");
Result
YesNo(const RString& sMessage, const RString& sID = "");

/* for DialogDrivers */
void
IgnoreMessage(const RString& sID);
}

#endif

/*
 * (c) 2003-2004 Glenn Maynard, Chris Danford
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
