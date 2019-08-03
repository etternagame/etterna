#ifndef DIALOG_BOX_DRIVER_WIN32_H
#define DIALOG_BOX_DRIVER_WIN32_H

#include "DialogDriver.h"

class DialogDriver_Win32 : public DialogDriver
{
  public:
	void Error(const RString& sMessage, const RString& sID);
	void OK(const RString& sMessage, const RString& sID);
	Dialog::Result OKCancel(const RString& sMessage, const RString& sID);
	Dialog::Result AbortRetryIgnore(const RString& sMessage,
									const RString& sID);
	Dialog::Result AbortRetry(const RString& sMessage, const RString& sID);
	Dialog::Result YesNo(const RString& sMessage, const RString& sID);
};

#endif
