local t = Def.ActorFrame {
    Name = "ScoreBoardFrame",
}

local ratios = {
    VerticalDividerLeftGap = 131 / 1920, -- from beginning of frame to left edge of divider
    VerticalDividerUpperGap = 29 / 1080, -- from top of frame to top of divider
    VerticalDividerLength = 250 / 1080,
    LeftButtonLeftGap = 4 / 1920, -- left edge of frame to left edge of text

    LocalUpperGap = 28 / 1080, -- edge of frame to top of text
    OnlineUpperGap = 77 / 1080, -- edge of frame to top of text
    AllScoresUpperGap = 166 / 1080, -- again
    TopScoresUpperGap = 198 / 1080, -- ...
    CurrentRateUpperGap = 230 / 1080,
    AllRatesUpperGap = 263 / 1080,

    ScoreListUpperGap = 26 / 1080, -- inner edge of divider to inner edge of glow of top item
    ScoreListLeftGap = 158 / 1920, -- left edge of frame to inner edge of glow

    ScoreItemWidth = 778 / 1920, -- inner edge of glow to inner edge (updated to end at divider length)
    ScoreItemHeight = 43 / 1080,
    ScoreItemSpacing = 16 / 1080, -- distance between items
    ScoreClearInfoSpace = 109 / 1920, -- about 14% of ScoreItemWidth
    ScoreMetaInfoSpace = 560 / 1920, -- about 72%
    ScorePlayerRateSpace = 109 / 1920, -- about 14%

    CursorVerticalSpan = 12 / 1080, -- visible cursor glow measured, doubled
    CursorHorizontalSpan = 12 / 1920, -- same

    LeftButtonWidth = 128 / 1920, -- guesstimation of max text width
    -- didnt measure height because it can be weird
}

local actuals = {
    VerticalDividerLeftGap = ratios.VerticalDividerLeftGap * SCREEN_WIDTH,
    VerticalDividerUpperGap = ratios.VerticalDividerUpperGap * SCREEN_HEIGHT,
    VerticalDividerLength = ratios.VerticalDividerLength * SCREEN_HEIGHT,
    LeftButtonLeftGap = ratios.LeftButtonLeftGap * SCREEN_WIDTH,
    LocalUpperGap = ratios.LocalUpperGap * SCREEN_HEIGHT,
    OnlineUpperGap = ratios.OnlineUpperGap * SCREEN_HEIGHT,
    AllScoresUpperGap = ratios.AllScoresUpperGap * SCREEN_HEIGHT,
    TopScoresUpperGap = ratios.TopScoresUpperGap * SCREEN_HEIGHT,
    CurrentRateUpperGap = ratios.CurrentRateUpperGap * SCREEN_HEIGHT,
    AllRatesUpperGap = ratios.AllRatesUpperGap * SCREEN_HEIGHT,
    ScoreListUpperGap = ratios.ScoreListUpperGap * SCREEN_HEIGHT,
    ScoreListLeftGap = ratios.ScoreListLeftGap * SCREEN_WIDTH,
    ScoreItemWidth = ratios.ScoreItemWidth * SCREEN_WIDTH,
    ScoreItemHeight = ratios.ScoreItemHeight * SCREEN_HEIGHT,
    ScoreItemSpacing = ratios.ScoreItemSpacing * SCREEN_HEIGHT,
    ScoreClearInfoSpace = ratios.ScoreClearInfoSpace * SCREEN_WIDTH,
    ScoreMetaInfoSpace = ratios.ScoreMetaInfoSpace * SCREEN_WIDTH,
    ScorePlayerRateSpace = ratios.ScorePlayerRateSpace * SCREEN_WIDTH,
    CursorVerticalSpan = ratios.CursorVerticalSpan * SCREEN_HEIGHT,
    CursorHorizontalSpan = ratios.CursorHorizontalSpan * SCREEN_WIDTH,
    LeftButtonWidth = ratios.LeftButtonWidth * SCREEN_WIDTH,
}

local translations = {
    NoReplayData = THEME:GetString("ScreenEvaluation", "NoReplayData"),
    You = THEME:GetString("ScreenEvaluation", "You"),
    LastScore = THEME:GetString("ScreenEvaluation", "LastScore"),
    NoLocalScoresRecorded = THEME:GetString("ScreenEvaluation", "NoLocalScoresRecorded"),
    ChartUnranked = THEME:GetString("ScreenEvaluation", "ChartUnranked"),
    FetchingScores = THEME:GetString("ScreenEvaluation", "FetchingScores"),
    NoOnlineScoresRecorded = THEME:GetString("ScreenEvaluation", "NoOnlineScoresRecorded"),
    ShowingXXofXScoresFormatStr = THEME:GetString("ScreenEvaluation", "ShowingXXofXScoresFormatStr"),
    Local = THEME:GetString("ScreenEvaluation", "Local"),
    Online = THEME:GetString("ScreenEvaluation", "Online"),
    AllScores = THEME:GetString("ScreenEvaluation", "AllScores"),
    TopScores = THEME:GetString("ScreenEvaluation", "TopScores"),
    CurrentRate = THEME:GetString("ScreenEvaluation", "CurrentRate"),
    AllRates = THEME:GetString("ScreenEvaluation", "AllRates"),
}

