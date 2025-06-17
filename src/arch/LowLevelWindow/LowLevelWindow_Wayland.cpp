#include "Etterna/Globals/global.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Models/Misc/DisplaySpec.h"
#include "archutils/Unix/WaylandHelper.h"

using namespace WaylandHelper;

#include "LowLevelWindow_Wayland.h"
#include <wayland-client.h>
#include <EGL/egl.h>

LowLevelWindow_Wayland::LowLevelWindow_Wayland()
{
	// Wayland-specific initialization can go here
	if (!OpenWaylandConnection()) {
		Locator::getLogger()->error("Failed to connect to Wayland display");
		RageException::Throw("Failed to connect to Wayland display");
	}

	// use OpenGL instead of OpenGL ES
	if (!eglBindAPI(EGL_OPENGL_API)) {
		Locator::getLogger()->error("Failed to bind OpenGL API");
		RageException::Throw("Failed to bind OpenGL API");
	}

	this->edisplay = EGL_NO_DISPLAY;

	this->main_window = nullptr;
	this->esurface = EGL_NO_SURFACE;
	this->ectx = EGL_NO_CONTEXT;

	Locator::getLogger()->info("LowLevelWindow_Wayland: Initialized");
}

LowLevelWindow_Wayland::~LowLevelWindow_Wayland()
{
	// Cleanup code for Wayland can go here
	Locator::getLogger()->info("LowLevelWindow_Wayland: cleaner called");

	if (main_window != nullptr) {
		DestroyWindow(main_window);
	}

	if (esurface != EGL_NO_SURFACE) {
		eglDestroySurface(edisplay, esurface);
	}

	if (ectx != EGL_NO_CONTEXT) {
		eglDestroyContext(edisplay, ectx);
	}

	if (edisplay != EGL_NO_DISPLAY) {
		eglTerminate(edisplay);
	}

	WaylandHelper::CloseWaylandConnection();
}

void*
LowLevelWindow_Wayland::GetProcAddress(const std::string& s)
{
	Locator::getLogger()->info("LowLevelWindow_Wayland: GetProcAddress called");
	return (void*)eglGetProcAddress(s.c_str());
}

