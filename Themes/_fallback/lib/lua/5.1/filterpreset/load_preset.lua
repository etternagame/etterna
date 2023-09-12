local json = require('lunajson')

local first_time_call_guard = true

-- Use default skillset names
local json_filter_categories = DeepCopy(ms.SkillSets)
table.insert(json_filter_categories, "Length")
table.insert(json_filter_categories, "ClearPercent")

local RateKey = "Rate"
local ExclusiveFilterKey = "ExclusiveFilter"
local HighestSkillsetOnlyKey = "HighestSkillsetOnly"
local HighestDifficultyOnlyKey = "HighestDifficultyOnly"

local function load_preset(filename, explicit_call, pn)
    explicit_call = explicit_call or false

    if not (explicit_call or first_time_call_guard) then return end
    if first_time_call_guard then first_time_call_guard = false end

    pn = pn or PLAYER_1
    filename = filename or "default.json"

    local full_path = (
        PROFILEMAN:GetProfileDir(pn_to_profile_slot(pn))
        .. '/filter_presets/'
        .. filename
        .. '.json'
    )

    local file_output = File.Read(full_path)
    if not file_output then return end


    local data = json.decode(file_output)

    for i = 1, #json_filter_categories do
        if data[json_filter_categories[i]] then
            local setting = data[json_filter_categories[i]]

            if setting.min then
                FILTERMAN:SetSSFilter(setting.min, i, 0)
            end

            if setting.max then
                FILTERMAN:SetSSFilter(setting.max, i, 1)
            end
        end
    end

    if data[RateKey] then
        local setting = data[RateKey]

        if setting.min then
            FILTERMAN:SetMinFilterRate(setting.min)
        end

        if setting.max then
            FILTERMAN:SetMaxFilterRate(setting.max)
        end
    end

    if data[ExclusiveFilterKey] then
        FILTERMAN:SetFilterMode(data[ExclusiveFilterKey])
    end

    if data[HighestSkillsetOnlyKey] then
        FILTERMAN:SetHighestSkillsetsOnly(data[HighestSkillsetOnlyKey])
    end

    if data[HighestDifficultyOnlyKey] then
        FILTERMAN:HighestDifficultyOnlyKey(data[HighestDifficultyOnlyKey])
    end

    if explicit_call then
        ms.ok(string.format("Loaded filter preset: %s", full_path))
    end
end

return load_preset
