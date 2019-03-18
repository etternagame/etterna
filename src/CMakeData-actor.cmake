list(APPEND SMDATA_ACTOR_BASE_SRC
  "Etterna/Actor/Base/Actor.cpp"
  "Etterna/Actor/Base/ActorFrame.cpp"
  "Etterna/Actor/Base/ActorFrameTexture.cpp"
  "Etterna/Actor/Base/ActorMultiVertex.cpp"
  "Etterna/Actor/Base/ActorScroller.cpp"
  "Etterna/Actor/Base/ActorSound.cpp"
  "Etterna/Actor/Base/ActorUtil.cpp"
  "Etterna/Actor/Base/AutoActor.cpp"
  "Etterna/Actor/Base/BitmapText.cpp"
  "Etterna/Actor/Base/Model.cpp"
  "Etterna/Actor/Base/ModelManager.cpp"
  "Etterna/Actor/Base/ModelTypes.cpp"
  "Etterna/Actor/Base/Quad.cpp"
  "Etterna/Actor/Base/RollingNumbers.cpp"
  "Etterna/Actor/Base/Sprite.cpp"
  "Etterna/Actor/Base/Tween.cpp"
)
list(APPEND SMDATA_ACTOR_BASE_HPP
  "Etterna/Actor/Base/Actor.h"
  "Etterna/Actor/Base/ActorFrame.h"
  "Etterna/Actor/Base/ActorFrameTexture.h"
  "Etterna/Actor/Base/ActorMultiVertex.h"
  "Etterna/Actor/Base/ActorScroller.h"
  "Etterna/Actor/Base/ActorSound.h"
  "Etterna/Actor/Base/ActorUtil.h"
  "Etterna/Actor/Base/AutoActor.h"
  "Etterna/Actor/Base/BitmapText.h"
  "Etterna/Actor/Base/Model.h"
  "Etterna/Actor/Base/ModelManager.h"
  "Etterna/Actor/Base/ModelTypes.h"
  "Etterna/Actor/Base/Quad.h"
  "Etterna/Actor/Base/RollingNumbers.h"
  "Etterna/Actor/Base/Sprite.h"
  "Etterna/Actor/Base/Tween.h"
)

source_group("Actors\\\\Base" FILES ${SMDATA_ACTOR_BASE_SRC} ${SMDATA_ACTOR_BASE_HPP})

list(APPEND SMDATA_ACTOR_GAMEPLAY_SRC
  "Etterna/Actor/Gameplay/ArrowEffects.cpp"
  "Etterna/Actor/Gameplay/Background.cpp"
  "Etterna/Actor/Gameplay/DancingCharacters.cpp"
  "Etterna/Actor/Gameplay/Foreground.cpp"
  "Etterna/Actor/Gameplay/GhostArrowRow.cpp"
  "Etterna/Actor/Gameplay/HoldJudgment.cpp"
  "Etterna/Actor/Gameplay/LifeMeter.cpp"
  "Etterna/Actor/Gameplay/LifeMeterBar.cpp"
  "Etterna/Actor/Gameplay/LyricDisplay.cpp"
  "Etterna/Actor/Gameplay/NoteDisplay.cpp"
  "Etterna/Actor/Gameplay/NoteField.cpp"
  "Etterna/Actor/Gameplay/Player.cpp"
  "Etterna/Actor/Gameplay/ReceptorArrow.cpp"
  "Etterna/Actor/Gameplay/ReceptorArrowRow.cpp"
)

list(APPEND SMDATA_ACTOR_GAMEPLAY_HPP
  "Etterna/Actor/Gameplay/ArrowEffects.h"
  "Etterna/Actor/Gameplay/Background.h"
  "Etterna/Actor/Gameplay/DancingCharacters.h"
  "Etterna/Actor/Gameplay/Foreground.h"
  "Etterna/Actor/Gameplay/GhostArrowRow.h"
  "Etterna/Actor/Gameplay/HoldJudgment.h"
  "Etterna/Actor/Gameplay/LifeMeter.h"
  "Etterna/Actor/Gameplay/LifeMeterBar.h"
  "Etterna/Actor/Gameplay/LyricDisplay.h"
  "Etterna/Actor/Gameplay/NoteDisplay.h"
  "Etterna/Actor/Gameplay/NoteField.h"
  "Etterna/Actor/Gameplay/Player.h"
  "Etterna/Actor/Gameplay/ReceptorArrow.h"
  "Etterna/Actor/Gameplay/ReceptorArrowRow.h"
)
source_group("Actors\\\\Gameplay" FILES ${SMDATA_ACTOR_GAMEPLAY_SRC} ${SMDATA_ACTOR_GAMEPLAY_HPP})

