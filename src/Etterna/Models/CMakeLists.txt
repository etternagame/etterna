list(APPEND SM_DATA_LUA_SRC
  "Lua/LuaBinding.cpp"
  "Lua/LuaExpressionTransform.cpp"
  "Lua/LuaReference.cpp"
)

list(APPEND SM_DATA_LUA_HPP
  "Lua/LuaBinding.h"
  "Lua/LuaExpressionTransform.h"
  "Lua/LuaReference.h"
)

source_group("Data Structures\\\\Lua" FILES ${SM_DATA_LUA_SRC} ${SM_DATA_LUA_HPP})

list(APPEND SM_DATA_FONT_SRC
  "Fonts/Font.cpp"
  "Fonts/FontCharAliases.cpp"
  "Fonts/FontCharmaps.cpp"
)

list(APPEND SM_DATA_FONT_HPP
  "Fonts/Font.h"
  "Fonts/FontCharAliases.h"
  "Fonts/FontCharmaps.h"
)

source_group("Data Structures\\\\Fonts" FILES ${SM_DATA_FONT_SRC} ${SM_DATA_FONT_HPP})

list(APPEND SM_DATA_NOTEDATA_SRC
  "NoteData/NoteData.cpp"
  "NoteData/NoteDataUtil.cpp"
  "NoteData/NoteDataWithScoring.cpp"
)

list(APPEND SM_DATA_NOTEDATA_HPP
  "NoteData/NoteData.h"
  "NoteData/NoteDataStructures.h"
  "NoteData/NoteDataUtil.h"
  "NoteData/NoteDataWithScoring.h"
)

source_group("Data Structures\\\\Note Data" FILES ${SM_DATA_NOTEDATA_SRC} ${SM_DATA_NOTEDATA_HPP})

list(APPEND SM_DATA_NOTELOAD_SRC
  "NoteLoaders/NotesLoader.cpp"
  "NoteLoaders/NotesLoaderBMS.cpp"
  "NoteLoaders/NotesLoaderDWI.cpp"
  "NoteLoaders/NotesLoaderJson.cpp"
  "NoteLoaders/NotesLoaderKSF.cpp"
  "NoteLoaders/NotesLoaderSM.cpp"
  "NoteLoaders/NotesLoaderSMA.cpp"
  "NoteLoaders/NotesLoaderSSC.cpp"
  "NoteLoaders/NotesLoaderETT.cpp"
  "NoteLoaders/NotesLoaderOSU.cpp"
)

list(APPEND SM_DATA_NOTELOAD_HPP
  "NoteLoaders/NotesLoader.h"
  "NoteLoaders/NotesLoaderBMS.h"
  "NoteLoaders/NotesLoaderDWI.h"
  "NoteLoaders/NotesLoaderJson.h"
  "NoteLoaders/NotesLoaderKSF.h"
  "NoteLoaders/NotesLoaderSM.h"
  "NoteLoaders/NotesLoaderSMA.h"
  "NoteLoaders/NotesLoaderSSC.h"
  "NoteLoaders/NotesLoaderETT.h"
  "NoteLoaders/NotesLoaderOSU.h"
)

source_group("Data Structures\\\\Notes Loaders" FILES ${SM_DATA_NOTELOAD_SRC} ${SM_DATA_NOTELOAD_HPP})

list(APPEND SM_DATA_NOTEWRITE_SRC
  "NoteWriters/NotesWriterDWI.cpp"
  "NoteWriters/NotesWriterJson.cpp"
  "NoteWriters/NotesWriterSM.cpp"
  "NoteWriters/NotesWriterSSC.cpp"
  "NoteWriters/NotesWriterETT.cpp"
)

list(APPEND SM_DATA_NOTEWRITE_HPP
  "NoteWriters/NotesWriterDWI.h"
  "NoteWriters/NotesWriterJson.h"
  "NoteWriters/NotesWriterSM.h"
  "NoteWriters/NotesWriterSSC.h"
  "NoteWriters/NotesWriterETT.h"
)