std::string
LowLevelWindow_Wayland::TryVideoMode(const VideoModeParams& p,
									 bool& bNewDeviceOut)
{
	// Implement video mode handling for Wayland
	Locator::getLogger()->info("LowLevelWindow_Wayland: TryVideoMode called");

	if (ectx == nullptr || p.bpp != CurrentParams.bpp) {
		if (edisplay != EGL_NO_DISPLAY) {
			eglTerminate(edisplay);
		}

		// get the EGL display
		edisplay = eglGetDisplay(display);
		if (edisplay == EGL_NO_DISPLAY) {
			Locator::getLogger()->error("Failed to get EGL display");
			return "Failed to get EGL display";
		}

		// initialize EGL
		if (!eglInitialize(edisplay, nullptr, nullptr)) {
			Locator::getLogger()->error("Failed to initialize EGL");
			return "Failed to initialize EGL";
		}

		EGLConfig eglConfig;
		EGLint numConfigs;

		ASSERT(p.bpp == 16 || p.bpp == 32);

		// initially assume 32 bit depth
		EGLint attribs[] = {
			EGL_RED_SIZE,
			8,
			EGL_GREEN_SIZE,
			8,
			EGL_BLUE_SIZE,
			8,
			EGL_DEPTH_SIZE,
			16,
			EGL_RENDERABLE_TYPE,
			EGL_OPENGL_BIT,
			EGL_SURFACE_TYPE,
			EGL_WINDOW_BIT,
			EGL_MIN_SWAP_INTERVAL,
			0,
			EGL_NONE,
		};

		// if the bpp is 16, set the bit depth to 16
		if (p.bpp == 16) {
			attribs[1] = 5;
			attribs[3] = 6;
			attribs[5] = 5;
		}

		if ((eglChooseConfig(edisplay, attribs, &eglConfig, 1, &numConfigs) !=
			   EGL_TRUE ||
			 numConfigs == 0)) {
			Locator::getLogger()->error("Failed to choose EGL config");
			return "Failed to choose EGL config";
		}

		if (main_window != nullptr) {
			DestroyWindow(main_window);
		}

		if (!MakeWindow(main_window, p.width, p.height)) {
			Locator::getLogger()->error("Failed to create Wayland window");
			return "Failed to create Wayland window";
		}

		if (esurface != EGL_NO_SURFACE) {
			eglDestroySurface(edisplay, esurface);
		}

		esurface =
		  eglCreateWindowSurface(edisplay, eglConfig, main_window, nullptr);
		if (esurface == EGL_NO_SURFACE) {
			Locator::getLogger()->error("Failed to create EGL window surface");
			return "Failed to create EGL window surface";
		}

		if (ectx != EGL_NO_CONTEXT) {
			eglDestroyContext(edisplay, ectx);
		}

		ectx = eglCreateContext(edisplay, eglConfig, EGL_NO_CONTEXT, nullptr);
		if (ectx == EGL_NO_CONTEXT) {
			Locator::getLogger()->error("Failed to create EGL context");
			return "Failed to create EGL context";
		}

		if (!eglMakeCurrent(edisplay, esurface, esurface, ectx)) {
			Locator::getLogger()->error("Failed to make EGL context current");
			return "Failed to make EGL context current";
		}

		xdg_toplevel_set_title(xdgtoplevel, p.sWindowTitle.c_str());
	} else {
		wl_egl_window_resize(main_window, p.width, p.height, 0, 0);
	}

	// TODO: it would probably be nice to provide window decorations but
	// there isnt anything in etterna to provide clientside decorations and
	// getting serverside decorations is a pain since the protocol for it is
	// still unstable

	// unset fullscreen is called regardless of if we are going to set it as
	// this ensures that the window actually moves to the correct screen
	xdg_toplevel_unset_fullscreen(xdgtoplevel);

	// set the fullscreen output if we are going fullscreen
	if (!p.windowed) {
		Locator::getLogger()->info(
		  "LowLevelWindow_Wayland: setting fullscreen to {}", p.sDisplayId);

		// find the wayland output with the same id as the one requested
		auto target =
		  std::find_if(WaylandHelper::outputs.begin(),
					   WaylandHelper::outputs.end(),
					   [&](const auto& output) {
						   return *output.second.name == p.sDisplayId;
					   });

		if (target == WaylandHelper::outputs.end()) {
			Locator::getLogger()->error(
			  "LowLevelWindow_Wayland: Failed to find output with id {}",
			  p.sDisplayId);
			return "Failed to find output with id";
		}

		// note: this currently seems to cause the fps to be locked to vsync on
		// nvidia but this seems to be an nvidia driver issue as this does not
		// happen on amd graphics
		xdg_toplevel_set_fullscreen(xdgtoplevel, target->first);
	}

	// enable/disable vsync
	// It would be better to use the tearing protocol here but it is staging and
	// seems to crash the nvidia driver for me
	// https://wayland.app/protocols/tearing-control-v1
	if (p.vsync) {
		eglSwapInterval(edisplay, 1);
	} else {
		eglSwapInterval(edisplay, 0);
	}

	CurrentParams = p;

	return "";
}

void
LowLevelWindow_Wayland::SwapBuffers()
{
	eglSwapBuffers(edisplay, esurface);
}

void
LowLevelWindow_Wayland::Update()
{
	wl_display_flush(display);

	// Check for Wayland errors
	if (wl_display_get_error(display) != 0) {
		Locator::getLogger()->error("Wayland display error: {}",
									wl_display_get_error(display));
	}
}

void
LowLevelWindow_Wayland::GetDisplaySpecs(DisplaySpecs& out) const
{
	// add display specs to out
	// This only adds the current outputs to the valid display specs This is
	// somewhat intentional since wayland doesnt seem to handle it very well if
	// a fullscreen application has a mismatched resolution
	for (const auto& output : WaylandHelper::outputs) {
		auto m = output.second;

		DisplayMode mode(
		  m.width, m.height, static_cast<float>(m.rate) / 1000.0f);
		DisplaySpec spec(*m.name,
						 "[" + *m.name + "] " + *m.description,
						 std::set<DisplayMode>{ mode },
						 mode,
						 RectI(0, 0, m.width, m.height));

		out.insert(spec);
	}
}

bool
LowLevelWindow_Wayland::SupportsRenderToTexture() const
{
	// Server must support pbuffers:
	const char* version = eglQueryString(edisplay, EGL_VERSION);
	float fVersion = strtof(version, nullptr);
	if (fVersion < 1.3f) {
		Locator::getLogger()->error("EGL version is too low: {}", version);
		return false;
	}

	return true;
}
