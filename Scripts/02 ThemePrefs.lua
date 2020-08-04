--- OptionRow handlers for options defined in
-- metrics.ini under [ScreenPlayerOptions]
local function OptionNameString(str)
	return THEME:GetString("OptionNames", str)
end

--[[ option rows ]]
-- screen filter
function OptionRowScreenFilter()
	return {
		Name = "ScreenFilter",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {
			THEME:GetString("OptionNames", "Off"),
			"0.1",
			"0.2",
			"0.3",
			"0.4",
			"0.5",
			"0.6",
			"0.7",
			"0.8",
			"0.9",
			"1.0"
		},
		LoadSelections = function(self, list, pn)
			local pName = ToEnumShortString(pn)
			local filterValue = playerConfig:get_data(pn_to_profile_slot(pn)).ScreenFilter
			local value = scale(filterValue, 0, 1, 1, #list)
			list[value] = true
		end,
		SaveSelections = function(self, list, pn)
			local pName = ToEnumShortString(pn)
			local found = false
			local value = 0
			for i = 1, #list do
				if not found then
					if list[i] == true then
						value = scale(i, 1, #list, 0, 1)
						found = true
					end
				end
			end
			playerConfig:get_data(pn_to_profile_slot(pn)).ScreenFilter = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
end

local RSChoices = {}
for i = 1, 250 do
	RSChoices[i] = tostring(i) .. "%"
end
function ReceptorSize()
	local t = {
		Name = "ReceptorSize",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = RSChoices,
		LoadSelections = function(self, list, pn)
			local prefs = playerConfig:get_data(pn_to_profile_slot(pn)).ReceptorSize
			list[prefs] = true
		end,
		SaveSelections = function(self, list, pn)
			local found = false
			for i = 1, #list do
				if not found then
					if list[i] == true then
						local value = i
						playerConfig:get_data(pn_to_profile_slot(pn)).ReceptorSize = value
						found = true
					end
				end
			end
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

local ErrorBarCountChoices = {}
for i = 1, 200 do
	ErrorBarCountChoices[i] = tostring(i)
end
function ErrorBarCount()
	local t = {
		Name = "ErrorBarCount",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = ErrorBarCountChoices,
		LoadSelections = function(self, list, pn)
			local prefs = playerConfig:get_data(pn_to_profile_slot(pn)).ErrorBarCount
			list[prefs] = true
		end,
		SaveSelections = function(self, list, pn)
			local found = false
			for i = 1, #list do
				if not found then
					if list[i] == true then
						local value = i
						playerConfig:get_data(pn_to_profile_slot(pn)).ErrorBarCount = value
						found = true
					end
				end
			end
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function JudgmentText()
	local t = {
		Name = "JudgmentText",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Hide"), THEME:GetString("OptionNames", "Show")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).JudgmentText
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			playerConfig:get_data(pn_to_profile_slot(pn)).JudgmentText = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function ComboText()
	local t = {
		Name = "ComboText",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Hide"), THEME:GetString("OptionNames", "Show")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).ComboText
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			playerConfig:get_data(pn_to_profile_slot(pn)).ComboText = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function DisplayPercent()
	local t = {
		Name = "DisplayPercent",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).DisplayPercent
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			playerConfig:get_data(pn_to_profile_slot(pn)).DisplayPercent = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function TargetTracker()
	local t = {
		Name = "TargetTracker",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).TargetTracker
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			playerConfig:get_data(pn_to_profile_slot(pn)).TargetTracker = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

local tChoices = {}
for i = 1, 99 do
	tChoices[i] = tostring(i) .. "%"
end
local tChoices2 = {
	"99.50%",
	"99.60%",
	"99.70%", -- AAA
	"99.80%", -- AAA.
	"99.90%", -- AAA:
	"99.955%", -- AAAA
	"99.97%", -- AAAA.
	"99.98%", -- AAAA:
	"99.996%", -- AAAAA
	"100%" -- impossible
}
for _,v in ipairs(tChoices2) do
	tChoices[#tChoices+1] = v
end
function TargetGoal()
	local t = {
		Name = "TargetGoal",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = tChoices,
		LoadSelections = function(self, list, pn)
			local prefsval = playerConfig:get_data(pn_to_profile_slot(pn)).TargetGoal
			local index = IndexOf(tChoices, prefsval .. "%")
			list[index] = true
		end,
		SaveSelections = function(self, list, pn)
			local found = false
			for i = 1, #list do
				if not found then
					if list[i] == true then
						local value = i
						playerConfig:get_data(pn_to_profile_slot(pn)).TargetGoal =
							tonumber(string.sub(tChoices[value], 1, #tChoices[value] - 1))
						found = true
					end
				end
			end
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function TargetTrackerMode()
	local t = {
		Name = "TargetTrackerMode",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "SetPercent"), THEME:GetString("OptionNames", "PersonalBest")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).TargetTrackerMode
			list[pref + 1] = true
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[2] then
				value = 1
			else
				value = 0
			end
			playerConfig:get_data(pn_to_profile_slot(pn)).TargetTrackerMode = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function JudgeCounter()
	local t = {
		Name = "JudgeCounter",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).JudgeCounter
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			playerConfig:get_data(pn_to_profile_slot(pn)).JudgeCounter = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function PlayerInfo()
	local t = {
		Name = "PlayerInfo",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).PlayerInfo
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			playerConfig:get_data(pn_to_profile_slot(pn)).PlayerInfo = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function CBHighlight()
	local t = {
		Name = "CBHighlight",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).CBHighlight
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			playerConfig:get_data(pn_to_profile_slot(pn)).CBHighlight = list[2]
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function CustomizeGameplay()
	local t = {
		Name = "CustomizeGameplay",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).CustomizeGameplay
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			playerConfig:get_data(pn_to_profile_slot(pn)).CustomizeGameplay = list[2]
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function CustomEvalWindows()
	local t = {
		Name = "CustomEvalWindows",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).CustomEvaluationWindowTimings
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			playerConfig:get_data(pn_to_profile_slot(pn)).CustomEvaluationWindowTimings = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function ErrorBar()
	local t = {
		Name = "ErrorBar",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On"), THEME:GetString("OptionNames", "EWMA")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).ErrorBar
			list[pref + 1] = true
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] == true then
				value = 0
			elseif list[2] == true then
				value = 1
			else
				value = 2
			end
			playerConfig:get_data(pn_to_profile_slot(pn)).ErrorBar = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function LeaderBoard()
	local t = {
		Name = "Leaderboard",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).leaderboardEnabled
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			playerConfig:get_data(pn_to_profile_slot(pn)).leaderboardEnabled = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function FullProgressBar()
	local t = {
		Name = "FullProgressBar",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).FullProgressBar
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			playerConfig:get_data(pn_to_profile_slot(pn)).FullProgressBar = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function MiniProgressBar()
	local t = {
		Name = "MiniProgressBar",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).MiniProgressBar
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			value = list[2]
			playerConfig:get_data(pn_to_profile_slot(pn)).MiniProgressBar = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function LaneCover()
	local t = {
		Name = "LaneCover",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {
			THEME:GetString("OptionNames", "Off"),
			THEME:GetString("OptionNames", "Sudden"), 
			THEME:GetString("OptionNames", "Hidden")
		},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).LaneCover
			list[pref + 1] = true
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] == true then
				value = 0
			elseif list[2] == true then
				value = 1
			else
				value = 2
			end
			playerConfig:get_data(pn_to_profile_slot(pn)).LaneCover = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function NPSDisplay()
	local t = {
		Name = "NPSDisplay",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectMultiple",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "NPSDisplay"), THEME:GetString("OptionNames", "NPSGraph")},
		LoadSelections = function(self, list, pn)
			local npsDisplay = playerConfig:get_data(pn_to_profile_slot(pn)).NPSDisplay
			local npsGraph = playerConfig:get_data(pn_to_profile_slot(pn)).NPSGraph
			if npsDisplay then
				list[1] = true
			end
			if npsGraph then
				list[2] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			playerConfig:get_data(pn_to_profile_slot(pn)).NPSDisplay = list[1]
			playerConfig:get_data(pn_to_profile_slot(pn)).NPSGraph = list[2]
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function BackgroundType()
	local t = {
		Name = "BackgroundType",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {
			THEME:GetString("OptionNames", "Default"),
			THEME:GetString("OptionNames", "StaticBG"), 
			THEME:GetString("OptionNames", "RandomBG")
		},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).BackgroundType
			list[pref] = true
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] then
				value = 1
			elseif list[2] then
				value = 2
			else
				value = 3
			end
			playerConfig:get_data(pn_to_profile_slot(pn)).BackgroundType = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function DefaultScoreType()
	local t = {
		Name = "DefaultScoreType",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {"DP", "PS", "MIGS", "Wife", "Waifu"},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().global.DefaultScoreType
			if pref == 1 then
				list[1] = true
			elseif pref == 2 then
				list[2] = true
			elseif pref == 3 then
				list[3] = true
			elseif pref == 4 then
				list[4] = true
			else
				list[5] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] == true then
				value = 1
			elseif list[2] == true then
				value = 2
			elseif list[3] == true then
				value = 3
			elseif list[4] == true then
				value = 4
			else
				value = 5
			end
			themeConfig:get_data().global.DefaultScoreType = value
			themeConfig:set_dirty()
			themeConfig:save()
		end
	}
	setmetatable(t, t)
	return t
end

function TipType()
	local t = {
		Name = "TipType",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {
			THEME:GetString("OptionNames", "Off"),
			THEME:GetString("OptionNames", "Tips"),
			THEME:GetString("OptionNames", "RandomPhrases")
		},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().global.TipType
			if pref == 1 then
				list[1] = true
			elseif pref == 2 then
				list[2] = true
			else
				list[3] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] == true then
				value = 1
			elseif list[2] == true then
				value = 2
			else
				value = 3
			end
			themeConfig:get_data().global.TipType = value
			themeConfig:set_dirty()
			themeConfig:save()
		end
	}
	setmetatable(t, t)
	return t
end

function SongBGEnabled()
	local t = {
		Name = "SongBGEnabled",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().global.SongBGEnabled
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] then
				value = false
			else
				value = true
			end
			themeConfig:get_data().global.SongBGEnabled = value
			themeConfig:set_dirty()
			themeConfig:save()
		end
	}
	setmetatable(t, t)
	return t
end

function SongBGMouseEnabled()
	local t = {
		Name = "SongBGMouseEnabled",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().global.SongBGMouseEnabled
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] then
				value = false
			else
				value = true
			end
			themeConfig:get_data().global.SongBGMouseEnabled = value
			themeConfig:set_dirty()
			themeConfig:save()
		end
	}
	setmetatable(t, t)
	return t
end

function EvalBGType()
	local t = {
		Name = "EvalBGType",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {
			THEME:GetString("OptionNames", "SongBackground"),
			THEME:GetString("OptionNames", "ClearGradeBackground"),
			THEME:GetString("OptionNames", "GradeBackground")
		},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().eval.SongBGType
			if pref == 1 then
				list[1] = true
			elseif pref == 2 then
				list[2] = true
			else
				list[3] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] == true then
				value = 1
			elseif list[2] == true then
				value = 2
			else
				value = 3
			end
			themeConfig:get_data().eval.SongBGType = value
			themeConfig:set_dirty()
			themeConfig:save()
		end
	}
	setmetatable(t, t)
	return t
end

function Particles()
	local t = {
		Name = "Particles",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().global.Particles
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] then
				value = false
			else
				value = true
			end
			themeConfig:get_data().global.Particles = value
			themeConfig:set_dirty()
			themeConfig:save()
		end
	}
	setmetatable(t, t)
	return t
end

function RateSort()
	local t = {
		Name = "RateSort",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().global.RateSort
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] then
				value = false
			else
				value = true
			end
			themeConfig:get_data().global.RateSort = value
			themeConfig:set_dirty()
			themeConfig:save()
		end
	}
	setmetatable(t, t)
	return t
end

function HelpMenu()
	local t = {
		Name = "HelpMenu",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().global.HelpMenu
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] then
				value = false
			else
				value = true
			end
			themeConfig:get_data().global.HelpMenu = value
			themeConfig:set_dirty()
			themeConfig:save()
		end
	}
	setmetatable(t, t)
	return t
end

function MeasureLines()
	local t = {
		Name = "MeasureLines",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().global.MeasureLines
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] then
				value = false
			else
				value = true
			end
			themeConfig:get_data().global.MeasureLines = value
			themeConfig:set_dirty()
			themeConfig:save()
			THEME:ReloadMetrics()
		end
	}
	setmetatable(t, t)
	return t
