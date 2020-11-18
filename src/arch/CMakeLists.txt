list(APPEND SMDATA_ARCH_SRC "RageDriver.cpp")
list(APPEND SMDATA_ARCH_HPP "arch_default.h" "RageDriver.h")
source_group("Arch Specific" FILES ${SMDATA_ARCH_SRC} ${SMDATA_ARCH_HPP})

list(APPEND SMDATA_ARCH_THREADS_HPP "Threads/Threads.h")
if(WIN32)
  list(APPEND SMDATA_ARCH_THREADS_HPP "Threads/Threads_Win32.h")
  list(APPEND SMDATA_ARCH_THREADS_SRC "Threads/Threads_Win32.cpp")
elseif(APPLE OR UNIX)
  list(APPEND SMDATA_ARCH_THREADS_HPP "Threads/Threads_Pthreads.h")
  list(APPEND SMDATA_ARCH_THREADS_SRC "Threads/Threads_Pthreads.cpp")
endif()
source_group("Arch Specific\\\\Threads" FILES ${SMDATA_ARCH_THREADS_SRC} ${SMDATA_ARCH_THREADS_HPP})

list(APPEND SMDATA_ARCH_SOUND_SRC
  "Sound/RageSoundDriver.cpp"
  "Sound/RageSoundDriver_Generic_Software.cpp"
  "Sound/RageSoundDriver_Null.cpp")
list(APPEND SMDATA_ARCH_SOUND_HPP "Sound/RageSoundDriver.h" "Sound/RageSoundDriver_Null.h")

if(WIN32)
  list(APPEND SMDATA_ARCH_SOUND_SRC
    "Sound/DSoundHelpers.cpp"
    "Sound/RageSoundDriver_DSound_Software.cpp"
    "Sound/RageSoundDriver_WaveOut.cpp"
    "Sound/RageSoundDriver_WDMKS.cpp")
  list(APPEND SMDATA_ARCH_SOUND_HPP
    "Sound/DSoundHelpers.h"
    "Sound/RageSoundDriver_DSound_Software.h"
    "Sound/RageSoundDriver_WaveOut.h"
    "Sound/RageSoundDriver_WDMKS.h")
  
elseif(APPLE)
  list(APPEND SMDATA_ARCH_SOUND_SRC "Sound/RageSoundDriver_AU.cpp")
  list(APPEND SMDATA_ARCH_SOUND_HPP "Sound/RageSoundDriver_AU.h")
  
else() # Unix
  if(PULSEAUDIO_FOUND)
    list(APPEND SMDATA_ARCH_SOUND_SRC "Sound/RageSoundDriver_PulseAudio.cpp")
    list(APPEND SMDATA_ARCH_SOUND_HPP "Sound/RageSoundDriver_PulseAudio.h")
  endif()
  
  if (ALSA_FOUND)
    list(APPEND SMDATA_ARCH_SOUND_SRC
      "Sound/ALSA9Dynamic.cpp"
      "Sound/ALSA9Helpers.cpp"
      "Sound/RageSoundDriver_ALSA9_Software.cpp")
    list(APPEND SMDATA_ARCH_SOUND_HPP
      "Sound/ALSA9Dynamic.h"
      "Sound/ALSA9Functions.h"
      "Sound/ALSA9Helpers.h"
      "Sound/RageSoundDriver_ALSA9_Software.h")
  endif()
  
  if (JACK_FOUND)
    list(APPEND SMDATA_ARCH_SOUND_SRC "Sound/RageSoundDriver_JACK.cpp")
    list(APPEND SMDATA_ARCH_SOUND_HPP "Sound/RageSoundDriver_JACK.h")
  endif()
  
  if (OSS_FOUND)
    list(APPEND SMDATA_ARCH_SOUND_SRC "Sound/RageSoundDriver_OSS.cpp")
    list(APPEND SMDATA_ARCH_SOUND_HPP "Sound/RageSoundDriver_OSS.h")
  endif()
endif()
source_group("Arch Specific\\\\Sound" FILES ${SMDATA_ARCH_SOUND_SRC} ${SMDATA_ARCH_SOUND_HPP})

list(APPEND SMDATA_ARCH_MOVIE_TEXTURE_SRC
  "MovieTexture/MovieTexture.cpp"
  "MovieTexture/MovieTexture_Generic.cpp"
  "MovieTexture/MovieTexture_Null.cpp")
list(APPEND SMDATA_ARCH_MOVIE_TEXTURE_HPP
  "MovieTexture/MovieTexture.h"
  "MovieTexture/MovieTexture_Generic.h"
  "MovieTexture/MovieTexture_Null.h")

