local focused = false
local t = Def.ActorFrame {
    Name = "ScoresPageFile",
    InitCommand = function(self)
        -- hide all general box tabs on startup
        self:diffusealpha(0)
    end,
    WheelSettledMessageCommand = function(self, params)
        -- cascade visual update to everything
        -- self:playcommand("Set", {song = params.song, group = params.group, hovered = params.hovered, steps = params.steps})
        self:playcommand("UpdateDisplay")
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params and params.tab ~= nil then
            if params.tab == SCUFF.scoretabindex then
                self:z(200)
                self:smooth(0.2)
                self:diffusealpha(1)
                -- if focused was already on, we double tapped
                -- enable "double tap" behavior
                if focused then
                    self:playcommand("Doubletap")
                end
                focused = true
                self:playcommand("UpdateDisplay")
            else
                self:z(-100)
                self:smooth(0.2)
                self:diffusealpha(0)
                focused = false
            end
        end
    end,
    CurrentRateChangedMessageCommand = function(self)
        self:playcommand("UpdateDisplay")
    end,
    ChangedStepsMessageCommand = function(self, params)
        self:playcommand("UpdateDisplay")
    end,
    UpdateDisplayCommand = function(self)
        self:playcommand("UpdateScores")
        self:playcommand("UpdateList")
    end
}

-- this can be nil if no scores exist in the profile
local mostRecentScore = SCOREMAN:GetMostRecentScore()

local ratios = {
    UpperLipHeight = 43 / 1080,
    LipSeparatorThickness = 2 / 1080,
    ItemUpperSpacing = 68 / 1080, -- top of frame to top of text, to push all the items down

    PageTextRightGap = 33 / 1920, -- right of frame, right of text
    PageTextUpperGap = 48 / 1080, -- top of frame, top of text

    ItemWidth = 776 / 1920, -- this should just be frame width
    ItemHeight = 45 / 1080, -- rough approximation of height (top of text to somewhere down below)
    ItemAllottedSpace = 381 / 1080, -- from top edge of top item to top edge of bottom item

    ItemIndexLeftGap = 11 / 1920, -- left edge of frame to left edge of number
    ItemIndexWidth = 38 / 1920, -- left edge of number to just before SSR

    LeftCenteredAlignmentLineLeftGap = 126 / 1920, -- from left edge of frame to half way between SSR and player name
    LeftCenteredAlignmentDistance = 8 / 1920, -- the distance to push each alignment away from the line above
    RightInfoLeftAlignLeftGap = 566 / 1920, -- left edge of frame to left edge of text/icons on right side
    RightInfoRightAlignRightGap = 33 / 1920, -- right edge of frame to right edge of text

    SSRRateWidth = 70 / 1920, -- rough measurement of the area needed for SSR (rate should be smaller but will be restricted the same)
    NameJudgmentWidth = 430 / 1920, -- same but for name and judgments

    TrophySize = 21 / 1080, -- height and width of the icon
    PlaySize = 19 / 1080,
    IconHeight = 19 / 1080, -- uhhhhh

    -- local page stuff
    SideBufferGap = 12 / 1920, -- from edge of frame to left end of leftmost items, and rightmost of all rightmost items (ideally)
    GradeUpperGap = 10 / 1080, -- from bottom of top lip to top of grade
    ClearTypeUpperGap = 74 / 1080, -- bottom of top lip to top of cleartype
    IconSetUpperGap = 109 / 1080, -- bottom of top lip to top of all icons under cleartype
    IconSetSpacing = 6 / 1920, -- distance between icons

    -- width of each of these is Width - DetailLineLeftGap - SideBufferGap*2
    DetailLineLeftGap = 118 / 1920, -- left edge to left edge of text
    DetailLine1TopGap = 12 / 1080, -- bottom of top lip to top of text
    DetailLine2TopGap = 37 / 1080, -- bottom of top lip to top of text
    DetailLine3TopGap = 62 / 1080, -- bottom of top lip to top of text
    DetailLine4TopGap = 87 / 1080, -- bottom of top lip to top of text
    DetailLine5TopGap = 112 / 1080, -- bottom of top lip to top of text

    -- entries necessary for the copy-paste judgment bars from evaluation
    JudgmentBarsTopGap = 145 / 1080, -- bottom of top lip to top of first judgment bar
    JudgmentBarHeight = 22 / 1080,
    JudgmentBarLength = 640 / 1920,
    JudgmentBarSpacing = 4 / 1080,
    JudgmentBarAllottedSpace = 129 / 1080,
    JudgmentNameLeftGap = 21 / 1920,
    JudgmentCountRightGap = 74 / 1920,

    ScoreRateListTopGap = 145 / 1080, -- bottom of top lip to top of frame
    ScoreRateListLeftGap = 664 / 1920, -- left edge to left edge of items
    ScoreRateListTopBuffer = 19 / 1080, -- top of frame to top of first item (the top of frame holds the "up" button)
    ScoreRateListAllottedSpace = 261 / 1080, -- top of top item to top of bottom item
    ScoreRateListBottomTopGap = 289 / 1080, -- really bad name: top of frame to top of "down" button
    ScoreRateListWidth = 100 / 1920, -- width of the text

    MainGraphicWidth = 640 / 1920, -- width of judgment bars and offset plot
    OffsetPlotHeight = 184 / 1080,
    OffsetPlotUpperGap = 308 / 1080, -- bottom of top lip to top of graph
}

