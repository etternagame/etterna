#include "Etterna/Globals/global.h"
#include "RageUtil/Utils/RageUtil.h"
#include "DialogDriver_MacOSX.h"
#include "RageUtil/Misc/RageThreads.h"
#include "Etterna/Globals/ProductInfo.h"
#include "Etterna/Singletons/InputFilter.h"
#include <CoreFoundation/CoreFoundation.h>

REGISTER_DIALOG_DRIVER_CLASS(MacOSX);

static CFOptionFlags
ShowAlert(CFOptionFlags flags,
		  const RString& sMessage,
		  CFStringRef OK,
		  CFStringRef alt = NULL,
		  CFStringRef other = NULL)
{
	CFOptionFlags result;
	CFStringRef text =
	  CFStringCreateWithCString(NULL, sMessage, kCFStringEncodingUTF8);

	if (text == NULL) {
		RString error =
		  ssprintf("CFString for dialog string \"%s\" could not be created.",
				   sMessage.c_str());
		WARN(error);
		DEBUG_ASSERT_M(false, error);
		return kCFUserNotificationDefaultResponse; // Is this better than
												   // displaying an "unknown
												   // error" message?
	}
	CFUserNotificationDisplayAlert(0.0,
								   flags,
								   NULL,
								   NULL,
								   NULL,
								   CFSTR(PRODUCT_FAMILY),
								   text,
								   OK,
								   alt,
								   other,
								   &result);
	CFRelease(text);

	// Flush all input that's accumulated while the dialog box was up.
	if (INPUTFILTER) {
		std::vector<InputEvent> dummy;
		INPUTFILTER->Reset();
		INPUTFILTER->GetInputEvents(dummy);
	}

	return result;
}

#define LSTRING(b, x)                                                          \
	CFBundleCopyLocalizedString((b), CFSTR(x), NULL, CFSTR("Localizable"))

void
DialogDriver_MacOSX::OK(const RString& sMessage, const RString& sID)
{
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFStringRef sDSA = LSTRING(bundle, "Don't show again");
	CFOptionFlags result =
	  ShowAlert(kCFUserNotificationNoteAlertLevel, sMessage, CFSTR("OK"), sDSA);

	CFRelease(sDSA);
	if (result == kCFUserNotificationAlternateResponse)
		Dialog::IgnoreMessage(sID);
}

void
DialogDriver_MacOSX::Error(const RString& sError, const RString& sID)
{
	ShowAlert(kCFUserNotificationStopAlertLevel, sError, CFSTR("OK"));
}

Dialog::Result
DialogDriver_MacOSX::OKCancel(const RString& sMessage, const RString& sID)
{
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFStringRef sOK = LSTRING(bundle, "OK");
	CFStringRef sCancel = LSTRING(bundle, "Cancel");
	CFOptionFlags result =
	  ShowAlert(kCFUserNotificationNoteAlertLevel, sMessage, sOK, sCancel);

	CFRelease(sOK);
	CFRelease(sCancel);
	switch (result) {
		case kCFUserNotificationDefaultResponse:
		case kCFUserNotificationCancelResponse:
			return Dialog::cancel;
		case kCFUserNotificationAlternateResponse:
			return Dialog::ok;
		default:
			FAIL_M(ssprintf("Invalid response: %d.", int(result)));
	}
}

Dialog::Result
DialogDriver_MacOSX::AbortRetryIgnore(const RString& sMessage,
									  const RString& sID)
{
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFStringRef sIgnore = LSTRING(bundle, "Ignore");
	CFStringRef sRetry = LSTRING(bundle, "Retry");
	CFStringRef sAbort = LSTRING(bundle, "Abort");
	CFOptionFlags result = ShowAlert(
	  kCFUserNotificationNoteAlertLevel, sMessage, sIgnore, sRetry, sAbort);

	CFRelease(sIgnore);
	CFRelease(sRetry);
	CFRelease(sAbort);
	switch (result) {
		case kCFUserNotificationDefaultResponse:
			Dialog::IgnoreMessage(sID);
			return Dialog::ignore;
		case kCFUserNotificationAlternateResponse:
			return Dialog::retry;
		case kCFUserNotificationOtherResponse:
		case kCFUserNotificationCancelResponse:
			return Dialog::abort;
		default:
			FAIL_M(ssprintf("Invalid response: %d.", int(result)));
	}
}

Dialog::Result
DialogDriver_MacOSX::AbortRetry(const RString& sMessage, const RString& sID)
{
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFStringRef sRetry = LSTRING(bundle, "Retry");
	CFStringRef sAbort = LSTRING(bundle, "Abort");
	CFOptionFlags result =
	  ShowAlert(kCFUserNotificationNoteAlertLevel, sMessage, sRetry, sAbort);

	CFRelease(sRetry);
	CFRelease(sAbort);
	switch (result) {
		case kCFUserNotificationDefaultResponse:
		case kCFUserNotificationCancelResponse:
			return Dialog::abort;
		case kCFUserNotificationAlternateResponse:
			return Dialog::retry;
		default:
			FAIL_M(ssprintf("Invalid response: %d.", int(result)));
	}
}

Dialog::Result
DialogDriver_MacOSX::YesNo(const RString& sMessage, const RString& sID)
{
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFStringRef sYes = LSTRING(bundle, "Yes");
	CFStringRef sNo = LSTRING(bundle, "No");
	CFOptionFlags result =
	  ShowAlert(kCFUserNotificationNoteAlertLevel, sMessage, sYes, sNo);

	CFRelease(sYes);
	CFRelease(sNo);
	switch (result) {
		case kCFUserNotificationDefaultResponse:
		case kCFUserNotificationCancelResponse:
			return Dialog::no;
		case kCFUserNotificationAlternateResponse:
			return Dialog::yes;
		default:
			FAIL_M(ssprintf("Invalid response: %d.", int(result)));
	}
}

/*
 * (c) 2003-2006 Steve Checkoway
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
