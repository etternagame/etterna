#include "Etterna/Globals/global.h"
#include "LowLevelWindow_Win32.h"
#include "archutils/Win32/DirectXHelpers.h"
#include "archutils/Win32/ErrorStrings.h"
#include "archutils/Win32/GraphicsWindow.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Graphics/RageDisplay.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "RageUtil/Graphics/Display/OpenGL/RageDisplay_OGL_Helpers.h"
#include "RageUtil/Graphics/Display/OpenGL/RageDisplay_OGL.h"

#include <GL/glew.h>

static PIXELFORMATDESCRIPTOR g_CurrentPixelFormat;
static HGLRC g_HGLRC = nullptr;
static HGLRC g_HGLRC_Background = nullptr;

static void
DestroyGraphicsWindowAndOpenGLContext()
{
	if (g_HGLRC != nullptr) {
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(g_HGLRC);
		g_HGLRC = nullptr;
	}

	if (g_HGLRC_Background != nullptr) {
		wglDeleteContext(g_HGLRC_Background);
		g_HGLRC_Background = nullptr;
	}

	ZERO(g_CurrentPixelFormat);

	GraphicsWindow::DestroyGraphicsWindow();
}

void*
LowLevelWindow_Win32::GetProcAddress(const std::string& s)
{
	void* pRet = (void*)wglGetProcAddress(s.c_str());
	if (pRet != nullptr)
		return pRet;

	return (void*)::GetProcAddress(GetModuleHandle(nullptr), s.c_str());
}

LowLevelWindow_Win32::LowLevelWindow_Win32()
{
	ASSERT(g_HGLRC == NULL);
	ASSERT(g_HGLRC_Background == NULL);

	GraphicsWindow::Initialize(false);
}

LowLevelWindow_Win32::~LowLevelWindow_Win32()
{
	DestroyGraphicsWindowAndOpenGLContext();
	GraphicsWindow::Shutdown();
}

void
LowLevelWindow_Win32::GetDisplaySpecs(DisplaySpecs& out) const
{
	GraphicsWindow::GetDisplaySpecs(out);
}

int
ChooseWindowPixelFormat(const VideoModeParams& p, PIXELFORMATDESCRIPTOR* pixfmt)
{
	ASSERT(GraphicsWindow::GetHwnd() != NULL);
	ASSERT(GraphicsWindow::GetHDC() != NULL);

	ZERO(*pixfmt);
	pixfmt->nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixfmt->nVersion = 1;
	pixfmt->dwFlags =
	  PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL;
	pixfmt->iPixelType = PFD_TYPE_RGBA;
	pixfmt->cColorBits = p.bpp == 16 ? 16 : 24;
	pixfmt->cDepthBits = 16;

	return ChoosePixelFormat(GraphicsWindow::GetHDC(), pixfmt);
}

void
DumpPixelFormat(const PIXELFORMATDESCRIPTOR& pfd)
{
	std::string str = ssprintf("Mode: ");
	bool bInvalidFormat = false;

	if (pfd.dwFlags & PFD_GENERIC_FORMAT) {
		if (pfd.dwFlags & PFD_GENERIC_ACCELERATED)
			str += "MCD ";
		else {
			str += "software ";
			bInvalidFormat = true;
		}
	} else {
		str += "ICD ";
	}

	if (pfd.iPixelType != PFD_TYPE_RGBA) {
		str += "indexed ";
		bInvalidFormat = true;
	}
	if (!(pfd.dwFlags & PFD_SUPPORT_OPENGL)) {
		str += "!OPENGL ";
		bInvalidFormat = true;
	}
	if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW)) {
		str += "!window ";
		bInvalidFormat = true;
	}
	if (!(pfd.dwFlags & PFD_DOUBLEBUFFER)) {
		str += "!dbuff ";
		bInvalidFormat = true;
	}

	str += ssprintf("%i (%i%i%i) ",
					pfd.cColorBits,
					pfd.cRedBits,
					pfd.cGreenBits,
					pfd.cBlueBits);
	if (pfd.cAlphaBits)
		str += ssprintf("%i alpha ", pfd.cAlphaBits);
	if (pfd.cDepthBits)
		str += ssprintf("%i depth ", pfd.cDepthBits);
	if (pfd.cStencilBits)
		str += ssprintf("%i stencil ", pfd.cStencilBits);
	if (pfd.cAccumBits)
		str += ssprintf("%i accum ", pfd.cAccumBits);

	if (bInvalidFormat)
		Locator::getLogger()->warn("Invalid format: {}", str.c_str());
	else if (PREFSMAN->m_verbose_log > 1)
		Locator::getLogger()->info(str);
}

/* This function does not reset the video mode if it fails, because we might be
 * trying yet another video mode, so we'd just thrash the display.  On fatal
 * error, LowLevelWindow_Win32::~LowLevelWindow_Win32 will call
 * GraphicsWindow::Shutdown(). */
