if (APPLE)
    list(APPEND SMDATA_OS_DARWIN_SRC
         "Darwin/Crash.cpp"
         "Darwin/DarwinThreadHelpers.cpp"
         "Darwin/HIDDevice.cpp"
         "Darwin/JoystickDevice.cpp"
         "Darwin/KeyboardDevice.cpp"
         "Darwin/MouseDevice.cpp"
         "Darwin/PumpDevice.cpp"
         "Darwin/SMMain.mm"
         "Darwin/SpecialDirs.cpp"
         "Darwin/VectorHelper.cpp")
    list(APPEND SMDATA_OS_DARWIN_HPP
         "Darwin/arch_setup.h"
         "Darwin/Crash.h"
         "Darwin/DarwinThreadHelpers.h"
         "Darwin/HIDDevice.h"
         "Darwin/JoystickDevice.h"
         "Darwin/KeyboardDevice.h"
         "Darwin/MouseDevice.h"
         "Darwin/PumpDevice.h"
         "Darwin/SpecialDirs.h"
         "Darwin/Etterna.pch" # precompiled header. :)
         "Darwin/VectorHelper.h")

    source_group("OS Specific\\\\Darwin" FILES ${SMDATA_OS_DARWIN_SRC} ${SMDATA_OS_DARWIN_HPP})


    list(APPEND SMDATA_OS_SRC ${SMDATA_OS_DARWIN_SRC})
    list(APPEND SMDATA_OS_HPP ${SMDATA_OS_DARWIN_HPP})
else ()
    if (WIN32)
        list(APPEND SMDATA_OS_SRC
             "Win32/AppInstance.cpp"
             "Win32/arch_setup.cpp"
             "Win32/arch_time.cpp"
             "Win32/CommandLine.cpp"
             "Win32/Crash.cpp"
             "Win32/CrashHandlerChild.cpp"
             "Win32/CrashHandlerNetworking.cpp"
             "Win32/DebugInfoHunt.cpp"
             "Win32/DialogUtil.cpp"
             "Win32/DirectXHelpers.cpp"
             "Win32/ErrorStrings.cpp"
             "Win32/GetFileInformation.cpp"
             "Win32/GotoURL.cpp"
             "Win32/GraphicsWindow.cpp"
             "Win32/MessageWindow.cpp"
             "Win32/RegistryAccess.cpp"
             "Win32/RestartProgram.cpp"
             "Win32/SpecialDirs.cpp"
             "Win32/USB.cpp"
             "Win32/VideoDriverInfo.cpp"
             "Win32/WindowIcon.cpp"
             "Win32/WindowsDialogBox.cpp"
             "Win32/WindowsResources.rc")

        list(APPEND SMDATA_OS_HPP
             "Win32/AppInstance.h"
             "Win32/arch_setup.h"
             "Win32/CommandLine.h"
             "Win32/Crash.h"
             "Win32/CrashHandlerInternal.h"
             "Win32/CrashHandlerNetworking.h"
             "Win32/DebugInfoHunt.h"
             "Win32/DialogUtil.h"
             "Win32/DirectXHelpers.h"
             "Win32/ErrorStrings.h"
             "Win32/GetFileInformation.h"
             "Win32/GotoURL.h"
             "Win32/GraphicsWindow.h"
             "Win32/MessageWindow.h"
             "Win32/RegistryAccess.h"
             "Win32/RestartProgram.h"
             "Win32/SpecialDirs.h"
             "Win32/USB.h"
             "Win32/VideoDriverInfo.h"
             "Win32/WindowIcon.h"
             "Win32/WindowsDialogBox.h"
             "Win32/WindowsResources.h")
    else () # Unix
        list(APPEND SMDATA_OS_SRC # TODO: X11 check, crash handler check
             "Unix/AssertionHandler.cpp"
             "Unix/EmergencyShutdown.cpp"
             "Unix/GetSysInfo.cpp"
             "Unix/RunningUnderValgrind.cpp"
             "Unix/SignalHandler.cpp"
             "Unix/SpecialDirs.cpp"
             "Unix/StackCheck.cpp")
        list(APPEND SMDATA_OS_HPP
             "Unix/arch_setup.h"
             "Unix/AssertionHandler.h"
             "Unix/EmergencyShutdown.h"
             "Unix/GetSysInfo.h"
             "Unix/RunningUnderValgrind.h"
             "Unix/SignalHandler.h"
             "Unix/SpecialDirs.h"
             "Common/gcc_byte_swaps.h")
        if (X11_FOUND)
            list(APPEND SMDATA_OS_SRC "Unix/X11Helper.cpp")
            list(APPEND SMDATA_OS_HPP "Unix/X11Helper.h")
        endif ()
        if (HAS_PTHREAD)
            list(APPEND SMDATA_OS_SRC "Common/PthreadHelpers.cpp")
            list(APPEND SMDATA_OS_HPP "Common/PthreadHelpers.h")
        endif ()
    endif ()
    source_group("OS Specific" FILES ${SMDATA_OS_SRC} ${SMDATA_OS_HPP})
endif ()

if (APPLE OR LINUX)
    if (WITH_CRASH_HANDLER)
        list(APPEND SMDATA_OS_UNIX_CRASH_SRC
             "Unix/Backtrace.cpp"
             "Unix/BacktraceNames.cpp"
             "Unix/CrashHandler.cpp"
             "Unix/CrashHandlerChild.cpp"
             "Unix/CrashHandlerInternal.cpp"
             "Unix/SignalHandler.cpp")
        list(APPEND SMDATA_OS_UNIX_CRASH_HPP
             "Unix/Backtrace.h"
             "Unix/BacktraceNames.h"
             "Unix/CrashHandler.h"
             "Unix/CrashHandlerInternal.h"
             "Unix/SignalHandler.h")
        source_group("OS Specific\\\\Unix" FILES ${SMDATA_OS_UNIX_CRASH_SRC} ${SMDATA_OS_UNIX_CRASH_HPP})

        list(APPEND SMDATA_OS_SRC ${SMDATA_OS_UNIX_CRASH_SRC})
        list(APPEND SMDATA_OS_HPP ${SMDATA_OS_UNIX_CRASH_HPP})
    endif ()
endif ()

target_sources(Etterna PUBLIC ${SMDATA_OS_SRC})