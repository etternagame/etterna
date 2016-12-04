--[[
ExportSelection; coded by freem

Much like its predecessor, it assumes you play as a single player in
non-course modes.

It spits out a string like this:
last played: $SONG by $ARTIST ($SONGGROUP) [$MODE $DIFFICULTY]

todo: it should display
song name, artist, pack name, player mods and song options

This requires FileUtils, which is built into sm-ssc and new SM5.
You can use this code with StepMania 4 alpha 5 if you have the standalone
FileUtils from http://kki.ajworld.net/misc/FileUtils.lua

[General Note]
Since StepMania/sm-ssc can only read and write files from within its ecosystem,
it's important that you make the location of LastPlayed.txt as simple as possible
(and not likely to get removed by the uninstaller when upgrading).

[Changes]
* added "No Song Selected" text



removed this for now - staiain

				-- get player modifiers
				local pState = GAMESTATE:GetPlayerState(PLAYER_1)
				local pOpts = pState:GetPlayerOptionsString('ModsLevel_Preferred')
				outStr = outStr .. " | "..pOpts

				-- song options
				local songOpts = GAMESTATE:GetSongOptionsString()
				outStr = outStr .. " | "..songOpts.." | "





--]]

local mp = GAMESTATE:GetMasterPlayerNumber();
-- path is where the file lives, relative to StepMania's root directory.
-- you can change this variable to point to another location if you want.
local path = "np.txt"
local noSongText = ""

return Def.ActorFrame{
	Def.Actor{
		Name="InfoUpdater";
		CancelCommand=function(self)
			File.Write(path,noSongText)
		end;
		UpdateStreamInfoMessageCommand=function(self)
			-- "Selection" (ScreenSelectMusic) or "Gameplay" (ScreenGameplay).

			-- build the string from the components
			-- yes I should be using a string.format but I don't care enough :D
			local outStr = "StepMania: ";

			local song = GAMESTATE:GetCurrentSong()
			if song then
				-- attach song title/artist
				local mainTitle = song:GetDisplayFullTitle()
				local artist = song:GetDisplayArtist()
				outStr = outStr .."Most likely... "..mainTitle .." by "..artist

				-- attach song group/pack
				local songGroup = song:GetGroupName()
				outStr = outStr .. " ("..songGroup..")"


				-- write string
				File.Write(path,outStr)
			else
				-- no song
				outStr = noSongText
				File.Write(path,outStr)
			end
		end;

		OnCommand=function(self)
			-- this assumes normal mode only.
			if not GAMESTATE:IsCourseMode() then
				local outStr = "last played: "
				local song = GAMESTATE:GetCurrentSong();
				if song then
					-- attach stepstype and difficulty
					local steps = GAMESTATE:GetCurrentSteps(mp)
					if steps then
						local st = string.gsub(ToEnumShortString(steps:GetStepsType()),"_","-")
						local diff = ToEnumShortString(steps:GetDifficulty())
						outStr = outStr .." [".. st .." ".. diff .."] "
					end;

					local sStats = STATSMAN:GetCurStageStats()
					local pStats = sStats:GetPlayerStageStats(mp)
					-- attach percent score
					local pScore = pStats:GetPercentDancePoints()*100
					outStr = outStr ..string.format("%.02f%%",pScore)

					-- attach judge counts
					local w1 = pStats:GetTapNoteScores('TapNoteScore_W1')
					local w2 = pStats:GetTapNoteScores('TapNoteScore_W2')
					local w3 = pStats:GetTapNoteScores('TapNoteScore_W3')
					local w4 = pStats:GetTapNoteScores('TapNoteScore_W4')
					local w5 = pStats:GetTapNoteScores('TapNoteScore_W5')
					local miss = pStats:GetTapNoteScores('TapNoteScore_Miss')
					outStr = outStr .." | "..w1.." / "..w2.." / "..w3.." / "..w4.." / "..w5.." / "..miss

					-- attach OK/NG counts
					local held = pStats:GetHoldNoteScores('HoldNoteScore_Held')
					local dropped = pStats:GetHoldNoteScores('HoldNoteScore_LetGo')
					outStr = outStr .." / Holds: ("..held .." / ".. dropped ..")"

					-- attach max combo
					local maxCombo = pStats:MaxCombo();
					local comboThreshold = THEME:GetMetric("Gameplay","MinScoreToContinueCombo")
					local gotFullCombo = pStats:FullComboOfScore(comboThreshold);
					local comboLabel = gotFullCombo and "Full" or "Max"
					outStr = outStr .."  ".. comboLabel .." Combo: ".. maxCombo
				end
			end
		end;
	};
};

-- this code is in the public domain because I don't really care about
-- copyrighting something like this.