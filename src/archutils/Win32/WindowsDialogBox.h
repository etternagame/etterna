/* WindowsDialogBox - Simplifies the creation of modal Windows dialog boxes. */

#ifndef WINDOWS_DIALOG_BOX_H
#define WINDOWS_DIALOG_BOX_H

#include <windows.h>

class WindowsDialogBox
{
  public:
	WindowsDialogBox();
	virtual ~WindowsDialogBox() {}
	void Run(int iDialog);

	HWND GetHwnd() { return m_hWnd; }

  protected:
	virtual BOOL HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return false;
	}

  private:
	static INT_PTR APIENTRY DlgProc(HWND hDlg,
									UINT msg,
									WPARAM wParam,
									LPARAM lParam);
	HWND m_hWnd;
};

#endif