-- we are expecting this file to be loaded with these params precalculated
-- in reference, Height is measured divider edge to divider edge
-- in reference, Width is the length of the horizontal divider
actuals.FrameWidth = Var("Width")
actuals.FrameHeight = Var("Height")
actuals.DividerThickness = Var("DividerThickness")
local itemCount = Var("ItemCount")
if itemCount == nil or itemCount < 0 then itemCount = 1 end

-- we dont actually use this but we pass it back just to make things work smoothly
local judgeSetting = (PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or GetTimingDifficulty())

local topButtonSize = 1
local bottomButtonSize = 0.7
local gradeSize = 0.8
local clearTypeSize = 0.65
local wifeJudgmentsSize = 0.6
local dateSSRSize = 0.6
local playerNameSize = 0.6
local rateSize = 0.6
local pageTextSize = 0.8
local loadingTextSize = 0.8
local textZoomFudge = 5

-- increase the highlight area height of the buttons
-- only for the left buttons, not the scoreitems
local buttonSizingFudge = 8
local buttonHoverAlpha = 0.8
local buttonRegularAlpha = 1
local itemBGHoverAlpha = 0.2
local itemBGHoverAnimationSeconds = 0.05

-- this can be nil if no scores exist in the profile
local mostRecentScore = SCOREMAN:GetMostRecentScore()

-- when moving the cursor from one place to another
local cursorAnimationSeconds = 0.05
-- when refreshing the score list
local scoreListAnimationSeconds = 0.05

t[#t+1] = Def.Quad {
    Name = "VerticalDivider",
    InitCommand = function(self)
        self:valign(0):halign(0)
        self:zoomto(actuals.DividerThickness, actuals.VerticalDividerLength)
        self:xy(actuals.VerticalDividerLeftGap, actuals.VerticalDividerUpperGap)
        registerActorToColorConfigElement(self, "main", "SeparationDivider")
    end
}

-- online and offline (default is offline)
local isLocal = true
-- current rate or not (default is current rate)
-- this is NOT the variable that controls the online scores
-- if isLocal is false, defer to allRates = not DLMAN:GetCurrentRateFilter()
-- but do not replace the variable
local allRates = false
-- all scores or top scores (online only)
-- dont have to modify this var with direct values, call DLMAN to update it
local allScores = not DLMAN:GetTopScoresOnlyFilter()

-- indicate whether or not we are currently fetching the leaderboard
local fetchingScores = false

-- this will distribute a given highscore to the offsetplot and the other eval elements
-- it will only work properly with a replay, so restrict it to replay-only scores
local function distributeScore(callerActor, highscore)
    if highscore == nil or not highscore:HasReplayData() then return end
    -- this highscore is an online score so must request ReplayData
    if highscore:GetScoreKey():find("^Online_") ~= nil then
        -- this gets replay data and calls the given function upon completion
        DLMAN:RequestOnlineScoreReplayData(highscore,
        function()
            callerActor:GetParent():GetParent():playcommand("UpdateScore", {score = highscore, judgeSetting = judgeSetting})
        end)
    else
        -- otherwise we can immediately do the thing
        callerActor:GetParent():GetParent():playcommand("UpdateScore", {score = highscore, judgeSetting = judgeSetting})
    end
end

