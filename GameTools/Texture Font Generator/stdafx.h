// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#endif

#define NOMINMAX
#include <algorithm>
using namespace std;

// Modify the following defines if you have to target a platform prior to the
// ones specified below. Refer to MSDN for the latest info on corresponding
// values for different platforms.
#ifndef WINVER // Allow use of features specific to Windows 95 and Windows NT 4
			   // or later.
#define WINVER                                                                 \
	0x0501 // Change this to the appropriate value to target Windows 98 and
		   // Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT // Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT                                                           \
	0x0501 // Change this to the appropriate value to target Windows 98 and
		   // Windows 2000 or later.
#endif

#ifndef _WIN32_WINDOWS // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS                                                         \
	0x0410 // Change this to the appropriate value to target Windows Me or
		   // later.
#endif

#ifndef _WIN32_IE // Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE                                                              \
	0x0400 // Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // some CString constructors will be
										   // explicit

// turns off MFC's hiding of some common and often safely ignored warning
// messages
#define _AFX_ALL_WARNINGS

#pragma warning(disable : 4996) // deprecated functions vs "ISO C++ conformant
								// names". (stricmp vs _stricmp)

#include <afxwin.h>  // MFC core and standard components
#include <afxext.h>  // MFC extensions
#include <afxdisp.h> // MFC Automation classes

#include <afxdtctl.h> // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h> // MFC support for Windows Common Controls
#endif				// _AFX_NO_AFXCMN_SUPPORT

//#include <gdiplus.h>
//#include <GdiPlusGraphics.h>

// This macro is the same as IMPLEMENT_OLECREATE, except it passes TRUE
// for the bMultiInstance parameter to the COleObjectFactory constructor.
// We want a separate instance of this application to be launched for
// each automation proxy object requested by automation controllers.
#ifndef IMPLEMENT_OLECREATE2
#define IMPLEMENT_OLECREATE2(                                                  \
  class_name, external_name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)        \
	AFX_DATADEF COleObjectFactory class_name::factory(                         \
	  class_name::guid, RUNTIME_CLASS(class_name), TRUE, _T(external_name));   \
	const AFX_DATADEF GUID class_name::guid = {                                \
		l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 }                          \
	};
#endif // IMPLEMENT_OLECREATE2

