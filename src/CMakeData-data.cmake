list(APPEND SM_DATA_LUA_SRC
  "Etterna/Models/Lua/LuaBinding.cpp"
  "Etterna/Models/Lua/LuaExpressionTransform.cpp"
  "Etterna/Models/Lua/LuaReference.cpp"
)

list(APPEND SM_DATA_LUA_HPP
  "Etterna/Models/Lua/LuaBinding.h"
  "Etterna/Models/Lua/LuaExpressionTransform.h"
  "Etterna/Models/Lua/LuaReference.h"
)

source_group("Data Structures\\\\Lua" FILES ${SM_DATA_LUA_SRC} ${SM_DATA_LUA_HPP})

list(APPEND SM_DATA_FONT_SRC
  "Etterna/Models/Fonts/Font.cpp"
  "Etterna/Models/Fonts/FontCharAliases.cpp"
  "Etterna/Models/Fonts/FontCharmaps.cpp"
)

list(APPEND SM_DATA_FONT_HPP
  "Etterna/Models/Fonts/Font.h"
  "Etterna/Models/Fonts/FontCharAliases.h"
  "Etterna/Models/Fonts/FontCharmaps.h"
)

source_group("Data Structures\\\\Fonts" FILES ${SM_DATA_FONT_SRC} ${SM_DATA_FONT_HPP})

list(APPEND SM_DATA_NOTEDATA_SRC
  "Etterna/Models/NoteData/NoteData.cpp"
  "Etterna/Models/NoteData/NoteDataUtil.cpp"
  "Etterna/Models/NoteData/NoteDataWithScoring.cpp"
)

list(APPEND SM_DATA_NOTEDATA_HPP
  "Etterna/Models/NoteData/NoteData.h"
  "Etterna/Models/NoteData/NoteDataStructures.h"
  "Etterna/Models/NoteData/NoteDataUtil.h"
  "Etterna/Models/NoteData/NoteDataWithScoring.h"
)

source_group("Data Structures\\\\Note Data" FILES ${SM_DATA_NOTEDATA_SRC} ${SM_DATA_NOTEDATA_HPP})

list(APPEND SM_DATA_NOTELOAD_SRC
  "Etterna/Models/NoteLoaders/NotesLoader.cpp"
  "Etterna/Models/NoteLoaders/NotesLoaderBMS.cpp"
  "Etterna/Models/NoteLoaders/NotesLoaderDWI.cpp"
  "Etterna/Models/NoteLoaders/NotesLoaderJson.cpp"
  "Etterna/Models/NoteLoaders/NotesLoaderKSF.cpp"
  "Etterna/Models/NoteLoaders/NotesLoaderSM.cpp"
  "Etterna/Models/NoteLoaders/NotesLoaderSMA.cpp"
  "Etterna/Models/NoteLoaders/NotesLoaderSSC.cpp"
  "Etterna/Models/NoteLoaders/NotesLoaderETT.cpp"
  "Etterna/Models/NoteLoaders/NotesLoaderOSU.cpp"
)

list(APPEND SM_DATA_NOTELOAD_HPP
  "Etterna/Models/NoteLoaders/NotesLoader.h"
  "Etterna/Models/NoteLoaders/NotesLoaderBMS.h"
  "Etterna/Models/NoteLoaders/NotesLoaderDWI.h"
  "Etterna/Models/NoteLoaders/NotesLoaderJson.h"
  "Etterna/Models/NoteLoaders/NotesLoaderKSF.h"
  "Etterna/Models/NoteLoaders/NotesLoaderSM.h"
  "Etterna/Models/NoteLoaders/NotesLoaderSMA.h"
  "Etterna/Models/NoteLoaders/NotesLoaderSSC.h"
  "Etterna/Models/NoteLoaders/NotesLoaderETT.h"
  "Etterna/Models/NoteLoaders/NotesLoaderOSU.h"
)

source_group("Data Structures\\\\Notes Loaders" FILES ${SM_DATA_NOTELOAD_SRC} ${SM_DATA_NOTELOAD_HPP})

list(APPEND SM_DATA_NOTEWRITE_SRC
  "Etterna/Models/NoteWriters/NotesWriterDWI.cpp"
  "Etterna/Models/NoteWriters/NotesWriterJson.cpp"
  "Etterna/Models/NoteWriters/NotesWriterSM.cpp"
  "Etterna/Models/NoteWriters/NotesWriterSSC.cpp"
  "Etterna/Models/NoteWriters/NotesWriterETT.cpp"
)

