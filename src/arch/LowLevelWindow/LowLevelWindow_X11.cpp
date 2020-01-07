#include "Etterna/Globals/global.h"
#include "LowLevelWindow_X11.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Misc/RageException.h"
#include "archutils/Unix/X11Helper.h"
#include "Etterna/Singletons/PrefsManager.h" // XXX
#include "RageUtil/Graphics/RageDisplay.h"   // VideoModeParams
#include "Etterna/Models/Misc/DisplayResolutions.h"
#include "Etterna/Models/Misc/LocalizedString.h"

#include "RageUtil/Graphics/RageDisplay_OGL_Helpers.h"
using namespace RageDisplay_Legacy_Helpers;
using namespace X11Helper;

#include <stack>
#include <math.h> // ceil()
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h> // All sorts of stuff...
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#if defined(HAVE_LIBXTST)
#include <X11/extensions/XTest.h>
#endif

static GLXContext g_pContext = NULL;
static GLXContext g_pBackgroundContext = NULL;
static Window g_AltWindow = None;
static Rotation g_OldRotation;
static int g_iOldSize;
XRRScreenConfiguration* g_pScreenConfig = NULL;

static LocalizedString FAILED_CONNECTION_XSERVER(
  "LowLevelWindow_X11",
  "Failed to establish a connection with the X server");
LowLevelWindow_X11::LowLevelWindow_X11()
{
	if (!OpenXConnection())
		RageException::Throw("%s",
							 FAILED_CONNECTION_XSERVER.GetValue().c_str());

	const int iScreen = DefaultScreen(Dpy);
	int iXServerVersion = XVendorRelease(Dpy); /* eg. 40201001 */
	int iMajor = iXServerVersion / 10000000;
	iXServerVersion %= 10000000;
	int iMinor = iXServerVersion / 100000;
	iXServerVersion %= 100000;
	int iRevision = iXServerVersion / 1000;
	iXServerVersion %= 1000;
	int iPatch = iXServerVersion;

	LOG->Info("Display: %s (screen %i)", DisplayString(Dpy), iScreen);
	LOG->Info("X server vendor: %s [%i.%i.%i.%i]",
			  XServerVendor(Dpy),
			  iMajor,
			  iMinor,
			  iRevision,
			  iPatch);
	LOG->Info("Server GLX vendor: %s [%s]",
			  glXQueryServerString(Dpy, iScreen, GLX_VENDOR),
			  glXQueryServerString(Dpy, iScreen, GLX_VERSION));
	LOG->Info("Client GLX vendor: %s [%s]",
			  glXGetClientString(Dpy, GLX_VENDOR),
			  glXGetClientString(Dpy, GLX_VERSION));
	m_bWasWindowed = true;
	g_pScreenConfig =
	  XRRGetScreenInfo(Dpy, RootWindow(Dpy, DefaultScreen(Dpy)));
	g_iOldSize = XRRConfigCurrentConfiguration(g_pScreenConfig, &g_OldRotation);
}

LowLevelWindow_X11::~LowLevelWindow_X11()
{
	// Reset the display
	if (!m_bWasWindowed) {
		XRRSetScreenConfig(Dpy,
						   g_pScreenConfig,
						   RootWindow(Dpy, DefaultScreen(Dpy)),
						   g_iOldSize,
						   g_OldRotation,
						   CurrentTime);

		XUngrabKeyboard(Dpy, CurrentTime);
	}
	if (g_pContext) {
		glXDestroyContext(Dpy, g_pContext);
		g_pContext = NULL;
	}
	if (g_pBackgroundContext) {
		glXDestroyContext(Dpy, g_pBackgroundContext);
		g_pBackgroundContext = NULL;
	}
	XRRFreeScreenConfigInfo(g_pScreenConfig);
	g_pScreenConfig = NULL;

	XDestroyWindow(Dpy, Win);
	Win = None;
	XDestroyWindow(Dpy, g_AltWindow);
	g_AltWindow = None;
	CloseXConnection();
}

void*
LowLevelWindow_X11::GetProcAddress(const RString& s)
{
	// XXX: We should check whether glXGetProcAddress or
	// glXGetProcAddressARB is available/not NULL, and go by that,
	// instead of assuming like this.
	return (void*)glXGetProcAddressARB((const GLubyte*)s.c_str());
}

