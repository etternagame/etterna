list(APPEND SMDATA_SCREEN_GAMEPLAY_SRC
  "ScreenGameplay.cpp"
  "ScreenGameplayNormal.cpp"
  "ScreenGameplaySyncMachine.cpp"
)

list(APPEND SMDATA_SCREEN_GAMEPLAY_HPP
  "ScreenGameplay.h"
  "ScreenGameplayNormal.h"
  "ScreenGameplaySyncMachine.h"
)

source_group("Screens\\\\Gameplay" FILES ${SMDATA_SCREEN_GAMEPLAY_SRC} ${SMDATA_SCREEN_GAMEPLAY_HPP})

list(APPEND SMDATA_SCREEN_OPTION_SRC
  "ScreenOptions.cpp"
  "ScreenOptionsEditProfile.cpp"
  "ScreenOptionsExportPackage.cpp"
  "ScreenOptionsManageProfiles.cpp"
  "ScreenOptionsMaster.cpp"
  "ScreenOptionsMasterPrefs.cpp"
  "ScreenOptionsToggleSongs.cpp"
)
list(APPEND SMDATA_SCREEN_OPTION_HPP
  "ScreenOptions.h"
  "ScreenOptionsEditProfile.h"
  "ScreenOptionsExportPackage.h"
  "ScreenOptionsManageProfiles.h"
  "ScreenOptionsMaster.h"
  "ScreenOptionsMasterPrefs.h"
  "ScreenOptionsToggleSongs.h"
)

source_group("Screens\\\\Options" FILES ${SMDATA_SCREEN_OPTION_SRC} ${SMDATA_SCREEN_OPTION_HPP})

list(APPEND SMDATA_SCREEN_REST_SRC
  "Screen.cpp"
  "ScreenDebugOverlay.cpp"
  "ScreenEvaluation.cpp"
  "ScreenExit.cpp"
  "ScreenInstallOverlay.cpp"
  "ScreenMapControllers.cpp"
  "ScreenMessage.cpp"
  "ScreenMiniMenu.cpp"
  "ScreenPackages.cpp"
  "ScreenPlayerOptions.cpp"
  "ScreenProfileLoad.cpp"
  "ScreenProfileSave.cpp"
  "ScreenPrompt.cpp"
  "ScreenSaveSync.cpp"
  "ScreenSelect.cpp"
  "ScreenSelectLanguage.cpp"
  "ScreenSelectMaster.cpp"
  "ScreenSelectMusic.cpp"
  "ScreenSelectProfile.cpp"
  "ScreenServiceAction.cpp"
  "ScreenSongOptions.cpp"
  "ScreenSplash.cpp"
  "ScreenStatsOverlay.cpp"
  "ScreenSyncOverlay.cpp"
  "ScreenSystemLayer.cpp"
  "ScreenTestInput.cpp"
  "ScreenTestSound.cpp"
  "ScreenTextEntry.cpp"
  "ScreenTitleMenu.cpp"
  "ScreenWithMenuElements.cpp"
)
list(APPEND SMDATA_SCREEN_REST_HPP
  "Screen.h"
  "ScreenDebugOverlay.h"
  "ScreenEvaluation.h"
  "ScreenExit.h"
  "ScreenInstallOverlay.h"
  "ScreenMapControllers.h"
  "ScreenMessage.h"
  "ScreenMiniMenu.h"
  "ScreenPackages.h"
  "ScreenPlayerOptions.h"
  "ScreenProfileLoad.h"
  "ScreenProfileSave.h"
  "ScreenPrompt.h"
  "ScreenSaveSync.h"
  "ScreenSelect.h"
  "ScreenSelectLanguage.h"
  "ScreenSelectMaster.h"
  "ScreenSelectMusic.h"
  "ScreenSelectProfile.h"
  "ScreenServiceAction.h"
  "ScreenSongOptions.h"
  "ScreenSplash.h"
  "ScreenStatsOverlay.h"
  "ScreenSyncOverlay.h"
  "ScreenSystemLayer.h"
  "ScreenTestInput.h"

  "ScreenTestSound.h"
  "ScreenTextEntry.h"
  "ScreenTitleMenu.h"
  "ScreenWithMenuElements.h"
)

source_group("Screens\\\\Others" FILES ${SMDATA_SCREEN_REST_SRC} ${SMDATA_SCREEN_REST_HPP})

list(APPEND SMDATA_SCREEN_NET_SRC
  "ScreenNetEvaluation.cpp"
  "ScreenNetRoom.cpp"
  "ScreenNetSelectBase.cpp"
  "ScreenNetSelectMusic.cpp"
  "ScreenNetworkOptions.cpp"
)

list(APPEND SMDATA_SCREEN_NET_HPP
  "ScreenNetEvaluation.h"
  "ScreenNetRoom.h"
  "ScreenNetSelectBase.h"
  "ScreenNetSelectMusic.h"
  "ScreenNetworkOptions.h"
)

if (WITH_NETWORKING)
  list(APPEND SMDATA_SCREEN_NET_SRC
    "ScreenSMOnlineLogin.cpp"
  )
  list(APPEND SMDATA_SCREEN_NET_HPP
    "ScreenSMOnlineLogin.h"
  )
endif()

source_group("Screens\\\\Network" FILES ${SMDATA_SCREEN_NET_SRC} ${SMDATA_SCREEN_NET_HPP})

list(APPEND SMDATA_ALL_SCREENS_SRC
  ${SMDATA_SCREEN_GAMEPLAY_SRC}
  ${SMDATA_SCREEN_OPTION_SRC}
  ${SMDATA_SCREEN_NET_SRC}
  ${SMDATA_SCREEN_REST_SRC}
)

list(APPEND SMDATA_ALL_SCREENS_HPP
  ${SMDATA_SCREEN_GAMEPLAY_HPP}
  ${SMDATA_SCREEN_OPTION_HPP}
  ${SMDATA_SCREEN_NET_HPP}
  ${SMDATA_SCREEN_REST_HPP}
)
