list(APPEND SMDATA_ACTOR_BASE_SRC
  "Actor.cpp"
  "ActorFrame.cpp"
  "ActorFrameTexture.cpp"
  "ActorMultiVertex.cpp"
  "ActorScroller.cpp"
  "ActorSound.cpp"
  "ActorUtil.cpp"
  "AutoActor.cpp"
  "BitmapText.cpp"
  "Model.cpp"
  "ModelManager.cpp"
  "ModelTypes.cpp"
  "Quad.cpp"
  "RollingNumbers.cpp"
  "Sprite.cpp"
  "Tween.cpp"
)
list(APPEND SMDATA_ACTOR_BASE_HPP
  "Actor.h"
  "ActorFrame.h"
  "ActorFrameTexture.h"
  "ActorMultiVertex.h"
  "ActorScroller.h"
  "ActorSound.h"
  "ActorUtil.h"
  "AutoActor.h"
  "BitmapText.h"
  "Model.h"
  "ModelManager.h"
  "ModelTypes.h"
  "Quad.h"
  "RollingNumbers.h"
  "Sprite.h"
  "Tween.h"
)

source_group("Actors\\\\Base" FILES ${SMDATA_ACTOR_BASE_SRC} ${SMDATA_ACTOR_BASE_HPP})

list(APPEND SMDATA_ACTOR_GAMEPLAY_SRC
  "ArrowEffects.cpp"
  "Background.cpp"
  "DancingCharacters.cpp"
  "Foreground.cpp"
  "GhostArrowRow.cpp"
  "HoldJudgment.cpp"
  "LifeMeter.cpp"
  "LifeMeterBar.cpp"
  "LyricDisplay.cpp"
  "NoteDisplay.cpp"
  "NoteField.cpp"
  "Player.cpp"
  "ReceptorArrow.cpp"
  "ReceptorArrowRow.cpp"
)

list(APPEND SMDATA_ACTOR_GAMEPLAY_HPP
  "ArrowEffects.h"
  "Background.h"
  "DancingCharacters.h"
  "Foreground.h"
  "GhostArrowRow.h"
  "HoldJudgment.h"
  "LifeMeter.h"
  "LifeMeterBar.h"
  "LyricDisplay.h"
  "NoteDisplay.h"
  "NoteField.h"
  "Player.h"
  "ReceptorArrow.h"
  "ReceptorArrowRow.h"
)
source_group("Actors\\\\Gameplay" FILES ${SMDATA_ACTOR_GAMEPLAY_SRC} ${SMDATA_ACTOR_GAMEPLAY_HPP})

list(APPEND SMDATA_ACTOR_MENU_SRC
  "BPMDisplay.cpp"
  "ComboGraph.cpp"
  "ControllerStateDisplay.cpp"
  "DifficultyList.cpp"
  "DualScrollBar.cpp"
  "GraphDisplay.cpp"
  "MenuTimer.cpp"
  "ModIcon.cpp"
  "ModIconRow.cpp"
  "MusicWheel.cpp"
  "MusicWheelItem.cpp"
  "OptionRow.cpp"
  "OptionsCursor.cpp"
  "OptionsList.cpp"
  "ScrollBar.cpp"
  "TextBanner.cpp"
  "WheelBase.cpp"
  "WheelItemBase.cpp"
)
list(APPEND SMDATA_ACTOR_MENU_HPP
  "BPMDisplay.h"
  "ComboGraph.h"
  "ControllerStateDisplay.h"
  "DifficultyList.h"
  "DualScrollBar.h"
  "GraphDisplay.h"
  "MenuTimer.h"
  "ModIcon.h"
  "ModIconRow.h"
  "MusicWheel.h"
  "MusicWheelItem.h"
  "OptionRow.h"
  "OptionsCursor.h"
  "OptionsList.h"
  "ScrollBar.h"
  "TextBanner.h"
  "WheelBase.h"
  "WheelItemBase.h"
)

if(WITH_NETWORKING)
  list(APPEND SMDATA_ACTOR_MENU_SRC
    "RoomInfoDisplay.cpp"
  )
  list(APPEND SMDATA_ACTOR_MENU_HPP
    "RoomInfoDisplay.h"
  )
endif()

source_group("Actors\\\\Menus" FILES ${SMDATA_ACTOR_MENU_SRC} ${SMDATA_ACTOR_MENU_HPP})

list(APPEND SMDATA_ACTOR_GAMEPLAY_MENU_SRC
  "BGAnimation.cpp"
  "BGAnimationLayer.cpp"
  "MeterDisplay.cpp"
  "StepsDisplay.cpp"
  "StreamDisplay.cpp"
  "Transition.cpp"
)

list(APPEND SMDATA_ACTOR_GAMEPLAY_MENU_HPP
  "BGAnimation.h"
  "BGAnimationLayer.h"
  "MeterDisplay.h"
  "StepsDisplay.h"
  "StreamDisplay.h"
  "Transition.h"
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