list(APPEND SM_DATA_NOTEWRITE_HPP
  "Etterna/Models/NoteWriters/NotesWriterDWI.h"
  "Etterna/Models/NoteWriters/NotesWriterJson.h"
  "Etterna/Models/NoteWriters/NotesWriterSM.h"
  "Etterna/Models/NoteWriters/NotesWriterSSC.h"
  "Etterna/Models/NoteWriters/NotesWriterETT.h"
)

source_group("Data Structures\\\\Notes Writers" FILES ${SM_DATA_NOTEWRITE_SRC} ${SM_DATA_NOTEWRITE_HPP})

list(APPEND SM_DATA_SCORE_SRC
  "Etterna/Models/ScoreKeepers/ScoreKeeper.cpp"
  "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.cpp"
)

list(APPEND SM_DATA_SCORE_HPP
  "Etterna/Models/ScoreKeepers/ScoreKeeper.h"
  "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"
)

source_group("Data Structures\\\\Score Keepers" FILES ${SM_DATA_SCORE_SRC} ${SM_DATA_SCORE_HPP})

list(APPEND SM_DATA_SONG_SRC
  "Etterna/Models/Songs/Song.cpp"
  "Etterna/Models/Songs/SongCacheIndex.cpp"
  "Etterna/Models/Songs/SongOptions.cpp"
  "Etterna/Models/Songs/SongPosition.cpp"
  "Etterna/Models/Songs/SongUtil.cpp"
)

list(APPEND SM_DATA_SONG_HPP
  "Etterna/Models/Songs/Song.h"
  "Etterna/Models/Songs/SongCacheIndex.h"
  "Etterna/Models/Songs/SongOptions.h"
  "Etterna/Models/Songs/SongPosition.h"
  "Etterna/Models/Songs/SongUtil.h"
)

source_group("Data Structures\\\\Songs" FILES ${SM_DATA_SONG_SRC} ${SM_DATA_SONG_HPP})

list(APPEND SM_DATA_STEPS_SRC
  "Etterna/Models/StepsAndStyles/Steps.cpp"
  "Etterna/Models/StepsAndStyles/StepsUtil.cpp"
  "Etterna/Models/StepsAndStyles/Style.cpp"
  "Etterna/Models/StepsAndStyles/StyleUtil.cpp"
)

list(APPEND SM_DATA_STEPS_HPP
  "Etterna/Models/StepsAndStyles/Steps.h"
  "Etterna/Models/StepsAndStyles/StepsUtil.h"
  "Etterna/Models/StepsAndStyles/Style.h"
  "Etterna/Models/StepsAndStyles/StyleUtil.h"
)

source_group("Data Structures\\\\Steps and Styles" FILES ${SM_DATA_STEPS_SRC} ${SM_DATA_STEPS_HPP})

list(APPEND SM_DATA_REST_SRC
  "Etterna/Models/Misc/AdjustSync.cpp"
  "Etterna/Models/Misc/AutoKeysounds.cpp"
  "Etterna/Models/Misc/BackgroundUtil.cpp"
  "Etterna/Models/Misc/ImageCache.cpp"
  "Etterna/Models/Misc/Character.cpp"
  "Etterna/Models/Misc/CodeDetector.cpp"
  "Etterna/Models/Misc/CodeSet.cpp"
  "Etterna/Models/Misc/CubicSpline.cpp"
  "Etterna/Models/Misc/Command.cpp"
  "Etterna/Models/Misc/CommonMetrics.cpp"
  "Etterna/Models/Misc/ControllerStateDisplay.cpp"
  "Etterna/Models/Misc/CreateZip.cpp"
  "Etterna/Models/Misc/CryptHelpers.cpp"
  "Etterna/Models/Misc/DateTime.cpp"
  "Etterna/Models/Misc/Difficulty.cpp"
  "Etterna/Models/Misc/EnumHelper.cpp"
  "Etterna/Models/Misc/Game.cpp"
  "Etterna/Models/Misc/GameCommand.cpp"
  "Etterna/Models/Misc/GameConstantsAndTypes.cpp"
  "Etterna/Models/Misc/GameInput.cpp"
  "Etterna/Models/Misc/GameplayAssist.cpp"
  "Etterna/Models/Misc/GamePreferences.cpp"
  "Etterna/Models/Misc/Grade.cpp"
  "Etterna/Models/Misc/HighScore.cpp"
  "Etterna/Models/Misc/JsonUtil.cpp"
  "Etterna/Models/Misc/LocalizedString.cpp"
  "Etterna/Models/Misc/LyricsLoader.cpp"
  "Etterna/Models/Misc/ModsGroup.cpp"
  "Etterna/Models/Misc/NoteTypes.cpp"
  "Etterna/Models/Misc/OptionRowHandler.cpp"
  "Etterna/Models/Misc/PlayerAI.cpp"
  "Etterna/Models/Misc/PlayerNumber.cpp"
  "Etterna/Models/Misc/PlayerOptions.cpp"
  "Etterna/Models/Misc/PlayerStageStats.cpp"
  "Etterna/Models/Misc/PlayerState.cpp"
  "Etterna/Models/Misc/Preference.cpp"
  "Etterna/Models/Misc/Profile.cpp"
  "Etterna/Models/Misc/XMLProfile.cpp"
  "Etterna/Models/Misc/DBProfile.cpp"
  "Etterna/Models/Misc/RadarValues.cpp"
  "Etterna/Models/Misc/RandomSample.cpp"
  "Etterna/Models/Misc/SampleHistory.cpp"
  "Etterna/Models/Misc/ScreenDimensions.cpp"
  "Etterna/Models/Misc/SoundEffectControl.cpp"
  "Etterna/Models/Misc/StageStats.cpp"
  "Etterna/Models/Misc/TimingData.cpp"
  "Etterna/Models/Misc/TimingSegments.cpp"
  "Etterna/Models/Misc/TitleSubstitution.cpp"
  "Etterna/Models/Misc/RoomWheel.cpp"
)

