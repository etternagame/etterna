#ifndef DIALOG_BOX_DRIVER_H
#define DIALOG_BOX_DRIVER_H

#include "Dialog.h"
#include "RageUtil/Utils/RageUtil.h"

class DialogDriver
{
  public:
	static DialogDriver* Create();

	virtual void Error(const std::string& sMessage, const std::string& sID)
	{
		printf("Error: %s\n", sMessage.c_str());
	}
	virtual void OK(const std::string& sMessage, const std::string& sID) {}
	virtual Dialog::Result OKCancel(const std::string& sMessage,
									const std::string& sID)
	{
		return Dialog::ok;
	}
	virtual Dialog::Result AbortRetryIgnore(const std::string& sMessage,
											const std::string& sID)
	{
		return Dialog::ignore;
	}
	virtual Dialog::Result AbortRetry(const std::string& sMessage,
									  const std::string& sID)
	{
		return Dialog::abort;
	}
	virtual Dialog::Result YesNo(const std::string& sMessage,
								 const std::string& sID)
	{
		return Dialog::no;
	}

	virtual std::string Init() { return std::string(); }
	virtual ~DialogDriver() {}
};
class DialogDriver_Null : public DialogDriver
{
};
#define USE_DIALOG_DRIVER_NULL

typedef DialogDriver* (*CreateDialogDriverFn)();
struct RegisterDialogDriver
{
	static std::map<istring, CreateDialogDriverFn>* g_pRegistrees;
	RegisterDialogDriver(const istring& sName, CreateDialogDriverFn pfn);
};
#define REGISTER_DIALOG_DRIVER_CLASS(name)                                     \
	static RegisterDialogDriver register_##name(                               \
	  #name, CreateClass<DialogDriver_##name, DialogDriver>)

#endif