if(APPLE)
  list(APPEND SMDATA_ARCH_MOVIE_TEXTURE_SRC "MovieTexture/MovieTexture_FFMpeg.cpp")
  list(APPEND SMDATA_ARCH_MOVIE_TEXTURE_HPP "MovieTexture/MovieTexture_FFMpeg.h")
  
elseif(MSVC)
  list(APPEND SMDATA_ARCH_MOVIE_TEXTURE_SRC "MovieTexture/MovieTexture_FFMpeg.cpp")
  list(APPEND SMDATA_ARCH_MOVIE_TEXTURE_HPP "MovieTexture/MovieTexture_FFMpeg.h")
  
else() # Unix
  if (${HAS_FFMPEG})
    list(APPEND SMDATA_ARCH_MOVIE_TEXTURE_SRC "MovieTexture/MovieTexture_FFMpeg.cpp")
    list(APPEND SMDATA_ARCH_MOVIE_TEXTURE_HPP "MovieTexture/MovieTexture_FFMpeg.h")
  endif()
  
endif()
source_group("Arch Specific\\\\Movie Texture" FILES ${SMDATA_ARCH_MOVIE_TEXTURE_SRC} ${SMDATA_ARCH_MOVIE_TEXTURE_HPP})

list(APPEND SMDATA_ARCH_LOWLEVEL_SRC "LowLevelWindow/LowLevelWindow.cpp")
list(APPEND SMDATA_ARCH_LOWLEVEL_HPP "LowLevelWindow/LowLevelWindow.h")
if(WIN32)
  list(APPEND SMDATA_ARCH_LOWLEVEL_SRC "LowLevelWindow/LowLevelWindow_Win32.cpp")
  list(APPEND SMDATA_ARCH_LOWLEVEL_HPP "LowLevelWindow/LowLevelWindow_Win32.h")
  
elseif(APPLE)
  list(APPEND SMDATA_ARCH_LOWLEVEL_SRC "LowLevelWindow/LowLevelWindow_MacOSX.mm")
  list(APPEND SMDATA_ARCH_LOWLEVEL_HPP "LowLevelWindow/LowLevelWindow_MacOSX.h")
  
else(UNIX)
    list(APPEND SMDATA_ARCH_LOWLEVEL_SRC "LowLevelWindow/LowLevelWindow_X11.cpp")
    list(APPEND SMDATA_ARCH_LOWLEVEL_HPP "LowLevelWindow/LowLevelWindow_X11.h")
  
endif(WIN32)
source_group("Arch Specific\\\\Low Level Window" FILES ${SMDATA_ARCH_LOWLEVEL_SRC} ${SMDATA_ARCH_LOWLEVEL_HPP})

list(APPEND SMDATA_ARCH_LOADING_SRC "LoadingWindow/LoadingWindow.cpp")
list(APPEND SMDATA_ARCH_LOADING_HPP "LoadingWindow/LoadingWindow.h")

if(WIN32)
  list(APPEND SMDATA_ARCH_LOADING_SRC "LoadingWindow/LoadingWindow_Win32.cpp")
  list(APPEND SMDATA_ARCH_LOADING_HPP "LoadingWindow/LoadingWindow_Win32.h")
else()
  list(APPEND SMDATA_ARCH_LOADING_HPP "LoadingWindow/LoadingWindow_Null.h")
  
  if(APPLE)
    list(APPEND SMDATA_ARCH_LOADING_SRC "LoadingWindow/LoadingWindow_MacOSX.mm")
    list(APPEND SMDATA_ARCH_LOADING_HPP "LoadingWindow/LoadingWindow_MacOSX.h")
    
  elseif(LINUX)
    
    if (GTK2_FOUND)
      list(APPEND SMDATA_ARCH_LOADING_SRC "LoadingWindow/LoadingWindow_Gtk.cpp")
      list(APPEND SMDATA_ARCH_LOADING_HPP "LoadingWindow/LoadingWindow_Gtk.h")
    endif()
    
  endif()
endif()
source_group("Arch Specific\\\\Loading Window" FILES ${SMDATA_ARCH_LOADING_SRC} ${SMDATA_ARCH_LOADING_HPP})

list(APPEND SMDATA_ARCH_INPUT_SRC "InputHandler/InputHandler.cpp" "InputHandler/InputHandler_MonkeyKeyboard.cpp")
list(APPEND SMDATA_ARCH_INPUT_HPP "InputHandler/InputHandler.h" "InputHandler/InputHandler_MonkeyKeyboard.h")

