local json = require('lunajson')

-- Use default skillset names
local json_filter_categories = DeepCopy(ms.SkillSets)
table.insert(json_filter_categories, "Length")
table.insert(json_filter_categories, "ClearPercent")

local RateKey = "Rate"
local ExclusiveFilterKey = "ExclusiveFilter"
local HighestSkillsetOnlyKey = "HighestSkillsetOnly"
local HighestDifficultyOnlyKey = "HighestDifficultyOnly"

local function save_preset(filename, pn)
    filename = filename or "default"
    pn = pn or PLAYER_1

    local filter_preset = {
        Rate = {
            min = math.round(FILTERMAN:GetMinFilterRate(), 1),
            max = math.round(FILTERMAN:GetMaxFilterRate(), 1)
        },
        ExclusiveFilter = FILTERMAN:GetFilterMode(),
        HighestSkillsetOnly = FILTERMAN:GetHighestSkillsetsOnly(),
        HighestDifficultyOnly = FILTERMAN:GetHighestDifficultyOnly()
    }

    for i = 1, #json_filter_categories do
        filter_preset[json_filter_categories[i]] = {
            min = FILTERMAN:GetSSFilter(i, 0),
            max = FILTERMAN:GetSSFilter(i, 1)
        }
    end

    local full_path = (
        PROFILEMAN:GetProfileDir(pn_to_profile_slot(pn))
        .. '/filter_presets/'
        .. filename
        .. '.json'
    )
    File.Write(full_path, json.encode(filter_preset))
    ms.ok(string.format("Saved file to: %s", full_path))
end

return save_preset
