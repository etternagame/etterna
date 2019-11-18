#ifndef DIALOG_BOX_DRIVER_H
#define DIALOG_BOX_DRIVER_H

#include "Dialog.h"
#include "RageUtil/Utils/RageUtil.h"

class DialogDriver
{
  public:
	static DialogDriver* Create();

	virtual void Error(const RString& sMessage, const RString& sID)
	{
		printf("Error: %s\n", sMessage.c_str());
	}
	virtual void OK(const RString& sMessage, const RString& sID) {}
	virtual Dialog::Result OKCancel(const RString& sMessage, const RString& sID)
	{
		return Dialog::ok;
	}
	virtual Dialog::Result AbortRetryIgnore(const RString& sMessage,
											const RString& sID)
	{
		return Dialog::ignore;
	}
	virtual Dialog::Result AbortRetry(const RString& sMessage,
									  const RString& sID)
	{
		return Dialog::abort;
	}
	virtual Dialog::Result YesNo(const RString& sMessage, const RString& sID)
	{
		return Dialog::no;
	}

	virtual RString Init() { return RString(); }
	virtual ~DialogDriver() {}
};
class DialogDriver_Null : public DialogDriver
{
};
#define USE_DIALOG_DRIVER_NULL

typedef DialogDriver* (*CreateDialogDriverFn)();
struct RegisterDialogDriver
{
	static map<istring, CreateDialogDriverFn>* g_pRegistrees;
	RegisterDialogDriver(const istring& sName, CreateDialogDriverFn pfn);
};
#define REGISTER_DIALOG_DRIVER_CLASS(name)                                     \
	static RegisterDialogDriver register_##name(                               \
	  #name, CreateClass<DialogDriver_##name, DialogDriver>)

#endif