local actuals = {
    UpperLipHeight = ratios.UpperLipHeight * SCREEN_HEIGHT,
    LipSeparatorThickness = ratios.LipSeparatorThickness * SCREEN_HEIGHT,
    ItemUpperSpacing = ratios.ItemUpperSpacing * SCREEN_HEIGHT,
    PageTextRightGap = ratios.PageTextRightGap * SCREEN_WIDTH,
    PageTextUpperGap = ratios.PageTextUpperGap * SCREEN_HEIGHT,
    ItemWidth = ratios.ItemWidth * SCREEN_WIDTH,
    ItemHeight = ratios.ItemHeight * SCREEN_HEIGHT,
    ItemAllottedSpace = ratios.ItemAllottedSpace * SCREEN_HEIGHT,
    ItemIndexLeftGap = ratios.ItemIndexLeftGap * SCREEN_WIDTH,
    ItemIndexWidth = ratios.ItemIndexWidth * SCREEN_WIDTH,
    LeftCenteredAlignmentLineLeftGap = ratios.LeftCenteredAlignmentLineLeftGap * SCREEN_WIDTH,
    LeftCenteredAlignmentDistance = ratios.LeftCenteredAlignmentDistance * SCREEN_WIDTH,
    RightInfoLeftAlignLeftGap = ratios.RightInfoLeftAlignLeftGap * SCREEN_WIDTH,
    RightInfoRightAlignRightGap = ratios.RightInfoRightAlignRightGap * SCREEN_WIDTH,
    SSRRateWidth = ratios.SSRRateWidth * SCREEN_WIDTH,
    NameJudgmentWidth = ratios.NameJudgmentWidth * SCREEN_WIDTH,
    TrophySize = ratios.TrophySize * SCREEN_HEIGHT,
    PlaySize = ratios.PlaySize * SCREEN_HEIGHT,
    IconHeight = ratios.IconHeight * SCREEN_HEIGHT,
    SideBufferGap = ratios.SideBufferGap * SCREEN_WIDTH,
    GradeUpperGap = ratios.GradeUpperGap * SCREEN_HEIGHT,
    ClearTypeUpperGap = ratios.ClearTypeUpperGap * SCREEN_HEIGHT,
    IconSetUpperGap = ratios.IconSetUpperGap * SCREEN_HEIGHT,
    IconSetSpacing = ratios.IconSetSpacing * SCREEN_WIDTH,
    DetailLineLeftGap = ratios.DetailLineLeftGap * SCREEN_WIDTH,
    DetailLine1TopGap = ratios.DetailLine1TopGap * SCREEN_HEIGHT,
    DetailLine2TopGap = ratios.DetailLine2TopGap * SCREEN_HEIGHT,
    DetailLine3TopGap = ratios.DetailLine3TopGap * SCREEN_HEIGHT,
    DetailLine4TopGap = ratios.DetailLine4TopGap * SCREEN_HEIGHT,
    DetailLine5TopGap = ratios.DetailLine5TopGap * SCREEN_HEIGHT,
    JudgmentBarsTopGap = ratios.JudgmentBarsTopGap * SCREEN_HEIGHT,
    JudgmentBarHeight = ratios.JudgmentBarHeight * SCREEN_HEIGHT,
    JudgmentBarLength = ratios.JudgmentBarLength * SCREEN_WIDTH,
    JudgmentBarSpacing = ratios.JudgmentBarSpacing * SCREEN_HEIGHT,
    JudgmentBarAllottedSpace = ratios.JudgmentBarAllottedSpace * SCREEN_HEIGHT,
    JudgmentNameLeftGap = ratios.JudgmentNameLeftGap * SCREEN_WIDTH,
    JudgmentCountRightGap = ratios.JudgmentCountRightGap * SCREEN_WIDTH,
    ScoreRateListTopGap = ratios.ScoreRateListTopGap * SCREEN_HEIGHT,
    ScoreRateListLeftGap = ratios.ScoreRateListLeftGap * SCREEN_WIDTH,
    ScoreRateListTopBuffer = ratios.ScoreRateListTopBuffer * SCREEN_HEIGHT,
    ScoreRateListAllottedSpace = ratios.ScoreRateListAllottedSpace * SCREEN_HEIGHT,
    ScoreRateListBottomTopGap = ratios.ScoreRateListBottomTopGap * SCREEN_HEIGHT,
    ScoreRateListWidth = ratios.ScoreRateListWidth * SCREEN_WIDTH,
    MainGraphicWidth = ratios.MainGraphicWidth * SCREEN_WIDTH,
    OffsetPlotHeight = ratios.OffsetPlotHeight * SCREEN_HEIGHT,
    OffsetPlotUpperGap = ratios.OffsetPlotUpperGap * SCREEN_HEIGHT,
}

-- scoping magic
do
    -- copying the provided ratios and actuals tables to have access to the sizing for the overall frame
    local rt = Var("ratios")
    for k,v in pairs(rt) do
        ratios[k] = v
    end
    local at = Var("actuals")
    for k,v in pairs(at) do
        actuals[k] = v
    end
end

t[#t+1] = Def.Quad {
    Name = "UpperLip",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.UpperLipHeight)
        self:diffusealpha(0.6)
        registerActorToColorConfigElement(self, "main", "SecondaryBackground")
    end
}

t[#t+1] = Def.Quad {
    Name = "LipTop",
    InitCommand = function(self)
        self:halign(0)
        self:zoomto(actuals.Width, actuals.LipSeparatorThickness)
        self:diffusealpha(0.3)
        registerActorToColorConfigElement(self, "main", "SeparationDivider")
    end
}

-- online and offline (default is offline)
local isLocal = true
-- all scores or top scores (online only)
-- dont have to modify this var with direct values, call DLMAN to update it
local allScores = not DLMAN:GetTopScoresOnlyFilter()

-- how many to display
local itemCount = 7
local scoreListAnimationSeconds = 0.05
local localPageAnimationSeconds = 0.1

local itemIndexSize = 0.95
local ssrTextSize = 0.9
local rateTextSize = 0.92
local nameTextSize = 0.9
local judgmentTextSize = 0.6
local wifePercentTextSize = 0.92
local dateTextSize = 0.6
local pageTextSize = 0.7
local loadingTextSize = 0.9
local textzoomFudge = 5

local buttonHoverAlpha = 0.6

local gradeTextSize = 2.2
local clearTypeTextSize = 1.15
local detailTextSize = 0.75
local rateTextSize = 0.65

