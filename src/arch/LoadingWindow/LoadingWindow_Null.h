#ifndef LOADING_WINDOW_NULL_H
#define LOADING_WINDOW_NULL_H

#include "LoadingWindow.h"

class LoadingWindow_Null : public LoadingWindow
{
  public:
	void SetText(const std::string& str) {}
	void SetSplash(const RageSurface* pSplash) {}
};
#define USE_LOADING_WINDOW_NULL

#endif
