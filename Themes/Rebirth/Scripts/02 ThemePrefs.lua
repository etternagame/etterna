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
        ExportOnChange = false,
        ExportOnCancel = true,
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
            local filterValue = playerConfig:get_data().ScreenFilter
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
            playerConfig:get_data().ScreenFilter = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = RSChoices,
        LoadSelections = function(self, list, pn)
            local prefs = playerConfig:get_data().ReceptorSize
            list[prefs] = true
        end,
        SaveSelections = function(self, list, pn)
            local found = false
            for i = 1, #list do
                if not found then
                    if list[i] == true then
                        local value = i
                        playerConfig:get_data().ReceptorSize = value
                        found = true
                    end
                end
            end
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = ErrorBarCountChoices,
        LoadSelections = function(self, list, pn)
            local prefs = playerConfig:get_data().ErrorBarCount
            list[prefs] = true
        end,
        SaveSelections = function(self, list, pn)
            local found = false
            for i = 1, #list do
                if not found then
                    if list[i] == true then
                        local value = i
                        playerConfig:get_data().ErrorBarCount = value
                        found = true
                    end
                end
            end
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Hide"), THEME:GetString("OptionNames", "Show")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().JudgmentText
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().JudgmentText = value
            playerConfig:set_dirty()
            playerConfig:save()
        end
    }
    setmetatable(t, t)
    return t
end

function JudgmentAnimations()
    local t = {
        Name = "JudgmentAnimations",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().JudgmentTweens
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().JudgmentTweens = value
            playerConfig:set_dirty()
            playerConfig:save()
        end
    }
    setmetatable(t, t)
    return t
end

function ComboTweens()
    local t = {
        Name = "ComboTweens",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().ComboTweens
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().ComboTweens = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Hide"), THEME:GetString("OptionNames", "Show")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().ComboText
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().ComboText = value
            playerConfig:set_dirty()
            playerConfig:save()
        end
    }
    setmetatable(t, t)
    return t
end

function ComboLabel()
    local t = {
        Name = "ComboLabel",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = false,
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Hide"), THEME:GetString("OptionNames", "Show")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().ComboLabel
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().ComboLabel = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().DisplayPercent
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().DisplayPercent = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().TargetTracker
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().TargetTracker = value
            playerConfig:set_dirty()
            playerConfig:save()
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
    "99.9935%", -- AAAAA
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = tChoices,
        LoadSelections = function(self, list, pn)
            local prefsval = playerConfig:get_data().TargetGoal
            local index = IndexOf(tChoices, prefsval .. "%")
            list[index] = true
        end,
        SaveSelections = function(self, list, pn)
            local found = false
            for i = 1, #list do
                if not found then
                    if list[i] == true then
                        local value = i
                        playerConfig:get_data().TargetGoal =
                            tonumber(string.sub(tChoices[value], 1, #tChoices[value] - 1))
                        found = true
                    end
                end
            end
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
		Choices = {THEME:GetString("OptionNames", "SetPercent"), THEME:GetString("OptionNames", "PersonalBest"), THEME:GetString("OptionNames", "PersonalBestReplay")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().TargetTrackerMode
            list[pref + 1] = true
        end,
        SaveSelections = function(self, list, pn)
            local value
			if list[2] then
				value = 1
			elseif list[3] then
				value = 2
			else
				value = 0
			end
            playerConfig:get_data().TargetTrackerMode = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().JudgeCounter
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().JudgeCounter = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().PlayerInfo
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().PlayerInfo = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().CBHighlight
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            playerConfig:get_data().CBHighlight = list[2]
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().CustomizeGameplay
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            playerConfig:get_data().CustomizeGameplay = list[2]
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().CustomEvaluationWindowTimings
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().CustomEvaluationWindowTimings = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On"), THEME:GetString("OptionNames", "EWMA")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().ErrorBar
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
            playerConfig:get_data().ErrorBar = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().leaderboardEnabled
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().leaderboardEnabled = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().FullProgressBar
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().FullProgressBar = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "Off"), THEME:GetString("OptionNames", "On")},
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().MiniProgressBar
            if pref then
                list[2] = true
            else
                list[1] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            local value
            value = list[2]
            playerConfig:get_data().MiniProgressBar = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {
            THEME:GetString("OptionNames", "Off"),
            THEME:GetString("OptionNames", "Sudden"),
            THEME:GetString("OptionNames", "Hidden")
        },
        LoadSelections = function(self, list, pn)
            local pref = playerConfig:get_data().LaneCover
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
            playerConfig:get_data().LaneCover = value
            playerConfig:set_dirty()
            playerConfig:save()
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
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {THEME:GetString("OptionNames", "NPSDisplay"), THEME:GetString("OptionNames", "NPSGraph")},
        LoadSelections = function(self, list, pn)
            local npsDisplay = playerConfig:get_data().NPSDisplay
            local npsGraph = playerConfig:get_data().NPSGraph
            if npsDisplay then
                list[1] = true
            end
            if npsGraph then
                list[2] = true
            end
        end,
        SaveSelections = function(self, list, pn)
            playerConfig:get_data().NPSDisplay = list[1]
            playerConfig:get_data().NPSGraph = list[2]
            playerConfig:set_dirty()
            playerConfig:save()
        end
    }
    setmetatable(t, t)
    return t
end

function StaticBackgrounds()
    local t = {
        Name = "StaticBG",
        LayoutType = "ShowAllInRow",
        SelectType = "SelectOne",
        OneChoiceForAllPlayers = true,
        ExportOnChange = false,
        ExportOnCancel = true,
        Choices = {
            THEME:GetString("OptionNames", "Default"),
            THEME:GetString("OptionNames", "StaticBG"),
        },
        LoadSelections = function(self, list, pn)
            local pref = themeConfig:get_data().global.StaticBackgrounds
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
            elseif list[2] then
                value = true
            end
            playerConfig:get_data().StaticBackgrounds = value
            playerConfig:set_dirty()
            playerConfig:save()
        end
    }
    setmetatable(t, t)
    return t
end
