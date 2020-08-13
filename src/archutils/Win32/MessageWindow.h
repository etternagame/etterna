/* MessageWindow - simplifies creation of windows that exist only to receive
 * messages. */

#ifndef MESSAGE_WINDOW_H
#define MESSAGE_WINDOW_H

#include <windows.h>

class MessageWindow
{
  public:
	MessageWindow(const std::string& sClassName);
	~MessageWindow();

	/* Run the message loop until WM_QUIT is received. */
	void Run();

	HWND GetHwnd() { return m_hWnd; }

  protected:
	virtual bool HandleMessage(UINT /* msg */,
							   WPARAM /* wParam */,
							   LPARAM /* lParam */)
	{
		return false;
	}
	void StopRunning();

  private:
	static LRESULT CALLBACK WndProc(HWND hWnd,
									UINT msg,
									WPARAM wParam,
									LPARAM lParam);
	HWND m_hWnd;
	bool m_bDone;
};

#endif
