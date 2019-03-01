list(APPEND SMDATA_GLOBAL_SINGLETON_SRC
  "AnnouncerManager.cpp"
  "CharacterManager.cpp"
  "CommandLineActions.cpp"
  "CryptManager.cpp"
  "FilterManager.cpp"
  "FontManager.cpp"
  "GameManager.cpp"
  "GameSoundManager.cpp"
  "GameState.cpp"
  "InputFilter.cpp"
  "InputMapper.cpp"
  "InputQueue.cpp"
  "LuaManager.cpp"
  "MessageManager.cpp"
  "NetworkSyncManager.cpp"
  "NoteSkinManager.cpp"
  "PrefsManager.cpp"
  "ProfileManager.cpp"
  "ScoreManager.cpp"
  "ScreenManager.cpp"
  "SongManager.cpp"
  "StatsManager.cpp"
  "ThemeManager.cpp"
  "DownloadManager.cpp"
)
list(APPEND SMDATA_GLOBAL_SINGLETON_HPP
  "AnnouncerManager.h"
  "CharacterManager.h"
  "CommandLineActions.h"
  "CryptManager.h"
  "FilterManager.h"
  "FontManager.h"
  "GameManager.h"
  "GameSoundManager.h"
  "GameState.h"
  "InputFilter.h"
  "InputMapper.h"
  "InputQueue.h"
  "LuaManager.h"
  "MessageManager.h"
  "NetworkSyncManager.h"
  "NoteSkinManager.h"
  "PrefsManager.h"
  "ProfileManager.h"
  "ScoreManager.h"
  "ScreenManager.h"
  "SongManager.h"
  "StatsManager.h"
  "ThemeManager.h"
  "DownloadManager.h"
)

if(WITH_NETWORKING)
  list(APPEND SMDATA_GLOBAL_SINGLETON_SRC
    "ezsockets.cpp"
  )
  list(APPEND SMDATA_GLOBAL_SINGLETON_HPP
    "ezsockets.h"
  )
endif()

source_group("Global Singletons" FILES ${SMDATA_GLOBAL_SINGLETON_SRC} ${SMDATA_GLOBAL_SINGLETON_HPP})