if(WIN32)
  list(APPEND SMDATA_ARCH_INPUT_SRC
    "InputHandler/InputHandler_DirectInput.cpp"
    "InputHandler/InputHandler_DirectInputHelper.cpp"
    "InputHandler/InputHandler_Win32_MIDI.cpp"
    "InputHandler/InputHandler_Win32_Para.cpp"
    "InputHandler/InputHandler_Win32_Pump.cpp")
  list(APPEND SMDATA_ARCH_INPUT_HPP
    "InputHandler/InputHandler_DirectInput.h"
    "InputHandler/InputHandler_DirectInputHelper.h"
    "InputHandler/InputHandler_Win32_MIDI.h"
    "InputHandler/InputHandler_Win32_Para.h"
    "InputHandler/InputHandler_Win32_Pump.h")
  if (NOT MSVC)
    list(APPEND SMDATA_ARCH_INPUT_SRC "InputHandler/InputHandler_SextetStream.cpp")
    list(APPEND SMDATA_ARCH_INPUT_HPP "InputHandler/InputHandler_SextetStream.h")
  endif()
elseif(APPLE)
  list(APPEND SMDATA_ARCH_INPUT_SRC "InputHandler/InputHandler_MacOSX_HID.cpp")
  list(APPEND SMDATA_ARCH_INPUT_HPP "InputHandler/InputHandler_MacOSX_HID.h")
else() # Unix/Linux
  if (LINUX)
    list(APPEND SMDATA_ARCH_INPUT_SRC
      "InputHandler/LinuxInputManager.cpp"
      "InputHandler/InputHandler_Linux_Joystick.cpp"
      "InputHandler/InputHandler_Linux_Event.cpp"
      "InputHandler/InputHandler_Linux_PIUIO.cpp"
      "InputHandler/InputHandler_SextetStream.cpp")
    list(APPEND SMDATA_ARCH_INPUT_SRC
      "InputHandler/LinuxInputManager.h"
      "InputHandler/InputHandler_Linux_Joystick.h"
      "InputHandler/InputHandler_Linux_Event.h"
      "InputHandler/InputHandler_Linux_PIUIO.h"
      "InputHandler/InputHandler_SextetStream.h")

  endif()
    list(APPEND SMDATA_ARCH_INPUT_SRC "InputHandler/InputHandler_X11.cpp")
    list(APPEND SMDATA_ARCH_INPUT_HPP "InputHandler/InputHandler_X11.h")
  
endif()
source_group("Arch Specific\\\\Input Handler" FILES ${SMDATA_ARCH_INPUT_SRC} ${SMDATA_ARCH_INPUT_HPP})

list(APPEND SMDATA_ARCH_DIALOG_SRC "Dialog/Dialog.cpp" "Dialog/DialogDriver.cpp")
list(APPEND SMDATA_ARCH_DIALOG_HPP "Dialog/Dialog.h" "Dialog/DialogDriver.h")
if(WIN32)
  list(APPEND SMDATA_ARCH_DIALOG_SRC "Dialog/DialogDriver_Win32.cpp")
  list(APPEND SMDATA_ARCH_DIALOG_HPP "Dialog/DialogDriver_Win32.h")
elseif(APPLE)
  list(APPEND SMDATA_ARCH_DIALOG_SRC "Dialog/DialogDriver_MacOSX.cpp")
  list(APPEND SMDATA_ARCH_DIALOG_HPP "Dialog/DialogDriver_MacOSX.h")
endif()

source_group("Arch Specific\\\\Dialog" FILES ${SMDATA_ARCH_DIALOG_SRC} ${SMDATA_ARCH_DIALOG_HPP})

list(APPEND SMDATA_ALL_ARCH_SRC
  ${SMDATA_ARCH_SRC}
  ${SMDATA_ARCH_DIALOG_SRC}
  ${SMDATA_ARCH_INPUT_SRC}
  ${SMDATA_ARCH_LOADING_SRC}
  ${SMDATA_ARCH_LOWLEVEL_SRC}
  ${SMDATA_ARCH_MOVIE_TEXTURE_SRC}
  ${SMDATA_ARCH_SOUND_SRC}
  ${SMDATA_ARCH_THREADS_SRC})
list(APPEND SMDATA_ALL_ARCH_HPP
  ${SMDATA_ARCH_HPP}
  ${SMDATA_ARCH_DIALOG_HPP}
  ${SMDATA_ARCH_INPUT_HPP}
  ${SMDATA_ARCH_LOADING_HPP}
  ${SMDATA_ARCH_LOWLEVEL_HPP}
  ${SMDATA_ARCH_MOVIE_TEXTURE_HPP}
  ${SMDATA_ARCH_SOUND_HPP}
  ${SMDATA_ARCH_THREADS_HPP})

target_sources(Etterna PUBLIC ${SMDATA_ALL_ARCH_SRC} ${SMDATA_ALL_ARCH_HPP})