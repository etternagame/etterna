local leaderboardEnabled =
    (NSMAN:IsETTP() and SCREENMAN:GetTopScreen() and SCREENMAN:GetTopScreen():GetName() == "ScreenNetStageInformation") or
    ((playerConfig:get_data().Leaderboard or 0) ~= 0 and DLMAN:IsLoggedIn())
if not leaderboardEnabled then
    return Def.ActorFrame {}
end
local isMulti = NSMAN:IsETTP() and SCREENMAN:GetTopScreen() and SCREENMAN:GetTopScreen():GetName():find("Net") ~= nil or false
local leaderboardIsLocal = not isMulti and (playerConfig:get_data().Leaderboard or 0) == 2

local toggledRateFilter = false
if not DLMAN:GetCurrentRateFilter() then
    DLMAN:ToggleRateFilter()
    toggledRateFilter = true
end

local jdgs = {
    -- Table of judgments for the judgecounters
    "TapNoteScore_W1",
    "TapNoteScore_W2",
    "TapNoteScore_W3",
    "TapNoteScore_W4",
    "TapNoteScore_W5",
    "TapNoteScore_Miss",
}

local textSize = 0.6

local entryActors = {}
local scoreboard = {}
local onlineScores = {}
local multiScores = {}
local curScore = {
    GetDisplayName = function()
        return DLMAN:GetUsername()
    end,
    GetWifeGrade = function(self)
        return GetGradeFromPercent(self.curWifeScore)
    end,
    GetWifeScore = function(self)
        return self.curWifeScore
    end,
    GetSkillsetSSR = function()
        return -1
    end,
    GetJudgmentString = function(self)
        local str = ""
        for i, v in ipairs(jdgs) do
            str = str .. self.jdgVals[v] .. " | "
        end
        return str .. "x" .. self.combo
    end,
    combo = 0,
    curWifeScore = 0,
    curGrade = "Grade_Tier02",
    jdgVals = {},
}
for i,v in ipairs(jdgs) do
    curScore.jdgVals[v] = 0;
end

-- scores in the leaderboard are sorted by this
local CRITERIA = "GetWifeScore"
local NUM_ENTRIES = 32
local VISIBLE_ENTRIES = 5
local ENTRY_HEIGHT = (IsUsingWideScreen() and 35 / 480 * SCREEN_HEIGHT or 20 / 480 * SCREEN_HEIGHT)
local WIDTH = SCREEN_WIDTH * (IsUsingWideScreen() and 0.3 or 0.275)

for i = 1, NUM_ENTRIES do
    entryActors[i] = {}
end

local t = Def.ActorFrame {
    Name = "Leaderboard",
    InitCommand = function(self)
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
            spacingInc = {5,1},
        })
    end,
    EndCommand = function(self)
        if toggledRateFilter then
            -- undo the toggle from above
            DLMAN:ToggleRateFilter()
        end
    end,
    OnCommand = function(self)
        self:playcommand("SetUpMovableValues")
        for i, entry in ipairs(entryActors) do
            for name, label in pairs(entry) do
                if scoreboard[i] ~= nil then
                    label:visible(not (not scoreboard[i]:GetDisplayName()) and i <= VISIBLE_ENTRIES)
                end
            end
        end
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.LeaderboardX, MovableValues.LeaderboardY)
        for i, entry in ipairs(entryActors) do
            entry.container:xy(0, (i-1) * ENTRY_HEIGHT * 1.3)
            entry.container:addy((i - 1) * MovableValues.LeaderboardSpacing)
        end
        self:zoomtowidth(MovableValues.LeaderboardWidth)
        self:zoomtoheight(MovableValues.LeaderboardHeight)
    end,
    Def.Quad {
        Name = "Background",
        InitCommand = function(self)
            self:visible(false)
            self:valign(0):halign(0)
            self:x(WIDTH/5)
        end,
        SetUpMovableValuesMessageCommand = function(self)
            if not allowedCustomization then return end
            self:zoomto(WIDTH, ENTRY_HEIGHT * 1.3 * (VISIBLE_ENTRIES) + ENTRY_HEIGHT + (MovableValues.LeaderboardSpacing * VISIBLE_ENTRIES) )
        end,
    }
}