list(APPEND SMDATA_ACTOR_MENU_SRC
  "Etterna/Actor/Menus/BPMDisplay.cpp"
  "Etterna/Actor/Menus/ComboGraph.cpp"
  "Etterna/Models/Misc/ControllerStateDisplay.cpp"
  "Etterna/Actor/Menus/DifficultyList.cpp"
  "Etterna/Actor/Menus/DualScrollBar.cpp"
  "Etterna/Actor/Menus/GraphDisplay.cpp"
  "Etterna/Actor/Menus/MenuTimer.cpp"
  "Etterna/Actor/Menus/ModIcon.cpp"
  "Etterna/Actor/Menus/ModIconRow.cpp"
  "Etterna/Actor/Menus/MusicWheel.cpp"
  "Etterna/Actor/Menus/MusicWheelItem.cpp"
  "Etterna/Actor/Menus/OptionRow.cpp"
  "Etterna/Actor/Menus/OptionsCursor.cpp"
  "Etterna/Actor/Menus/OptionsList.cpp"
  "Etterna/Actor/Menus/ScrollBar.cpp"
  "Etterna/Actor/Menus/TextBanner.cpp"
  "Etterna/Actor/Menus/WheelBase.cpp"
  "Etterna/Actor/Menus/WheelItemBase.cpp"
  "Etterna/Actor/Menus/RoomInfoDisplay.cpp"
)
list(APPEND SMDATA_ACTOR_MENU_HPP
  "Etterna/Actor/Menus/BPMDisplay.h"
  "Etterna/Actor/Menus/ComboGraph.h"
  "Etterna/Models/Misc/ControllerStateDisplay.h"
  "Etterna/Actor/Menus/DifficultyList.h"
  "Etterna/Actor/Menus/DualScrollBar.h"
  "Etterna/Actor/Menus/GraphDisplay.h"
  "Etterna/Actor/Menus/MenuTimer.h"
  "Etterna/Actor/Menus/ModIcon.h"
  "Etterna/Actor/Menus/ModIconRow.h"
  "Etterna/Actor/Menus/MusicWheel.h"
  "Etterna/Actor/Menus/MusicWheelItem.h"
  "Etterna/Actor/Menus/OptionRow.h"
  "Etterna/Actor/Menus/OptionsCursor.h"
  "Etterna/Actor/Menus/OptionsList.h"
  "Etterna/Actor/Menus/ScrollBar.h"
  "Etterna/Actor/Menus/TextBanner.h"
  "Etterna/Actor/Menus/WheelBase.h"
  "Etterna/Actor/Menus/WheelItemBase.h"
  "Etterna/Actor/Menus/RoomInfoDisplay.h"
)


source_group("Actors\\\\Menus" FILES ${SMDATA_ACTOR_MENU_SRC} ${SMDATA_ACTOR_MENU_HPP})

list(APPEND SMDATA_ACTOR_GAMEPLAY_MENU_SRC
  "Etterna/Actor/GameplayAndMenus/BGAnimation.cpp"
  "Etterna/Actor/GameplayAndMenus/BGAnimationLayer.cpp"
  "Etterna/Actor/GameplayAndMenus/MeterDisplay.cpp"
  "Etterna/Actor/GameplayAndMenus/StepsDisplay.cpp"
  "Etterna/Actor/GameplayAndMenus/StreamDisplay.cpp"
  "Etterna/Actor/GameplayAndMenus/Transition.cpp"
)

list(APPEND SMDATA_ACTOR_GAMEPLAY_MENU_HPP
  "Etterna/Actor/GameplayAndMenus/BGAnimation.h"
  "Etterna/Actor/GameplayAndMenus/BGAnimationLayer.h"
  "Etterna/Actor/GameplayAndMenus/MeterDisplay.h"
  "Etterna/Actor/GameplayAndMenus/StepsDisplay.h"
  "Etterna/Actor/GameplayAndMenus/StreamDisplay.h"
  "Etterna/Actor/GameplayAndMenus/Transition.h"
)

source_group("Actors\\\\Gameplay and Menus" FILES ${SMDATA_ACTOR_GAMEPLAY_MENU_SRC} ${SMDATA_ACTOR_GAMEPLAY_MENU_HPP})

list(APPEND SMDATA_ALL_ACTORS_SRC
  ${SMDATA_ACTOR_BASE_SRC}
  ${SMDATA_ACTOR_GAMEPLAY_SRC}
  ${SMDATA_ACTOR_MENU_SRC}
  ${SMDATA_ACTOR_GAMEPLAY_MENU_SRC}
)
list(APPEND SMDATA_ALL_ACTORS_HPP
  ${SMDATA_ACTOR_BASE_HPP}
  ${SMDATA_ACTOR_GAMEPLAY_HPP}
  ${SMDATA_ACTOR_MENU_HPP}
  ${SMDATA_ACTOR_GAMEPLAY_MENU_HPP}
)
