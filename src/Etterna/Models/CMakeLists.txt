list(APPEND LUA_SRC
	"Lua/LuaBinding.cpp"
	"Lua/LuaExpressionTransform.cpp"
	"Lua/LuaReference.cpp")
list(APPEND LUA_HPP
	"Lua/LuaBinding.h"
	"Lua/LuaExpressionTransform.h"
	"Lua/LuaReference.h")

list(APPEND FONT_SRC
	"Fonts/Font.cpp"
	"Fonts/FontCharAliases.cpp"
	"Fonts/FontCharmaps.cpp")

list(APPEND FONT_HPP
	"Fonts/Font.h"
	"Fonts/FontCharAliases.h"
	"Fonts/FontCharmaps.h")

list(APPEND HIGHSCORE_SRC
	"HighScore/HighScore.cpp"
	"HighScore/Replay.cpp"
	"HighScore/ReplayConstantsAndTypes.cpp")

list(APPEND HIGHSCORE_HPP
	"HighScore/HighScore.h"
	"HighScore/Replay.h"
	"HighScore/ReplayConstantsAndTypes.h")

list(APPEND NOTEDATA_SRC
	"NoteData/NoteData.cpp"
	"NoteData/NoteDataUtil.cpp"
	"NoteData/NoteDataWithScoring.cpp")
list(APPEND NOTEDATA_HPP
	"NoteData/NoteData.h"
	"NoteData/NoteDataStructures.h"
	"NoteData/NoteDataUtil.h"
	"NoteData/NoteDataWithScoring.h")

list(APPEND NOTELOAD_SRC
	"NoteLoaders/NotesLoader.cpp"
	"NoteLoaders/NotesLoaderBMS.cpp"
	"NoteLoaders/NotesLoaderDWI.cpp"
	"NoteLoaders/NotesLoaderKSF.cpp"
	"NoteLoaders/NotesLoaderSM.cpp"
	"NoteLoaders/NotesLoaderSMA.cpp"
	"NoteLoaders/NotesLoaderSSC.cpp"
	"NoteLoaders/NotesLoaderETT.cpp"
	"NoteLoaders/NotesLoaderOSU.cpp")

list(APPEND NOTELOAD_HPP
	"NoteLoaders/NotesLoader.h"
	"NoteLoaders/NotesLoaderBMS.h"
	"NoteLoaders/NotesLoaderDWI.h"
	"NoteLoaders/NotesLoaderKSF.h"
	"NoteLoaders/NotesLoaderSM.h"
	"NoteLoaders/NotesLoaderSMA.h"
	"NoteLoaders/NotesLoaderSSC.h"
	"NoteLoaders/NotesLoaderETT.h"
	"NoteLoaders/NotesLoaderOSU.h")

list(APPEND NOTEWRITE_SRC
	"NoteWriters/NotesWriterDWI.cpp"
	"NoteWriters/NotesWriterSM.cpp"
	"NoteWriters/NotesWriterSSC.cpp"
	"NoteWriters/NotesWriterETT.cpp")
list(APPEND NOTEWRITE_HPP
	"NoteWriters/NotesWriterDWI.h"
	"NoteWriters/NotesWriterSM.h"
	"NoteWriters/NotesWriterSSC.h"
	"NoteWriters/NotesWriterETT.h")


list(APPEND SCORE_SRC
	"ScoreKeepers/ScoreKeeper.cpp"
	"ScoreKeepers/ScoreKeeperNormal.cpp")
list(APPEND SCORE_HPP
	"ScoreKeepers/ScoreKeeper.h"
	"ScoreKeepers/ScoreKeeperNormal.h")

list(APPEND SONG_SRC
	"Songs/Song.cpp"
	"Songs/SongCacheIndex.cpp"
	"Songs/SongOptions.cpp"
	"Songs/SongPosition.cpp"
	"Songs/SongUtil.cpp")
list(APPEND SONG_HPP
	"Songs/Song.h"
	"Songs/SongCacheIndex.h"
	"Songs/SongOptions.h"
	"Songs/SongPosition.h"
	"Songs/SongUtil.h")

