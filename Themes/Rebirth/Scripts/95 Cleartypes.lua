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

-- for a reverse mapping of the above 2 tables
local reversetypes = {}
for i, ct in ipairs(typetable) do
    reversetypes[ct] = i
end
local reversestypes = {}
for i, ct in ipairs(stypetable) do
    reversestypes[ct] = i
end

local typecolors = {
    -- colors corresponding to cleartype
    [1] = colorByClearType("MFC"),
    [2] = colorByClearType("WF"),
    [3] = colorByClearType("SDP"),
    [4] = colorByClearType("PFC"),
    [5] = colorByClearType("BF"),
    [6] = colorByClearType("SDG"),
    [7] = colorByClearType("FC"),
    [8] = colorByClearType("MF"),
    [9] = colorByClearType("SDCB"),
    [10] = colorByClearType("Clear"),
    [11] = colorByClearType("Failed"),
    [12] = colorByClearType("Invalid"),
    [13] = colorByClearType("NoPlay"),
    [14] = colorByClearType("None"),
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
function getClearTypeText(index)
    return typetranslations[index]
end

function getShortClearTypeText(index)
    return stypetable[index]
end

function getClearTypeColor(index)
    return typecolors[index]
end

function getClearTypeIndexFromValue(value)
    local c = reversetypes[value]
    if c == nil then
        c = reversestypes[value]
    end
    return c
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
        elseif misscount > 0 then
            -- getting a cb forces one of these cleartypes
            if misscount == 1 then
                clearlevel = 8 -- missflag
            elseif misscount > 1 and misscount < 10 then
                clearlevel = 9 -- SDCB
            else
                clearlevel = 10 -- Clear
            end
        elseif misscount == 0 then
            -- it is at least an FC, but what type of FC?
            if greatcount > 0 then
                if greatcount < 10 and greatcount > 1 then
                    clearlevel = 6 -- SDG
                elseif greatcount == 1 then
                    clearlevel = 5 -- BlackFlag
                else
                    clearlevel = 7 -- FC
                end
            else
                if perfcount < 10 and perfcount > 1 then
                    clearlevel = 3 -- SDP
                elseif perfcount == 1 then
                    clearlevel = 2 -- WhiteFlag
                elseif perfcount == 0 then
                    clearlevel = 1 -- MFC
                else
                    clearlevel = 4 -- PFC
                end
            end
        else
            -- negative misses?
            clearlevel = 12
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
    song = SONGMAN:GetSongByChartKey(score:GetChartKey())
    steps = SONGMAN:GetStepsByChartKey(score:GetChartKey())
    profile = GetPlayerOrMachineProfile(PLAYER_1)
    if score ~= nil and song ~= nil and steps ~= nil then
        playCount = profile:GetSongNumTimesPlayed(song)
        grade = score:GetWifeGrade()
        perfcount = score:GetTapNoteScore("TapNoteScore_W2")
        greatcount = score:GetTapNoteScore("TapNoteScore_W3")
        misscount =
            score:GetTapNoteScore("TapNoteScore_Miss") + score:GetTapNoteScore("TapNoteScore_W5") +
            score:GetTapNoteScore("TapNoteScore_W4")
        local totaltapcount = perfcount + greatcount + misscount + score:GetTapNoteScore("TapNoteScore_W1")
        -- if the chart has no notes, you never played it even if you did
        -- or if the score has no notes, same
        -- this is useless information
        if steps:GetRelevantRadars()[1] == 0 or totaltapcount == 0 then
            getClearTypeItem(13, ret)
        end
    end


    return clearTypes(grade, playCount, perfcount, greatcount, misscount, ret) or getClearTypeItem(12, ret)
end
