#include "LowLevelWindowVK_X11.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Graphics/RageDisplay.h"
#include "archutils/Unix/X11Helper.h"
#include "Etterna/Models/Misc/DisplaySpec.h"
#include "Etterna/Globals/GameLoop.h"
#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cstring>
#include <set>
#include <cmath>
#include <exception>
#include <assert.h>

using namespace X11Helper;

const std::string ID_XSCREEN = "XSCREEN_RANDR";

static std::string FAILED_CONNECTION_XSERVER(
  "LowLevelWindowVK_X11: "
  "Failed to establish a connection with the X server");

bool
LowLevelWindowVK_X11::NetWMSupported(Display* Dpy, Atom feature) const
{
	Atom net_supported = XInternAtom(Dpy, "_NET_SUPPORTED", False);
	Atom actual_type_return = BadAtom;
	int actual_format_return = 0;
	unsigned long nitems_return = 0;
	unsigned long bytes_after_return = 0;
	Atom* prop_return;
	Status status =
	  XGetWindowProperty(Dpy,
						 RootWindow(Dpy, DefaultScreen(Dpy)),
						 net_supported,
						 0,
						 8192,
						 False,
						 XA_ATOM,
						 &actual_type_return,
						 &actual_format_return,
						 &nitems_return,
						 &bytes_after_return,
						 reinterpret_cast<unsigned char**>(&prop_return));
	if (status != Success) {
		return false;
	}

	auto supported =
	  std::find(prop_return, prop_return + nitems_return, feature) !=
	  prop_return + nitems_return;
	XFree(prop_return);
	return supported;
}

inline float
calcRandRRefresh(unsigned long iPixelClock, int iHTotal, int iVTotal)
{
	return (iPixelClock) / (iHTotal * iVTotal);
}

LowLevelWindowVK_X11::LowLevelWindowVK_X11()
{
	if (!OpenXConnection())
		throw std::runtime_error(FAILED_CONNECTION_XSERVER);

	if (XRRQueryVersion(Dpy, &m_iRandRVerMajor, &m_iRandRVerMinor) &&
		m_iRandRVerMajor >= 1 && m_iRandRVerMinor >= 2)
		m_bUseXRandR12 = true;

	const int iScreen = DefaultScreen(Dpy);
	Locator::getLogger()->info(
	  "Display: {} (screen {})", DisplayString(Dpy), iScreen);
	int iXServerVersion = XVendorRelease(Dpy);
	int iMajor = iXServerVersion / 10000000;
	iXServerVersion %= 10000000;
	int iMinor = iXServerVersion / 100000;
	iXServerVersion %= 100000;
	int iRevision = iXServerVersion / 1000;
	iXServerVersion %= 1000;
	int iPatch = iXServerVersion;
	Locator::getLogger()->info("X server vendor: {} [{}.{}.{}.{}]",
							   XServerVendor(Dpy),
							   iMajor,
							   iMinor,
							   iRevision,
							   iPatch);

	m_bWasWindowed = true;
	m_pScreenConfig =
	  XRRGetScreenInfo(Dpy, RootWindow(Dpy, DefaultScreen(Dpy)));
}

LowLevelWindowVK_X11::~LowLevelWindowVK_X11()
{
	if (!m_bWasWindowed) {
		if (m_bChangedScreenSize) {
			XRRSetScreenConfig(Dpy,
							   m_pScreenConfig,
							   RootWindow(Dpy, DefaultScreen(Dpy)),
							   m_iOldSize,
							   m_OldRotation,
							   CurrentTime);
		}
		if (m_usedCrtc != None) {
			XRRScreenResources* res = XRRGetScreenResources(Dpy, Win);
			XRRCrtcInfo* conf = XRRGetCrtcInfo(Dpy, res, m_usedCrtc);
			XRRSetCrtcConfig(Dpy,
							 res,
							 m_usedCrtc,
							 conf->timestamp,
							 conf->x,
							 conf->y,
							 m_originalRandRMode,
							 conf->rotation,
							 conf->outputs,
							 conf->noutput);
			XRRFreeScreenResources(res);
			XRRFreeCrtcInfo(conf);
		}
		XUngrabKeyboard(Dpy, CurrentTime);
	}

	if (Win != None) {
		XDestroyWindow(Dpy, Win);
		Win = None;
	}
	CloseXConnection();
}