list(APPEND SM_DATA_REST_HPP
  "Etterna/Models/Misc/AdjustSync.h"
  "Etterna/Models/Misc/AutoKeysounds.h"
  "Etterna/Models/Misc/BackgroundUtil.h"
  "Etterna/Models/Misc/ImageCache.h"
  "Etterna/Models/Misc/Character.h"
  "Etterna/Models/Misc/CodeDetector.h"
  "Etterna/Models/Misc/CodeSet.h"
  "Etterna/Models/Misc/Command.h"
  "Etterna/Models/Misc/CommonMetrics.h"
  "Etterna/Models/Misc/ControllerStateDisplay.h"
  "Etterna/Models/Misc/CreateZip.h"
  "Etterna/Models/Misc/CryptHelpers.h"
  "Etterna/Models/Misc/CubicSpline.h"
  "Etterna/Models/Misc/DateTime.h"
  "Etterna/Models/Misc/DisplayResolutions.h"
  "Etterna/Models/Misc/Difficulty.h"
  "Etterna/Models/Misc/EnumHelper.h"
  "Etterna/Models/Misc/EnumHelper.h"
  "Etterna/Models/Misc/Foreach.h"
  "Etterna/Models/Misc/Game.h"
  "Etterna/Models/Misc/GameCommand.h"
  "Etterna/Models/Misc/GameConstantsAndTypes.h"
  "Etterna/Models/Misc/GameInput.h"
  "Etterna/Models/Misc/GameplayAssist.h"
  "Etterna/Models/Misc/GamePreferences.h"
  "Etterna/Models/Misc/Grade.h"
  "Etterna/Models/Misc/HighScore.h"
  "Etterna/Models/Misc/InputEventPlus.h"
  "Etterna/Models/Misc/JsonUtil.h"
  "Etterna/Models/Misc/LocalizedString.h"
  "Etterna/Models/Misc/LyricsLoader.h"
  "Etterna/Models/Misc/ModsGroup.h"
  "Etterna/Models/Misc/NoteTypes.h"
  "Etterna/Models/Misc/OptionRowHandler.h"
  "Etterna/Models/Misc/PlayerAI.h"
  "Etterna/Models/Misc/PlayerNumber.h"
  "Etterna/Models/Misc/PlayerOptions.h"
  "Etterna/Models/Misc/PlayerStageStats.h"
  "Etterna/Models/Misc/PlayerState.h"
  "Etterna/Models/Misc/Preference.h"
  "Etterna/Models/Misc/Profile.h"
  "Etterna/Models/Misc/XMLProfile.h"
  "Etterna/Models/Misc/DBProfile.h"
  "Etterna/Models/Misc/RadarValues.h"
  "Etterna/Models/Misc/RandomSample.h"
  "Etterna/Models/Misc/SampleHistory.h"
  "Etterna/Models/Misc/ScreenDimensions.h"
  "Etterna/Models/Misc/SoundEffectControl.h"
  "Etterna/Models/Misc/SubscriptionManager.h"
  "Etterna/Models/Misc/StageStats.h"
  "Etterna/Models/Misc/ThemeMetric.h"
  "Etterna/Models/Misc/TimingData.h"
  "Etterna/Models/Misc/TimingSegments.h"
  "Etterna/Models/Misc/TitleSubstitution.h"
  "Etterna/Models/Misc/RoomWheel.h"
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
