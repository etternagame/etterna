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
Error(const std::string& sError, const std::string& sID = "");
void
OK(const std::string& sMessage, const std::string& sID = "");
Result
OKCancel(const std::string& sMessage, const std::string& sID = "");
Result
AbortRetryIgnore(const std::string& sMessage, const std::string& sID = "");
Result
AbortRetry(const std::string& sMessage, const std::string& sID = "");
Result
YesNo(const std::string& sMessage, const std::string& sID = "");

/* for DialogDrivers */
void
IgnoreMessage(const std::string& sID);
}

#endif
