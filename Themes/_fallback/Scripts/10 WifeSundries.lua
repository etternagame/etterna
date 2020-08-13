-- **Big thing that contains all sorts of fun stuff related to ms based scoring and adapting existing systems to it**
-- probably need to reorganize large parts of this
-- global here
ms = {}

-- *Values and stuff*
-- Make the distinction between ms timed and non-timed judgments

-- Radar values people on earth actually care about
ms.RelevantRadarsShort = {
	THEME:GetString("RadarCategoryShort", "Notes")..":",
	THEME:GetString("RadarCategoryShort", "Jumps")..":",
	THEME:GetString("RadarCategoryShort", "Hands")..":",
	THEME:GetString("RadarCategoryShort", "Holds")..":",
	THEME:GetString("RadarCategoryShort", "Mines")..":",
	THEME:GetString("RadarCategoryShort", "TotalTaps")..":",
	THEME:GetString("RadarCategoryShort", "Rolls")..":",
	THEME:GetString("RadarCategoryShort", "Lifts")..":",
	THEME:GetString("RadarCategoryShort", "Fakes")..":"
}

ms.RelevantRadars = {
	"RadarCategory_Notes",
	"RadarCategory_Jumps",
	"RadarCategory_Hands",
	"RadarCategory_Holds",
	"RadarCategory_Mines",
	"RadarCategory_TapsAndHolds",
	"RadarCategory_Rolls",
	"RadarCategory_Lifts",
	"RadarCategory_Fakes"
}

-- lists just for displayed judgment counts
ms.JudgeCount = {
	"TapNoteScore_W1",
	"TapNoteScore_W2",
	"TapNoteScore_W3",
	"TapNoteScore_W4",
	"TapNoteScore_W5",
	"TapNoteScore_Miss",
	"HoldNoteScore_Held",
	"HoldNoteScore_LetGo"
}

ms.JudgeCountInverse = {
	TapNoteScore_W1 = 1,
	TapNoteScore_W2 = 2,
	TapNoteScore_W3 = 3,
	TapNoteScore_W4 = 4,
	TapNoteScore_W5 = 5,
	TapNoteScore_Miss = 6,
	HoldNoteScore_Held = 7,
	HoldNoteScore_LetGo = 8
}

ms.SkillSets = {
	"Overall",
	"Stream",
	"Jumpstream",
	"Handstream",
	"Stamina",
	"JackSpeed",
	"Chordjack",
	"Technical"
}

ms.SkillSetsShort = {
	"Overall",
	"Stream",
	"JS",
	"HS",
	"Stam",
	"Jack Speed",
	"Chordjack",
	"Tech"
}

ms.SkillSetsTranslatedByName = {
	Overall = THEME:GetString("Skillsets", "Overall"),
	Stream = THEME:GetString("Skillsets", "Stream"),
	Jumpstream = THEME:GetString("Skillsets", "Jumpstream"),
	Handstream = THEME:GetString("Skillsets", "Handstream"),
	Stamina = THEME:GetString("Skillsets", "Stamina"),
	JackSpeed = THEME:GetString("Skillsets", "JackSpeed"),
	Chordjack = THEME:GetString("Skillsets", "Chordjack"),
	Technical = THEME:GetString("Skillsets", "Technical"),
}

ms.SkillSetsTranslated = {
	THEME:GetString("Skillsets", "Overall"),
	THEME:GetString("Skillsets", "Stream"),
	THEME:GetString("Skillsets", "Jumpstream"),
	THEME:GetString("Skillsets", "Handstream"),
	THEME:GetString("Skillsets", "Stamina"),
	THEME:GetString("Skillsets", "JackSpeed"),
	THEME:GetString("Skillsets", "Chordjack"),
	THEME:GetString("Skillsets", "Technical"),
}

ms.JudgeScalers = GAMESTATE:GetTimingScales()

local musicstr = THEME:GetString("GeneralInfo", "RateMusicString")

-- **Functions**
function ms.ok(m)
	if not m then
		SCREENMAN:SystemMessage("nahbro")
	else
		SCREENMAN:SystemMessage(m)
	end
end

--[[
	Print a thing to the error console (F3+F6+8 to enable, F3+F6+9 to make it persist)
	str: anything that can be used with tostring()
]]
function ms.p(str)
	MESSAGEMAN:Broadcast("ScriptError", {message = tostring(str)})
end

--[[
	This runs the LuaJIT profiler.
	Shows the approximate line of section of Lua that are being used the most.
	This will cause a minor fps drop.
]]
function ms.startjitprofiler()
	local profile = require("jit.profile")
	local tH = {}
	local tHS = {}
	profile.start(
		"li1",
		function(th, samples, vmmode)
			local f = require("jit.profile").dumpstack(th, "pl", 1)
			tH[f] = 1 + (tH[f] or 0)
			if not tHS[f] then
				tHS[f] = {}
			end
			tHS[f][vmmode] = (tHS[f][vmmode] or 0) + 1
		end
	)
	local function dump(o)
		if type(o) == "table" then
			local s = "{ "
			for k, v in pairs(o) do
				if type(k) ~= "number" then
					k = '"' .. k .. '"'
				end
				s = s .. "[" .. k .. "] = " .. dump(v) .. ",\n"
			end
			return s .. "} "
		else
			return tostring(o)
		end
	end
	SCREENMAN:GetTopScreen():setInterval(
		function()
			local tmp = {}
			local n = 0
			for k, v in pairs(tH) do
				tmp[n + 1] = {k, v}
				n = n + 1
			end
			table.sort(
				tmp,
				function(a, b)
					return a[2] > b[2]
				end
			)
			local str = ""
			for _, v in ipairs(tmp) do
				str = str .. dump(v[1]) .. " =" .. tostring(v[2]) .. "\n"
			end

			SCREENMAN:SystemMessage(str)
		end,
		1
	)
