#ifndef DIALOG_BOX_DRIVER_MACOSX_H
#define DIALOG_BOX_DRIVER_MACOSX_H

#include "DialogDriver.h"

class DialogDriver_MacOSX : public DialogDriver
{
  public:
	void Error(const RString& sError, const RString& sID);
	void OK(const RString& sMessage, const RString& sID);
	Dialog::Result OKCancel(const RString& sMessage, const RString& sID);
	Dialog::Result AbortRetryIgnore(const RString& sMessage,
									const RString& sID);
	Dialog::Result AbortRetry(const RString& sMessage, const RString& sID);
	Dialog::Result YesNo(const RString& sMessage, const RString& sID);
};
#define USE_DIALOG_DRIVER_COCOA

#endif
