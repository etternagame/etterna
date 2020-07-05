#ifndef LOADING_WINDOW_MACOSX_H
#define LOADING_WINDOW_MACOSX_H

#include "LoadingWindow.h"
/** @brief Loading window for Mac OS X. */
class LoadingWindow_MacOSX : public LoadingWindow
{
  public:
	LoadingWindow_MacOSX();
	~LoadingWindow_MacOSX();
	void SetText(const std::string& str);
	void SetSplash(const RageSurface* pSplash);
	void SetProgress(const int progress);
	void SetTotalWork(const int totalWork);
	void SetIndeterminate(bool indeterminate);
};
#define USE_LOADING_WINDOW_MACOSX

#endif
