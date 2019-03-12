list(APPEND SMDATA_GLOBAL_SINGLETON_SRC
  "Etterna/Singletons/AnnouncerManager.cpp"
  "Etterna/Singletons/CharacterManager.cpp"
  "Etterna/Singletons/CommandLineActions.cpp"
  "Etterna/Singletons/CryptManager.cpp"
  "Etterna/Singletons/FilterManager.cpp"
  "Etterna/Singletons/FontManager.cpp"
  "Etterna/Singletons/GameManager.cpp"
  "Etterna/Singletons/GameSoundManager.cpp"
  "Etterna/Singletons/GameState.cpp"
  "Etterna/Singletons/InputFilter.cpp"
  "Etterna/Singletons/InputMapper.cpp"
  "Etterna/Singletons/InputQueue.cpp"
  "Etterna/Singletons/LuaManager.cpp"
  "Etterna/Singletons/MessageManager.cpp"
  "Etterna/Singletons/NetworkSyncManager.cpp"
  "Etterna/Singletons/NoteSkinManager.cpp"
  "Etterna/Singletons/PrefsManager.cpp"
  "Etterna/Singletons/ProfileManager.cpp"
  "Etterna/Singletons/ScoreManager.cpp"
  "Etterna/Singletons/ScreenManager.cpp"
  "Etterna/Singletons/SongManager.cpp"
  "Etterna/Singletons/StatsManager.cpp"
  "Etterna/Singletons/ThemeManager.cpp"
  "Etterna/Singletons/DownloadManager.cpp"
)
list(APPEND SMDATA_GLOBAL_SINGLETON_HPP
  "Etterna/Singletons/AnnouncerManager.h"
  "Etterna/Singletons/CharacterManager.h"
  "Etterna/Singletons/CommandLineActions.h"
  "Etterna/Singletons/CryptManager.h"
  "Etterna/Singletons/FilterManager.h"
  "Etterna/Singletons/FontManager.h"
  "Etterna/Singletons/GameManager.h"
  "Etterna/Singletons/GameSoundManager.h"
  "Etterna/Singletons/GameState.h"
  "Etterna/Singletons/InputFilter.h"
  "Etterna/Singletons/InputMapper.h"
  "Etterna/Singletons/InputQueue.h"
  "Etterna/Singletons/LuaManager.h"
  "Etterna/Singletons/MessageManager.h"
  "Etterna/Singletons/NetworkSyncManager.h"
  "Etterna/Singletons/NoteSkinManager.h"
  "Etterna/Singletons/PrefsManager.h"
  "Etterna/Singletons/ProfileManager.h"
  "Etterna/Singletons/ScoreManager.h"
  "Etterna/Singletons/ScreenManager.h"
  "Etterna/Singletons/SongManager.h"
  "Etterna/Singletons/StatsManager.h"
  "Etterna/Singletons/ThemeManager.h"
  "Etterna/Singletons/DownloadManager.h"
)

if(WITH_NETWORKING)
  list(APPEND SMDATA_GLOBAL_SINGLETON_SRC
    "Etterna/Singletons/ezsockets.cpp"
  )
  list(APPEND SMDATA_GLOBAL_SINGLETON_HPP
    "Etterna/Singletons/ezsockets.h"
  )
endif()

source_group("Global Singletons" FILES ${SMDATA_GLOBAL_SINGLETON_SRC} ${SMDATA_GLOBAL_SINGLETON_HPP})
