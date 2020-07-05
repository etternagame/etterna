#ifndef DIALOG_BOX_DRIVER_MACOSX_H
#define DIALOG_BOX_DRIVER_MACOSX_H

#include "DialogDriver.h"

class DialogDriver_MacOSX : public DialogDriver
{
  public:
	void Error(const std::string& sError, const std::string& sID);
	void OK(const std::string& sMessage, const std::string& sID);
	Dialog::Result OKCancel(const std::string& sMessage, const std::string& sID);
	Dialog::Result AbortRetryIgnore(const std::string& sMessage,
									const std::string& sID);
	Dialog::Result AbortRetry(const std::string& sMessage, const std::string& sID);
	Dialog::Result YesNo(const std::string& sMessage, const std::string& sID);
};
#define USE_DIALOG_DRIVER_COCOA

#endif