local function leaderboardSortingFunction(h1, h2)
    return h1[CRITERIA](h1) > h2[CRITERIA](h2)
end

local function scoreUsingMultiScore(idx)
    return {
        GetDisplayName = function()
            return multiScores[idx] and multiScores[idx].user or nil
        end,
        GetWifeGrade = function()
            return multiScores[idx] and GetGradeFromPercent(multiScores[idx].wife) or "Grade_Tier01"
        end,
        GetWifeScore = function()
            return multiScores[idx] and multiScores[idx].wife or -5000000
        end,
        GetSkillsetSSR = function()
            return -1
        end,
        GetJudgmentString = function()
            return multiScores[idx] and multiScores[idx].jdgstr or ""
        end,
    }
end

local function setUpOnlineScores()
    if leaderboardIsLocal then
        onlineScores = {}
        -- get your own scores
        local localrtTable = getRateTable()
        if localrtTable ~= nil then
            -- only display if scores on this rate are found
            local found = false
            local eps = 0.01
            localrates, localrateIndex = getUsedRates(localrtTable)
            for i,r in ipairs(localrates) do
                local raten = tonumber(r:gsub("x", ""), 10)
                if math.abs(raten - getCurRateValue()) < eps then
                    localrateIndex = i
                    found = true
                end
            end
            if found then
                onlineScores = localrtTable[localrates[localrateIndex]]
            end
        end
    elseif isMulti then
        -- get scores from multi
        multiScores = NSMAN:GetMPLeaderboard()
        for i = 1, NUM_ENTRIES do
            onlineScores[i] = scoreUsingMultiScore(i)
        end
    else
        -- get scores from chart leaderboards
        onlineScores = DLMAN:GetChartLeaderBoard(GAMESTATE:GetCurrentSteps():GetChartKey()) or {}
    end

    -- hard limiting
    if #onlineScores > NUM_ENTRIES then
        for i = NUM_ENTRIES, #onlineScores do
            onlineScores[i] = nil
        end
    end

    table.sort(onlineScores, leaderboardSortingFunction)
end

local function findCurscoreInScoreboard()
    for i, s in ipairs(scoreboard) do
        if s == curScore then
            return i
        end
    end
    return 1 -- how
end

local function emplaceCurscore()
    scoreboard = onlineScores
    local done = false
    local ind = #scoreboard

    if not isMulti then
        local inserted = false
        for i = 1, #scoreboard do
            if not inserted and leaderboardSortingFunction(scoreboard[i], curScore) then
                table.insert(scoreboard, i, curScore)
                inserted = true
                ind = i
                break
            end
        end
        if not inserted then
            table.insert(scoreboard, curScore)
            ind = #scoreboard
        end
    end
    table.sort(scoreboard, leaderboardSortingFunction)
    return ind
end