void
LowLevelWindowVK_X11::RestoreOutputConfig()
{
	if (m_bChangedScreenSize) {
		XRRSetScreenConfig(Dpy,
						   m_pScreenConfig,
						   RootWindow(Dpy, DefaultScreen(Dpy)),
						   m_iOldSize,
						   m_OldRotation,
						   CurrentTime);
	}
	if (m_usedCrtc != None) {
		assert(m_bUseXRandR12);
		XRRScreenResources* res = XRRGetScreenResources(Dpy, Win);
		XRRCrtcInfo* conf = XRRGetCrtcInfo(Dpy, res, m_usedCrtc);
		XRRSetCrtcConfig(Dpy,
						 res,
						 m_usedCrtc,
						 conf->timestamp,
						 conf->x,
						 conf->y,
						 m_originalRandRMode,
						 conf->rotation,
						 conf->outputs,
						 conf->noutput);
		XRRFreeScreenResources(res);
		XRRFreeCrtcInfo(conf);
	}
	m_iOldSize = None;
	m_bChangedScreenSize = false;
	m_usedCrtc = None;
	m_OldRotation = RR_Rotate_0;
}

std::string
LowLevelWindowVK_X11::TryVideoMode(const VideoModeParams& p,
								   bool& bNewDeviceOut)
{
	// We're going to be interested in MapNotify/ConfigureNotify events in this
	// routine, so ensure our event mask includes these, restore it on exit
	XWindowAttributes winAttrib;
	auto restore = [&](XWindowAttributes* attr) {
		XSelectInput(Dpy, Win, attr->your_event_mask);
	};
	auto restoreAttrib = std::unique_ptr<XWindowAttributes, decltype(restore)>(
	  &winAttrib, restore);

	// These might change if we're rendering at different resolution than window
	int windowWidth = p.width;
	int windowHeight = p.height;
	bool renderOffscreen = false;

	if (CurrentParams == nullptr) {
		CurrentParams = std::make_unique<ActualVideoModeParams>();
	}

	if (p.bpp != CurrentParams->bpp || m_bWasWindowed != p.windowed) {
		// Different depth, or we didn't make a window before. New context.
		bNewDeviceOut = true;

		int visAttribs[32];
		int i = 0;
		assert(p.bpp == 16 || p.bpp == 32);

		int screen = DefaultScreen(Dpy);
		int depth = DefaultDepth(Dpy, screen);
		Visual* visual = DefaultVisual(Dpy, screen);
		// I get strange behavior if I add override redirect after creating the
		// window. So, let's recreate the window when changing that state.
		if (!MakeWindow(
			  Win, screen, depth, visual, p.width, p.height, !p.windowed))
			return "Failed to create the window.";

		char* szWindowTitle = const_cast<char*>(p.sWindowTitle.c_str());
		XChangeProperty(Dpy,
						Win,
						XA_WM_NAME,
						XA_STRING,
						8,
						PropModeReplace,
						reinterpret_cast<unsigned char*>(szWindowTitle),
						strlen(szWindowTitle));

		XGetWindowAttributes(Dpy, Win, &winAttrib);
		XSelectInput(Dpy,
					 Win,
					 winAttrib.your_event_mask | StructureNotifyMask |
					   PropertyChangeMask);

		wmDeleteMessage = XInternAtom(Dpy, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(Dpy, Win, &wmDeleteMessage, 1);

		XMapWindow(Dpy, Win);

		XEvent ev;
		do {
			XWindowEvent(Dpy, Win, StructureNotifyMask, &ev);
		} while (ev.type != MapNotify);
	} else {
		// We're remodeling the existing window, and not touching the context.
		bNewDeviceOut = false;

		XGetWindowAttributes(Dpy, Win, &winAttrib);
		XSelectInput(Dpy,
					 Win,
					 winAttrib.your_event_mask | StructureNotifyMask |
					   PropertyChangeMask);

		if (!p.windowed) {
			// X11 is an asynchronous beast. If we're resizing an existing
			// window directly (i.e. override-redirect as opposed to asking the
			// WM to do it) and don't wait for the window to actually be
			// resized, we'll get unexpected results from glViewport() etc. I
			// don't know why, or why it *doesn't* break in the slower process
			// of waiting for the WM to resize the window.

			// So, set the event mask so we're notified when the window is
			// resized... Send the resize command...
			XResizeWindow(Dpy,
						  Win,
						  static_cast<unsigned int>(p.width),
						  static_cast<unsigned int>(p.height));

			// We'll wait for the notification once we've done everything else,
			// to save time.
		}
	}

	float rate = 60; // Will be unchanged if windowed. Not sure I care.

	if (!p.windowed) {
		RestoreOutputConfig();

		if (p.sDisplayId == ID_XSCREEN || p.sDisplayId.empty()) {
			// If the user changed the resolution while StepMania was windowed
			// we overwrite the resolution to restore with it at exit.
			m_iOldSize =
			  XRRConfigCurrentConfiguration(m_pScreenConfig, &m_OldRotation);
			m_bWasWindowed = false;

			// Find a matching mode.
			int iSizesXct;
			XRRScreenSize* pSizesX =
			  XRRSizes(Dpy, DefaultScreen(Dpy), &iSizesXct);
			assert(iSizesXct != 0 &&
				   "Couldn't get resolution list from X server");

			int iSizeMatch = -1;

			for (int i = 0; i < iSizesXct; ++i) {
				if (pSizesX[i].width == p.width &&
					pSizesX[i].height == p.height) {
					iSizeMatch = i;
					break;
				}
			}
			if (iSizeMatch != m_iOldSize) {
				m_bChangedScreenSize = true;
			}

			// Set this mode.
			// XXX: This doesn't handle if the config has changed since we
			// queried it (see man Xrandr)
			Status s = XRRSetScreenConfig(Dpy,
										  m_pScreenConfig,
										  RootWindow(Dpy, DefaultScreen(Dpy)),
										  iSizeMatch,
										  1,
										  CurrentTime);
			if (s) {
				return "Failed to set screen config";
			}

			XMoveWindow(Dpy, Win, 0, 0);

			XRaiseWindow(Dpy, Win);

			// We want to prevent the WM from catching anything that comes from
			// the keyboard. We should do this every time on fullscreen and not
			// only we entering from windowed mode because we could lose focus
			// at resolution change and that will leave the user input locked.
			while (XGrabKeyboard(
			  Dpy, Win, True, GrabModeAsync, GrabModeAsync, CurrentTime))
				;

		} else {
			assert(m_bUseXRandR12);
			/* === Configuring a specific CRTC === */
			// Arcane and undocumented but PROPER XRandR 1.2 method.
			// What we do is directly reconfigure the CRTC of the primary
			// display, Which prevents the (RandR) screen itself from resizing,
			// and therefore leaving user's desktop unmolested.
			Locator::getLogger()->info("LowLevelWindow_X11: Using XRandR");

			XRRScreenResources* scrRes = XRRGetScreenResources(Dpy, Win);
			assert(scrRes != NULL);
			assert(scrRes->ncrtc > 0);
			assert(scrRes->noutput > 0);
			assert(scrRes->nmode > 0);

			// If an output name has been specified, search for it
			RROutput targetOut = None;
			if (p.sDisplayId.length() > 0) {
				for (unsigned int i = 0;
					 i < scrRes->noutput && targetOut == None;
					 ++i) {
					XRROutputInfo* outInfo =
					  XRRGetOutputInfo(Dpy, scrRes, scrRes->outputs[i]);
					std::string outName =
					  std::string(outInfo->name,
								  static_cast<unsigned int>(outInfo->nameLen));
					if (p.sDisplayId == outName) {
						targetOut = scrRes->outputs[i];
					}
					XRRFreeOutputInfo(outInfo);
				}
			}
			if (targetOut == None) {
				Locator::getLogger()->info(
				  "Did not find display output {}, trying another",
				  p.sDisplayId.c_str());
				// didn't find named output, pick primary/or at least one that
				// works
				if (m_iRandRVerMajor >= 1 && m_iRandRVerMinor >= 3) {
					// RandR 1.3 can tell us what the primary display is.
					targetOut = XRRGetOutputPrimary(Dpy, Win);
				} else {
					// Only RandR 1.2. We'll look for a "Connected" output, or
					// if we can't find that, (it is possible the connection
					// state could be unknown), we'll at least look for an
					// output with a CRTC driving it
					RROutput connected = None, hasCrtc = None;
					for (unsigned int i = 0; i < scrRes->noutput; ++i) {
						XRROutputInfo* outInfo =
						  XRRGetOutputInfo(Dpy, scrRes, scrRes->outputs[i]);
						if (outInfo->connection ==
							RR_Connected) { // Check for CONNECTED state:
											// Connected == 0
							connected = scrRes->outputs[i];
						}
						if (outInfo->crtc != None) {
							hasCrtc = outInfo->crtc;
						}
						XRRFreeOutputInfo(outInfo);
					}
					targetOut = connected != None ? connected : hasCrtc;
					assert(targetOut != None);
				}
			}

			// if the target output is not currently being driven by a crtc,
			// find an unused crtc that can be connected to it
			XRROutputInfo* tgtOutInfo =
			  XRRGetOutputInfo(Dpy, scrRes, targetOut);
			if (tgtOutInfo == NULL) {
				XRRFreeScreenResources(scrRes);
				return "Failed to find XRROutput";
			}

			RRCrtc tgtOutCrtc = tgtOutInfo->crtc;
			if (tgtOutCrtc == None) {
				for (unsigned int i = 0; i < tgtOutInfo->ncrtc; ++i) {
					XRRCrtcInfo* crtcInfo =
					  XRRGetCrtcInfo(Dpy, scrRes, tgtOutInfo->crtcs[i]);
					if (crtcInfo->mode == None) {
						tgtOutCrtc = tgtOutInfo->crtcs[i];
					}
					XRRFreeCrtcInfo(crtcInfo);
				}
			}
			assert(tgtOutCrtc != None);

			XRRCrtcInfo* oldConf = XRRGetCrtcInfo(Dpy, scrRes, tgtOutCrtc);

			float fRefreshDiff = 99999;
			float fRefreshRate = 0;
			RRMode mode = None;
			// A quirk of XRandR is that the width and height are as the display
			// controller ("CRTC") sees it, which means height and width are
			// flipped if there's rotation going on.
			const bool bPortrait =
			  (oldConf->rotation & (RR_Rotate_90 | RR_Rotate_270)) != 0;
			// Find a mode that matches our exact wanted resolution,
			// with as close to our desired refresh rate as possible.
			for (int i = 0; i < scrRes->nmode; i++) {
				const XRRModeInfo& thisMI = scrRes->modes[i];
				const unsigned int modeWidth =
				  bPortrait ? thisMI.height : thisMI.width;
				const unsigned int modeHeight =
				  bPortrait ? thisMI.width : thisMI.height;
				if (modeWidth == p.width && modeHeight == p.height) {
					float fTempRefresh = calcRandRRefresh(
					  thisMI.dotClock, thisMI.hTotal, thisMI.vTotal);
					float fTempDiff = std::abs(p.rate - fTempRefresh);
					if ((p.rate != REFRESH_DEFAULT &&
						 fTempDiff < fRefreshDiff) ||
						(p.rate == REFRESH_DEFAULT &&
						 fTempRefresh > fRefreshRate)) {
						int j;
						// Ensure that the output supports the mode
						for (j = 0; j < tgtOutInfo->nmode; j++)
							if (tgtOutInfo->modes[j] == scrRes->modes[i].id) {
								mode = tgtOutInfo->modes[j];
								break;
							}

						if (j < tgtOutInfo->nmode) {
							fRefreshRate = fTempRefresh;
							fRefreshDiff = fTempDiff;
						}
					}
				}
			}
			rate = roundf(fRefreshRate);

			m_usedCrtc = tgtOutCrtc;
			m_originalRandRMode = oldConf->mode;

			const std::string tgtOutName = std::string(
			  tgtOutInfo->name, static_cast<unsigned int>(tgtOutInfo->nameLen));
			Locator::getLogger()->info(
			  "XRandR output config using CRTC {} in mode {}, "
			  "driving output %s",
			  m_usedCrtc,
			  mode,
			  tgtOutName.c_str());
			// and FIRE!
			Status s = XRRSetCrtcConfig(Dpy,
										scrRes,
										m_usedCrtc,
										oldConf->timestamp,
										oldConf->x,
										oldConf->y,
										mode,
										oldConf->rotation,
										oldConf->outputs,
										oldConf->noutput);
			if (s) {
				XRRFreeCrtcInfo(oldConf);
				XRRFreeOutputInfo(tgtOutInfo);
				XRRFreeScreenResources(scrRes);
				return "Failed to set CRTC config";
			}

			// We don't move to absolute 0,0 because that may be in the area of
			// a different output. Instead we preserved the corner of our CRTC;
			// go to that.
			XMoveWindow(Dpy, Win, oldConf->x, oldConf->y);

			// Final cleanup
			XRRFreeCrtcInfo(oldConf);
			XRRFreeOutputInfo(tgtOutInfo);
			XRRFreeScreenResources(scrRes);
		}
		m_bWasWindowed = false;

		XRaiseWindow(Dpy, Win);

		// We want to prevent the WM from catching anything that comes from the
		// keyboard. We should do this every time on fullscreen and not only we
		// entering from windowed mode because we could lose focus at resolution
		// change and that will leave the user input locked.
		while (XGrabKeyboard(
		  Dpy, Win, True, GrabModeAsync, GrabModeAsync, CurrentTime))
			;
	} else // if(p.windowed)
	{
		if (!m_bWasWindowed) {
			// Return the display to the mode it was in before we fullscreened.
			RestoreOutputConfig();
			XUngrabKeyboard(Dpy, CurrentTime);
			m_bWasWindowed = true;
		}

		Atom net_wm_state = XInternAtom(Dpy, "_NET_WM_STATE", False);
		Atom fullscreen_state =
		  XInternAtom(Dpy, "_NET_WM_STATE_FULLSCREEN", False);
		Atom maximized_vert =
		  XInternAtom(Dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
		Atom maximized_horz =
		  XInternAtom(Dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
		// if FSBW, find matching monitor, move window to its origin,
		// then set fullscreen hint, and set the CurrentParams.outWidth,
		// CurrentParams.outHeight to the values of that display otherwise set
		// the size hints and disable MAXIMIZED_*
		if (p.bWindowIsFullscreenBorderless) {
			auto specs = DisplaySpecs{};
			GetDisplaySpecs(specs);
			auto target = std::find_if(
			  specs.begin(), specs.end(), [&](const DisplaySpec& spec) {
				  return p.sDisplayId == spec.id() &&
						 spec.currentMode() != nullptr;
			  });
			// If we didn't find a matching DisplaySpec for the requested ID,
			// pick the first one with a current mode
			if (target == specs.end()) {
				target = std::find_if(
				  specs.begin(), specs.end(), [&](const DisplaySpec& spec) {
					  return spec.currentMode() != nullptr;
				  });
			}
			// If we _still_ haven't found anything (unlikely), then just give
			// up
			if (target == specs.end()) {
				return "Unable to find destination monitor for fullscreen "
					   "borderless";
			}

			windowWidth = target->currentMode()->width;
			windowHeight = target->currentMode()->height;

			if (windowWidth != p.width || windowHeight != p.height) {
				renderOffscreen = true;
			}

			// Reset anything that might've been set previously:
			// (1) Undo Min/Max size bounds
			// (2) Remove FULLSCREEN/MAXIMIZED_{HORIZ,VERT} hints
			// Without doing this, WM may not let us move/resize window to new
			// display Give Window manager the chance to react to changes
			// (otherwise, Mutter had problems properly reacting to moving a
			// _NET_WM_STATE_FULLSCREEN window to a different output
			//   and fullscreen resetting FULLSCREEN hint.
			XSizeHints hints;
			hints.flags = 0;
			XSetWMNormalHints(Dpy, Win, &hints);
#if defined(HAVE_XINERAMA)
			if (!g_bUseXinerama || !SetWMFullscreenMonitors(*target))
#endif
			{
				SetWMState(winAttrib.root, Win, 0, maximized_horz);
				SetWMState(winAttrib.root, Win, 0, maximized_vert);
				SetWMState(winAttrib.root, Win, 0, fullscreen_state);

				XFlush(Dpy);
				XResizeWindow(Dpy,
							  Win,
							  static_cast<unsigned int>(windowWidth),
							  static_cast<unsigned int>(windowHeight));
				XMoveWindow(Dpy,
							Win,
							target->currentBounds().left,
							target->currentBounds().top);
				XRaiseWindow(Dpy, Win);

				SetWMState(winAttrib.root, Win, 1, fullscreen_state);
				SetWMState(winAttrib.root, Win, 1, maximized_horz);
				SetWMState(winAttrib.root, Win, 1, maximized_vert);
			}
		} else {
			windowWidth = p.width;
			windowHeight = p.height;

			SetWMState(winAttrib.root, Win, 0, fullscreen_state);
			// Make a window fixed size, don't let resize it or maximize it.
			// Do this before resizing the window so that pane-style WMs (Ion,
			// ratpoison) don't resize us back inappropriately.
			{
				XSizeHints hints;

				hints.flags = PMinSize | PMaxSize | PWinGravity;
				hints.min_width = hints.max_width = windowWidth;
				hints.min_height = hints.max_height = windowHeight;
				hints.win_gravity = CenterGravity;

				XSetWMNormalHints(Dpy, Win, &hints);
			}
			/* Workaround for metacity and compiz: if the window have the same
			 * resolution or higher than the screen, it gets automaximized even
			 * when the window is set to not let it happen. This happens when
			 * changing from fullscreen to window mode and our screen resolution
			 * is bigger. */
			{
				SetWMState(winAttrib.root, Win, 1, maximized_vert);
				SetWMState(winAttrib.root, Win, 1, maximized_horz);

				// This one is needed for compiz, if the window reaches out of
				// bounds of the screen it becames destroyed, only the window,
				// the program is left running. Commented out per the patch at
				// http://ssc.ajworld.net/sm-ssc/bugtracker/view.php?id=398
				// XMoveWindow( Dpy, Win, 0, 0 );
			}
		}
	}

	CurrentParams = std::make_unique<ActualVideoModeParams>(p);
	CurrentParams->windowWidth = windowWidth;
	CurrentParams->windowHeight = windowHeight;
	CurrentParams->renderOffscreen = renderOffscreen;
	assert(rate > 0);
	CurrentParams->rate = static_cast<int>(roundf(rate));

	return "";
}

void
LowLevelWindowVK_X11::Update()
{
	XEvent event;
	if (XCheckTypedEvent(Dpy, ClientMessage, &event) &&
		event.xclient.data.l[0] == wmDeleteMessage) {
		GameLoop::setUserQuit();
	}
}

void
LowLevelWindowVK_X11::GetDisplaySpecs(DisplaySpecs& out) const
{
	int screenNum = DefaultScreen(Dpy);
	Screen* screen = ScreenOfDisplay(Dpy, screenNum);

	XWindowAttributes winAttr = XWindowAttributes();
	if (XGetWindowAttributes(Dpy, Win, &winAttr)) {
		screen = winAttr.screen;
		screenNum = XScreenNumberOfScreen(screen);
	}

	// Create a display spec for the entire X screen itself
	// First get current config
	Rotation curRotation;
	XRRScreenConfiguration* screenConf = XRRGetScreenInfo(Dpy, Win);
	const short curRate = XRRConfigCurrentRate(screenConf);
	SizeID curSizeId = XRRConfigCurrentConfiguration(screenConf, &curRotation);
	// curRotation does not factor into how we report supported XScreen sizes:
	// XRR reports the supported *screen* sizes with height/width swapped
	// appropriately for currently configured rotation. Supported sizes for
	// *output* modes (below) DO NOT account for screen rotation

	std::set<DisplayMode> screenModes;
	int nsizes = 0;
	XRRScreenSize* screenSizes = XRRSizes(Dpy, screenNum, &nsizes);
	DisplayMode screenCurMode = { 0 };
	for (unsigned int szIdx = 0, mode_idx = 0; szIdx < nsizes; ++szIdx) {
		XRRScreenSize& size = screenSizes[szIdx];
		int nrates = 0;
		short* rates = XRRRates(Dpy, screenNum, szIdx, &nrates);
		for (unsigned int rIdx = 0; rIdx < nrates; ++rIdx, ++mode_idx) {
			DisplayMode m = { static_cast<unsigned int>(size.width),
							  static_cast<unsigned int>(size.height),
							  static_cast<double>(rates[rIdx]) };
			screenModes.insert(m);
			if (rates[rIdx] == curRate && szIdx == curSizeId) {
				screenCurMode = m;
			}
		}
	}
	const RectI screenBounds(
	  0, 0, screenSizes[curSizeId].width, screenSizes[curSizeId].height);
	const DisplaySpec screenSpec(
	  ID_XSCREEN, "X Screen", screenModes, screenCurMode, screenBounds, true);
	out.insert(screenSpec);
	// XRRScreenSize array from XRRSizes does *not* have to be returned
	// (valgrind said XFree was an invalid free in a small test program, there
	// is no XRRFreeScreenSize, etc)
	XRRFreeScreenConfigInfo(screenConf);

	if (m_bUseXRandR12) {
		// Build per-output DisplaySpecs

		// First, get the list of resolutions that'll be referenced (by RRMode)
		// in each OutputInfo
		XRRScreenResources* scrRes = XRRGetScreenResources(Dpy, Win);
		std::map<RRMode, DisplayMode> outputModes;
		for (unsigned int i = 0; i < scrRes->nmode; ++i) {
			const XRRModeInfo& mode = scrRes->modes[i];
			DisplayMode m = { mode.width,
							  mode.height,
							  calcRandRRefresh(
								mode.dotClock, mode.hTotal, mode.vTotal) };
			outputModes[mode.id] = m;
		}

		// Now, for each output, build a corresponding DisplaySpec
		for (unsigned int outIdx = 0; outIdx < scrRes->noutput; ++outIdx) {
			XRROutputInfo* outInfo =
			  XRRGetOutputInfo(Dpy, scrRes, scrRes->outputs[outIdx]);
			if (outInfo->nmode > 0) {
				// Get the current configuration of the Output, if it's being
				// driven by a crtc
				RRMode curRRMode = None;
				bool bPortrait = false;
				int crtcX = 0, crtcY = 0;
				if (outInfo->crtc != None) {
					XRRCrtcInfo* conf =
					  XRRGetCrtcInfo(Dpy, scrRes, outInfo->crtc);
					curRRMode = conf->mode;
					bPortrait =
					  (conf->rotation & (RR_Rotate_90 | RR_Rotate_270)) != 0;
					crtcX = conf->x;
					crtcY = conf->y;
					XRRFreeCrtcInfo(conf);
				}
				// Get all supported modes, noting which one, if any, is
				// currently active
				std::set<DisplayMode> outputSupported;
				DisplayMode outputCurMode = { 0 };
				RectI outBounds;
				for (unsigned int modeIdx = 0; modeIdx < outInfo->nmode;
					 ++modeIdx) {
					DisplayMode mode = outputModes[outInfo->modes[modeIdx]];
					unsigned int modeWidth =
					  bPortrait ? mode.height : mode.width;
					unsigned int modeHeight =
					  bPortrait ? mode.width : mode.height;
					DisplayMode m = { modeWidth, modeHeight, mode.refreshRate };
					outputSupported.insert(m);
					if (curRRMode != None &&
						outInfo->modes[modeIdx] == curRRMode) {
						outputCurMode = m;
						outBounds = RectI(
						  crtcX, crtcY, crtcX + modeWidth, crtcY + modeHeight);
					}
				}
				const std::string outId(
				  outInfo->name, static_cast<unsigned int>(outInfo->nameLen));
				const std::string outName(outId);
				if (curRRMode != None) {
					out.insert(DisplaySpec(outId,
										   outName,
										   outputSupported,
										   outputCurMode,
										   outBounds));
				} else {
					out.insert(DisplaySpec(outId, outName, outputSupported));
				}
			}
			XRRFreeOutputInfo(outInfo);
		}
		XRRFreeScreenResources(scrRes);
	}
}

const ActualVideoModeParams*
LowLevelWindowVK_X11::GetActualVideoModeParams() const
{
	return CurrentParams.get();
}

bool
LowLevelWindowVK_X11::SupportsFullscreenBorderlessWindow() const
{
	Atom fullscreen = XInternAtom(Dpy, "_NET_WM_STATE_FULLSCREEN", False);
	return NetWMSupported(Dpy, fullscreen);
}