source_group("Data Structures\\\\Notes Writers" FILES ${SM_DATA_NOTEWRITE_SRC} ${SM_DATA_NOTEWRITE_HPP})

list(APPEND SM_DATA_SCORE_SRC
  "ScoreKeepers/ScoreKeeper.cpp"
  "ScoreKeepers/ScoreKeeperNormal.cpp"
)

list(APPEND SM_DATA_SCORE_HPP
  "ScoreKeepers/ScoreKeeper.h"
  "ScoreKeepers/ScoreKeeperNormal.h"
)

source_group("Data Structures\\\\Score Keepers" FILES ${SM_DATA_SCORE_SRC} ${SM_DATA_SCORE_HPP})

list(APPEND SM_DATA_SONG_SRC
  "Songs/Song.cpp"
  "Songs/SongCacheIndex.cpp"
  "Songs/SongOptions.cpp"
  "Songs/SongPosition.cpp"
  "Songs/SongUtil.cpp"
)

list(APPEND SM_DATA_SONG_HPP
  "Songs/Song.h"
  "Songs/SongCacheIndex.h"
  "Songs/SongOptions.h"
  "Songs/SongPosition.h"
  "Songs/SongUtil.h"
)

source_group("Data Structures\\\\Songs" FILES ${SM_DATA_SONG_SRC} ${SM_DATA_SONG_HPP})

list(APPEND SM_DATA_STEPS_SRC
  "StepsAndStyles/Steps.cpp"
  "StepsAndStyles/StepsUtil.cpp"
  "StepsAndStyles/Style.cpp"
  "StepsAndStyles/StyleUtil.cpp"
)

list(APPEND SM_DATA_STEPS_HPP
  "StepsAndStyles/Steps.h"
  "StepsAndStyles/StepsUtil.h"
  "StepsAndStyles/Style.h"
  "StepsAndStyles/StyleUtil.h"
)

source_group("Data Structures\\\\Steps and Styles" FILES ${SM_DATA_STEPS_SRC} ${SM_DATA_STEPS_HPP})

list(APPEND SM_DATA_REST_SRC
  "Misc/AdjustSync.cpp"
  "Misc/AutoKeysounds.cpp"
  "Misc/BackgroundUtil.cpp"
  "Misc/ImageCache.cpp"
  "Misc/Character.cpp"
  "Misc/CodeDetector.cpp"
  "Misc/CodeSet.cpp"
  "Misc/CubicSpline.cpp"
  "Misc/Command.cpp"
  "Misc/CommonMetrics.cpp"
  "Misc/ControllerStateDisplay.cpp"
  "Misc/CreateZip.cpp"
  "Misc/CryptHelpers.cpp"
  "Misc/DateTime.cpp"
  "Misc/Difficulty.cpp"
  "Misc/EnumHelper.cpp"
  "Misc/Game.cpp"
  "Misc/GameCommand.cpp"
  "Misc/GameConstantsAndTypes.cpp"
  "Misc/GameInput.cpp"
  "Misc/GameplayAssist.cpp"
  "Misc/GamePreferences.cpp"
  "Misc/Grade.cpp"
  "Misc/HighScore.cpp"
  "Misc/JsonUtil.cpp"
  "Misc/LocalizedString.cpp"
  "Misc/LyricsLoader.cpp"
  "Misc/ModsGroup.cpp"
  "Misc/NoteTypes.cpp"
  "Misc/OptionRowHandler.cpp"
  "Misc/PlayerAI.cpp"
  "Misc/PlayerNumber.cpp"
  "Misc/PlayerOptions.cpp"
  "Misc/PlayerStageStats.cpp"
  "Misc/PlayerState.cpp"
  "Misc/Preference.cpp"
  "Misc/Profile.cpp"
  "Misc/XMLProfile.cpp"
  "Misc/DBProfile.cpp"
  "Misc/RadarValues.cpp"
  "Misc/RandomSample.cpp"
  "Misc/SampleHistory.cpp"
  "Misc/ScreenDimensions.cpp"
  "Misc/SoundEffectControl.cpp"
  "Misc/StageStats.cpp"
  "Misc/TimingData.cpp"
  "Misc/TimingSegments.cpp"
  "Misc/TitleSubstitution.cpp"
  "Misc/RoomWheel.cpp"
)