-- functionally create the score list
-- this is basically a slimmed version of the Evaluation Scoreboard
local function createList()
    local page = 1
    local maxPage = 1
    local scorelistframe = nil

    local function movePage(n)
        if maxPage <= 1 then
            return
        end

        -- math to make pages loop both directions
        local nn = (page + n) % (maxPage + 1)
        if nn == 0 then
            nn = n > 0 and 1 or maxPage
        end
        page = nn

        if scorelistframe then
            scorelistframe:playcommand("MovedPage")
        end
    end

    -- yes, we do have a country filter
    -- we don't tell anyone
    -- the Global country just shows everyone
    local dlmanScoreboardCountryFilter = "Global"

    local scores = {} -- online scores
    local localscore = nil -- local score
    local localscoreIndex = 1
    local localrtTable = nil -- local rate table (scores)
    local localrateIndex = 1
    local localrates = nil

    -- to prevent bombing the server repeatedly with leaderboard requests
    -- chartkeys to booleans
    local alreadyRequestedLeaderboard = {}
    -- to signify whether or not an active request for a chart leaderboard is taking place
    local fetchingScores = {}

    local t = Def.ActorFrame {
        Name = "ScoreListFrame",
        BeginCommand = function(self)
            scorelistframe = self
        end,
        UpdateScoresCommand = function(self)
            if self:IsInvisible() then return end
            -- HACK BEHAVIOR: use SCUFFMAN's general tab index to pretend we aren't on the scores tab if isLocal is false
            -- this lets us change rate for the CurrentRate stuff in the online tab
            if not isLocal then
                SCUFF.generaltab = -1
            else
                SCUFF.generaltab = SCUFF.scoretabindex
            end
            -- end hack
            page = 1

            -- no steps, no scores.
            local steps = GAMESTATE:GetCurrentSteps()
            if steps == nil then
                scores = {}
                if isLocal then localrtTable = nil end
                return
            end

            -- local scoreboard is somewhere else
            if isLocal then
                scores = {}
                localrtTable = getRateTable()
                local sng = GAMESTATE:GetCurrentSong()
                if localrtTable ~= nil then
                    localrates, localrateIndex = getUsedRates(localrtTable)
                    localscoreIndex = 1
                end
                return
            else
                -- operate with dlman scores
                -- ... everything here is determined by internal bools set by the toggle buttons
                scores = DLMAN:GetChartLeaderBoard(steps:GetChartKey(), dlmanScoreboardCountryFilter)

                -- if scores comes back nil, then the chart is unranked
                if scores == nil then
                    if steps then
                        local kee = steps:GetChartKey()
                        fetchingScores[kee] = false
                    end
                    return
                end

                -- this is the initial request for the leaderboard which should only end up running once
                -- and when it finishes, it loops back through this command
                -- on the second passthrough, the leaderboard is hopefully filled out
                if #scores == 0 then
                    if steps then
                        local kee = steps:GetChartKey()
                        if not alreadyRequestedLeaderboard[kee] then
                            alreadyRequestedLeaderboard[kee] = true
                            fetchingScores[kee] = true
                            DLMAN:RequestChartLeaderBoardFromOnline(
                                steps:GetChartKey(),
                                function(leaderboard)
                                    -- disallow replacing the leaderboard if the request doesnt match the current steps
                                    local s = GAMESTATE:GetCurrentSteps()
                                    if s and s:GetChartKey() == kee then
                                        fetchingScores[kee] = false
                                        self:queuecommand("UpdateScores")
                                        self:queuecommand("UpdateList")
                                    end
                                end
                            )
                        end
                    end
                end
            end
        end,
        UpdateListCommand = function(self)
            if self:IsInvisible() then return end

            -- local logic
            if isLocal then
                if localrtTable ~= nil and localrates ~= nil and localrates[localrateIndex] ~= nil and localrtTable[localrates[localrateIndex]] ~= nil then
                    localscore = localrtTable[localrates[localrateIndex]][localscoreIndex]
                else
                    localscore = nil
                end
            else
                localscore = nil
            end


            -- online logic
            if scores == nil then
                maxPage = 1
            else
                maxPage = math.ceil(#scores / itemCount)
            end

            for i = 1, itemCount do
                local index = (page - 1) * itemCount + i
                self:GetChild("ScoreItem_"..i):playcommand("SetScore", {scoreIndex = index})
            end
            self:GetParent():playcommand("UpdateButtons")
        end,
        MovedPageCommand = function(self)
            self:playcommand("UpdateList")
        end,
        ToggleCurrentRateCommand = function(self)
            if isLocal then
                -- the local scoreboard is sorted by rate always
            else
                DLMAN:ToggleRateFilter()
            end
            self:playcommand("UpdateScores")
            self:playcommand("UpdateList")
        end,
        ToggleAllScoresCommand = function(self)
            if DLMAN:IsLoggedIn() then
                DLMAN:ToggleTopScoresOnlyFilter()
                self:playcommand("UpdateScores")
                self:playcommand("UpdateList")
            end
        end,
        ToggleLocalCommand = function(self)
            -- this will allow Online -> Local if you happen to be in an impossible state
            if not isLocal or DLMAN:IsLoggedIn() then
                isLocal = not isLocal
            end
            self:playcommand("UpdateScores")
            self:playcommand("UpdateList")
        end,
        ToggleInvalidCommand = function(self)
            if DLMAN:IsLoggedIn() then
                DLMAN:ToggleCCFilter()
                self:playcommand("UpdateScores")
                self:playcommand("UpdateList")
            end
        end,
        UpdateLoginStatusCommand = function(self)
            local nowloggedin = DLMAN:IsLoggedIn()
            if not isLocal and not nowloggedin then
                isLocal = true
                self:playcommand("UpdateScores")
                self:playcommand("UpdateList")
            end
        end,
        DoubletapCommand = function(self)
            -- if double tapping the scores tab, swap online/local
            -- convenience :)
            self:playcommand("ToggleLocal")
        end
    }


    local function createItem(i)
        local score = nil
        local scoreIndex = i

        return Def.ActorFrame {
            Name = "ScoreItem_"..i,
            InitCommand = function(self)
                self:y((actuals.ItemAllottedSpace / (itemCount - 1)) * (i-1) + actuals.ItemUpperSpacing)
            end,
            SetScoreCommand = function(self, params)
                scoreIndex = params.scoreIndex
                -- nil scores table: unranked chart
                if scores ~= nil then
                    score = scores[scoreIndex]
                else
                    score = nil
                end
                self:finishtweening()
                self:diffusealpha(0)
                if score ~= nil then
                    self:smooth(scoreListAnimationSeconds * i)
                    self:diffusealpha(1)
                end
            end,

            LoadFont("Common Normal") .. {
                Name = "Index",
                InitCommand = function(self)
                    self:halign(0):valign(1)
                    self:xy(actuals.ItemIndexLeftGap, actuals.ItemHeight)
                    self:zoom(itemIndexSize)
                    self:maxwidth(actuals.ItemIndexWidth / itemIndexSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        self:settextf("%d.", scoreIndex)
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "SSR",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.LeftCenteredAlignmentLineLeftGap - actuals.LeftCenteredAlignmentDistance)
                    self:zoom(ssrTextSize)
                    self:maxwidth(actuals.SSRRateWidth / ssrTextSize - textzoomFudge)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local ssr = score:GetSkillsetSSR("Overall")
                        self:settextf("%05.2f", ssr)
                        self:diffuse(colorByMSD(ssr))
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Rate",
                InitCommand = function(self)
                    self:halign(1):valign(1)
                    self:xy(actuals.LeftCenteredAlignmentLineLeftGap - actuals.LeftCenteredAlignmentDistance, actuals.ItemHeight)
                    self:zoom(rateTextSize)
                    self:maxwidth(actuals.SSRRateWidth / rateTextSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local rt = score:GetMusicRate()
                        self:settext(getRateString(rt))
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "PlayerName",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(actuals.LeftCenteredAlignmentLineLeftGap + actuals.LeftCenteredAlignmentDistance)
                    self:zoom(nameTextSize)
                    self:maxwidth(actuals.NameJudgmentWidth / nameTextSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local n = score:GetName()
                        if n == "" then
                            n = "<No Name>"
                        elseif n == "#P1#" then
                            if score:GetScoreKey() == mostRecentScore:GetScoreKey() then
                                n = "Last Score"
                            else
                                n = "You"
                            end
                        end
                        self:settext(n)
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "JudgmentsAndCombo",
                InitCommand = function(self)
                    self:halign(0):valign(1)
                    self:xy(actuals.LeftCenteredAlignmentLineLeftGap + actuals.LeftCenteredAlignmentDistance, actuals.ItemHeight)
                    self:zoom(judgmentTextSize)
                    self:maxwidth(actuals.NameJudgmentWidth / judgmentTextSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        -- a HighScore method does exist to produce this but we want to be able to modify it from the theme
                        local jgMaStr = tostring(score:GetTapNoteScore("TapNoteScore_W1"))
                        local jgPStr = tostring(score:GetTapNoteScore("TapNoteScore_W2"))
                        local jgGrStr = tostring(score:GetTapNoteScore("TapNoteScore_W3"))
                        local jgGoStr = tostring(score:GetTapNoteScore("TapNoteScore_W4"))
                        local jgBStr = tostring(score:GetTapNoteScore("TapNoteScore_W5"))
                        local jgMiStr = tostring(score:GetTapNoteScore("TapNoteScore_Miss"))
                        local comboStr = tostring(score:GetMaxCombo())
                        self:settextf("%s  |  %s  |  %s  |  %s  |  %s  |  %s  x%s", jgMaStr, jgPStr, jgGrStr, jgGoStr, jgBStr, jgMiStr, comboStr)
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "WifePercent",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.ItemWidth - actuals.RightInfoRightAlignRightGap)
                    self:zoom(wifePercentTextSize)
                    -- 5/6 the space allowed vs the date
                    -- the icons take up the other half probably
                    self:maxwidth((actuals.ItemWidth - actuals.RightInfoLeftAlignLeftGap - actuals.RightInfoRightAlignRightGap) / 6 * 4 / wifePercentTextSize)
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    self:playcommand("SetScore")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local ws = score:GetWifeScore()
                        local wifeStr = ""
                        if ws < 0.99 then
                            wifeStr = string.format("%05.2f%%", notShit.floor(ws * 10000) / 100)
                        else
                            wifeStr = string.format("%05.4f%%", notShit.floor(ws * 1000000) / 10000)
                        end
                        local grade = GetGradeFromPercent(score:GetWifeScore())
                        self:settext(wifeStr)
                        self:diffuse(colorByGrade(grade))
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "DateTime",
                InitCommand = function(self)
                    self:halign(1):valign(1)
                    self:xy(actuals.ItemWidth - actuals.RightInfoRightAlignRightGap, actuals.ItemHeight)
                    self:zoom(dateTextSize)
                    self:maxwidth((actuals.ItemWidth - actuals.RightInfoLeftAlignLeftGap - actuals.RightInfoRightAlignRightGap) / dateTextSize - textzoomFudge)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        self:settext(score:GetDate())
                    end
                end
            },
            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "showReplay")) .. {
                Name = "ShowReplay",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.RightInfoLeftAlignLeftGap)
                    self:zoomto(actuals.PlaySize, actuals.PlaySize)
                    registerActorToColorConfigElement(self, "main", "IconColor")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        if score:HasReplayData() then
                            self:diffusealpha(1)
                        else
                            self:diffusealpha(0)
                        end
                    end
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    local sng = GAMESTATE:GetCurrentSteps()
                    DLMAN:RequestOnlineScoreReplayData(
                        score,
                        function()
                            local scr = SCREENMAN:GetTopScreen()
                            local sng2 = GAMESTATE:GetCurrentSteps()
                            if sng and sng2 and sng:GetChartKey() == sng2:GetChartKey() then
                                local success = SCREENMAN:GetTopScreen():PlayReplay(score)
                                if success then
                                    SCREENMAN:set_input_redirected(PLAYER_1, false)
                                end
                            end
                        end
                    )
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(buttonHoverAlpha)
                    TOOLTIP:SetText("Show Replay")
                    TOOLTIP:Show()
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(1)
                    TOOLTIP:Hide()
                end
            },
            --[[ -- can't view online eval screens
            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "showEval")) .. {
                Name = "ShowEval",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.RightInfoLeftAlignLeftGap + actuals.TrophySize * 1.2)
                    self:zoomto(actuals.TrophySize, actuals.TrophySize)
                    registerActorToColorConfigElement(self, "main", "IconColor")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        if score:HasReplayData() then
                            self:diffusealpha(1)
                        else
                            self:diffusealpha(0)
                        end
                    end
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end

                    local success = SCREENMAN:GetTopScreen():ShowEvalScreenForScore(score)
                    if success then
                        SCREENMAN:set_input_redirected(PLAYER_1, false)
                    end
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end

                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end

                    self:diffusealpha(1)
                end
            },]]
        }
    end


    local function localRateFrame()
        local ratecount = 9 -- really just how many items we want spots for
        local ratepage = 1
        local maxratepage = 1
        local framecopy = nil

        local function moveRatePage(n)
            ratepage = clamp(ratepage + n, 1, maxratepage)
            if framecopy ~= nil then
                framecopy:GetParent():playcommand("UpdateList")
            end
        end

        local function rateItem(i)
            local index = i
            return UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "RateButton_"..i,
                InitCommand = function(self)
                    self:valign(0)
                    self:x(actuals.ScoreRateListWidth / 2)
                    self:y(actuals.ScoreRateListTopBuffer + actuals.ScoreRateListAllottedSpace / ratecount * (i-1))
                    self:zoom(rateTextSize)
                    self:maxwidth(actuals.ScoreRateListWidth / rateTextSize - textzoomFudge)
                    self:settext("")
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    if index == localrateIndex then
                        self:diffusealpha(0.6)
                    else
                        self:diffusealpha(1)
                    end
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    localrateIndex = index
                    localscoreIndex = 1
                    -- [this] [rateframe] [localpage] [scorelist]
                    self:GetParent():GetParent():GetParent():playcommand("UpdateList")
                end,
                UpdateListCommand = function(self)
                    index = i + ((ratepage-1) * ratecount)
                    local count = 0
                    if localrtTable == nil then
                        self:settext("")
                        return
                    end
                    if localrtTable[localrates[index]] ~= nil then
                        count = #localrtTable[localrates[index]]
                    end
                    if index == localrateIndex then
                        self:diffusealpha(0.6)
                    else
                        self:diffusealpha(1)
                    end
                    if index <= #localrates then
                        self:settextf("%s (%d)", localrates[index], count)
                    else
                        self:settext("")
                    end
                end
            }
        end

        local t = Def.ActorFrame {
            Name = "RateFrame",
            InitCommand = function(self)
                framecopy = self
            end,
            BeginCommand = function(self)
                local snm = SCREENMAN:GetTopScreen():GetName()
                local anm = self:GetName()
                CONTEXTMAN:RegisterToContextSet(snm, "Main1", anm)

                local selectPressed = false
                SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                    -- require context is set and the general box is set to this tab
                    if not CONTEXTMAN:CheckContextSet(snm, "Main1") or SCUFF.generaltab ~= SCUFF.scoretabindex then
                        selectPressed = false
                        return
                    end

                    if event.type == "InputEventType_FirstPress" then
                        if event.button == "EffectUp" and localrtTable ~= nil and localrates ~= nil and #localrates > 0 then
                            if selectPressed then
                                localrateIndex = clamp(localrateIndex+1, 1, #localrates)
                                if localrateIndex > ratepage * ratecount then
                                    ratepage = ratepage + 1
                                end
                            else
                                local max = #localrtTable[localrates[localrateIndex]]
                                local beforeindex = localscoreIndex
                                localscoreIndex = clamp(localscoreIndex+1, 1, max)
                            end
                            self:GetParent():GetParent():playcommand("UpdateList")
                        elseif event.button == "EffectDown" and localrtTable ~= nil and localrates ~= nil and #localrates > 0 then
                            if selectPressed then
                                localrateIndex = clamp(localrateIndex-1, 1, #localrates)
                                if localrateIndex < (ratepage-1) * ratecount + 1 then
                                    ratepage = ratepage - 1
                                end
                            else
                                local max = #localrtTable[localrates[localrateIndex]]
                                local beforeindex = localscoreIndex
                                localscoreIndex = clamp(localscoreIndex-1, 1, max)
                            end
                            self:GetParent():GetParent():playcommand("UpdateList")
                        elseif event.button == "Select" then
                            selectPressed = true
                        end
                    elseif event.type == "InputEventType_Release" then
                        if event.button == "Select" then
                            selectPressed = false
                        end
                    end
                end)
            end,
            UpdateScoresCommand = function(self)
                maxratepage = 1
                ratepage = 1
            end,
            UpdateListCommand = function(self)
                if localrtTable ~= nil and localrates ~= nil then
                    maxratepage = math.ceil(#localrates / ratecount)
                else
                    maxratepage = 1
                end
            end,

            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "PageUp",
                InitCommand = function(self)
                    self:valign(0)
                    self:xy(actuals.ScoreRateListWidth / 2, 0)
                    self:settext("^")
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                UpdateListCommand = function(self)
                    if ratepage-1 < 1 or maxratepage == 1 then
                        self:diffusealpha(0)
                    else
                        if isOver(self) then
                            self:diffusealpha(buttonHoverAlpha)
                        else
                            self:diffusealpha(1)
                        end
                    end
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(1)
                end,
                MouseDownCommand = function(self, params)
                    moveRatePage(-1)
                end
            },
            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "PageDown",
                InitCommand = function(self)
                    self:valign(0)
                    self:xy(actuals.ScoreRateListWidth / 2, actuals.ScoreRateListBottomTopGap)
                    self:rotationz(180)
                    self:settext("^")
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                UpdateListCommand = function(self)
                    if ratepage+1 > maxratepage or maxratepage == 1 then
                        self:diffusealpha(0)
                    else
                        if isOver(self) then
                            self:diffusealpha(buttonHoverAlpha)
                        else
                            self:diffusealpha(1)
                        end
                    end
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(1)
                end,
                MouseDownCommand = function(self, params)
                    moveRatePage(1)
                end
            }
        }

        for i = 1, ratecount do
            t[#t+1] = rateItem(i)
        end

        return t
    end


    for i = 1, itemCount do
        t[#t+1] = createItem(i)
    end

    -- we have placed the local page into its own frame for management of everything quickly
    -- when local, this is visible
    -- when online, this is not visible (and the score list should show up)
    -- etc
    t[#t+1] = Def.ActorFrame {
        Name = "LocalScorePageFrame",
        InitCommand = function(self)
            self:y(actuals.UpperLipHeight)
        end,
        UpdateListCommand = function(self)
            if localscore ~= nil then
                self:finishtweening()
                self:smooth(localPageAnimationSeconds)
                self:diffusealpha(1)
            else
                self:finishtweening()
                self:smooth(localPageAnimationSeconds)
                self:diffusealpha(0)
            end
        end,

        LoadFont("Common Normal") .. {
            Name = "Grade",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.SideBufferGap, actuals.GradeUpperGap)
                self:zoom(gradeTextSize)
                self:maxwidth((actuals.DetailLineLeftGap - actuals.SideBufferGap) / gradeTextSize - textzoomFudge)
                self:settext("")
            end,
            ColorConfigUpdatedMessageCommand = function(self)
                self:playcommand("UpdateList")
            end,
            UpdateListCommand = function(self)
                if localscore ~= nil then
                    local grade = THEME:GetString("Grade", ToEnumShortString(localscore:GetWifeGrade()))
                    self:diffuse(colorByGrade(localscore:GetWifeGrade()))
                    self:settext(grade)
                end
            end
        },
        LoadFont("Common Normal") .. {
            Name = "ClearType",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.SideBufferGap, actuals.ClearTypeUpperGap)
                self:zoom(clearTypeTextSize)
                self:maxwidth((actuals.DetailLineLeftGap - actuals.SideBufferGap) / clearTypeTextSize - textzoomFudge)
                self:settext("")
            end,
            ColorConfigUpdatedMessageCommand = function(self)
                self:playcommand("UpdateList")
            end,
            UpdateListCommand = function(self)
                if localscore ~= nil then
                    local ct = getClearTypeFromScore(localscore, 0)
                    self:diffuse(getClearTypeFromScore(localscore, 2))
                    self:settext(ct)
                end
            end
        },
        UIElements.SpriteButton(1, 1, THEME:GetPathG("", "upload")) .. {
            Name = "UploadButton",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.SideBufferGap, actuals.IconSetUpperGap)
                self:zoomto(actuals.PlaySize, actuals.IconHeight)
                registerActorToColorConfigElement(self, "main", "IconColor")
            end,
            UpdateListCommand = function(self)
                if localscore ~= nil then
                    if localscore:HasReplayData() and DLMAN:IsLoggedIn() then
                        self:diffusealpha(1)
                    else
                        self:diffusealpha(0)
                    end
                end
            end,
            MouseOverCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(buttonHoverAlpha)
                TOOLTIP:SetText("Upload Score")
                TOOLTIP:Show()
            end,
            MouseOutCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(1)
                TOOLTIP:Hide()
            end,
            MouseDownCommand = function(self, params)
                if self:IsInvisible() then return end
                if localscore ~= nil and localscore:HasReplayData() then
                    DLMAN:SendReplayDataForOldScore(localscore:GetScoreKey())
                    local steps = GAMESTATE:GetCurrentSteps()
                    ms.ok(string.format("Uploading Score (chart %s key %s)", steps:GetChartKey(), localscore:GetScoreKey()))
                end
            end
        },
        UIElements.SpriteButton(1, 1, THEME:GetPathG("", "showEval")) .. {
            Name = "ShowEval",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.SideBufferGap + actuals.PlaySize + actuals.IconSetSpacing, actuals.IconSetUpperGap)
                self:zoomto(actuals.TrophySize, actuals.IconHeight)
                registerActorToColorConfigElement(self, "main", "IconColor")
            end,
            UpdateListCommand = function(self)
                if localscore ~= nil then
                    if localscore:HasReplayData() then
                        self:diffusealpha(1)
                    else
                        self:diffusealpha(0)
                    end
                end
            end,
            MouseOverCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(buttonHoverAlpha)
                TOOLTIP:SetText("Show Evaluation")
                TOOLTIP:Show()
            end,
            MouseOutCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(1)
                TOOLTIP:Hide()
            end,
            MouseDownCommand = function(self, params)
                if self:IsInvisible() then return end
                if localscore ~= nil and localscore:HasReplayData() then
                    local success = SCREENMAN:GetTopScreen():ShowEvalScreenForScore(localscore)
                    if success then
                        SCREENMAN:set_input_redirected(PLAYER_1, false)
                    end
                end
            end
        },
        UIElements.SpriteButton(1, 1, THEME:GetPathG("", "showReplay")) .. {
            Name = "ShowReplay",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.SideBufferGap + actuals.PlaySize + actuals.TrophySize + actuals.IconSetSpacing * 2, actuals.IconSetUpperGap)
                self:zoomto(actuals.PlaySize, actuals.IconHeight)
                registerActorToColorConfigElement(self, "main", "IconColor")
            end,
            UpdateListCommand = function(self)
                if localscore ~= nil then
                    if localscore:HasReplayData() then
                        self:diffusealpha(1)
                    else
                        self:diffusealpha(0)
                    end
                end
            end,
            MouseOverCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(buttonHoverAlpha)
                TOOLTIP:SetText("Show Replay")
                TOOLTIP:Show()
            end,
            MouseOutCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(1)
                TOOLTIP:Hide()
            end,
            MouseDownCommand = function(self, params)
                if self:IsInvisible() then return end
                if localscore ~= nil and localscore:HasReplayData() then
                    local success = SCREENMAN:GetTopScreen():PlayReplay(localscore)
                    if success then
                        SCREENMAN:set_input_redirected(PLAYER_1, false)
                    end
                end
            end
        },
        LoadFont("Common Normal") .. {
            Name = "SSRPercentWVJudge",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.DetailLineLeftGap, actuals.DetailLine1TopGap)
                self:zoom(detailTextSize)
                self:maxwidth((actuals.Width - actuals.SideBufferGap - actuals.DetailLineLeftGap) / detailTextSize - textzoomFudge)
                self:settext("11.11 | 11.11% (Wife 0 | Judge 0)")
            end,
            ColorConfigUpdatedMessageCommand = function(self)
                self:playcommand("UpdateList")
            end,
            UpdateListCommand = function(self)
                if localscore ~= nil then
                    local ssrstr = string.format("%5.2f", localscore:GetSkillsetSSR("Overall"))
                    local ssrcolr = colorByMSD(localscore:GetSkillsetSSR("Overall"))
                    local wife = localscore:GetWifeScore()
                    local wifecolr = colorByGrade(localscore:GetWifeGrade())
                    local wv = "Wife "..localscore:GetWifeVers()
                    local judge = 4
                    if PREFSMAN:GetPreference("SortBySSRNormPercent") == false then
                        judge = table.find(ms.JudgeScalers, notShit.round(localscore:GetJudgeScale(), 2))
                    end
                    if not judge then judge = 4 end
                    if judge < 4 then judge = 4 end
                    local js = judge ~= 9 and judge or "Justice"
                    local perc = ""
                    if wife < 0.99 then
                        perc = string.format("%05.2f%%", notShit.floor(wife * 10000) / 100)
                    else
                        perc = string.format("%05.4f%%", notShit.floor(wife * 1000000) / 10000)
                    end
                    self:ClearAttributes()
                    self:diffuse(COLORS:getMainColor("PrimaryText"))
                    self:diffusealpha(1)
                    self:settextf("%s | %s (%s | Judge %s)", ssrstr, perc, wv, js)
                    self:AddAttribute(0, {Length = #ssrstr, Diffuse = ssrcolr})
                    self:AddAttribute(#string.format("%s | ", ssrstr), {Length = #perc, Diffuse = wifecolr})
                end
            end
        },
        LoadFont("Common Normal") .. {
            Name = "CBs",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.DetailLineLeftGap, actuals.DetailLine2TopGap)
                self:zoom(detailTextSize)
                self:maxwidth((actuals.Width - actuals.SideBufferGap - actuals.DetailLineLeftGap) / detailTextSize - textzoomFudge)
                self:settext("")
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            UpdateListCommand = function(self)
                if localscore ~= nil then
                    local mc = getScoreComboBreaks(localscore)
                    if mc ~= nil then
                        self:settextf("Combo Breaks: %s", mc)
                    else
                        self:settext("Combo Breaks: -")
                    end
                end
            end
        },
        LoadFont("Common Normal") .. {
            Name = "MaxCombo",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.DetailLineLeftGap, actuals.DetailLine3TopGap)
                self:zoom(detailTextSize)
                self:maxwidth((actuals.Width - actuals.SideBufferGap - actuals.DetailLineLeftGap) / detailTextSize - textzoomFudge)
                self:settext("")
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            UpdateListCommand = function(self)
                if localscore ~= nil then
                    local mc = localscore:GetMaxCombo()
                    self:settextf("Max Combo: %d", mc)
                end
            end
        },
        LoadFont("Common Normal") .. {
            Name = "DateAchieved",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.DetailLineLeftGap, actuals.DetailLine4TopGap)
                self:zoom(detailTextSize)
                self:maxwidth((actuals.Width - actuals.SideBufferGap - actuals.DetailLineLeftGap) / detailTextSize - textzoomFudge)
                self:settext("")
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            UpdateListCommand = function(self)
                if localscore ~= nil then
                    local d = getScoreDate(localscore)
                    self:settextf("Date Achieved: %s", d)
                end
            end
        },
        LoadFont("Common Normal") .. {
            Name = "Mods",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.DetailLineLeftGap, actuals.DetailLine5TopGap)
                self:zoom(detailTextSize)
                self:maxwidth((actuals.Width - actuals.SideBufferGap - actuals.DetailLineLeftGap) / detailTextSize - textzoomFudge)
                self:settext("")
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            UpdateListCommand = function(self)
                if localscore ~= nil then
                    local m = getModifierTranslations(localscore:GetModifiers())
                    self:settextf("Mods: %s", m)
                end
            end
        },
        localRateFrame() .. {
            InitCommand = function(self)
                self:xy(actuals.ScoreRateListLeftGap, actuals.ScoreRateListTopGap)
            end
        },
        LoadActorWithParams("../../judgmentBars.lua", {
            sizing = {
                JudgmentBarHeight = actuals.JudgmentBarHeight,
                JudgmentBarLength = actuals.JudgmentBarLength,
                JudgmentBarSpacing = actuals.JudgmentBarSpacing,
                JudgmentBarAllottedSpace = actuals.JudgmentBarAllottedSpace,
                JudgmentNameLeftGap = actuals.JudgmentNameLeftGap,
                JudgmentCountRightGap = actuals.JudgmentCountRightGap,
            },
            textSizeMultiplier = 0.6
        }) .. {
            InitCommand = function(self)
                self:xy(actuals.SideBufferGap, actuals.JudgmentBarsTopGap)
            end,
            UpdateListCommand = function(self)
                if localscore ~= nil then
                    local judgeSetting = (PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or table.find(ms.JudgeScalers, notShit.round(localscore:GetJudgeScale(), 2)))
                    self:playcommand("Set", {score = localscore, judgeSetting = judgeSetting})
                end
            end
        },
        LoadActorWithParams("../../offsetplot.lua", {sizing = {Width = actuals.MainGraphicWidth, Height = actuals.OffsetPlotHeight}, textsize = 0.43}) .. {
            InitCommand = function(self)
                self:xy(actuals.SideBufferGap, actuals.OffsetPlotUpperGap)
            end,
            UpdateListCommand = function(self, params)
                if localscore == nil then return end
                local steps = GAMESTATE:GetCurrentSteps()
                local judgeSetting = (PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or table.find(ms.JudgeScalers, notShit.round(localscore:GetJudgeScale(), 2)))
                if steps ~= nil then
                    if localscore:HasReplayData() then
                        local offsets = localscore:GetOffsetVector()
                        -- for online offset vectors a 180 offset is a miss
                        for i, o in ipairs(offsets) do
                            if o >= 180 then
                                offsets[i] = 1000
                            end
                        end
                        local tracks = localscore:GetTrackVector()
                        local types = localscore:GetTapNoteTypeVector()
                        local noterows = localscore:GetNoteRowVector()
                        local holds = localscore:GetHoldNoteVector()
                        local timingdata = steps:GetTimingData()
                        local lastSecond = steps:GetLastSecond()

                        self:playcommand("LoadOffsets", {
                            offsetVector = offsets,
                            trackVector = tracks,
                            timingData = timingdata,
                            noteRowVector = noterows,
                            typeVector = types,
                            holdVector = holds,
                            maxTime = lastSecond,
                            judgeSetting = judgeSetting,
                            columns = steps:GetNumColumns(),
                            rejudged = true,
                        })
                        self:hurrytweening(0.2)
                    else
                        self:playcommand("LoadOffsets", {
                            offsetVector = {},
                            trackVector = {},
                            timingData = nil,
                            noteRowVector = {},
                            typeVector = {},
                            holdVector = {},
                            maxTime = 1,
                            judgeSetting = 4,
                            columns = steps:GetNumColumns(),
                            rejudged = true,
                        })
                        self:hurrytweening(0.1)
                    end
                end
            end
        }
    }


    t[#t+1] = Def.Quad {
        Name = "MouseWheelRegion",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:diffusealpha(0)
            self:zoomto(actuals.Width, actuals.Height)
        end,
        MouseScrollMessageCommand = function(self, params)
            if isOver(self) and focused then
                if isLocal then
                    if localrtTable ~= nil and localrates ~= nil and #localrates > 0 then
                        local max = #localrtTable[localrates[localrateIndex]]
                        local beforeindex = localscoreIndex
                        if params.direction == "Up" then
                            localscoreIndex = clamp(localscoreIndex-1, 1, max)
                        else
                            localscoreIndex = clamp(localscoreIndex+1, 1, max)
                        end
                        if localscoreIndex ~= beforeindex then
                            self:GetParent():playcommand("UpdateList")
                        end
                    end
                else
                    if params.direction == "Up" then
                        movePage(-1)
                    else
                        movePage(1)
                    end
                end
            end
        end
    }

    t[#t+1] = LoadFont("Common Normal") .. {
        Name = "LoadingText",
        InitCommand = function(self)
            self:y((actuals.ItemAllottedSpace / (itemCount - 1)) + actuals.ItemUpperSpacing)
            self:x(actuals.Width / 2)
            self:maxwidth(actuals.Width / loadingTextSize - textzoomFudge)
            self:diffusealpha(0)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        UpdateListCommand = function(self)
            self:finishtweening()
            self:smooth(scoreListAnimationSeconds)
            local steps = GAMESTATE:GetCurrentSteps()

            if isLocal then
                if localrtTable == nil and GAMESTATE:GetCurrentSong() ~= nil then
                    self:diffusealpha(1)
                    self:settext("No local scores recorded")
                else
                    self:diffusealpha(0)
                    self:settext("")
                end
                return
            end

            if scores == nil then
                self:diffusealpha(1)
                self:settext("Chart is unranked")
            elseif #scores == 0 and steps and fetchingScores[steps:GetChartKey()] == true then
                self:diffusealpha(1)
                self:settext("Fetching scores...")
            elseif #scores == 0 and steps and fetchingScores[steps:GetChartKey()] == false then
                self:diffusealpha(1)
                self:settext("No online scores recorded")
            else
                self:diffusealpha(0)
                self:settext("")
            end
        end
    }

    t[#t+1] = UIElements.TextToolTip(1, 1, "Common Normal") .. {
        Name = "PageText",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:xy(actuals.Width - actuals.PageTextRightGap, actuals.PageTextUpperGap)
            self:zoom(pageTextSize)
            self:maxwidth(actuals.Width / pageTextSize - textzoomFudge)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        UpdateListCommand = function(self)
            -- nil scores = no scores
            if scores == nil then
                self:settext("0-0/0")
                return
            end

            if isLocal then
                if localrtTable ~= nil and localrates ~= nil and #localrates > 0 then
                    local mx = #localrtTable[localrates[localrateIndex]]
                    self:settextf("%d/%d", localscoreIndex, mx)
                else
                    self:settext("")
                end
            else
                local lb = clamp((page-1) * (itemCount) + 1, 0, #scores)
                local ub = clamp(page * itemCount, 0, #scores)
                self:settextf("%d-%d/%d", lb, ub, #scores)
            end
        end,
        MouseDownCommand = function(self, params)
            local dir = 0
            if params.event == "DeviceButton_left mouse button" then
                dir = 1
            elseif params.event == "DeviceButton_right mouse button" then
                dir = -1
            else
                return
            end
            if focused then
                if isLocal then
                    if localrtTable ~= nil and localrates ~= nil and #localrates > 0 then
                        local max = #localrtTable[localrates[localrateIndex]]
                        local beforeindex = localscoreIndex
                        localscoreIndex = clamp(localscoreIndex+dir, 1, max)
                        if localscoreIndex ~= beforeindex then
                            self:GetParent():playcommand("UpdateList")
                        end
                    end
                else
                    movePage(dir)
                end
            end
        end,
        MouseOverCommand = function(self)
            self:diffusealpha(buttonHoverAlpha)
        end,
        MouseOutCommand = function(self)
            self:diffusealpha(1)
        end,
    }

    -- this list defines the appearance of the lower lip buttons
    -- the text can be anything
    -- preferably the defaults are the first position
    -- the inner lists have to be length 2
    local choiceNames = {
        {"Show Online", "Show Local"},
        {"Top Scores", "All Scores"},
        {"Hide Invalid", "Show Invalid"},
        {"Current Rate", "All Rates"},
    }

    -- this list defines how each choice finds its default choiceName index
    -- basically, we dont always want to pick 1
    -- and we want the indices dependent on a lot of variables
    -- like maybe you want one thing to be 1 if true but something else 1 if false
    -- should return true for index 1 and false for index 2
    local choiceTextIndexGetters = {
        function()  -- local/online
            return isLocal
        end,

        function()  -- top/all scores
            return not allScores
        end,

        function() -- invalid score toggle
            -- true means invalid scores are hidden
            return not DLMAN:GetCCFilter()
        end,

        function() -- current/all rates
            return (DLMAN:GetCurrentRateFilter() and not isLocal)
        end,
    }

    -- what happens when you click each button corresponding to the above list
    -- actor tree:
    --      File, ExtraFrameInBetween, ChoiceParentFrame, self
    local choiceFunctions = {
        -- Local/Online
        function(self)
            self:GetParent():GetParent():playcommand("ToggleLocal")
        end,

        -- Top/All Scores
        function(self)
            self:GetParent():GetParent():playcommand("ToggleAllScores")
        end,

        -- Invalid Score Toggle
        function(self)
            self:GetParent():GetParent():playcommand("ToggleInvalid")
        end,

        -- Current/All Rate
        function(self)
            self:GetParent():GetParent():playcommand("ToggleCurrentRate")
        end,
    }

    -- which choices should only appear if in the online tab
    local choiceOnlineOnly = {
        false,   -- local/online
        true,   -- top/all scores
        true,   -- toggle invalid scores
        true,  -- current/all rates
    }

    local choiceTextSize = 0.7

    local function createChoices()
        local function createChoice(i)
            -- controls the position of the index in the choiceNames sublist
            local nameIndex = (choiceTextIndexGetters[i]() and 1 or 2)

            return UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "Button_" ..i,
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")

                    -- this position is the center of the text
                    -- divides the space into slots for the choices then places them half way into them
                    -- should work for any count of choices
                    -- and the maxwidth will make sure they stay nonoverlapping
                    self:x((actuals.Width / #choiceNames) * (i-1) + (actuals.Width / #choiceNames / 2))
                    txt:zoom(choiceTextSize)
                    txt:maxwidth(actuals.Width / #choiceNames / choiceTextSize - textzoomFudge)
                    txt:settext(choiceNames[i][nameIndex])
                    registerActorToColorConfigElement(txt, "main", "PrimaryText")
                    bg:zoomto(actuals.Width / #choiceNames, actuals.UpperLipHeight)

                    self.visibleOnlineCheck = function(self)
                        if choiceOnlineOnly[i] and isLocal or not DLMAN:IsLoggedIn() then
                            self:diffusealpha(0)
                            return false
                        else
                            self:diffusealpha(1)
                            return true
                        end
                    end
                    self.hoverAlphaCheck = function(self)
                        if isOver(bg) then
                            self:diffusealpha(buttonHoverAlpha)
                        else
                            self:diffusealpha(1)
                        end
                    end
                end,
                UpdateToggleStatusCommand = function(self)
                    -- for online only elements, hide if not online
                    if not self:visibleOnlineCheck() then return end

                    -- have to separate things because order of execution gets wacky
                    self:playcommand("UpdateText")
                end,
                UpdateTextCommand = function(self)
                    local txt = self:GetChild("Text")
                    nameIndex = (choiceTextIndexGetters[i]() and 1 or 2)
                    txt:settext(choiceNames[i][nameIndex])
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "OnMouseDown" then
                        if choiceFunctions[i] then
                            choiceFunctions[i](self)
                        end
                        self:playcommand("UpdateText")
                        self:hoverAlphaCheck()
                    end
                end,
                RolloverUpdateCommand = function(self, params)
                    if self:IsInvisible() then return end
                    self:hoverAlphaCheck()
                end
            }
        end
        local t = Def.ActorFrame {
            Name = "Choices",
            InitCommand = function(self)
                self:y(actuals.UpperLipHeight / 2)
            end,
            UpdateButtonsMessageCommand = function(self)
                allScores = not DLMAN:GetTopScoresOnlyFilter()
                self:playcommand("UpdateToggleStatus")
            end,
            UpdateLoginStatusCommand = function(self)
                self:playcommand("UpdateToggleStatus")
            end,
        }
        for i = 1, #choiceNames do
            t[#t+1] = createChoice(i)
        end
        return t
    end

    t[#t+1] = createChoices()

    return t
end

t[#t+1] = createList()

return t
