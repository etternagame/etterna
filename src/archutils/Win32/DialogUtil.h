#ifndef DialogUtil_H
#define DialogUtil_H

#include <windows.h>

namespace DialogUtil {
void
SetHeaderFont(HWND hdlg, int nID);
void
LocalizeDialogAndContents(HWND hdlg);
};

#endif
