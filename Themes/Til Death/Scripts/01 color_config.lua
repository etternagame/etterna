local defaultConfig = {

	main = {
		highlight = "#614080",
		frames = "#000111",
		enabled = "#4CBB17",
		disabled = "#666666",
		negative = "#FF9999",
		positive = "#9654FD"
	},

	clearType = {
		MFC		= "#66ccff",
		WF		= "#dddddd",
		SDP 	= "#cc8800",
		PFC 	= "#eeaa00",
		BF		= "#999999",
		SDG		= "#448844",
		FC		= "#66cc66",
		MF		= "#cc6666",
		SDCB	= "#666666",
		Clear	= "#33aaff",
		Failed	= "#e61e25",
		Invalid	= "#e61e25",
		NoPlay	= "#666666",
		None	= "#666666",
	},

	difficulty = {
		Difficulty_Beginner	= "#66ccff",		-- light blue
		Difficulty_Easy		= "#099948",		-- green
		Difficulty_Medium	= "#ddaa00",		-- yellow
		Difficulty_Hard		= "#ff6666",		-- red
		Difficulty_Challenge= "#c97bff",	-- light blue
		Difficulty_Edit 	= "#666666",	-- gray
		Difficulty_Couple	= "#ed0972",			-- hot pink
		Difficulty_Routine	= "#ff9a00",			-- orange
		Beginner	= "#66ccff",		
		Easy		= "#099948",		-- green
		Medium		= "#ddaa00",		-- yellow
		Hard		= "#ff6666",		-- red
		Challenge 		= "#c97bff",	-- Purple
		Edit 		= "#666666",	-- gray
		Couple		= "#ed0972",			-- hot pink
		Routine		= "#ff9a00"			-- orange
	},

	difficultyVivid = {
		Difficulty_Beginner	= "#0099ff",		-- light blue
		Difficulty_Easy		= "#00ff00",		-- green
		Difficulty_Medium	= "#ffff00",		-- yellow
		Difficulty_Hard		= "#ff0000",		-- red
		Difficulty_Challenge= "#cc66ff",	-- light blue
		Difficulty_Edit 	= "#666666",	-- gray
		Difficulty_Couple	= "#ed0972",			-- hot pink
		Difficulty_Routine	= "#ff9a00",			-- orange
		Beginner	= "#0099ff",		-- light blue
		Easy		= "#00ff00",		-- green
		Medium		= "#ffff00",		-- yellow
		Hard		= "#ff0000",		-- red
		Challenge 		= "#cc66ff",	-- Purple
		Edit 		= "#666666",	-- gray
		Couple		= "#ed0972",			-- hot pink
		Routine		= "#ff9a00"			-- orange
	},

	grade = {
		Grade_Tier01	= "#66ccff", -- AAAA
		Grade_Tier02	= "#eebb00", -- AAA
		Grade_Tier03	= "#66cc66", -- AA
		Grade_Tier04	= "#da5757", -- A
		Grade_Tier05	= "#5b78bb", -- B
		Grade_Tier06	= "#c97bff", -- C
		Grade_Tier07	= "#8c6239", -- D
		Grade_Tier08	= "#000000", -- ITG PLS
		Grade_Tier09	= "#000000", -- ITG PLS
		Grade_Tier10	= "#000000", -- ITG PLS
		Grade_Tier11	= "#000000", -- ITG PLS
		Grade_Tier12	= "#000000", -- ITG PLS
		Grade_Tier13	= "#000000", -- ITG PLS
		Grade_Tier14	= "#000000", -- ITG PLS
		Grade_Tier15	= "#000000", -- ITG PLS
		Grade_Tier16	= "#000000", -- ITG PLS
		Grade_Tier17	= "#000000", -- ITG PLS
		Grade_Failed	= "#cdcdcd", -- F
		Grade_None		= "#666666", -- no play
	},

	judgment = { -- Colors of each Judgment types
		TapNoteScore_W1 = "#99ccff",
		TapNoteScore_W2	= "#f2cb30",
		TapNoteScore_W3	 = "#14cc8f",
		TapNoteScore_W4	= "#1ab2ff",
		TapNoteScore_W5	= "#ff1ab3",
		TapNoteScore_Miss = "#cc2929",			
		HoldNoteScore_Held = "#f2cb30",	
		HoldNoteScore_LetGo = "#cc2929"
	},

	songLength = {
		normal = "#FFFFFF", -- normal
		long = "#ff9a00", --orange
		marathon = "#da5757" -- red
	},
}

colorConfig = create_setting("colorConfig", "colorConfig.lua", defaultConfig,-1)
colorConfig:load()

--keys to current table. Assumes a depth of 2.
local curColor = {"",""}

function getTableKeys()
	return curColor
end

function setTableKeys(table)
	curColor = table 
end

function getMainColor(type)
	return color(colorConfig:get_data().main[type])
end

function getGradeColor (grade)
	return color(colorConfig:get_data().grade[grade]) or color(colorConfig:get_data().grade['Grade_None']);
end

function getDifficultyColor(diff)
	return color(colorConfig:get_data().difficulty[diff]) or color("#ffffff");
end

function getVividDifficultyColor(diff)
	return color(colorConfig:get_data().difficultyVivid[diff]) or color("#ffffff")
end

function offsetToJudgeColor(offset,scale)
	local offset = math.abs(offset)
	if not scale then
		scale = PREFSMAN:GetPreference("TimingWindowScale")
	end
	if offset <= scale*PREFSMAN:GetPreference("TimingWindowSecondsW1") then
		return color(colorConfig:get_data().judgment["TapNoteScore_W1"])
	elseif offset <= scale*PREFSMAN:GetPreference("TimingWindowSecondsW2") then
		return color(colorConfig:get_data().judgment["TapNoteScore_W2"])
	elseif offset <= scale*PREFSMAN:GetPreference("TimingWindowSecondsW3") then
		return color(colorConfig:get_data().judgment["TapNoteScore_W3"])
	elseif offset <= scale*PREFSMAN:GetPreference("TimingWindowSecondsW4") then
		return color(colorConfig:get_data().judgment["TapNoteScore_W4"])
	elseif offset <= scale*PREFSMAN:GetPreference("TimingWindowSecondsW5") then
		return color(colorConfig:get_data().judgment["TapNoteScore_W5"])
	else
		return color(colorConfig:get_data().judgment["TapNoteScore_Miss"])
	end
end

function byJudgment(judge)
	return color(colorConfig:get_data().judgment[judge])
end

function byDifficulty(diff)
	return color(colorConfig:get_data().difficulty[diff])
end

-- Colorized stuff
function ByMSD(x)
	if x then
		return HSV(math.max(95 - (x/40)*150, -50), 0.9, 0.9)
	end
	return HSV(0, 0.9, 0.9)
end

function ByMusicLength(x)
	if x then
		x = math.min(x,600)
		return HSV(math.max(95 - (x/900)*150, -50), 0.9, 0.9)
	end
	return HSV(0, 0.9, 0.9)
end
