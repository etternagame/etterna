list(APPEND SMDATA_SCREEN_GAMEPLAY_SRC
  "Etterna/Screen/Gameplay/ScreenGameplay.cpp"
  "Etterna/Screen/Gameplay/ScreenGameplayNormal.cpp"
  "Etterna/Screen/Gameplay/ScreenGameplaySyncMachine.cpp"
)

list(APPEND SMDATA_SCREEN_GAMEPLAY_HPP
  "Etterna/Screen/Gameplay/ScreenGameplay.h"
  "Etterna/Screen/Gameplay/ScreenGameplayNormal.h"
  "Etterna/Screen/Gameplay/ScreenGameplaySyncMachine.h"
)

source_group("Screens\\\\Gameplay" FILES ${SMDATA_SCREEN_GAMEPLAY_SRC} ${SMDATA_SCREEN_GAMEPLAY_HPP})

list(APPEND SMDATA_SCREEN_OPTION_SRC
  "Etterna/Screen/Options/ScreenOptions.cpp"
  "Etterna/Screen/Options/ScreenOptionsEditProfile.cpp"
  "Etterna/Screen/Options/ScreenOptionsManageProfiles.cpp"
  "Etterna/Screen/Options/ScreenOptionsMaster.cpp"
  "Etterna/Screen/Options/ScreenOptionsMasterPrefs.cpp"
)
list(APPEND SMDATA_SCREEN_OPTION_HPP
  "Etterna/Screen/Options/ScreenOptions.h"
  "Etterna/Screen/Options/ScreenOptionsEditProfile.h"
  "Etterna/Screen/Options/ScreenOptionsManageProfiles.h"
  "Etterna/Screen/Options/ScreenOptionsMaster.h"
  "Etterna/Screen/Options/ScreenOptionsMasterPrefs.h"
)

source_group("Screens\\\\Options" FILES ${SMDATA_SCREEN_OPTION_SRC} ${SMDATA_SCREEN_OPTION_HPP})

list(APPEND SMDATA_SCREEN_REST_SRC
  "Etterna/Screen/Others/Screen.cpp"
  "Etterna/Screen/Others/ScreenDebugOverlay.cpp"
  "Etterna/Screen/Others/ScreenEvaluation.cpp"
  "Etterna/Screen/Others/ScreenExit.cpp"
  "Etterna/Screen/Others/ScreenInstallOverlay.cpp"
  "Etterna/Screen/Others/ScreenMapControllers.cpp"
  "Etterna/Screen/Others/ScreenMessage.cpp"
  "Etterna/Screen/Others/ScreenMiniMenu.cpp"
  "Etterna/Screen/Others/ScreenPlayerOptions.cpp"
  "Etterna/Screen/Others/ScreenProfileLoad.cpp"
  "Etterna/Screen/Others/ScreenProfileSave.cpp"
  "Etterna/Screen/Others/ScreenPrompt.cpp"
  "Etterna/Screen/Others/ScreenSaveSync.cpp"
  "Etterna/Screen/Others/ScreenSelect.cpp"
  "Etterna/Screen/Others/ScreenSelectLanguage.cpp"
  "Etterna/Screen/Others/ScreenSelectMaster.cpp"
  "Etterna/Screen/Others/ScreenSelectMusic.cpp"
  "Etterna/Screen/Others/ScreenSelectProfile.cpp"
  "Etterna/Screen/Others/ScreenServiceAction.cpp"
  "Etterna/Screen/Others/ScreenSongOptions.cpp"
  "Etterna/Screen/Others/ScreenSplash.cpp"
  "Etterna/Screen/Others/ScreenStatsOverlay.cpp"
  "Etterna/Screen/Others/ScreenSyncOverlay.cpp"
  "Etterna/Screen/Others/ScreenSystemLayer.cpp"
  "Etterna/Screen/Others/ScreenTestInput.cpp"
  "Etterna/Screen/Others/ScreenTestSound.cpp"
  "Etterna/Screen/Others/ScreenTextEntry.cpp"
  "Etterna/Screen/Others/ScreenTitleMenu.cpp"
  "Etterna/Screen/Others/ScreenWithMenuElements.cpp"
)
list(APPEND SMDATA_SCREEN_REST_HPP
  "Etterna/Screen/Others/Screen.h"
  "Etterna/Screen/Others/ScreenDebugOverlay.h"
  "Etterna/Screen/Others/ScreenEvaluation.h"
  "Etterna/Screen/Others/ScreenExit.h"
  "Etterna/Screen/Others/ScreenInstallOverlay.h"
  "Etterna/Screen/Others/ScreenMapControllers.h"
  "Etterna/Screen/Others/ScreenMessage.h"
  "Etterna/Screen/Others/ScreenMiniMenu.h"
  "Etterna/Screen/Others/ScreenPlayerOptions.h"
  "Etterna/Screen/Others/ScreenProfileLoad.h"
  "Etterna/Screen/Others/ScreenProfileSave.h"
  "Etterna/Screen/Others/ScreenPrompt.h"
  "Etterna/Screen/Others/ScreenSaveSync.h"
  "Etterna/Screen/Others/ScreenSelect.h"
  "Etterna/Screen/Others/ScreenSelectLanguage.h"
  "Etterna/Screen/Others/ScreenSelectMaster.h"
  "Etterna/Screen/Others/ScreenSelectMusic.h"
  "Etterna/Screen/Others/ScreenSelectProfile.h"
  "Etterna/Screen/Others/ScreenServiceAction.h"
  "Etterna/Screen/Others/ScreenSongOptions.h"
  "Etterna/Screen/Others/ScreenSplash.h"
  "Etterna/Screen/Others/ScreenStatsOverlay.h"
  "Etterna/Screen/Others/ScreenSyncOverlay.h"
  "Etterna/Screen/Others/ScreenSystemLayer.h"
  "Etterna/Screen/Others/ScreenTestInput.h"

  "Etterna/Screen/Others/ScreenTestSound.h"
  "Etterna/Screen/Others/ScreenTextEntry.h"
  "Etterna/Screen/Others/ScreenTitleMenu.h"
  "Etterna/Screen/Others/ScreenWithMenuElements.h"
)

source_group("Screens\\\\Others" FILES ${SMDATA_SCREEN_REST_SRC} ${SMDATA_SCREEN_REST_HPP})

list(APPEND SMDATA_SCREEN_NET_SRC
  "Etterna/Screen/Network/ScreenNetEvaluation.cpp"
  "Etterna/Screen/Network/ScreenNetRoom.cpp"
  "Etterna/Screen/Network/ScreenNetSelectBase.cpp"
  "Etterna/Screen/Network/ScreenNetSelectMusic.cpp"
  "Etterna/Screen/Network/ScreenNetworkOptions.cpp"
  "Etterna/Screen/Network/ScreenSMOnlineLogin.cpp"
)

list(APPEND SMDATA_SCREEN_NET_HPP
  "Etterna/Screen/Network/ScreenNetEvaluation.h"
  "Etterna/Screen/Network/ScreenNetRoom.h"
  "Etterna/Screen/Network/ScreenNetSelectBase.h"
  "Etterna/Screen/Network/ScreenNetSelectMusic.h"
  "Etterna/Screen/Network/ScreenNetworkOptions.h"
  "Etterna/Screen/Network/ScreenSMOnlineLogin.h"
)

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
