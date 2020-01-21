list(APPEND SCREEN_GAMEPLAY_SRC
	"Gameplay/ScreenGameplay.cpp"
	"Gameplay/ScreenGameplayNormal.cpp"
	"Gameplay/ScreenGameplaySyncMachine.cpp"
	"Gameplay/ScreenGameplayReplay.cpp"
	"Gameplay/ScreenGameplayPractice.cpp")
list(APPEND SCREEN_GAMEPLAY_HPP
	"Gameplay/ScreenGameplay.h"
	"Gameplay/ScreenGameplayNormal.h"
	"Gameplay/ScreenGameplaySyncMachine.h"
	"Gameplay/ScreenGameplayReplay.h"
	"Gameplay/ScreenGameplayPractice.h")

list(APPEND SCREEN_OPTION_SRC
	"Options/ScreenOptions.cpp"
	"Options/ScreenOptionsEditProfile.cpp"
	"Options/ScreenOptionsManageProfiles.cpp"
	"Options/ScreenOptionsMaster.cpp"
	"Options/ScreenOptionsMasterPrefs.cpp")
list(APPEND SCREEN_OPTION_HPP
	"Options/ScreenOptions.h"
	"Options/ScreenOptionsEditProfile.h"
	"Options/ScreenOptionsManageProfiles.h"
	"Options/ScreenOptionsMaster.h"
	"Options/ScreenOptionsMasterPrefs.h")

list(APPEND SCREEN_REST_SRC
	"Others/Screen.cpp"
	"Others/ScreenDebugOverlay.cpp"
	"Others/ScreenEvaluation.cpp"
	"Others/ScreenExit.cpp"
	"Others/ScreenInstallOverlay.cpp"
	"Others/ScreenMapControllers.cpp"
	"Others/ScreenMessage.cpp"
	"Others/ScreenMiniMenu.cpp"
	"Others/ScreenPlayerOptions.cpp"
	"Others/ScreenProfileLoad.cpp"
	"Others/ScreenProfileSave.cpp"
	"Others/ScreenPrompt.cpp"
	"Others/ScreenSaveSync.cpp"
	"Others/ScreenSelect.cpp"
	"Others/ScreenSelectLanguage.cpp"
	"Others/ScreenSelectMaster.cpp"
	"Others/ScreenSelectMusic.cpp"
	"Others/ScreenSelectProfile.cpp"
	"Others/ScreenServiceAction.cpp"
	"Others/ScreenSongOptions.cpp"
	"Others/ScreenSplash.cpp"
	"Others/ScreenStatsOverlay.cpp"
	"Others/ScreenSyncOverlay.cpp"
	"Others/ScreenSystemLayer.cpp"
	"Others/ScreenTestInput.cpp"
	"Others/ScreenTestSound.cpp"
	"Others/ScreenTextEntry.cpp"
	"Others/ScreenTitleMenu.cpp"
	"Others/ScreenWithMenuElements.cpp")
list(APPEND SCREEN_REST_HPP
	"Others/Screen.h"
	"Others/ScreenDebugOverlay.h"
	"Others/ScreenEvaluation.h"
	"Others/ScreenExit.h"
	"Others/ScreenInstallOverlay.h"
	"Others/ScreenMapControllers.h"
	"Others/ScreenMessage.h"
	"Others/ScreenMiniMenu.h"
	"Others/ScreenPlayerOptions.h"
	"Others/ScreenProfileLoad.h"
	"Others/ScreenProfileSave.h"
	"Others/ScreenPrompt.h"
	"Others/ScreenSaveSync.h"
	"Others/ScreenSelect.h"
	"Others/ScreenSelectLanguage.h"
	"Others/ScreenSelectMaster.h"
	"Others/ScreenSelectMusic.h"
	"Others/ScreenSelectProfile.h"
	"Others/ScreenServiceAction.h"
	"Others/ScreenSongOptions.h"
	"Others/ScreenSplash.h"
	"Others/ScreenStatsOverlay.h"
	"Others/ScreenSyncOverlay.h"
	"Others/ScreenSystemLayer.h"
	"Others/ScreenTestInput.h"
	"Others/ScreenTestSound.h"
	"Others/ScreenTextEntry.h"
	"Others/ScreenTitleMenu.h"
	"Others/ScreenWithMenuElements.h")

list(APPEND SCREEN_NET_SRC
	"Network/ScreenNetEvaluation.cpp"
	"Network/ScreenNetRoom.cpp"
	"Network/ScreenNetSelectBase.cpp"
	"Network/ScreenNetSelectMusic.cpp"
	"Network/ScreenNetworkOptions.cpp"
	"Network/ScreenSMOnlineLogin.cpp")

list(APPEND SCREEN_NET_HPP
	"Network/ScreenNetEvaluation.h"
	"Network/ScreenNetRoom.h"
	"Network/ScreenNetSelectBase.h"
	"Network/ScreenNetSelectMusic.h"
	"Network/ScreenNetworkOptions.h"
	"Network/ScreenSMOnlineLogin.h")

list(APPEND ALL_SCREENS_SRC
	${SCREEN_GAMEPLAY_SRC}
	${SCREEN_OPTION_SRC}
	${SCREEN_NET_SRC}
	${SCREEN_REST_SRC})

list(APPEND ALL_SCREENS_HPP
	${SCREEN_GAMEPLAY_HPP}
	${SCREEN_OPTION_HPP}
	${SCREEN_NET_HPP}
	${SCREEN_REST_HPP})

target_sources(Etterna PUBLIC ${ALL_SCREENS_SRC} ${ALL_SCREENS_HPP})
