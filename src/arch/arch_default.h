#ifndef ARCH_DEFAULT_H
#define ARCH_DEFAULT_H

/* Define the default driver sets. */
#ifdef _WIN32
#include "LoadingWindow/LoadingWindow_Win32.h"
#include "LowLevelWindow/LowLevelWindow_Win32.h"
#define DEFAULT_INPUT_DRIVER_LIST "DirectInput,Pump,Para"
#define DEFAULT_MOVIE_DRIVER_LIST "FFMpeg,DShow,Null"
#define DEFAULT_SOUND_DRIVER_LIST "WaveOut,DirectSound-sw,WDMKS,Null"

#elif defined(__APPLE__)
#include "LoadingWindow/LoadingWindow_MacOSX.h"
#include "LowLevelWindow/LowLevelWindow_MacOSX.h"
#define DEFAULT_INPUT_DRIVER_LIST "HID"
#define DEFAULT_MOVIE_DRIVER_LIST "FFMpeg,Null"
#define DEFAULT_SOUND_DRIVER_LIST "AudioUnit,Null"

#elif defined(__unix__)
#include "LowLevelWindow/LowLevelWindow_X11.h"

#if defined(HAVE_GTK)
#include "LoadingWindow/LoadingWindow_Gtk.h"
#endif
#if __unix__
#define DEFAULT_INPUT_DRIVER_LIST "X11,LinuxEvent,LinuxJoystick"
#else
#define DEFAULT_INPUT_DRIVER_LIST "X11"
#endif
#define DEFAULT_MOVIE_DRIVER_LIST "FFMpeg,Null"
// ALSA comes first, as the system may have OSS compat but we don't want to use
// it if it's actually an ALSA wrapper.
// Then try OSS before daemon drivers so we're going direct instead of
// unwittingly starting a daemon.
// JACK gives us an explicit option to NOT start a daemon, so try it third,
// as PulseAudio will successfully Init() but not actually work if the
// PulseAudio daemon has been suspended by/for jackd.
#define DEFAULT_SOUND_DRIVER_LIST "ALSA-sw,OSS,JACK,Pulse,Null"
#else
#error Which arch?
#endif

/* All use these. */
#include "LoadingWindow/LoadingWindow_Null.h"

#endif