list(APPEND SM_DATA_REST_HPP
  "Misc/AdjustSync.h"
  "Misc/AutoKeysounds.h"
  "Misc/BackgroundUtil.h"
  "Misc/ImageCache.h"
  "Misc/Character.h"
  "Misc/CodeDetector.h"
  "Misc/CodeSet.h"
  "Misc/Command.h"
  "Misc/CommonMetrics.h"
  "Misc/ControllerStateDisplay.h"
  "Misc/CreateZip.h"
  "Misc/CryptHelpers.h"
  "Misc/CubicSpline.h"
  "Misc/DateTime.h"
  "Misc/DisplayResolutions.h"
  "Misc/Difficulty.h"
  "Misc/EnumHelper.h"
  "Misc/EnumHelper.h"
  "Misc/Foreach.h"
  "Misc/Game.h"
  "Misc/GameCommand.h"
  "Misc/GameConstantsAndTypes.h"
  "Misc/GameInput.h"
  "Misc/GameplayAssist.h"
  "Misc/GamePreferences.h"
  "Misc/Grade.h"
  "Misc/HighScore.h"
  "Misc/InputEventPlus.h"
  "Misc/JsonUtil.h"
  "Misc/LocalizedString.h"
  "Misc/LyricsLoader.h"
  "Misc/ModsGroup.h"
  "Misc/NoteTypes.h"
  "Misc/OptionRowHandler.h"
  "Misc/PlayerAI.h"
  "Misc/PlayerNumber.h"
  "Misc/PlayerOptions.h"
  "Misc/PlayerStageStats.h"
  "Misc/PlayerState.h"
  "Misc/Preference.h"
  "Misc/Profile.h"
  "Misc/XMLProfile.h"
  "Misc/DBProfile.h"
  "Misc/RadarValues.h"
  "Misc/RandomSample.h"
  "Misc/SampleHistory.h"
  "Misc/ScreenDimensions.h"
  "Misc/SoundEffectControl.h"
  "Misc/SubscriptionManager.h"
  "Misc/StageStats.h"
  "Misc/ThemeMetric.h"
  "Misc/TimingData.h"
  "Misc/TimingSegments.h"
  "Misc/TitleSubstitution.h"
  "Misc/RoomWheel.h"
)

source_group("Data Structures\\\\Misc Objects" FILES ${SM_DATA_REST_SRC} ${SM_DATA_REST_HPP})

list(APPEND SMDATA_ALL_DATA_SRC
  ${SM_DATA_FONT_SRC}
  ${SM_DATA_LUA_SRC}
  ${SM_DATA_NOTEDATA_SRC}
  ${SM_DATA_NOTELOAD_SRC}
  ${SM_DATA_NOTEWRITE_SRC}
  ${SM_DATA_SCORE_SRC}
  ${SM_DATA_SONG_SRC}
  ${SM_DATA_STEPS_SRC}
  ${SM_DATA_REST_SRC}
)

list(APPEND SMDATA_ALL_DATA_HPP
  ${SM_DATA_FONT_HPP}
  ${SM_DATA_LUA_HPP}
  ${SM_DATA_NOTEDATA_HPP}
  ${SM_DATA_NOTELOAD_HPP}
  ${SM_DATA_NOTEWRITE_HPP}
  ${SM_DATA_SCORE_HPP}
  ${SM_DATA_SONG_HPP}
  ${SM_DATA_STEPS_HPP}
  ${SM_DATA_REST_HPP}
)

target_sources(Etterna PUBLIC ${SMDATA_ALL_DATA_SRC})