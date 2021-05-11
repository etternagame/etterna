#ifndef LOADING_WINDOW_MACOSX_H
#define LOADING_WINDOW_MACOSX_H

#include "LoadingWindow.h"
#include "Core/Platform/Window/GLFWWindowBackend.hpp"

#include <memory>

/** @brief Loading window for Mac OS X. */
class LoadingWindow_MacOSX : public LoadingWindow {
public:
	LoadingWindow_MacOSX();
	~LoadingWindow_MacOSX() override;
	void SetText(const std::string& str) override;
	void SetSplash(const RageSurface* pSplash) override {}
	void SetProgress(int progress) override;
	void SetTotalWork(int totalWork) override;
	void SetIndeterminate(bool indeterminate) override;

private:
    std::unique_ptr<Core::Platform::Window::GLFWWindowBackend> window;
};
#define USE_LOADING_WINDOW_MACOSX

#endif