end

--[[
	Stop the LuaJIT profiler only if it has already been started.
]]
function ms.stopjitprofiler()
	local profile = require("jit.profile")
	profile.stop()
end

function ms.type(m)
	SCREENMAN:SystemMessage(type(m))
end

--- Table sum designed for ms tables (optional starting position, default to 0, and absolute value flag)
function ms.tableSum(t, n, abs)
	local o = 0
	for i = n or 0, #t do
		if abs then
			o = o + math.abs(t[i])
		else
			o = o + t[i]
		end
	end
	return o
end

function wifeMean(t)
	local c = #t
	local m = 0
	if c == 0 then
		return 0
	end
	local o = 0
	for i = 1, c do
		if t[i] ~= 1000 and t[i] ~= -1100 then
			o = o + t[i]
		else
			m = m + 1
		end
	end
	return o / (c - m)
end

function wifeAbsMean(t)
	local c = #t
	local m = 0
	if c == 0 then
		return 0
	end
	local o = 0
	for i = 1, c do
		if t[i] ~= 1000 and t[i] ~= -1100 then
			o = o + math.abs(t[i])
		else
			m = m + 1
		end
	end
	return o / (c - m)
end

function wifeSd(t)
	local u = wifeMean(t)
	local u2 = 0
	local m = 0
	for i = 1, #t do
		if t[i] ~= 1000 and t[i] ~= -1100 then
			u2 = u2 + (t[i] - u) ^ 2
		else
			m = m + 1
		end
	end
	return math.sqrt(u2 / (#t - 1 - m))
end

function wifeRange(t)
	local x, y = 10000, 0
	for i = 1, #t do
		if math.abs(t[i]) <= 180 then		-- some replays (online ones i think?) are flagging misses as 1100 for some reason
			if math.abs(t[i]) < math.abs(x) then
				x = t[i]
			end
			if math.abs(t[i]) > math.abs(y) then
				y = t[i]
			end
		end
	end
	return x, y
end

function IndexOf(t, x)
	for k, v in pairs(t) do
		if v == x then
			return k
		end
	end
	return nil
end

-- **Very slight efficiency rewrites**
notShit = {}
function notShit.floor(x, y)
	y = 10 ^ (y or 0)
	return math.floor(x * y) / y
end

function notShit.ceil(x, y)
	y = 10 ^ (y or 0)
	return math.ceil(x * y) / y
end

-- seriously what is math and how does it work
function notShit.round(x, y)
	y = 10 ^ (y or 0)
	return math.floor(x * y + 0.5) / y
end

-- Grabs the currently selected rate as a string in the form of "r.rrx" while dropping trailing 0s
function getCurRateString()
	return getRateString(getCurRateValue())
end

function getRateString(x)
	return string.format("%.2f", x):gsub("%.?0+$", "") .. "x"
end

function getCurRateDisplayString()
	return getRateDisplayString(getCurRateString())
end

function getRateDisplayString(x)
	if x == "1x" then
		x = "1.0x"
	elseif x == "2x" then
		x = "2.0x"
	end
	return x .. musicstr
end

function getCurRateValue()
	return notShit.round(GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(), 3)
end

function getCurKey()
	return GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey()
end

-- returns a string of keys for a table
function showKeys(t)
	local o = {}
	for k, v in pairs(t) do
		o[#o + 1] = k
	end
	return table.concat(o, ",") -- apparently theres an alias for this in this game who knew
end

-- same but returns a table (array)
function tableKeys(t)
	local o = {}
	for k, v in pairs(t) do
		o[#o + 1] = k
	end
	return o
end

function formLink(x, y)
	x[y] = x[y] or {}
	return x[y]
end

function GetPlayableTime()
	local step = GAMESTATE:GetCurrentSteps(PLAYER_1)
	return step:GetLengthSeconds()
end

function ChangeMusicRate(rate, params)
	if params.Name == "PrevScore" and rate < 2.95 and (getTabIndex() == 0 or getTabIndex() == 1) then
		GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate(rate + 0.1)
		GAMESTATE:GetSongOptionsObject("ModsLevel_Song"):MusicRate(rate + 0.1)
		GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(rate + 0.1)
		MESSAGEMAN:Broadcast("CurrentRateChanged")
	elseif params.Name == "NextScore" and rate > 0.55 and (getTabIndex() == 0 or getTabIndex() == 1) then
		GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate(rate - 0.1)
		GAMESTATE:GetSongOptionsObject("ModsLevel_Song"):MusicRate(rate - 0.1)
		GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(rate - 0.1)
		MESSAGEMAN:Broadcast("CurrentRateChanged")
	end

	if params.Name == "PrevRate" and rate < 3 and (getTabIndex() == 0 or getTabIndex() == 1) then
		GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate(rate + 0.05)
		GAMESTATE:GetSongOptionsObject("ModsLevel_Song"):MusicRate(rate + 0.05)
		GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(rate + 0.05)
		MESSAGEMAN:Broadcast("CurrentRateChanged")
	elseif params.Name == "NextRate" and rate > 0.5 and (getTabIndex() == 0 or getTabIndex() == 1) then
		GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate(rate - 0.05)
		GAMESTATE:GetSongOptionsObject("ModsLevel_Song"):MusicRate(rate - 0.05)
		GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(rate - 0.05)
		MESSAGEMAN:Broadcast("CurrentRateChanged")
	end
end

-- hur dur floats
for i = 1, #ms.JudgeScalers do
	ms.JudgeScalers[i] = notShit.round(ms.JudgeScalers[i], 2)
end