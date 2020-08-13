/* WindowsFileIO - Windows device I/O. */

#ifndef WIN32_USB_H
#define WIN32_USB_H

#include <vector>
#include <windows.h>

class WindowsFileIO
{
  public:
	WindowsFileIO();
	~WindowsFileIO();
	bool Open(const std::string& sPath, int iBlockSize);
	bool IsOpen() const;

	/* Nonblocking read.  size must always be the same.  Returns the number of
	 * bytes read, or 0. */
	int read(void* p);
	static int read_several(const vector<WindowsFileIO*>& sources,
							void* p,
							int& actual,
							float timeout);

  private:
	void queue_read();
	int finish_read(void* p);

	HANDLE m_Handle;
	OVERLAPPED m_Overlapped;
	char* m_pBuffer;
	int m_iBlockSize;
};

/* WindowsFileIO - Windows USB I/O */
class USBDevice
{
  public:
	int GetPadEvent();
	bool Open(int iVID,
			  int iPID,
			  int iBlockSize,
			  int iNum,
			  void (*pfnInit)(HANDLE));
	bool IsOpen() const;

	WindowsFileIO m_IO;
};

#endif
