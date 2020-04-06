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
