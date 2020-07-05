/* LoadingWindow_Gtk - Loading window for GTK (usually X) */

#ifndef LOADING_WINDOW_GTK
#define LOADING_WINDOW_GTK

#include "LoadingWindow.h"

class LoadingWindow_Gtk : public LoadingWindow
{
  public:
	LoadingWindow_Gtk();
	std::string Init();
	~LoadingWindow_Gtk();
	void SetText(const std::string& str);
	void SetIcon(const RageSurface* pIcon);
	void SetSplash(const RageSurface* pSplash);
	void SetProgress(const int progress);
	void SetTotalWork(const int totalWork);
	void SetIndeterminate(bool indeterminate);
};
#define USE_LOADING_WINDOW_GTK

#endif
