--- Theme Preferences
-- @module 02_ThemePrefs
--[[
ThemePrefs: handles the underlying structure for ThemePrefs, so any themes
built off of this can simply declare their prefs and default values, and
access them through this system.

v0.7.2: Jan. 10, 2012. Added ThemePrefs.ForceSave function [freem]
v0.7.1: Dec. 28, 2010. Added language support.
v0.7.0: Dec. 15, 2010. Initial version.

vyhd wrote this for sm-ssc. <3 you guys
--]]
-- local function to handle themed error strings
-- (and to ensure we're getting all of them from the same section)
local function GetString(name)
	return THEME:GetString("ThemePrefs", name)
end

function PrintTable(tbl)
	Trace("Printing table")
	for k, v in pairs(tbl) do
		Trace(("[%s] -> %s"):format(tostring(k), tostring(v)))
	end
end

local ThemePrefsPath = "Save/ThemePrefs.ini"
local FallbackTheme = "_fallback"

-- This will be set on load.
local PrefsTable = nil

-- Gets the name of the current theme using themeInfo
-- if available and the ThemeManager name otherwise.
local function GetThemeName()
	return themeInfo and themeInfo.Name or THEME:GetThemeDisplayName()
end

-- Given a preference name, returns the table it's in. Checks the current
-- theme first, then _fallback, then all other sections, in that order.
local function ResolveTable(pref)
	-- check the section for this theme
	local name = GetThemeName()
	local val = PrefsTable[name][pref]

	if val ~= nil then
		--Trace( ("ResolveTable(%s): found in %s"):format(pref,name) )
		return PrefsTable[name]
	end

	-- not in the current theme; check the fallback if it exists
	if PrefsTable[FallbackTheme] then
		val = PrefsTable[FallbackTheme][pref]
		if val ~= nil then
			--Trace( ("ResolveTable(%s): found in fallback"):format(pref) )
			return PrefsTable[FallbackTheme]
		end
	end

	-- not there either. check every section.
	-- XXX: we should do this less redundantly.
	for section, _ in pairs(PrefsTable) do
		val = PrefsTable[section][pref]
		if val ~= nil then
			--Trace( ("ResolveTable(%s): found in section %s"):format(pref,section) )
			return PrefsTable[section]
		end
	end

	-- not found at all
	Trace(("ResolveTable(%s): pref not found"):format(pref))
	return nil
end

ThemePrefs = {
	NeedsSaved = false,
	-- Loads preferences from Save/ThemePrefs.ini, then adds theme
	-- preferences (and default values if applicable) to PrefsTable.
	-- Only read from disk once, when _fallback calls this; we just
	-- need the base set once to add prefs onto.
	Init = function(prefs, bLoadFromDisk)
		-- If we don't have IniFile, we can't read/write from/to disk
		if not IniFile then
			Warn(GetString("IniFileMissing"))
		end

		--		Trace( ("ThemePrefs.Init(prefs, %s)"):format(tostring(bLoadFromDisk)) )
		if bLoadFromDisk then
			Trace("ThemePrefs.Init: loading from disk")
			if not ThemePrefs.Load() then
				return false
			end
		end

		--		Trace( "ThemePrefs.Init: not loading from disk" )

		-- create the section if it doesn't exist
		local section = GetThemeName()
		--		Trace( ("ThemePrefs.Init: Theme name is \"%s\""):format(section) )
		PrefsTable[section] = PrefsTable[section] and PrefsTable[section] or {}

		--Trace( "Using section " .. section )

		-- if the key doesn't exist, add it with our default value
		for k, tbl in pairs(prefs) do
			if PrefsTable[section][k] == nil then
				Trace(k .. " doesn't exist, creating")
				PrefsTable[section][k] = tbl.Default
			end
		end

		--		PrintTable( PrefsTable[section] )
	end,
	Load = function()
		if not IniFile then
			return false
		end
		PrefsTable = IniFile.ReadFile(ThemePrefsPath)
		return true
	end,
	Save = function()
		--		Trace( "ThemePrefs.Save" )
		if IniFile and ThemePrefs.NeedsSaved then
			IniFile.WriteFile(ThemePrefsPath, PrefsTable)
			ThemePrefs.NeedsSaved = false
			return
		end
	end,
	-- for when you absolutely have to save, no matter what NeedsSaved says.
	ForceSave = function()
		if not IniFile then
			return false
		end
		ThemePrefs.NeedsSaved = false
		IniFile.WriteFile(ThemePrefsPath, PrefsTable)
	end,
	Get = function(name)
		--Trace( ("ThemePrefs.Get(%s)"):format(name) )
		local tbl = ResolveTable(name)
		if tbl then
			return tbl[name]
		end
		Warn("Get: " .. GetString("UnknownPreference"):format(name))
		return nil
	end,
	Set = function(name, value)
		--Trace( ("ThemePrefs.Set(%s, %s)"):format(name, tostring(value)) )
		local tbl = ResolveTable(name)
		if tbl then
			ThemePrefs.NeedsSaved = true
			tbl[name] = value
			return
		end
		Warn("Set: " .. GetString("UnknownPreference"):format(name))
	end
}

-- global aliases
GetThemePref = ThemePrefs.Get
SetThemePref = ThemePrefs.Set

local OE = OptEffect:Reverse()

-- bring in SpecialScoring from default.

function InitUserPrefs()
	if GetUserPref("UserPrefScoringMode") == nil then
		SetUserPref("UserPrefScoringMode", "DDR Extreme")
	end
end

function getScreenOptionsInputLines()
    if HOOKS.GetArchName():upper():find("^WINDOWS") ~= nil then
        -- windows
        return "1,2,3,AH,AS,5,6,8,7,DS,WindowsKey,KeyboardLayout"
    else
        -- mac and linux
        return "1,2,3,AH,AS,5,6,8,7,DS"
    end
end

-- Practice Mode Lua version because I don't know what else to do
function PracticeMode()
	local t = {
		Name = "PracticeMode",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
        ExportOnCancel = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = GAMESTATE:IsPracticeMode()
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			GAMESTATE:SetPracticeMode(value)
		end
	}
	setmetatable(t, t)
	return t
end

function JudgeDifficulty()
	local t = {
		Name = "TimingWindowScale",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
        ExportOnCancel = false,
		Choices = {"4", "5", "6", "7", "8", THEME:GetString("OptionNames", "Justice")},
		LoadSelections = function(self, list, pn)
			local td = math.max(GetTimingDifficulty() - 3, 1)
			-- value is a judge number rather than a timing scale
			list[td] = true
		end,
		SaveSelections = function(self, list, pn)
			local val = 1.0
			local found = false
			for i = 1, #list do
				if not found then
					if list[i] == true then
						value = notShit.round(GAMESTATE:GetTimingScales()[i+3], 2)
						found = true
					end
				end
			end
			-- value is a timing scale rather than a judge number
			SetTimingDifficulty(value)
		end
	}
	setmetatable(t, t)
	return t
end

function RateList()
    local ratelist = {}
    do
        local startrate = MIN_MUSIC_RATE -- 0.05
        local upperrate = MAX_MUSIC_RATE -- 3.0
        local increment = 0.05
        while startrate <= upperrate do
            ratelist[#ratelist+1] = tostring(startrate) .. "x"
            startrate = notShit.round(startrate + increment, 2)
        end
    end

    local t = {
        Name = "RateList",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = true,
        ExportOnCancel = true,
        Choices = ratelist,
        LoadSelections = function(self, list, pn)
            local rateindex = 1
            local rate = notShit.round(GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(), 2)
            local acceptable_delta = 0.025 / 2
            for i = 1, #ratelist do
                local r = tonumber(ratelist[i]:sub(1, -2))
                if r == rate or (rate - acceptable_delta <= r and rate + acceptable_delta >= r) then
                    rateindex = i
                    break
                end
            end
            list[rateindex] = true
        end,
        SaveSelections = function(self, list, pn)
            for i, v in ipairs(list) do
                if v == true then
                    local r = notShit.round(tonumber(ratelist[i]:sub(1, -2)), 2)
                    GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate(r)
                    GAMESTATE:GetSongOptionsObject("ModsLevel_Song"):MusicRate(r)
                    GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(r)
                    MESSAGEMAN:Broadcast("RateListOptionSaved", {rate = getCurRateValue()})
                    MESSAGEMAN:Broadcast("CurrentRateChanged")
                    break
                end
            end
        end,
		NotifyOfSelection = function(self, pn, choice)
			MESSAGEMAN:Broadcast("RateListOptionChanged", {rate = getCurRateValue()})
		end
    }
    setmetatable(t, t)
    return t
end

function InputDebounceTime()
    local delaylist = {}
    do
		-- in milliseconds, 100 is pretty egregious
        local start = 0
        local upper = 0.100
        local increment = 0.001
        while start <= upper do
			-- these rounds should force it to be milliseconds only
            delaylist[#delaylist+1] = tostring(notShit.round(start * 1000)) .. "ms"
            start = notShit.round(start + increment, 3)
        end
    end

    local t = {
        Name = "InputDebounceTime",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = true,
        ExportOnCancel = true,
        Choices = delaylist,
        LoadSelections = function(self, list, pn)
            local rateindex = 1
            local rate = notShit.round(PREFSMAN:GetPreference("InputDebounceTime"), 4)
            local acceptable_delta = 0.0005
            for i = 1, #delaylist do
                local r = tonumber(delaylist[i]:sub(1, -3)) / 1000
                if r == rate or (rate - acceptable_delta <= r and rate + acceptable_delta >= r) then
                    rateindex = i
                    break
                end
            end
            list[rateindex] = true
        end,
        SaveSelections = function(self, list, pn)
            for i, v in ipairs(list) do
                if v == true then
                    local r = notShit.round(tonumber(delaylist[i]:sub(1, -3)) / 1000, 3)
					PREFSMAN:SetPreference("InputDebounceTime", r)
                    break
                end
            end
        end,
		NotifyOfSelection = function(self, pn, choice)
			MESSAGEMAN:Broadcast("InputDebounceOptionChanged", {value = PREFSMAN:GetPreference("InputDebounceTime")})
		end
    }
    setmetatable(t, t)
    return t
end

function ScrollDebounceTime() -- Modified input debounce time.
    local delaylist = {}
    do
		-- in milliseconds, 100 is pretty egregious
		-- ^^^ true.. But why not let people do what they want
        local start = 0
        local upper = 0.100
        local increment = 0.001
        while start <= upper do
			-- these rounds should force it to be milliseconds only
            delaylist[#delaylist+1] = tostring(notShit.round(start * 1000)) .. "ms"
            start = notShit.round(start + increment, 3)
        end
    end

    local t = {
        Name = "ScrollDebounceTime",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = true,
        ExportOnCancel = true,
        Choices = delaylist,
        LoadSelections = function(self, list, pn)
            local rateindex = 1
            local rate = notShit.round(PREFSMAN:GetPreference("ScrollDebounceTime"), 4)
            local acceptable_delta = 0.0005
            for i = 1, #delaylist do
                local r = tonumber(delaylist[i]:sub(1, -3)) / 1000
                if r == rate or (rate - acceptable_delta <= r and rate + acceptable_delta >= r) then
                    rateindex = i
                    break
                end
            end
            list[rateindex] = true
        end,
        SaveSelections = function(self, list, pn)
            for i, v in ipairs(list) do
                if v == true then
                    local r = notShit.round(tonumber(delaylist[i]:sub(1, -3)) / 1000, 3)
					PREFSMAN:SetPreference("ScrollDebounceTime", r)
                    break
                end
            end
        end,
		NotifyOfSelection = function(self, pn, choice)
			MESSAGEMAN:Broadcast("ScrollDebounceOptionChanged", {value = PREFSMAN:GetPreference("ScrollDebounceTime")})
		end
    }
    setmetatable(t, t)
    return t
end

function FrameLimitGlobal()
    local delaylist = {"0","30","40","50","60","70","80","90"}
    do
        local start = 100
        local upper = 1000
        local increment = 50
        while start <= upper do
            delaylist[#delaylist+1] = tostring(start)
            start = start + increment
        end
		start = 2000
        upper = 5000
        increment = 1000
        while start <= upper do
            delaylist[#delaylist+1] = tostring(start)
            start = start + increment
        end
    end

    local t = {
        Name = "FrameLimitGlobal",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = true,
        ExportOnCancel = true,
        Choices = delaylist,
        LoadSelections = function(self, list, pn)
            local rateindex = 1
			local diff = 999999999
            local rate = notShit.round(PREFSMAN:GetPreference("FrameLimit"), 0)
            for i = 1, #delaylist do
                local r = tonumber(delaylist[i])
                if r == rate then
                    rateindex = i
                    break
                end
				if math.abs(r - rate) < diff then
					diff = math.abs(r-rate)
					rateindex = i
				else
					-- assuming sorted/sequential, exit early
					break
				end
            end
            list[rateindex] = true
        end,
        SaveSelections = function(self, list, pn)
            for i, v in ipairs(list) do
                if v == true then
                    local r = notShit.round(tonumber(delaylist[i]), 0)
					PREFSMAN:SetPreference("FrameLimit", r)
                    break
                end
            end
        end,
		NotifyOfSelection = function(self, pn, choice)
			MESSAGEMAN:Broadcast("FrameLimitGlobalOptionChanged", {value = PREFSMAN:GetPreference("FrameLimit")})
		end
    }
    setmetatable(t, t)
    return t
end

function FrameLimitGameplay()
    local delaylist = {"0","30","40","50","60","70","80","90"}
    do
        local start = 100
        local upper = 1000
        local increment = 50
        while start <= upper do
            delaylist[#delaylist+1] = tostring(start)
            start = start + increment
        end
		start = 2000
        upper = 5000
        increment = 1000
        while start <= upper do
            delaylist[#delaylist+1] = tostring(start)
            start = start + increment
        end
    end

    local t = {
        Name = "FrameLimitGameplay",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = delaylist,
        LoadSelections = function(self, list, pn)
            local rateindex = 1
			local diff = 999999999
            local rate = notShit.round(PREFSMAN:GetPreference("FrameLimitGameplay"), 0)
            for i = 1, #delaylist do
                local r = tonumber(delaylist[i])
                if r == rate then
                    rateindex = i
                    break
                end
				if math.abs(r - rate) < diff then
					diff = math.abs(r-rate)
					rateindex = i
				else
					-- assuming sorted/sequential, exit early
					break
				end
            end
            list[rateindex] = true
        end,
        SaveSelections = function(self, list, pn)
            for i, v in ipairs(list) do
                if v == true then
                    local r = notShit.round(tonumber(delaylist[i]), 0)
					PREFSMAN:SetPreference("FrameLimitGameplay", r)
                    break
                end
            end
        end,
		NotifyOfSelection = function(self, pn, choice)
			MESSAGEMAN:Broadcast("FrameLimitGameplayOptionChanged", {value = PREFSMAN:GetPreference("FrameLimitGameplay")})
		end
    }
    setmetatable(t, t)
    return t
end

function VisualDelaySeconds()
    local delaylist = {}
    do
		-- in milliseconds, 100 is pretty egregious
        local start = -0.100
        local upper = 0.100
        local increment = 0.001
        while start <= upper do
			-- these rounds should force it to be milliseconds only
            delaylist[#delaylist+1] = tostring(notShit.round(start * 1000)) .. "ms"
            start = notShit.round(start + increment, 3)
        end
    end

    local t = {
        Name = "VisualDelaySeconds",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = true,
        ExportOnCancel = true,
        Choices = delaylist,
        LoadSelections = function(self, list, pn)
            local rateindex = 1
            local rate = notShit.round(PREFSMAN:GetPreference("VisualDelaySeconds"), 4)
            local acceptable_delta = 0.0005
            for i = 1, #delaylist do
                local r = tonumber(delaylist[i]:sub(1, -3)) / 1000
                if r == rate or (rate - acceptable_delta <= r and rate + acceptable_delta >= r) then
                    rateindex = i
                    break
                end
            end
            list[rateindex] = true
        end,
        SaveSelections = function(self, list, pn)
            for i, v in ipairs(list) do
                if v == true then
                    local r = notShit.round(tonumber(delaylist[i]:sub(1, -3)) / 1000, 3)
					PREFSMAN:SetPreference("VisualDelaySeconds", r)
                    break
                end
            end
        end,
		NotifyOfSelection = function(self, pn, choice)
			MESSAGEMAN:Broadcast("VisualDelayOptionChanged", {value = PREFSMAN:GetPreference("VisualDelaySeconds")})
		end
    }
    setmetatable(t, t)
    return t
end

function GlobalOffsetSeconds()
    local numlist = {}
    do
		-- in milliseconds, 100 is pretty egregious
        local start = -0.200
        local upper = 0.200
        local increment = 0.001
        while start <= upper do
			-- these rounds should force it to be milliseconds only
            numlist[#numlist+1] = tostring(notShit.round(start * 1000)) .. "ms"
            start = notShit.round(start + increment, 3)
        end
    end

    local t = {
        Name = "GlobalOffsetSeconds",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = true,
        ExportOnCancel = true,
        Choices = numlist,
        LoadSelections = function(self, list, pn)
            local rateindex = 1
            local rate = notShit.round(PREFSMAN:GetPreference("GlobalOffsetSeconds"), 4)
            local acceptable_delta = 0.0005
            for i = 1, #numlist do
                local r = tonumber(numlist[i]:sub(1, -3)) / 1000
                if r == rate or (rate - acceptable_delta <= r and rate + acceptable_delta >= r) then
                    rateindex = i
                    break
                end
            end
            list[rateindex] = true
        end,
        SaveSelections = function(self, list, pn)
            for i, v in ipairs(list) do
                if v == true then
                    local r = notShit.round(tonumber(numlist[i]:sub(1, -3)) / 1000, 3)
					PREFSMAN:SetPreference("GlobalOffsetSeconds", r)
                    break
                end
            end
        end,
		NotifyOfSelection = function(self, pn, choice)
			MESSAGEMAN:Broadcast("GlobalOffsetOptionChanged", {value = PREFSMAN:GetPreference("GlobalOffsetSeconds")})
		end
    }
    setmetatable(t, t)
    return t
end

function GranularHiddenOffset()
    local HOlist = {}
    do
        local startperc = -50
        local upperperc = 150
        local increment = 10
        while startperc <= upperperc do
            HOlist[#HOlist+1] = tostring(startperc) .. "%"
            startperc = notShit.round(startperc + increment, 2)
        end
    end
	local t = {
        Name = "GranularHiddenOffset",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = true,
        ExportOnCancel = true,
        Choices = HOlist,
	LoadSelections = function(self, list, pn)
		local HOindex = 1
		local HiddenOffset = notShit.round(GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Current"):HiddenOffset(), 1)
		local acceptable_delta = 0.05 / 2
		for i = 1, #HOlist do
			local ho = tonumber(HOlist[i]:sub(1, -2)/100)
			if ho == HiddenOffset or (HiddenOffset - acceptable_delta <= ho and HiddenOffset + acceptable_delta >= ho) then
				HOindex = i
				break
			end
		end
		list[HOindex] = true
	end,
	SaveSelections = function(self, list, pn)
		for i, v in ipairs(list) do
			if v == true then
				local ho = notShit.round(tonumber(HOlist[i]:sub(1, -2)/100), 2)
				GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred"):HiddenOffset(ho)
				GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Song"):HiddenOffset(ho)
				GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Current"):HiddenOffset(ho)
				break
			end
		end
	end
	}
    setmetatable(t, t)
	return t
end

function GranularSuddenOffset()
    local SOlist = {}
    do
        local startperc = -50
        local upperperc = 150
        local increment = 10
        while startperc <= upperperc do
            SOlist[#SOlist+1] = tostring(startperc) .. "%"
            startperc = notShit.round(startperc + increment, 2)
        end
    end
	local t = {
        Name = "GranularSuddenOffset",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = true,
        ExportOnCancel = true,
        Choices = SOlist,
	LoadSelections = function(self, list, pn)
		local SOindex = 1
		local SuddenOffset = notShit.round(GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Current"):SuddenOffset(), 1)
		local acceptable_delta = 0.05 / 2
		for i = 1, #SOlist do
			local so = tonumber(SOlist[i]:sub(1, -2)/100)
			if so == SuddenOffset or (SuddenOffset - acceptable_delta <= so and SuddenOffset + acceptable_delta >= so) then
				SOindex = i
				break
			end
		end
		list[SOindex] = true
	end,
	SaveSelections = function(self, list, pn)
		for i, v in ipairs(list) do
			if v == true then
				local so = notShit.round(tonumber(SOlist[i]:sub(1, -2)/100), 2)
				GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred"):SuddenOffset(so)
				GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Song"):SuddenOffset(so)
				GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Current"):SuddenOffset(so)
				break
			end
		end
	end
	}
    setmetatable(t, t)
	return t
end

function SoundVolumeControl()
    local numlist = {}
    do
        local start = 0
        local upper = 1
        local increment = 0.05
        while start <= upper do
			-- these rounds should force it to be milliseconds only
            numlist[#numlist+1] = tostring(notShit.round(start * 100)) .. "%"
            start = notShit.round(start + increment, 3)
        end
    end

    local t = {
        Name = "SoundVolume",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = true,
        ExportOnChange = true,
        ExportOnCancel = false,
        Choices = numlist,
        LoadSelections = function(self, list, pn)
            local rateindex = 1
            local rate = notShit.round(PREFSMAN:GetPreference("SoundVolume"), 4)
            local acceptable_delta = 0.0005
            for i = 1, #numlist do
                local r = tonumber(numlist[i]:sub(1, -2)) / 100
                if r == rate or (rate - acceptable_delta <= r and rate + acceptable_delta >= r) then
                    rateindex = i
                    break
                end
            end
            list[rateindex] = true
        end,
        SaveSelections = function(self, list, pn)
            for i, v in ipairs(list) do
                if v == true then
                    local r = notShit.round(tonumber(numlist[i]:sub(1, -2)) / 100, 3)
					PREFSMAN:SetPreference("SoundVolume", r)
					SOUND:SetVolume(r)
                    break
                end
            end
        end,
		NotifyOfSelection = function(self, pn, choice)
			MESSAGEMAN:Broadcast("SoundVolumeOptionChanged", {value = PREFSMAN:GetPreference("SoundVolume")})
		end
    }
    setmetatable(t, t)
    return t
end

function DisableWindowsKeyInGameplay()
	local effectsMask = 2^OE["OptEffect_SavePreferences"]
	effectsMask = effectsMask + 2^OE["OptEffect_ApplyGraphics"]

    local t = {
		Name = "DisableWindowsKey",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
        ExportOnCancel = false,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = PREFSMAN:GetPreference("DisableWindowsKey")
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			PREFSMAN:SetPreference("DisableWindowsKey", value)

            return 0
		end
	}
	setmetatable(t, t)
	return t
end

function MaxTextureResolutionOption()
    local effectsMask = 2^OE["OptEffect_SavePreferences"]
    effectsMask = effectsMask + 2^OE["OptEffect_ApplyGraphics"]
    local numlist = {"256", "512", "1024", "2048", "4096", "8192", "Unlimited"}

    local t = {
		Name = "MaxTextureResolution",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
        ExportOnCancel = false,
		Choices = numlist,
		LoadSelections = function(self, list, pn)
			local pref = PREFSMAN:GetPreference("MaxTextureResolution")
			for i = 1, #numlist do
                local res = tonumber(numlist[i])
                if res == nil then res = 1048576 end
                if res == pref then list[i] = true break end
            end
		end,
		SaveSelections = function(self, list, pn)
			for i, v in ipairs(list) do
                if v == true then
                    local res = tonumber(numlist[i])
					if res == nil then res = 1048576 end
                    PREFSMAN:SetPreference("MaxTextureResolution", res)
                    break
                end
            end
            return effectsMask
		end
	}
	setmetatable(t, t)
	return t
end

function FixKeyboardLayout()

    local t = {
		Name = "FixKeyboardLayout",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
        ExportOnCancel = false,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = PREFSMAN:GetPreference("FixKeyboardLayout")
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			PREFSMAN:SetPreference("FixKeyboardLayout", value)

            return 0
		end
	}
	setmetatable(t, t)
	return t
end