local function scoreList()
    local page = 1
    local maxPage = 1
    local scorelistframe = nil

    local function movePage(n)
        if maxPage <= 1 then
            return
        end
        -- the tooltip gets stuck on if it is visible and page changes
        TOOLTIP:Hide()

        -- math to make pages loop both directions
        local nn = (page + n) % (maxPage + 1)
        if nn == 0 then
            nn = n > 0 and 1 or maxPage
        end
        page = nn

        scorelistframe:playcommand("MovedPage")
    end

    -- yes, we do have a country filter
    -- we don't tell anyone
    -- the Global country just shows everyone
    local dlmanScoreboardCountryFilter = "Global"

    local mostRecentScore = SCOREMAN:GetMostRecentScore()
    local selectedScorekey = mostRecentScore:GetScoreKey()
    local scores = {}

    -- to prevent bombing the server repeatedly with leaderboard requests
    local alreadyRequestedLeaderboard = false

    local t = Def.ActorFrame {
        Name = "ScoreListFrame",
        BeginCommand = function(self)
            scorelistframe = self
        end,
        InitCommand = function(self)
            self:xy(actuals.ScoreListLeftGap, actuals.ScoreListUpperGap)
        end,
        OnCommand = function(self)
            self:playcommand("UpdateScores")
            -- set the current page so we immediately see where our score is
            if scores ~= nil and #scores > 0 then
                local ind = 1
                for i, s in ipairs(scores) do
                    if s:GetScoreKey() == mostRecentScore:GetScoreKey() then
                        ind = i
                        break
                    end
                end
                page = 1 + math.floor((ind-1) / itemCount)
            end

            self:playcommand("UpdateList")
        end,
        SetCommand = function(self, params)
            -- only used to update the judge setting for the purpose of updating to the correct judge
            -- when picking new scores in the list
            if params.judgeSetting ~= nil then
                judgeSetting = params.judgeSetting
            end
        end,
        UpdateScoresCommand = function(self)
            page = 1
            if isLocal then
                local scoresByRate = getRateTable(getScoresByKey(PLAYER_1))

                if allRates then
                    -- place every single score for the file into a table
                    scores = {}
                    for _, scoresAtRate in pairs(scoresByRate) do
                        for __, s in pairs(scoresAtRate) do
                            scores[#scores + 1] = s
                        end
                    end
                    -- sort it by Overall SSR
                    table.sort(scores,
                    function(a,b)
                        return a:GetSkillsetSSR("Overall") > b:GetSkillsetSSR("Overall")
                    end)
                else
                    -- place only the scores for this rate into a table
                    -- it is already sorted by percent
                    -- the first half of this returns nil sometimes
                    -- particularly for scores that dont actually exist in your profile, like online replays
                    -- in those cases, we put the mostRecentScore into its own fallback table
                    -- mostRecentScore will never be nil. if it is, the game actually crashes before it gets here.
                    if scoresByRate == nil then
                        scores = {}
                    else
                        scores = scoresByRate[getRate(mostRecentScore)] or {mostRecentScore}
                    end
                end
            else
                local steps = GAMESTATE:GetCurrentSteps()
                -- operate with dlman scores
                -- ... everything here is determined by internal bools set by the toggle buttons
                scores = DLMAN:GetChartLeaderBoard(steps:GetChartKey(), dlmanScoreboardCountryFilter)

                -- if scores comes back nil, then the chart is unranked
                if scores == nil then
                    if steps then
                        fetchingScores = false
                    end
                    return
                end

                -- this is the initial request for the leaderboard which should only end up running once
                -- and when it finishes, it loops back through this command
                -- on the second passthrough, the leaderboard is hopefully filled out
                if #scores == 0 then
                    if steps then
                        if not alreadyRequestedLeaderboard then
                            alreadyRequestedLeaderboard = true
                            fetchingScores = true
                            DLMAN:RequestChartLeaderBoardFromOnline(
                                steps:GetChartKey(),
                                function(leaderboard)
                                    fetchingScores = false
                                    self:queuecommand("UpdateScores")
                                    self:queuecommand("UpdateList")
                                end
                            )
                        end
                    end
                end
            end
        end,
        UpdateListCommand = function(self)
            if scores == nil then
                maxPage = 1
            else
                maxPage = math.ceil(#scores / itemCount)
            end

            self:GetChild("Cursor"):diffusealpha(0)

            for i = 1, itemCount do
                local index = (page - 1) * itemCount + i
                self:GetChild("ScoreItem_"..i):playcommand("SetScore", {scoreIndex = index})
            end
            MESSAGEMAN:Broadcast("UpdateButtons")
        end,
        MovedPageCommand = function(self)
            self:playcommand("UpdateList")
        end,
        ToggleCurrentRateMessageCommand = function(self)
            if isLocal then
                allRates = not allRates
            else
                DLMAN:ToggleRateFilter()
            end
            self:playcommand("UpdateScores")
            self:playcommand("UpdateList")
        end,
        ToggleAllScoresMessageCommand = function(self)
            DLMAN:ToggleTopScoresOnlyFilter()
            self:playcommand("UpdateScores")
            self:playcommand("UpdateList")
        end,
        ToggleLocalMessageCommand = function(self)
            self:playcommand("UpdateScores")
            self:playcommand("UpdateList")
        end,
        UpdateLoginStatusCommand = function(self)
            -- handling any online status update
            -- reset scores and list if in an impossible state
            if not DLMAN:IsLoggedIn() and not isLocal then
                isLocal = true
                self:playcommand("UpdateScores")
                self:playcommand("UpdateList")
            else
                -- for this state, we just make sure the buttons are correct
                -- we should be in local screen and offline/online
                MESSAGEMAN:Broadcast("UpdateButtons")
            end
        end,
    }
    local function scoreItem(i)
        local score = nil
        local scoreIndex = i

        return Def.ActorFrame {
            Name = "ScoreItem_"..i,
            InitCommand = function(self)
                self:y((i-1) * actuals.ScoreItemSpacing + (i-1) * actuals.ScoreItemHeight)
            end,
            ColorConfigUpdatedMessageCommand = function(self)
                -- handles most colorings
                self:playcommand("SetScore", {scoreIndex = scoreIndex})
            end,
            SetScoreCommand = function(self, params)
                scoreIndex = params.scoreIndex
                -- nil scores table: unranked chart
                if scores == nil then
                    score = nil
                else
                    score = scores[scoreIndex]
                end
                self:finishtweening()
                self:diffusealpha(0)
                if score ~= nil then
                    self:smooth(scoreListAnimationSeconds * i)
                    self:diffusealpha(1)
                    if score:GetScoreKey() == selectedScorekey then
                        self:GetParent():GetChild("Cursor"):playcommand("SetIndex", {index = i})
                    end
                end
            end,

            UIElements.QuadButton(1, 1) .. {
                Name = "Button",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:zoomto(actuals.ScoreItemWidth, actuals.ScoreItemHeight)
                    self:diffusealpha(0)
                    registerActorToColorConfigElement(self, "main", "SecondaryBackground")
                end,
                MouseDownCommand = function(self)
                    if score ~= nil and score:HasReplayData() then
                        selectedScorekey = score:GetScoreKey()
                        self:GetParent():GetParent():GetChild("Cursor"):playcommand("SetIndex", {index = i})
                        -- this actor great grandparent is the ScoreBoardFrame (the file level t)
                        distributeScore(self:GetParent():GetParent():GetParent(), score)
                    end
                end,
                MouseOverCommand = function(self) self:playcommand("RolloverUpdate",{update = "over"}) end,
                MouseOutCommand = function(self) self:playcommand("RolloverUpdate",{update = "out"}) end,
                RolloverUpdateCommand = function(self, params)
                    -- hovering
                    if self:GetParent():GetDiffuseAlpha() ~= 0 then
                        if params.update == "over" then
                            self:finishtweening()
                            self:smooth(itemBGHoverAnimationSeconds)
                            self:diffusealpha(itemBGHoverAlpha)
                        elseif params.update == "out" then
                            self:smooth(itemBGHoverAnimationSeconds)
                            self:diffusealpha(0)
                        end
                    end

                    -- replay data tooltip
                    if score ~= nil and not score:HasReplayData() then
                        if params.update == "over" then
                            TOOLTIP:Show()
                            TOOLTIP:SetText(translations["NoReplayData"])
                        elseif params.update == "out" then
                            TOOLTIP:Hide()
                        end
                    end
                end,
                SetScoreCommand = function(self)
                    -- hovering
                    if self:GetParent():GetDiffuseAlpha() ~= 0 then
                        if isOver(self) then
                            self:finishtweening()
                            self:smooth(itemBGHoverAnimationSeconds)
                            self:diffusealpha(itemBGHoverAlpha)
                        else
                            self:diffusealpha(0)
                        end
                    end

                    -- replay data tooltip
                    if score ~= nil and isOver(self) and not score:HasReplayData() then
                        TOOLTIP:Show()
                        TOOLTIP:SetText(translations["NoReplayData"])
                    elseif score ~= nil and isOver(self) and score:HasReplayData() then
                        TOOLTIP:Hide()
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Grade",
                InitCommand = function(self)
                    self:xy(actuals.ScoreClearInfoSpace / 2, actuals.ScoreItemHeight / 4)
                    self:zoom(gradeSize)
                    self:maxwidth(actuals.ScoreClearInfoSpace / gradeSize - textZoomFudge)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local gra = THEME:GetString("Grade", ToEnumShortString(score:GetWifeGrade()))
                        self:settext(gra)
                        self:diffuse(colorByGrade(score:GetWifeGrade()))
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "ClearType",
                InitCommand = function(self)
                    self:xy(actuals.ScoreClearInfoSpace / 2, actuals.ScoreItemHeight / 4 * 3)
                    self:zoom(clearTypeSize)
                    self:maxwidth(actuals.ScoreClearInfoSpace / clearTypeSize - textZoomFudge)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local txt = getClearTypeFromScore(score, 0)
                        local color = getClearTypeFromScore(score, 2)
                        self:settext(txt)
                        self:diffuse(color)
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "PercentAndJudgments",
                InitCommand = function(self)
                    self:halign(0)
                    self:xy(actuals.ScoreClearInfoSpace, actuals.ScoreItemHeight / 4)
                    self:zoom(wifeJudgmentsSize)
                    self:maxwidth(actuals.ScoreMetaInfoSpace / wifeJudgmentsSize - textZoomFudge)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        -- when you want individual colors for every single judgment in a continuous string.......
                        local wifeStr = string.format("%05.2f%%", notShit.floor(score:GetWifeScore() * 10000) / 100)
                        local jgMaStr = tostring(score:GetTapNoteScore("TapNoteScore_W1"))
                        local jgPStr = tostring(score:GetTapNoteScore("TapNoteScore_W2"))
                        local jgGrStr = tostring(score:GetTapNoteScore("TapNoteScore_W3"))
                        local jgGoStr = tostring(score:GetTapNoteScore("TapNoteScore_W4"))
                        local jgBStr = tostring(score:GetTapNoteScore("TapNoteScore_W5"))
                        local jgMiStr = tostring(score:GetTapNoteScore("TapNoteScore_Miss"))
                        local diff_use = COLORS:getMainColor("SecondaryText")
                        if score:GetChordCohesion() then
                            diff_use = COLORS:getColor("evaluation", "ChordCohesionOnScore")
                        end
                        self:ClearAttributes()
                        self:diffuse(diff_use)
                        self:diffusealpha(1)
                        self:settextf("%s | %s - %s - %s - %s - %s - %s", wifeStr, jgMaStr, jgPStr, jgGrStr, jgGoStr, jgBStr, jgMiStr)
                        -- could have probably used a loop to do this
                        self:AddAttribute(#string.format("%s | ", wifeStr), {Length = #jgMaStr, Zoom = wifeJudgmentsSize, Diffuse = colorByJudgment("TapNoteScore_W1")})
                        self:AddAttribute(#string.format("%s | %s - ", wifeStr, jgMaStr), {Length = #jgPStr, Zoom = wifeJudgmentsSize, Diffuse = colorByJudgment("TapNoteScore_W2")})
                        self:AddAttribute(#string.format("%s | %s - %s - ", wifeStr, jgMaStr, jgPStr), {Length = #jgGrStr, Zoom = wifeJudgmentsSize, Diffuse = colorByJudgment("TapNoteScore_W3")})
                        self:AddAttribute(#string.format("%s | %s - %s - %s - ", wifeStr, jgMaStr, jgPStr, jgGrStr), {Length = #jgGoStr, Zoom = wifeJudgmentsSize, Diffuse = colorByJudgment("TapNoteScore_W4")})
                        self:AddAttribute(#string.format("%s | %s - %s - %s - %s - ", wifeStr, jgMaStr, jgPStr, jgGrStr, jgGoStr), {Length = #jgBStr, Zoom = wifeJudgmentsSize, Diffuse = colorByJudgment("TapNoteScore_W5")})
                        self:AddAttribute(#string.format("%s | %s - %s - %s - %s - %s - ", wifeStr, jgMaStr, jgPStr, jgGrStr, jgGoStr, jgBStr), {Length = #jgMiStr, Zoom = wifeJudgmentsSize, Diffuse = colorByJudgment("TapNoteScore_Miss")})
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "DateAndSSR",
                InitCommand = function(self)
                    self:halign(0)
                    self:xy(actuals.ScoreClearInfoSpace, actuals.ScoreItemHeight / 4 * 3)
                    self:zoom(dateSSRSize)
                    self:maxwidth(actuals.ScoreMetaInfoSpace / dateSSRSize - textZoomFudge)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local date = score:GetDate()
                        local m, d, y = expandDateString(date)
                        local dstr = string.format("%s %s, %s", m, d, y)
                        local ssr = score:GetSkillsetSSR("Overall")
                        local ssrStr = string.format("%05.2f", ssr)
                        local diff_use = COLORS:getMainColor("SecondaryText")
                        if score:GetChordCohesion() then
                            diff_use = COLORS:getColor("evaluation", "ChordCohesionOnScore")
                        end
                        self:ClearAttributes()
                        self:diffuse(diff_use)
                        self:diffusealpha(1)
                        self:settextf("%s | %s", ssrStr, dstr)
                        self:AddAttribute(0, {Length = #ssrStr, Zoom = dateSSRSize, Diffuse = colorByMSD(ssr)})
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "PlayerName",
                InitCommand = function(self)
                    self:xy(actuals.ScoreItemWidth - actuals.ScorePlayerRateSpace / 2, actuals.ScoreItemHeight / 4)
                    self:zoom(playerNameSize)
                    self:maxwidth(actuals.ScorePlayerRateSpace / playerNameSize - textZoomFudge)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local n = score:GetName()
                        if n == "" then
                            if isLocal then
                                n = translations["You"]
                            end
                        elseif n == "#P1#" then
                            if score:GetScoreKey() == mostRecentScore:GetScoreKey() then
                                n = translations["LastScore"]
                            else
                                n = translations["You"]
                            end
                        end
                        self:settext(n)
                        if score:GetChordCohesion() then
                            self:diffuse(COLORS:getColor("evaluation", "ChordCohesionOnScore"))
                        else
                            self:diffuse(COLORS:getColor("main", "SecondaryText"))
                        end
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Rate",
                InitCommand = function(self)
                    self:xy(actuals.ScoreItemWidth - actuals.ScorePlayerRateSpace / 2, actuals.ScoreItemHeight / 4 * 3)
                    self:zoom(rateSize)
                    self:maxwidth(actuals.ScorePlayerRateSpace / rateSize - textZoomFudge)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local rt = score:GetMusicRate()
                        self:settext(getRateString(rt))
                        if score:GetChordCohesion() then
                            self:diffuse(COLORS:getColor("evaluation", "ChordCohesionOnScore"))
                        else
                            self:diffuse(COLORS:getColor("main", "SecondaryText"))
                        end
                    end
                end
            }
        }
    end
    t[#t+1] = Def.Sprite {
        Name = "Cursor",
        Texture = THEME:GetPathG("", "scoreboardGlow"),
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:x(-actuals.CursorHorizontalSpan / 2)
            self:zoomto(actuals.ScoreItemWidth + actuals.CursorHorizontalSpan, actuals.ScoreItemHeight + actuals.CursorVerticalSpan)
            self:diffusealpha(0)
        end,
        SetIndexCommand = function(self, params)
            local i = params.index
            if scores[i] == nil then
                self:diffusealpha(0)
            else
                self:finishtweening()
                if self:GetDiffuseAlpha() ~= 0 then
                    self:linear(cursorAnimationSeconds)
                else
                    self:diffusealpha(1)
                end
                self:y((i-1) * actuals.ScoreItemSpacing + (i-1) * actuals.ScoreItemHeight - actuals.CursorVerticalSpan / 2)
            end
        end,
    }

    for i = 1, itemCount do
        t[#t+1] = scoreItem(i)
    end

    t[#t+1] = Def.Quad {
        Name = "MouseWheelRegion",
        InitCommand = function(self)
            self:halign(0):valign(0)
            -- have to offset by the parent position...
            self:xy(-actuals.ScoreListLeftGap, -actuals.ScoreListUpperGap)
            self:diffusealpha(0)
            self:zoomto(actuals.FrameWidth, actuals.FrameHeight)
        end,
        MouseScrollMessageCommand = function(self, params)
            if isOver(self) then
                if params.direction == "Up" then
                    movePage(-1)
                else
                    movePage(1)
                end
            end
        end
    }

    t[#t+1] = LoadFont("Common Normal") .. {
        Name = "LoadingText",
        InitCommand = function(self)
            self:valign(0)
            self:x(actuals.ScoreItemWidth / 2)
            self:maxwidth(actuals.ScoreItemWidth / loadingTextSize - textZoomFudge)
            self:diffusealpha(0)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        UpdateListCommand = function(self)
            self:finishtweening()
            self:smooth(scoreListAnimationSeconds)
            local steps = GAMESTATE:GetCurrentSteps()

            if isLocal then
                if scores ~= nil and #scores == 0 then
                    self:diffusealpha(1)
                    self:settext(translations["NoLocalScoresRecorded"])
                else
                    self:diffusealpha(0)
                    self:settext("")
                end
                return
            end

            if scores == nil then
                self:diffusealpha(1)
                self:settext(translations["ChartUnranked"])
            elseif #scores == 0 and steps and fetchingScores == true then
                self:diffusealpha(1)
                self:settext(translations["FetchingScores"])
            elseif #scores == 0 and steps and fetchingScores == false then
                self:diffusealpha(1)
                self:settext(translations["NoOnlineScoresRecorded"])
            else
                self:diffusealpha(0)
                self:settext("")
            end
        end
    }

    t[#t+1] = LoadFont("Common Normal") .. {
        Name = "PageText",
        InitCommand = function(self)
            self:valign(1)
            self:xy(actuals.ScoreItemWidth / 2, actuals.VerticalDividerLength)
            self:zoom(pageTextSize)
            self:maxwidth(actuals.ScoreItemWidth / pageTextSize - textZoomFudge)
            self:settext("")
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        UpdateListCommand = function(self)
             -- nil scores = no scores
             if scores == nil then
                self:settext("0-0/0")
                return
            end

            local lb = clamp((page-1) * (itemCount) + 1, 0, #scores)
            local ub = clamp(page * itemCount, 0, #scores)
            self:settextf(translations["ShowingXXofXScoresFormatStr"], lb, ub, #scores)
        end
    }

    return t
end



t[#t+1] = Def.ActorFrame {
    Name = "LeftButtons",
    InitCommand = function(self)
        self:x(actuals.LeftButtonLeftGap)
    end,
    UpdateButtonsMessageCommand = function(self)
        allScores = not DLMAN:GetTopScoresOnlyFilter()
        self:playcommand("UpdateToggleStatus")
    end,
    ColorConfigUpdatedMessageCommand = function(self)
        self:playcommand("UpdateToggleStatus")
    end,

    UIElements.TextButton(1, 1, "Common Normal") .. {
        Name = "LocalButton",
        InitCommand = function(self)
            self:y(actuals.LocalUpperGap)
            local txt = self:GetChild("Text")
            txt:valign(0):halign(0)
            txt:zoom(topButtonSize)
            txt:maxwidth((actuals.VerticalDividerLeftGap - actuals.LeftButtonLeftGap) / topButtonSize - textZoomFudge)
            txt:settext(translations["Local"])
            registerActorToColorConfigElement(txt, "main", "SecondaryText")
            local bg = self:GetChild("BG")
            bg:valign(0):halign(0)
            bg:zoomto(actuals.LeftButtonWidth, txt:GetZoomedHeight() + buttonSizingFudge)
            bg:y(-buttonSizingFudge / 2)
        end,
        UpdateToggleStatusCommand = function(self)
            -- lit when isLocal is true
            if not isLocal then
                self:GetChild("Text"):strokecolor(color("0,0,0,0"))
            else
                self:GetChild("Text"):strokecolor(Brightness(COLORS:getMainColor("SecondaryText"), 0.75))
            end
        end,
        ClickCommand = function(self, params)
            if params.update == "OnMouseDown" then
                isLocal = true
                MESSAGEMAN:Broadcast("ToggleLocal")
            end
        end,
        RolloverUpdateCommand = function(self, params)
            if params.update == "in" then
                self:diffusealpha(buttonHoverAlpha)
            else
                self:diffusealpha(1)
            end
        end
    },
    UIElements.TextButton(1, 1, "Common Normal") .. {
        Name = "OnlineButton",
        InitCommand = function(self)
            self:y(actuals.OnlineUpperGap)
            local txt = self:GetChild("Text")
            txt:valign(0):halign(0)
            txt:zoom(topButtonSize)
            txt:maxwidth((actuals.VerticalDividerLeftGap - actuals.LeftButtonLeftGap) / topButtonSize - textZoomFudge)
            txt:settext(translations["Online"])
            registerActorToColorConfigElement(txt, "main", "SecondaryText")
            local bg = self:GetChild("BG")
            bg:valign(0):halign(0)
            bg:zoomto(actuals.LeftButtonWidth, txt:GetZoomedHeight() + buttonSizingFudge)
            bg:y(-buttonSizingFudge / 2)
            self:playcommand("UpdateToggleStatus")
        end,
        UpdateToggleStatusCommand = function(self)
            if not DLMAN:IsLoggedIn() then
                self:diffusealpha(0)
            else
                if isOver(self) then
                    self:diffusealpha(buttonHoverAlpha)
                else
                    self:diffusealpha(1)
                end
            end

            -- lit when isLocal is false
            if isLocal then
                self:GetChild("Text"):strokecolor(color("0,0,0,0"))
            else
                self:GetChild("Text"):strokecolor(Brightness(COLORS:getMainColor("SecondaryText"), 0.75))
            end
        end,
        ClickCommand = function(self, params)
            if self:IsInvisible() then return end
            if params.update == "OnMouseDown" then
                isLocal = false
                MESSAGEMAN:Broadcast("ToggleLocal")
            end
        end,
        RolloverUpdateCommand = function(self, params)
            if self:IsInvisible() then return end
            if params.update == "in" then
                self:diffusealpha(buttonHoverAlpha)
            else
                self:diffusealpha(1)
            end
        end
    },

    UIElements.TextButton(1, 1, "Common Normal") .. {
        Name = "AllScoresButton",
        InitCommand = function(self)
            self:y(actuals.AllScoresUpperGap)
            local txt = self:GetChild("Text")
            txt:valign(0):halign(0)
            txt:zoom(bottomButtonSize)
            txt:maxwidth((actuals.VerticalDividerLeftGap - actuals.LeftButtonLeftGap) / bottomButtonSize - textZoomFudge)
            txt:settext(translations["AllScores"])
            registerActorToColorConfigElement(txt, "main", "SecondaryText")
            local bg = self:GetChild("BG")
            bg:valign(0):halign(0)
            bg:zoomto(actuals.LeftButtonWidth, txt:GetZoomedHeight() + buttonSizingFudge)
            bg:y(-buttonSizingFudge / 2)
        end,
        UpdateToggleStatusCommand = function(self)
            -- invisible if local
            if isLocal then
                self:diffusealpha(0)
            else
                if isOver(self) then
                    self:diffusealpha(buttonHoverAlpha)
                else
                    self:diffusealpha(1)
                end
            end
            -- lit if allScores is true
            if not allScores then
                self:GetChild("Text"):strokecolor(color("0,0,0,0"))
            else
                self:GetChild("Text"):strokecolor(Brightness(COLORS:getMainColor("SecondaryText"), 0.75))
            end
        end,
        ClickCommand = function(self, params)
            if self:IsInvisible() then return end
            if params.update == "OnMouseDown" then
                MESSAGEMAN:Broadcast("ToggleAllScores")
            end
        end,
        RolloverUpdateCommand = function(self, params)
            if self:IsInvisible() then return end
            if params.update == "in" then
                self:diffusealpha(buttonHoverAlpha)
            else
                self:diffusealpha(1)
            end
        end
    },
    UIElements.TextButton(1, 1, "Common Normal") .. {
        Name = "TopScoresButton",
        InitCommand = function(self)
            self:y(actuals.TopScoresUpperGap)
            local txt = self:GetChild("Text")
            txt:valign(0):halign(0)
            txt:zoom(bottomButtonSize)
            txt:maxwidth((actuals.VerticalDividerLeftGap - actuals.LeftButtonLeftGap) / bottomButtonSize - textZoomFudge)
            txt:settext(translations["TopScores"])
            registerActorToColorConfigElement(txt, "main", "SecondaryText")
            local bg = self:GetChild("BG")
            bg:valign(0):halign(0)
            bg:zoomto(actuals.LeftButtonWidth, txt:GetZoomedHeight() + buttonSizingFudge)
            bg:y(-buttonSizingFudge / 2)
        end,
        UpdateToggleStatusCommand = function(self)
            -- invisible if local
            if isLocal then
                self:diffusealpha(0)
            else
                if isOver(self) then
                    self:diffusealpha(buttonHoverAlpha)
                else
                    self:diffusealpha(1)
                end
            end
            -- lit if allScores is false
            if allScores then
                self:GetChild("Text"):strokecolor(color("0,0,0,0"))
            else
                self:GetChild("Text"):strokecolor(Brightness(COLORS:getMainColor("SecondaryText"), 0.75))
            end
        end,
        ClickCommand = function(self, params)
            if self:IsInvisible() then return end
            if params.update == "OnMouseDown" then
                MESSAGEMAN:Broadcast("ToggleAllScores")
            end
        end,
        RolloverUpdateCommand = function(self, params)
            if self:IsInvisible() then return end
            if params.update == "in" then
                self:diffusealpha(buttonHoverAlpha)
            else
                self:diffusealpha(1)
            end
        end
    },
    UIElements.TextButton(1, 1, "Common Normal") .. {
        Name = "CurrentRateButton",
        InitCommand = function(self)
            self:y(actuals.CurrentRateUpperGap)
            local txt = self:GetChild("Text")
            txt:valign(0):halign(0)
            txt:zoom(bottomButtonSize)
            txt:maxwidth((actuals.VerticalDividerLeftGap - actuals.LeftButtonLeftGap) / bottomButtonSize - textZoomFudge)
            txt:settext(translations["CurrentRate"])
            registerActorToColorConfigElement(txt, "main", "SecondaryText")
            local bg = self:GetChild("BG")
            bg:valign(0):halign(0)
            bg:zoomto(actuals.LeftButtonWidth, txt:GetZoomedHeight() + buttonSizingFudge)
            bg:y(-buttonSizingFudge / 2)
        end,
        UpdateToggleStatusCommand = function(self)
            -- lit if allRates is false
            if (allRates and isLocal) or (not DLMAN:GetCurrentRateFilter() and not isLocal) then
                self:GetChild("Text"):strokecolor(color("0,0,0,0"))
            else
                self:GetChild("Text"):strokecolor(Brightness(COLORS:getMainColor("SecondaryText"), 0.75))
            end
        end,
        ClickCommand = function(self, params)
            if params.update == "OnMouseDown" then
                MESSAGEMAN:Broadcast("ToggleCurrentRate")
            end
        end,
        RolloverUpdateCommand = function(self, params)
            if params.update == "in" then
                self:diffusealpha(buttonHoverAlpha)
            else
                self:diffusealpha(1)
            end
        end
    },
    UIElements.TextButton(1, 1, "Common Normal") .. {
        Name = "AllRatesButton",
        InitCommand = function(self)
            self:y(actuals.AllRatesUpperGap)
            local txt = self:GetChild("Text")
            txt:valign(0):halign(0)
            txt:zoom(bottomButtonSize)
            txt:maxwidth((actuals.VerticalDividerLeftGap - actuals.LeftButtonLeftGap) / bottomButtonSize - textZoomFudge)
            txt:settext(translations["AllRates"])
            registerActorToColorConfigElement(txt, "main", "SecondaryText")
            local bg = self:GetChild("BG")
            bg:valign(0):halign(0)
            bg:zoomto(actuals.LeftButtonWidth, txt:GetZoomedHeight() + buttonSizingFudge)
            bg:y(-buttonSizingFudge / 2)
        end,
        UpdateToggleStatusCommand = function(self)
            -- lit if allRates is true
            if (not allRates and isLocal) or (DLMAN:GetCurrentRateFilter() and not isLocal) then
                self:GetChild("Text"):strokecolor(color("0,0,0,0"))
            else
                self:GetChild("Text"):strokecolor(Brightness(COLORS:getMainColor("SecondaryText"), 0.75))
            end
        end,
        ClickCommand = function(self, params)
            if params.update == "OnMouseDown" then
                MESSAGEMAN:Broadcast("ToggleCurrentRate")
            end
        end,
        RolloverUpdateCommand = function(self, params)
            if params.update == "in" then
                self:diffusealpha(buttonHoverAlpha)
            else
                self:diffusealpha(1)
            end
        end
    }

}

t[#t+1] = scoreList()


return t
