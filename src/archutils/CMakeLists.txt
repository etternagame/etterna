if(APPLE)
	list(APPEND OS_DARWIN_SRC
		"Darwin/Crash.cpp"
		"Darwin/DarwinThreadHelpers.cpp"
		"Darwin/HIDDevice.cpp"
		"Darwin/JoystickDevice.cpp"
		"Darwin/KeyboardDevice.cpp"
		"Darwin/MouseDevice.cpp"
		"Darwin/PumpDevice.cpp"
		"Darwin/SMMain.mm"
		"Darwin/VectorHelper.cpp")

	list(APPEND OS_DARWIN_HPP
		"Darwin/arch_setup.h"
		"Darwin/Crash.h"
		"Darwin/DarwinThreadHelpers.h"
		"Darwin/HIDDevice.h"
		"Darwin/JoystickDevice.h"
		"Darwin/KeyboardDevice.h"
		"Darwin/MouseDevice.h"
		"Darwin/PumpDevice.h"
		"Darwin/VectorHelper.h")

	list(APPEND OS_SRC ${OS_DARWIN_SRC})
	list(APPEND OS_HPP ${OS_DARWIN_HPP})

endif()

if(WIN32)
	list(APPEND OS_SRC
		"Win32/AppInstance.cpp"
		"Win32/arch_setup.cpp"
		"Win32/arch_time.cpp"
		"Win32/CommandLine.cpp"
		"Win32/DialogUtil.cpp"
		"Win32/DirectXHelpers.cpp"
		"Win32/ErrorStrings.cpp"
		"Win32/GetFileInformation.cpp"
		"Win32/GraphicsWindow.cpp"
		"Win32/MessageWindow.cpp"
		"Win32/RegistryAccess.cpp"
		"Win32/USB.cpp"
		"Win32/VideoDriverInfo.cpp"
		"Win32/WindowIcon.cpp"
		"Win32/WindowsDialogBox.cpp"
		"Win32/WindowsResources.rc"
		"Win32/Etterna.manifest")

	list(APPEND OS_HPP
		"Win32/AppInstance.h"
		"Win32/arch_setup.h"
		"Win32/CommandLine.h"
		"Win32/DialogUtil.h"
		"Win32/DirectXHelpers.h"
		"Win32/ErrorStrings.h"
		"Win32/GetFileInformation.h"
		"Win32/GraphicsWindow.h"
		"Win32/MessageWindow.h"
		"Win32/RegistryAccess.h"
		"Win32/USB.h"
		"Win32/VideoDriverInfo.h"
		"Win32/WindowIcon.h"
		"Win32/WindowsDialogBox.h"
		"Win32/WindowsResources.h")
endif()

if(NOT APPLE AND UNIX)
	list(APPEND OS_SRC # TODO: X11 check, crash handler check
		"Unix/GetSysInfo.cpp"
		"Unix/RunningUnderValgrind.cpp"
		"Unix/SignalHandler.cpp"
		"Unix/StackCheck.cpp")

	list(APPEND OS_HPP
		"Unix/arch_setup.h"
		"Unix/GetSysInfo.h"
		"Unix/RunningUnderValgrind.h"
		"Unix/SignalHandler.h"
		"Common/gcc_byte_swaps.h")

	list(APPEND OS_SRC "Unix/X11Helper.cpp")
	list(APPEND OS_HPP "Unix/X11Helper.h")
	list(APPEND OS_SRC "Common/PthreadHelpers.cpp")
	list(APPEND OS_HPP "Common/PthreadHelpers.h")
endif()

if(UNIX) # Includes Apple
	list(APPEND OS_UNIX_CRASH_SRC
		"Unix/SignalHandler.cpp")

	list(APPEND OS_UNIX_CRASH_HPP
		"Unix/SignalHandler.h")

	list(APPEND OS_SRC ${OS_UNIX_CRASH_SRC})
	list(APPEND OS_HPP ${OS_UNIX_CRASH_HPP})
endif()

target_sources(Etterna PUBLIC ${OS_SRC} ${OS_HPP})
