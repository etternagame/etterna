#ifndef LOADING_WINDOW_H
#define LOADING_WINDOW_H

#include <Etterna/Globals/global.h>

struct RageSurface;
/** @brief Opens and displays the loading banner. */
class LoadingWindow
{
  public:
	static LoadingWindow* Create();

	virtual RString Init() { return RString(); }
	virtual ~LoadingWindow() = default;

	virtual void SetText(const RString& str) = 0;
	virtual void SetIcon(const RageSurface* pIcon) {}
	virtual void SetSplash(const RageSurface* pSplash) {}
	virtual void SetProgress(const int progress) { m_progress = progress; }
	virtual void SetTotalWork(const int totalWork) { m_totalWork = totalWork; }
	virtual void SetIndeterminate(bool indeterminate)
	{
		m_indeterminate = indeterminate;
	}

  protected:
	int m_progress;
	int m_totalWork;
	bool m_indeterminate;
};

#endif
