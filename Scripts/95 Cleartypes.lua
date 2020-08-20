--------------------------------------------
-- Methods for generating ClearType texts --
--------------------------------------------

local stypetable = {
	-- Shorthand Versions of ClearType
	[1] = "MFC",
	[2] = "WF",
	[3] = "SDP",
	[4] = "PFC",
	[5] = "BF",
	[6] = "SDG",
	[7] = "FC",
	[8] = "MF",
	[9] = "SDCB",
	[10] = "Clear",
	[11] = "Failed",
	[12] = "Invalid",
	[13] = "No Play",
	[14] = "-", -- song is nil
}

local typetable = {
	-- ClearType texts
	[1] = "MFC",
	[2] = "WF",
	[3] = "SDP",
	[4] = "PFC",
	[5] = "BF",
	[6] = "SDG",
	[7] = "FC",
	[8] = "MF",
	[9] = "SDCB",
	[10] = "Clear",
	[11] = "Failed",
	[12] = "Invalid",
	[13] = "No Play",
	[14] = "-",
}

local typecolors = {
	-- colors corresponding to cleartype
	[1] = color(colorConfig:get_data().clearType["MFC"]),
	[2] = color(colorConfig:get_data().clearType["WF"]),
	[3] = color(colorConfig:get_data().clearType["SDP"]),
	[4] = color(colorConfig:get_data().clearType["PFC"]),
	[5] = color(colorConfig:get_data().clearType["BF"]),
	[6] = color(colorConfig:get_data().clearType["SDG"]),
	[7] = color(colorConfig:get_data().clearType["FC"]),
	[8] = color(colorConfig:get_data().clearType["MF"]),
	[9] = color(colorConfig:get_data().clearType["SDCB"]),
	[10] = color(colorConfig:get_data().clearType["Clear"]),
	[11] = color(colorConfig:get_data().clearType["Failed"]),
	[12] = color(colorConfig:get_data().clearType["Invalid"]),
	[13] = color(colorConfig:get_data().clearType["NoPlay"]),
	[14] = color(colorConfig:get_data().clearType["None"]),
}

local typetranslations = {
	THEME:GetString("ClearTypes", "MFC"),
	THEME:GetString("ClearTypes", "WF"),
	THEME:GetString("ClearTypes", "SDP"),
	THEME:GetString("ClearTypes", "PFC"),
	THEME:GetString("ClearTypes", "BF"),
	THEME:GetString("ClearTypes", "SDG"),
	THEME:GetString("ClearTypes", "FC"),
	THEME:GetString("ClearTypes", "MF"),
	THEME:GetString("ClearTypes", "SDCB"),
	THEME:GetString("ClearTypes", "Clear"),
	THEME:GetString("ClearTypes", "Failed"),
	THEME:GetString("ClearTypes", "Invalid"),
	THEME:GetString("ClearTypes", "No Play"),
	"-",
}

-- Methods for other uses (manually setting colors/text, etc.)
local function getClearTypeText(index)
	return typetranslations[index]
end

local function getShortClearTypeText(index)
	return stypetable[index]
end

local function getClearTypeColor(index)
	return typecolors[index]
end

local function getClearTypeItem(clearlevel, ret)
	if ret == 0 then
		return typetable[clearlevel]
	elseif ret == 1 then
		return stypetable[clearlevel]
	elseif ret == 2 then
		return typecolors[clearlevel]
	else
		return clearlevel
	end
end

-- ClearTypes based on judgments or special cases
local function clearTypes(grade, playCount, perfcount, greatcount, misscount, returntype)
	grade = grade or 0
	playcount = playcount or 0
	misscount = misscount or 0
	clearlevel = 13 -- no play

	if grade == 0 then
		if playcount == 0 then
			clearlevel = 13
		end
	else
		if grade == "Grade_Failed" then -- failed
			clearlevel = 11
		elseif perfcount < 10 and perfcount > 1 then -- SDP
			clearlevel = 3
		elseif greatcount < 10 and greatcount > 1 then -- SDG
			clearlevel = 6
		elseif perfcount == 1 and greatcount + misscount == 0 then -- whiteflag
			clearlevel = 2
		elseif greatcount == 1 and misscount == 0 then -- blackflag
			clearlevel = 5
		elseif perfcount + greatcount + misscount == 0 then -- MFC
			clearlevel = 1
		elseif greatcount + misscount == 0 then -- PFC
			clearlevel = 4
		elseif misscount == 0 then -- FC
			clearlevel = 7
		else
			if misscount == 1 then -- missflag
				clearlevel = 8
			elseif misscount < 10 and misscount > 0 then -- SDCB
				clearlevel = 9
			else
				clearlevel = 10 -- Clear
			end
		end
	end
	return getClearTypeItem(clearlevel, returntype)
end

-- Returns the cleartype given the score
function getClearTypeFromScore(score, ret)
	local song
	local steps
	local profile
	local playCount = 0
	local greatcount = 0
	local perfcount = 0
	local misscount = 0
	
	if score == nil then
		return getClearTypeItem(13, ret)
	end
	song = GAMESTATE:GetCurrentSong()
	steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
	profile = GetPlayerOrMachineProfile(PLAYER_1)
	if score ~= nil and song ~= nil and steps ~= nil then
		playCount = profile:GetSongNumTimesPlayed(song)
		grade = score:GetGrade()
		perfcount = score:GetTapNoteScore("TapNoteScore_W2")
		greatcount = score:GetTapNoteScore("TapNoteScore_W3")
		misscount =
			score:GetTapNoteScore("TapNoteScore_Miss") + score:GetTapNoteScore("TapNoteScore_W5") +
			score:GetTapNoteScore("TapNoteScore_W4")
	end


	return clearTypes(grade, playCount, perfcount, greatcount, misscount, ret) or typetable[12]
end