local function scoreEntry(i)
    local entry = Def.ActorFrame {
        Name = "Entry_"..i,
        InitCommand = function(self)
            entryActors[i]["container"] = self
            self.update = function(self, hs)
                self:visible(not (not hs) and i <= VISIBLE_ENTRIES)
            end
            self:update(scoreboard[i])
        end,
    }

    local labelContainer = Def.ActorFrame {
        Name = "Label",
        InitCommand = function(self)
            self:x(WIDTH / 5)
        end,
        Def.Quad {
            Name = "Background",
            InitCommand = function(self)
                self:valign(0):halign(0)
                self:zoomto(WIDTH, ENTRY_HEIGHT)
                self:diffuse(getLeaderboardColor("Background"))
                self:diffusealpha(1)
            end,
        }
    }

    local y = 0
    local function addLabel(name, onUpdate, x, y)
        y = (y or 0) - (IsUsingWideScreen() and 0 or ENTRY_HEIGHT / 3.2)
        labelContainer[#labelContainer+1] = LoadFont("Common Normal") .. {
            Name = name,
            InitCommand = function(self)
                entryActors[i][name] = self
                self:halign(0)
                self:zoom(textSize)
                self:xy(x, 10 + y)
                self:settext("")
                self:diffuse(getLeaderboardColor("Text"))
                self:diffusealpha(1)

                self.update = function(self, hs, rank)
                    if hs then
                        self:visible(true)
                        onUpdate(self, hs, rank)
                    else
                        self:visible(false)
                    end
                end
                self:update(scoreboard[i])
            end,
        }
    end

    addLabel(
        "rank",
        function(self, hs, rank)
            if rank ~= nil then
                if rank >= NUM_ENTRIES then
                    self:settext(tostring(rank) .. "+")
                else
                    self:settext(tostring(rank))
                end
            else
                self:settext(tostring(i))
            end
        end,
        5,
        ENTRY_HEIGHT / 4
    )
    addLabel(
        "ssr",
        function(self, hs)
            local ssr = hs:GetSkillsetSSR("Overall")
            if ssr < 0 then
                self:settext("")
            else
                self:settextf("%.2f", ssr):diffuse(colorByMSD(ssr))
            end
        end,
        WIDTH / 15,
        ENTRY_HEIGHT/4
    )
    addLabel(
        "name",
        function(self, hs)
            local n = hs:GetDisplayName()
            self:settext(n or "")
            if entryActor then
                entryActor:visible(not (not n))
            end
        end,
        WIDTH / 5
    )
    addLabel(
        "wife",
        function(self, hs)
            self:settextf("%05.2f%%", hs:GetWifeScore() * 100):diffuse(colorByGrade(hs:GetWifeGrade()))
        end,
        WIDTH / 1.3
    )
    addLabel(
        "grade",
        function(self, hs)
            self:settext(getGradeStrings(hs:GetWifeGrade()))
            self:diffuse(colorByGrade(hs:GetWifeGrade()))
            self:halign(0.5)
        end,
        WIDTH / 1.2,
        ENTRY_HEIGHT / 2
    )
    addLabel(
        "judges",
        function(self, hs)
            self:settext(hs:GetJudgmentString():gsub("I", "|"))
        end,
        WIDTH / 5,
        ENTRY_HEIGHT / 2
    )
    entry[#entry+1] = labelContainer
    return entry
end
for i = 1, NUM_ENTRIES do
    t[#t + 1] = scoreEntry(i)
end

setUpOnlineScores()
emplaceCurscore()

t.ComboChangedMessageCommand = function(self, params)
    curScore.combo = params.PlayerStageStats and params.PlayerStageStats:GetCurrentCombo() or params.OldCombo
end
t.JudgmentMessageCommand = function(self, params)
    if curScore.jdgVals[params.Judgment] then
        curScore.jdgVals[params.Judgment] = params.Val
    end
    -- params.curWifeScore retrieves the Judgment Message curWifeScore which is a raw number for calculations; very large
    -- the online highscore curWifeScore is the wife percent...
    -- params.WifePercent is our current calculated wife percent.
    local old = curScore.curWifeScore
    curScore.curWifeScore = notShit.floor(params.WifePercent * 10000) / 1000000
    if isMulti then
        multiScores = NSMAN:GetMPLeaderboard()
    end
    if old ~= curScore.curWifeScore then
        table.sort(scoreboard, leaderboardSortingFunction)

        local myscoreIndex = findCurscoreInScoreboard()

        for i, entry in ipairs(entryActors) do
            for name, label in pairs(entry) do
                label:update(scoreboard[i])
            end
        end

        if myscoreIndex > VISIBLE_ENTRIES then
            for name, label in pairs(entryActors[VISIBLE_ENTRIES + 1]) do
                label:update(curScore, myscoreIndex)
                label:visible(true)
            end
        end

    end
end

return t
