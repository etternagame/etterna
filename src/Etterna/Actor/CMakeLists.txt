list(APPEND ACTOR_BASE_SRC
	"Base/Actor.cpp"
	"Base/ActorFrame.cpp"
	"Base/ActorFrameTexture.cpp"
	"Base/ActorMultiVertex.cpp"
	"Base/ActorProxy.cpp"
	"Base/ActorScroller.cpp"
	"Base/ActorSound.cpp"
	"Base/ActorUtil.cpp"
	"Base/AutoActor.cpp"
	"Base/BitmapText.cpp"
	"Base/Model.cpp"
	"Base/ModelManager.cpp"
	"Base/ModelTypes.cpp"
	"Base/Quad.cpp"
	"Base/RollingNumbers.cpp"
	"Base/Sprite.cpp"
	"Base/Tween.cpp")
list(APPEND ACTOR_BASE_HPP
	"Base/Actor.h"
	"Base/ActorFrame.h"
	"Base/ActorFrameTexture.h"
	"Base/ActorMultiVertex.h"
	"Base/ActorProxy.h"
	"Base/ActorScroller.h"
	"Base/ActorSound.h"
	"Base/ActorUtil.h"
	"Base/AutoActor.h"
	"Base/BitmapText.h"
	"Base/Model.h"
	"Base/ModelManager.h"
	"Base/ModelTypes.h"
	"Base/Quad.h"
	"Base/RollingNumbers.h"
	"Base/Sprite.h"
	"Base/Tween.h")

list(APPEND ACTOR_GAMEPLAY_SRC
	"Gameplay/ArrowEffects.cpp"
	"Gameplay/Background.cpp"
	"Gameplay/Foreground.cpp"
	"Gameplay/GhostArrowRow.cpp"
	"Gameplay/HoldJudgment.cpp"
	"Gameplay/LifeMeter.cpp"
	"Gameplay/LifeMeterBar.cpp"
	"Gameplay/LyricDisplay.cpp"
	"Gameplay/NoteDisplay.cpp"
	"Gameplay/NoteField.cpp"
	"Gameplay/NoteFieldPreview.cpp"
	"Gameplay/Player.cpp"
	"Gameplay/PlayerPractice.cpp"
	"Gameplay/PlayerReplay.cpp"
	"Gameplay/ReceptorArrow.cpp"
	"Gameplay/ReceptorArrowRow.cpp")

list(APPEND ACTOR_GAMEPLAY_HPP
	"Gameplay/ArrowEffects.h"
	"Gameplay/Background.h"
	"Gameplay/Foreground.h"
	"Gameplay/GhostArrowRow.h"
	"Gameplay/HoldJudgment.h"
	"Gameplay/LifeMeter.h"
	"Gameplay/LifeMeterBar.h"
	"Gameplay/LyricDisplay.h"
	"Gameplay/NoteDisplay.h"
	"Gameplay/NoteField.h"
	"Gameplay/NoteFieldPreview.h"
	"Gameplay/Player.h"
	"Gameplay/PlayerPractice.h"
	"Gameplay/PlayerReplay.h"
	"Gameplay/ReceptorArrow.h"
	"Gameplay/ReceptorArrowRow.h")

list(APPEND ACTOR_MENU_SRC
	"Menus/BPMDisplay.cpp"
	"Menus/ComboGraph.cpp"
	"Menus/DifficultyList.cpp"
	"Menus/DualScrollBar.cpp"
	"Menus/GraphDisplay.cpp"
	"Menus/MenuTimer.cpp"
	"Menus/ModIcon.cpp"
	"Menus/ModIconRow.cpp"
	"Menus/MusicWheel.cpp"
	"Menus/MusicWheelItem.cpp"
	"Menus/OptionRow.cpp"
	"Menus/OptionsCursor.cpp"
	"Menus/OptionsList.cpp"
	"Menus/ScrollBar.cpp"
	"Menus/TextBanner.cpp"
	"Menus/WheelBase.cpp"
	"Menus/WheelItemBase.cpp"
	"Menus/RoomInfoDisplay.cpp")

list(APPEND ACTOR_MENU_HPP
	"Menus/BPMDisplay.h"
	"Menus/ComboGraph.h"
	"Menus/DifficultyList.h"
	"Menus/DualScrollBar.h"
	"Menus/GraphDisplay.h"
	"Menus/MenuTimer.h"
	"Menus/ModIcon.h"
	"Menus/ModIconRow.h"
	"Menus/MusicWheel.h"
	"Menus/MusicWheelItem.h"
	"Menus/OptionRow.h"
	"Menus/OptionsCursor.h"
	"Menus/OptionsList.h"
	"Menus/ScrollBar.h"
	"Menus/TextBanner.h"
	"Menus/WheelBase.h"
	"Menus/WheelItemBase.h"
	"Menus/RoomInfoDisplay.h")

list(APPEND ACTOR_GAMEPLAY_MENU_SRC
	"GameplayAndMenus/BGAnimation.cpp"
	"GameplayAndMenus/BGAnimationLayer.cpp"
	"GameplayAndMenus/MeterDisplay.cpp"
	"GameplayAndMenus/StepsDisplay.cpp"
	"GameplayAndMenus/StreamDisplay.cpp"
	"GameplayAndMenus/Transition.cpp")

list(APPEND ACTOR_GAMEPLAY_MENU_HPP
	"GameplayAndMenus/BGAnimation.h"
	"GameplayAndMenus/BGAnimationLayer.h"
	"GameplayAndMenus/MeterDisplay.h"
	"GameplayAndMenus/StepsDisplay.h"
	"GameplayAndMenus/StreamDisplay.h"
	"GameplayAndMenus/Transition.h")

list(APPEND ALL_ACTORS_SRC
	${ACTOR_BASE_SRC}
	${ACTOR_GAMEPLAY_SRC}
	${ACTOR_MENU_SRC}
	${ACTOR_GAMEPLAY_MENU_SRC})

list(APPEND ALL_ACTORS_HPP
	${ACTOR_BASE_HPP}
	${ACTOR_GAMEPLAY_HPP}
	${ACTOR_MENU_HPP}
	${ACTOR_GAMEPLAY_MENU_HPP})

target_sources(Etterna PUBLIC ${ALL_ACTORS_SRC} ${ALL_ACTORS_HPP})