std::string
LowLevelWindow_Win32::TryVideoMode(const VideoModeParams& p,
								   bool& bNewDeviceOut)
{
	// Locator::getLogger()->warn( "LowLevelWindow_Win32::TryVideoMode" );

	ASSERT_M(p.bpp == 16 || p.bpp == 32, ssprintf("%i", p.bpp));

	bNewDeviceOut = false;

	/* We're only allowed to change the pixel format of a window exactly once.
	 */
	bool bCanSetPixelFormat = true;

	/* Do we have an old window? */
	if (GraphicsWindow::GetHwnd() == nullptr) {
		/* No.  Always create and show the window before changing the video
		 * mode. Otherwise, some other window may have focus, and changing the
		 * video mode will cause that window to be resized. */
		bNewDeviceOut = true;
		GraphicsWindow::CreateGraphicsWindow(p);
	} else {
		/* We already have a window.  Assume that its pixel format has already
		 * been set. */
		bCanSetPixelFormat = false;
	}

	ASSERT(GraphicsWindow::GetHwnd() != NULL);

	/* Set the display mode: switch to a fullscreen mode or revert to windowed
	 * mode. */
	if (PREFSMAN->m_verbose_log > 1)
		Locator::getLogger()->trace("SetScreenMode ...");
	std::string sErr = GraphicsWindow::SetScreenMode(p);
	if (!sErr.empty())
		return sErr;

	PIXELFORMATDESCRIPTOR pixfmt;
	int iPixelFormat = ChooseWindowPixelFormat(p, &pixfmt);
	if (iPixelFormat == 0) {
		/* Destroy the window. */
		DestroyGraphicsWindowAndOpenGLContext();
		return "Pixel format not found";
	}

	bool bNeedToSetPixelFormat = false;
	{
		/* We'll need to recreate it if the pixel format is going to change.  We
		 * aren't allowed to change the pixel format twice. */
		PIXELFORMATDESCRIPTOR DestPixelFormat;
		ZERO(DestPixelFormat);
		DescribePixelFormat(GraphicsWindow::GetHDC(),
							iPixelFormat,
							sizeof(PIXELFORMATDESCRIPTOR),
							&DestPixelFormat);
		if (memcmp(&DestPixelFormat,
				   &g_CurrentPixelFormat,
				   sizeof(PIXELFORMATDESCRIPTOR))) {
			if (PREFSMAN->m_verbose_log > 1)
				Locator::getLogger()->trace("Reset: pixel format changing");
			bNeedToSetPixelFormat = true;
		}
	}

	if (bNeedToSetPixelFormat && !bCanSetPixelFormat) {
		/*
		 * The screen mode has changed, so we need to set the pixel format.  If
		 * we're not allowed to do so, destroy the window and make a new one.
		 *
		 * For some reason, if we destroy the old window before creating the new
		 * one, the "maximized apps go under the taskbar" glitch will happen
		 * when we quit. We have to create the new window first.
		 */
		Locator::getLogger()->trace("Mode requires new pixel format, and we've already set one; "
				   "resetting OpenGL context");
		if (g_HGLRC != nullptr) {
			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(g_HGLRC);
			g_HGLRC = nullptr;
			wglDeleteContext(g_HGLRC_Background);
			g_HGLRC_Background = nullptr;
		}

		bNewDeviceOut = true;
	}

	/* If we deleted the OpenGL context above, also recreate the window.
	 * Otherwise, just reconfigure it. */
	GraphicsWindow::CreateGraphicsWindow(p, bNewDeviceOut);

	if (bNeedToSetPixelFormat) {
		/* Set the pixel format. */
		if (!SetPixelFormat(GraphicsWindow::GetHDC(), iPixelFormat, &pixfmt)) {
			/* Destroy the window. */
			DestroyGraphicsWindowAndOpenGLContext();

			return werr_ssprintf(GetLastError(), "Pixel format failed");
		}

		DescribePixelFormat(GraphicsWindow::GetHDC(),
							iPixelFormat,
							sizeof(g_CurrentPixelFormat),
							&g_CurrentPixelFormat);

		DumpPixelFormat(g_CurrentPixelFormat);
	}

	if (g_HGLRC == nullptr) {
		g_HGLRC = wglCreateContext(GraphicsWindow::GetHDC());
		if (g_HGLRC == nullptr) {
			DestroyGraphicsWindowAndOpenGLContext();
			return hr_ssprintf(GetLastError(), "wglCreateContext");
		}

		g_HGLRC_Background = wglCreateContext(GraphicsWindow::GetHDC());
		if (g_HGLRC_Background == nullptr) {
			DestroyGraphicsWindowAndOpenGLContext();
			return hr_ssprintf(GetLastError(), "wglCreateContext");
		}

		if (!wglShareLists(g_HGLRC, g_HGLRC_Background)) {
			Locator::getLogger()->warn(werr_ssprintf(GetLastError(), "wglShareLists failed"));
			wglDeleteContext(g_HGLRC_Background);
			g_HGLRC_Background = nullptr;
		}

		if (!wglMakeCurrent(GraphicsWindow::GetHDC(), g_HGLRC)) {
			DestroyGraphicsWindowAndOpenGLContext();
			return hr_ssprintf(GetLastError(), "wglCreateContext");
		}
	}
	return std::string(); // we set the video mode successfully
}