list(APPEND STEPS_SRC
	"StepsAndStyles/Steps.cpp"
	"StepsAndStyles/StepsUtil.cpp"
	"StepsAndStyles/Style.cpp"
	"StepsAndStyles/StyleUtil.cpp")
list(APPEND STEPS_HPP
	"StepsAndStyles/Steps.h"
	"StepsAndStyles/StepsUtil.h"
	"StepsAndStyles/Style.h"
	"StepsAndStyles/StyleUtil.h")

list(APPEND REST_SRC
	"Misc/AdjustSync.cpp"
	"Misc/AutoKeysounds.cpp"
	"Misc/BackgroundUtil.cpp"
	"Misc/ImageCache.cpp"
	"Misc/CodeDetector.cpp"
	"Misc/CodeSet.cpp"
	"Misc/CubicSpline.cpp"
	"Misc/Command.cpp"
	"Misc/CommonMetrics.cpp"
	"Misc/ControllerStateDisplay.cpp"
	"Misc/DateTime.cpp"
	"Misc/Difficulty.cpp"
	"Misc/DisplaySpec.cpp"
	"Misc/EnumHelper.cpp"
	"Misc/Game.cpp"
	"Misc/GameCommand.cpp"
	"Misc/GameConstantsAndTypes.cpp"
	"Misc/GameInput.cpp"
	"Misc/GameplayAssist.cpp"
	"Misc/GamePreferences.cpp"
	"Misc/Grade.cpp"
	"Misc/LocalizedString.cpp"
	"Misc/LyricsLoader.cpp"
	"Misc/ModsGroup.cpp"
	"Misc/NoteTypes.cpp"
	"Misc/OptionRowHandler.cpp"
	"Misc/PlayerInfo.cpp"
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
	"Misc/RoomWheel.cpp")

list(APPEND REST_HPP
	"Misc/AdjustSync.h"
	"Misc/AutoKeysounds.h"
	"Misc/BackgroundUtil.h"
	"Misc/ImageCache.h"
	"Misc/CodeDetector.h"
	"Misc/CodeSet.h"
	"Misc/Command.h"
	"Misc/CommonMetrics.h"
	"Misc/ControllerStateDisplay.h"
	"Misc/CubicSpline.h"
	"Misc/DateTime.h"
	"Misc/DisplaySpec.h"
	"Misc/Difficulty.h"
	"Misc/EnumHelper.h"
	"Misc/Foreach.h"
	"Misc/Game.h"
	"Misc/GameCommand.h"
	"Misc/GameConstantsAndTypes.h"
	"Misc/GameInput.h"
	"Misc/GameplayAssist.h"
	"Misc/GamePreferences.h"
	"Misc/Grade.h"
	"Misc/InputEventPlus.h"
	"Misc/LocalizedString.h"
	"Misc/LyricsLoader.h"
	"Misc/ModsGroup.h"
	"Misc/NoteTypes.h"
	"Misc/OptionRowHandler.h"
	"Misc/PlayerInfo.h"
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
	"Misc/RoomWheel.h")

list(APPEND ALL_DATA_SRC
  ${FONT_SRC}
  ${LUA_SRC}
  ${HIGHSCORE_SRC}
  ${NOTEDATA_SRC}
  ${NOTELOAD_SRC}
  ${NOTEWRITE_SRC}
  ${SCORE_SRC}
  ${SONG_SRC}
  ${STEPS_SRC}
  ${REST_SRC})

list(APPEND ALL_DATA_HPP
  ${FONT_HPP}
  ${LUA_HPP}
  ${HIGHSCORE_HPP}
  ${NOTEDATA_HPP}
  ${NOTELOAD_HPP}
  ${NOTEWRITE_HPP}
  ${SCORE_HPP}
  ${SONG_HPP}
  ${STEPS_HPP}
  ${REST_HPP})

target_sources(Etterna PUBLIC ${ALL_DATA_SRC} ${ALL_DATA_HPP})