end

function ShowVisualizer()
	local t = {
		Name = "ShowVisualizer",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().global.ShowVisualizer
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] then
				value = false
			else
				value = true
			end
			themeConfig:get_data().global.ShowVisualizer = value
			themeConfig:set_dirty()
			themeConfig:save()
		end
	}
	setmetatable(t, t)
	return t
end

function InstantSearch()
	local t = {
		Name = "InstantSearch",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().global.InstantSearch
			if pref then
				list[2] = true
			else
				list[1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			if list[1] then
				value = false
			else
				value = true
			end
			themeConfig:get_data().global.InstantSearch = value
			themeConfig:set_dirty()
			themeConfig:save()
		end
	}
	setmetatable(t, t)
	return t
end

function ProgressBar()
	local keymode = getCurrentKeyMode()
	local t = {
		Name = "ProgressBar",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {THEME:GetString("OptionNames", "Bottom"), THEME:GetString("OptionNames", "Top")},
		LoadSelections = function(self, list, pn)
			local pref = playerConfig:get_data(pn_to_profile_slot(pn)).GameplayXYCoordinates[keymode].ProgressBarPos
			if pref then
				list[pref + 1] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value = playerConfig:get_data(pn_to_profile_slot(pn)).GameplayXYCoordinates[keymode].ProgressBarPos
			if list[1] == true then
				if value ~= 0 then
					value = 0
					playerConfig:get_data(pn_to_profile_slot(pn)).GameplayXYCoordinates[keymode].FullProgressBarY = SCREEN_BOTTOM - 30
				end
			elseif value ~= 1 then
				value = 1
				playerConfig:get_data(pn_to_profile_slot(pn)).GameplayXYCoordinates[keymode].FullProgressBarY = 20
			end
			playerConfig:get_data(pn_to_profile_slot(pn)).GameplayXYCoordinates[keymode].ProgressBarPos = value
			playerConfig:set_dirty(pn_to_profile_slot(pn))
			playerConfig:save(pn_to_profile_slot(pn))
		end
	}
	setmetatable(t, t)
	return t
end

function NPSWindow()
	local t = {
		Name = "NPSWindow",
		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = true,
		Choices = {"1", "2", "3", "4", "5"},
		LoadSelections = function(self, list, pn)
			local pref = themeConfig:get_data().NPSDisplay.MaxWindow
			if pref then
				list[pref] = true
			end
		end,
		SaveSelections = function(self, list, pn)
			local value
			for k, v in ipairs(list) do
				if v then
					value = k
				end
			end
			themeConfig:get_data().NPSDisplay.MaxWindow = value
			themeConfig:set_dirty()
			themeConfig:save()
		end
	}
	setmetatable(t, t)
	return t
end