static LocalizedString OPENGL_NOT_AVAILABLE(
  "LowLevelWindow_Win32",
  "OpenGL hardware acceleration is not available.");
bool
LowLevelWindow_Win32::IsSoftwareRenderer(std::string& sError)
{
	std::string sVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	std::string sRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

	if (sVendor == "Microsoft Corporation" && sRenderer == "GDI Generic") {
		sError = OPENGL_NOT_AVAILABLE;
		return true;
	}

	return false;
}

void
LowLevelWindow_Win32::SwapBuffers()
{
	::SwapBuffers(GraphicsWindow::GetHDC());
}

void
LowLevelWindow_Win32::Update()
{
	GraphicsWindow::Update();
}

const ActualVideoModeParams*
LowLevelWindow_Win32::GetActualVideoModeParams() const
{
	return static_cast<ActualVideoModeParams*>(GraphicsWindow::GetParams());
}

class RenderTarget_Win32 : public RenderTarget
{
  public:
	RenderTarget_Win32(LowLevelWindow_Win32* pWind);
	virtual ~RenderTarget_Win32();

	void Create(const RenderTargetParam& param,
				int& iTextureWidthOut,
				int& iTextureHeightOut);
	unsigned int GetTexture() const { return m_texHandle; }
	void StartRenderingTo();
	void FinishRenderingTo();

	virtual bool InvertY() const { return true; }

  private:
	LowLevelWindow_Win32* m_pWind;
	int m_width;
	int m_height;
	GLuint m_texHandle;
	HDC m_hOldDeviceContext;
	HGLRC m_hOldRenderContext;
};

RenderTarget_Win32::RenderTarget_Win32(LowLevelWindow_Win32* pWind)
{
	m_pWind = pWind;
	m_texHandle = 0;
	m_hOldDeviceContext = nullptr;
	m_hOldRenderContext = nullptr;
}

RenderTarget_Win32::~RenderTarget_Win32()
{
	glDeleteTextures(1,
					 &m_texHandle); // deleting a 0 texture is safe and ignored
}

void
RenderTarget_Win32::Create(const RenderTargetParam& param,
						   int& iTextureWidthOut,
						   int& iTextureHeightOut)
{
	m_Param = param;
	m_width = param.iWidth;
	m_height = param.iHeight;

	FlushGLErrors();

	glGenTextures(1, &m_texHandle);
	ASSERT(m_texHandle > 0);
	glBindTexture(GL_TEXTURE_2D, m_texHandle);

	int iTextureWidth = power_of_two(param.iWidth);
	int iTextureHeight = power_of_two(param.iHeight);
	iTextureWidthOut = iTextureWidth;
	iTextureHeightOut = iTextureHeight;

	GLenum internalformat;
	GLenum type = param.bWithAlpha ? GL_RGBA : GL_RGB;
	if (param.bFloat && GLEW_ARB_texture_float)
		internalformat = param.bWithAlpha ? GL_RGBA16F_ARB : GL_RGB16F_ARB;
	else
		internalformat = param.bWithAlpha ? GL_RGBA8 : GL_RGB8;

	glTexImage2D(GL_TEXTURE_2D,
				 0,
				 internalformat,
				 iTextureWidth,
				 iTextureHeight,
				 0,
				 type,
				 GL_UNSIGNED_BYTE,
				 nullptr);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	AssertNoGLError();
}

void
RenderTarget_Win32::StartRenderingTo()
{
	m_hOldDeviceContext = wglGetCurrentDC();
	m_hOldRenderContext = wglGetCurrentContext();

	BOOL successful = wglMakeCurrent(GraphicsWindow::GetHDC(), g_HGLRC);
	ASSERT_M(successful == TRUE,
			 "wglMakeCurrent failed in RenderTarget_Win32::StartRenderingTo()");

	FlushGLErrors();
	glBindTexture(GL_TEXTURE_2D, m_texHandle);
	AssertNoGLError();
}

void
RenderTarget_Win32::FinishRenderingTo()
{
	FlushGLErrors();

	glBindTexture(GL_TEXTURE_2D, m_texHandle);
	AssertNoGLError();

	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_width, m_height);

	glBindTexture(GL_TEXTURE_2D, 0);

	AssertNoGLError();

	BOOL successful = wglMakeCurrent(m_hOldDeviceContext, m_hOldRenderContext);
	ASSERT_M(
	  successful == TRUE,
	  "wglMakeCurrent failed in RenderTarget_Win32::FinishRenderingTo()");

	m_hOldDeviceContext = nullptr;
	m_hOldRenderContext = nullptr;
}

RenderTarget*
LowLevelWindow_Win32::CreateRenderTarget()
{
	return new RenderTarget_Win32(this);
}

/*
 * (c) 2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