RString
LowLevelWindow_X11::TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut)
{
#ifdef __unix__
	/* nVidia cards:
	 * This only works the first time we set up a window; after that, the
	 * drivers appear to cache the value, so you have to actually restart
	 * the program to change it again. */
	static char buf[128];
	strcpy(buf, "__GL_SYNC_TO_VBLANK=");
	strcat(buf, p.vsync ? "1" : "0");
	putenv(buf);
#endif

	if (g_pContext == NULL || p.bpp != CurrentParams.bpp ||
		m_bWasWindowed != p.windowed) {
		// Different depth, or we didn't make a window before. New context.
		bNewDeviceOut = true;

		int visAttribs[32];
		int i = 0;
		ASSERT(p.bpp == 16 || p.bpp == 32);

		if (p.bpp == 32) {
			visAttribs[i++] = GLX_RED_SIZE;
			visAttribs[i++] = 8;
			visAttribs[i++] = GLX_GREEN_SIZE;
			visAttribs[i++] = 8;
			visAttribs[i++] = GLX_BLUE_SIZE;
			visAttribs[i++] = 8;
		} else {
			visAttribs[i++] = GLX_RED_SIZE;
			visAttribs[i++] = 5;
			visAttribs[i++] = GLX_GREEN_SIZE;
			visAttribs[i++] = 6;
			visAttribs[i++] = GLX_BLUE_SIZE;
			visAttribs[i++] = 5;
		}

		visAttribs[i++] = GLX_DEPTH_SIZE;
		visAttribs[i++] = 16;
		visAttribs[i++] = GLX_RGBA;
		visAttribs[i++] = GLX_DOUBLEBUFFER;

		visAttribs[i++] = None;

		XVisualInfo* xvi = glXChooseVisual(Dpy, DefaultScreen(Dpy), visAttribs);
		if (xvi == NULL)
			return "No visual available for that depth.";

		// I get strange behavior if I add override redirect after creating the
		// window. So, let's recreate the window when changing that state.
		if (!MakeWindow(Win,
						xvi->screen,
						xvi->depth,
						xvi->visual,
						p.width,
						p.height,
						!p.windowed))
			return "Failed to create the window.";

		if (!MakeWindow(g_AltWindow,
						xvi->screen,
						xvi->depth,
						xvi->visual,
						p.width,
						p.height,
						!p.windowed))
			FAIL_M("Failed to create the alt window."); // Should this be fatal?

		char* szWindowTitle = const_cast<char*>(p.sWindowTitle.c_str());
		XChangeProperty(Dpy,
						Win,
						XA_WM_NAME,
						XA_STRING,
						8,
						PropModeReplace,
						reinterpret_cast<unsigned char*>(szWindowTitle),
						strlen(szWindowTitle));

		if (g_pContext)
			glXDestroyContext(Dpy, g_pContext);
		if (g_pBackgroundContext)
			glXDestroyContext(Dpy, g_pBackgroundContext);
		g_pContext = glXCreateContext(Dpy, xvi, NULL, True);
		g_pBackgroundContext = glXCreateContext(Dpy, xvi, g_pContext, True);

		glXMakeCurrent(Dpy, Win, g_pContext);

		// Map the window, ensuring we get the MapNotify event
		XWindowAttributes winAttrib;
		XGetWindowAttributes(Dpy, Win, &winAttrib);
		XSelectInput(Dpy, Win, winAttrib.your_event_mask | StructureNotifyMask);
		XMapWindow(Dpy, Win);

		// Wait until we actually have a mapped window before trying to
		// use it!
		XEvent event;
		do {
			XNextEvent(Dpy, &event);
		} while (event.type != MapNotify);

		// Set the event mask back to what it was
		XSelectInput(Dpy, Win, winAttrib.your_event_mask);
	} else {
		// We're remodeling the existing window, and not touching the context.
		bNewDeviceOut = false;
	}

	if (!p.windowed) {
		if (m_bWasWindowed) {
			// If the user changed the resolution while StepMania was windowed
			// we overwrite the resolution to restore with it at exit.
			g_iOldSize =
			  XRRConfigCurrentConfiguration(g_pScreenConfig, &g_OldRotation);
			m_bWasWindowed = false;
		}

		// Find a matching mode.
		int iSizesXct;
		XRRScreenSize* pSizesX = XRRSizes(Dpy, DefaultScreen(Dpy), &iSizesXct);
		ASSERT_M(iSizesXct != 0, "Couldn't get resolution list from X server");

		int iSizeMatch = -1;

		for (int i = 0; i < iSizesXct; ++i) {
			if (pSizesX[i].width == p.width && pSizesX[i].height == p.height) {
				iSizeMatch = i;
				break;
			}
		}

		// Set this mode.
		// XXX: This doesn't handle if the config has changed since we queried
		// it (see man Xrandr)
		XRRSetScreenConfig(Dpy,
						   g_pScreenConfig,
						   RootWindow(Dpy, DefaultScreen(Dpy)),
						   iSizeMatch,
						   1,
						   CurrentTime);

		XRaiseWindow(Dpy, Win);

		// We want to prevent the WM from catching anything that comes from the
		// keyboard. We should do this every time on fullscreen and not only we
		// entering from windowed mode because we could lose focus at resolution
		// change and that will leave the user input locked.
		XGrabKeyboard(
		  Dpy, Win, True, GrabModeAsync, GrabModeAsync, CurrentTime);
	} else {
		if (!m_bWasWindowed) {
			XRRSetScreenConfig(Dpy,
							   g_pScreenConfig,
							   RootWindow(Dpy, DefaultScreen(Dpy)),
							   g_iOldSize,
							   g_OldRotation,
							   CurrentTime);
			// In windowed mode, we actually want the WM to function normally.
			// Release any previous grab.
			XUngrabKeyboard(Dpy, CurrentTime);
			m_bWasWindowed = true;
		}
	}
	// NOTE: nVidia's implementation of this is broken by default.
	// The only ways around this are mucking with xorg.conf or querying
	// nvidia-settings with "$ nvidia-settings -t -q RefreshRate".
	int rate = XRRConfigCurrentRate(g_pScreenConfig);

	// Make a window fixed size, don't let resize it or maximize it.
	// Do this before resizing the window so that pane-style WMs (Ion,
	// ratpoison) don't resize us back inappropriately.
	{
		XSizeHints hints;

		hints.flags = PMinSize | PMaxSize | PWinGravity;
		hints.min_width = hints.max_width = p.width;
		hints.min_height = hints.max_height = p.height;
		hints.win_gravity = CenterGravity;

		XSetWMNormalHints(Dpy, Win, &hints);
	}

	/* Workaround for metacity and compiz: if the window have the same
	 * resolution or higher than the screen, it gets automaximized even
	 * when the window is set to not let it happen. This happens when
	 * changing from fullscreen to window mode and our screen resolution
	 * is bigger. */
	{
		XEvent xev;
		Atom wm_state = XInternAtom(Dpy, "_NET_WM_STATE", False);
		Atom maximized_vert =
		  XInternAtom(Dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
		Atom maximized_horz =
		  XInternAtom(Dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);

		memset(&xev, 0, sizeof(xev));
		xev.type = ClientMessage;
		xev.xclient.window = Win;
		xev.xclient.message_type = wm_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = 1;
		xev.xclient.data.l[1] = maximized_vert;
		xev.xclient.data.l[2] = 0;

		XSendEvent(
		  Dpy, DefaultRootWindow(Dpy), False, SubstructureNotifyMask, &xev);
		xev.xclient.data.l[1] = maximized_horz;
		XSendEvent(
		  Dpy, DefaultRootWindow(Dpy), False, SubstructureNotifyMask, &xev);

		// This one is needed for compiz, if the window reaches out of bounds of
		// the screen it becames destroyed, only the window, the program is left
		// running. Commented out per the patch at
		// http://ssc.ajworld.net/sm-ssc/bugtracker/view.php?id=398
		// XMoveWindow( Dpy, Win, 0, 0 );
	}

	// Resize the window.
	XResizeWindow(Dpy, Win, p.width, p.height);

	CurrentParams = p;
	CurrentParams.rate = rate;
	return ""; // Success
}

void
LowLevelWindow_X11::LogDebugInformation() const
{
	LOG->Info("Direct rendering: %s",
			  glXIsDirect(Dpy, glXGetCurrentContext()) ? "yes" : "no");
}

bool
LowLevelWindow_X11::IsSoftwareRenderer(RString& sError)
{
	if (glXIsDirect(Dpy, glXGetCurrentContext()))
		return false;

	sError = "Direct rendering is not available.";
	return true;
}

void
LowLevelWindow_X11::SwapBuffers()
{
	glXSwapBuffers(Dpy, Win);

	if (true) {
		// Disable the screensaver.
#if defined(HAVE_LIBXTST)
		// This causes flicker.
		// XForceScreenSaver( Dpy, ScreenSaverReset );

		/* Instead, send a null relative mouse motion, to trick X into thinking
		 * there has been user activity.
		 *
		 * This also handles XScreenSaver; XForceScreenSaver only handles the
		 * internal X11 screen blanker.
		 *
		 * This will delay the X blanker, DPMS and XScreenSaver from activating,
		 * and will disable the blanker and XScreenSaver if they're already
		 * active (unless XSS is locked). For some reason, it doesn't un-blank
		 * DPMS if it's already active.
		 */

		XLockDisplay(Dpy);

		int event_base, error_base, major, minor;
		if (XTestQueryExtension(
			  Dpy, &event_base, &error_base, &major, &minor)) {
			XTestFakeRelativeMotionEvent(Dpy, 0, 0, 0);
			XSync(Dpy, False);
		}

		XUnlockDisplay(Dpy);
#endif
	}
}

void
LowLevelWindow_X11::GetDisplayResolutions(DisplayResolutions& out) const
{
	int iSizesXct;
	XRRScreenSize* pSizesX = XRRSizes(Dpy, DefaultScreen(Dpy), &iSizesXct);
	ASSERT_M(iSizesXct != 0, "Couldn't get resolution list from X server");

	for (int i = 0; i < iSizesXct; ++i) {
		DisplayResolution res = { (unsigned)pSizesX[i].width,
								  (unsigned)pSizesX[i].height,
								  true };
		out.insert(res);
	}
}

bool
LowLevelWindow_X11::SupportsThreadedRendering()
{
	return g_pBackgroundContext != NULL;
}

class RenderTarget_X11 : public RenderTarget
{
  public:
	RenderTarget_X11(LowLevelWindow_X11* pWind);
	~RenderTarget_X11();

	void Create(const RenderTargetParam& param,
				int& iTextureWidthOut,
				int& iTextureHeightOut);
	unsigned GetTexture() const { return m_iTexHandle; }
	void StartRenderingTo();
	void FinishRenderingTo();

	// Copying from the Pbuffer to the texture flips Y.
	virtual bool InvertY() const { return true; }

  private:
	int m_iWidth, m_iHeight;
	LowLevelWindow_X11* m_pWind;
	GLXPbuffer m_iPbuffer;
	GLXContext m_pPbufferContext;
	unsigned int m_iTexHandle;

	GLXContext m_pOldContext;
	GLXDrawable m_pOldDrawable;
};

RenderTarget_X11::RenderTarget_X11(LowLevelWindow_X11* pWind)
{
	m_pWind = pWind;
	m_iPbuffer = 0;
	m_pPbufferContext = NULL;
	m_iTexHandle = 0;
	m_pOldContext = NULL;
	m_pOldDrawable = 0;
}

RenderTarget_X11::~RenderTarget_X11()
{
	if (m_pPbufferContext)
		glXDestroyContext(Dpy, m_pPbufferContext);
	if (m_iPbuffer)
		glXDestroyPbuffer(Dpy, m_iPbuffer);
	if (m_iTexHandle)
		glDeleteTextures(1, reinterpret_cast<GLuint*>(&m_iTexHandle));
}

/* Note that although the texture size may need to be a power of 2,
 * the Pbuffer does not. */
void
RenderTarget_X11::Create(const RenderTargetParam& param,
						 int& iTextureWidthOut,
						 int& iTextureHeightOut)
{
	// ASSERT( param.iWidth == power_of_two(param.iWidth) && param.iHeight ==
	// power_of_two(param.iHeight) );

	m_iWidth = param.iWidth;
	m_iHeight = param.iHeight;

	/* NOTE: int casts on GLX_DONT_CARE are for -Werror=narrowing */
	int pConfigAttribs[] = { GLX_DRAWABLE_TYPE,
							 GLX_PBUFFER_BIT,
							 GLX_RENDER_TYPE,
							 GLX_RGBA_BIT,

							 GLX_RED_SIZE,
							 8,
							 GLX_GREEN_SIZE,
							 8,
							 GLX_BLUE_SIZE,
							 8,
							 GLX_ALPHA_SIZE,
							 param.bWithAlpha ? 8 : (int)GLX_DONT_CARE,

							 GLX_DOUBLEBUFFER,
							 False,
							 GLX_DEPTH_SIZE,
							 param.bWithDepthBuffer ? 16 : (int)GLX_DONT_CARE,
							 None };
	int iConfigs;
	GLXFBConfig* pConfigs =
	  glXChooseFBConfig(Dpy, DefaultScreen(Dpy), pConfigAttribs, &iConfigs);
	ASSERT(pConfigs);

	const int pPbufferAttribs[] = {
		GLX_PBUFFER_WIDTH, param.iWidth, GLX_PBUFFER_HEIGHT, param.iHeight, None
	};

	for (int i = 0; i < iConfigs; ++i) {
		m_iPbuffer = glXCreatePbuffer(Dpy, pConfigs[i], pPbufferAttribs);
		if (m_iPbuffer == 0)
			continue;

		XVisualInfo* pVisual = glXGetVisualFromFBConfig(Dpy, pConfigs[i]);
		m_pPbufferContext = glXCreateContext(Dpy, pVisual, g_pContext, True);
		ASSERT(m_pPbufferContext);
		XFree(pVisual);
		break;
	}

	ASSERT(m_iPbuffer);

	// allocate OpenGL texture resource
	glGenTextures(1, reinterpret_cast<GLuint*>(&m_iTexHandle));
	glBindTexture(GL_TEXTURE_2D, m_iTexHandle);

	LOG->Trace("n %i, %ix%i", m_iTexHandle, param.iWidth, param.iHeight);
	while (glGetError() != GL_NO_ERROR)
		;

	int iTextureWidth = power_of_two(param.iWidth);
	int iTextureHeight = power_of_two(param.iHeight);
	iTextureWidthOut = iTextureWidth;
	iTextureHeightOut = iTextureHeight;

	glTexImage2D(GL_TEXTURE_2D,
				 0,
				 param.bWithAlpha ? GL_RGBA8 : GL_RGB8,
				 iTextureWidth,
				 iTextureHeight,
				 0,
				 param.bWithAlpha ? GL_RGBA : GL_RGB,
				 GL_UNSIGNED_BYTE,
				 NULL);
	GLenum error = glGetError();
	ASSERT_M(error == GL_NO_ERROR, GLToString(error));

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void
RenderTarget_X11::StartRenderingTo()
{
	m_pOldContext = glXGetCurrentContext();
	m_pOldDrawable = glXGetCurrentDrawable();
	glXMakeCurrent(Dpy, m_iPbuffer, m_pPbufferContext);

	glViewport(0, 0, m_iWidth, m_iHeight);
}

void
RenderTarget_X11::FinishRenderingTo()
{
	glFlush();

	glBindTexture(GL_TEXTURE_2D, m_iTexHandle);

	while (glGetError() != GL_NO_ERROR)
		;

	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_iWidth, m_iHeight);

	GLenum error = glGetError();
	ASSERT_M(error == GL_NO_ERROR, GLToString(error));

	glBindTexture(GL_TEXTURE_2D, 0);

	glXMakeCurrent(Dpy, m_pOldDrawable, m_pOldContext);
	m_pOldContext = NULL;
	m_pOldDrawable = 0;
}

bool
LowLevelWindow_X11::SupportsRenderToTexture() const
{
	// Server must support pbuffers:
	const int iScreen = DefaultScreen(Dpy);
	float fVersion =
	  strtof(glXQueryServerString(Dpy, iScreen, GLX_VERSION), NULL);
	if (fVersion < 1.3f)
		return false;

	return true;
}

RenderTarget*
LowLevelWindow_X11::CreateRenderTarget()
{
	return new RenderTarget_X11(this);
}

void
LowLevelWindow_X11::BeginConcurrentRenderingMainThread()
{
	/* Move the main thread, which is going to be loading textures, etc.
	 * but not rendering, to an undisplayed window. This results in
	 * smoother rendering. */
	bool b = glXMakeCurrent(Dpy, g_AltWindow, g_pContext);
	ASSERT(b);
}

void
LowLevelWindow_X11::EndConcurrentRenderingMainThread()
{
	bool b = glXMakeCurrent(Dpy, Win, g_pContext);
	ASSERT(b);
}

void
LowLevelWindow_X11::BeginConcurrentRendering()
{
	bool b = glXMakeCurrent(Dpy, Win, g_pBackgroundContext);
	ASSERT(b);
}

void
LowLevelWindow_X11::EndConcurrentRendering()
{
	bool b = glXMakeCurrent(Dpy, None, NULL);
	ASSERT(b);
}
