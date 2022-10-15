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

-- make text buttons slightly taller
local textButtonHeightFudgeScalarMultiplier = 1.6

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
    RightInfoRightAlignRightGap = 28 / 1920, -- right edge of frame to right edge of text

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

local translations = {
    LastScore = THEME:GetString("ScreenSelectMusic Scores", "LastScore"),
    You = THEME:GetString("ScreenSelectMusic Scores", "You"),
    NoName = THEME:GetString("ScreenSelectMusic Scores", "NoName"),
    ShowOffsetPlot = THEME:GetString("ScreenSelectMusic Scores", "ShowOffsetPlot"),
    ShowReplay = THEME:GetString("ScreenSelectMusic Scores", "ShowReplay"),
    ShowingJudge4Plot = THEME:GetString("ScreenSelectMusic Scores", "ShowingJudge4Plot"),
    ScoreBy = THEME:GetString("ScreenSelectMusic Scores", "ScoreBy"),
    HowToCloseOffsetPlot = THEME:GetString("ScreenSelectMusic Scores", "HowToCloseOffsetPlot"),
    UploadScore = THEME:GetString("ScreenSelectMusic Scores", "UploadScore"),
    UploadScorePack = THEME:GetString("ScreenSelectMusic Scores", "UploadScorePack"),
    ShowEvaluation = THEME:GetString("ScreenSelectMusic Scores", "ShowEvaluation"),
    JudgeDifficulty = THEME:GetString("ScreenSelectMusic Scores", "JudgeDifficulty"),
    JudgeJustice = THEME:GetString("ScreenSelectMusic Scores", "JudgeJustice"),
    ComboBreakers = THEME:GetString("ScreenSelectMusic Scores", "ComboBreakers"),
    MaxCombo = THEME:GetString("ScreenSelectMusic Scores", "MaxCombo"),
    DateAchieved = THEME:GetString("ScreenSelectMusic Scores", "DateAchieved"),
    ScoreModsUsed = THEME:GetString("ScreenSelectMusic Scores", "ScoreModsUsed"),
    NoLocalScoresRecorded = THEME:GetString("ScreenSelectMusic Scores", "NoLocalScoresRecorded"),
    ChartUnranked = THEME:GetString("ScreenSelectMusic Scores", "ChartUnranked"),
    FetchingScores = THEME:GetString("ScreenSelectMusic Scores", "FetchingScores"),
    NoOnlineScoresRecorded = THEME:GetString("ScreenSelectMusic Scores", "NoOnlineScoresRecorded"),
    ShowOnlineScores = THEME:GetString("ScreenSelectMusic Scores", "ShowOnlineScores"),
    ShowLocalScores = THEME:GetString("ScreenSelectMusic Scores", "ShowLocalScores"),
    ShowTopScores = THEME:GetString("ScreenSelectMusic Scores", "ShowTopScores"),
    ShowAllScores = THEME:GetString("ScreenSelectMusic Scores", "ShowAllScores"),
    HideInvalidScores = THEME:GetString("ScreenSelectMusic Scores", "HideInvalidScores"),
    ShowInvalidScores = THEME:GetString("ScreenSelectMusic Scores", "ShowInvalidScores"),
    CurrentRateOnly = THEME:GetString("ScreenSelectMusic Scores", "CurrentRateOnly"),
    AllRates = THEME:GetString("ScreenSelectMusic Scores", "AllRates"),
}

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

local gradeTextSize = 1.1
local clearTypeTextSize = 1.15
local detailTextSize = 0.75
local rateTextSize = 0.65
local onlinePlotTextSize = 0.8

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

            -- hide the online offset plot
            self:playcommand("HidePlot")

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
            self:playcommand("HidePlot")
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

        -- expecting self to be UIElements.TextButton or a Def.BitmapText
        -- the purpose is to color based on CC On
        local function diffuseScore(self, category, element)
            local txt = self
            if self.GetChild ~= nil then
                txt = self:GetChild("Text") or self
            end
            if score ~= nil and score:GetChordCohesion() then
                EGGMAN.gegagoogoo(txt, score:GetChartKey()):diffuse(COLORS:getColor("generalBox", "ChordCohesionOnScore"))
            else
                txt:stopeffect():diffuse(COLORS:getColor(category, element))
            end
        end

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
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "Rate",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:halign(1):valign(1)
                    bg:halign(1):valign(1)
                    self:xy(actuals.LeftCenteredAlignmentLineLeftGap - actuals.LeftCenteredAlignmentDistance, actuals.ItemHeight)
                    txt:zoom(rateTextSize)
                    txt:maxwidth(actuals.SSRRateWidth / rateTextSize - textzoomFudge)
                    bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                    diffuseScore(self, "main", "SecondaryText")
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    diffuseScore(self, "main", "SecondaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        local rt = score:GetMusicRate()
                        txt:settext(getRateString(rt))
                        bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                        diffuseScore(self, "main", "SecondaryText")
                    end
                end,
                RolloverUpdateCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "in" then
                        self:diffusealpha(buttonHoverAlpha)
                    else
                        self:diffusealpha(1)
                    end
                end,
                ClickCommand = function(self, params)
                    if params.update ~= "OnMouseDown" then return end
                    if self:IsInvisible() then return end
                    if params.event == "DeviceButton_left mouse button" then
                        setMusicRate(score:GetMusicRate())
                    end
                end,
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "PlayerName",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:halign(0):valign(0)
                    bg:halign(0):valign(0)
                    self:x(actuals.LeftCenteredAlignmentLineLeftGap + actuals.LeftCenteredAlignmentDistance)
                    txt:zoom(nameTextSize)
                    txt:maxwidth(actuals.NameJudgmentWidth / nameTextSize - textzoomFudge)
                    diffuseScore(self, "main", "PrimaryText")
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    diffuseScore(self, "main", "PrimaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        local n = score:GetName()
                        if n == "" then
                            n = translations["NoName"]
                        elseif n == "#P1#" then
                            if score:GetScoreKey() == mostRecentScore:GetScoreKey() then
                                n = translations["LastScore"]
                            else
                                n = translations["You"]
                            end
                        end
                        txt:settext(n)
                        bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                        diffuseScore(self, "main", "PrimaryText")
                    end
                end,
                RolloverUpdateCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "in" then
                        self:diffusealpha(buttonHoverAlpha)
                    else
                        self:diffusealpha(1)
                    end
                end,
                ClickCommand = function(self, params)
                    if params.update ~= "OnMouseDown" then return end
                    if self:IsInvisible() then return end
                    if params.event == "DeviceButton_left mouse button" then
                        if score ~= nil then
                            local url = "https://etternaonline.com/user/" .. score:GetDisplayName()
                            GAMESTATE:ApplyGameCommand("urlnoexit," .. url)
                        end
                    end
                end,
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "JudgmentsAndCombo",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:halign(0):valign(1)
                    bg:halign(0):valign(1)
                    self:xy(actuals.LeftCenteredAlignmentLineLeftGap + actuals.LeftCenteredAlignmentDistance, actuals.ItemHeight)
                    txt:zoom(judgmentTextSize)
                    txt:maxwidth(actuals.NameJudgmentWidth / judgmentTextSize - textzoomFudge)
                    diffuseScore(self, "main", "SecondaryText")
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    diffuseScore(self, "main", "SecondaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")

                        -- a HighScore method does exist to produce this but we want to be able to modify it from the theme
                        local jgMaStr = tostring(score:GetTapNoteScore("TapNoteScore_W1"))
                        local jgPStr = tostring(score:GetTapNoteScore("TapNoteScore_W2"))
                        local jgGrStr = tostring(score:GetTapNoteScore("TapNoteScore_W3"))
                        local jgGoStr = tostring(score:GetTapNoteScore("TapNoteScore_W4"))
                        local jgBStr = tostring(score:GetTapNoteScore("TapNoteScore_W5"))
                        local jgMiStr = tostring(score:GetTapNoteScore("TapNoteScore_Miss"))
                        local comboStr = tostring(score:GetMaxCombo())
                        txt:settextf("%s  |  %s  |  %s  |  %s  |  %s  |  %s  x%s", jgMaStr, jgPStr, jgGrStr, jgGoStr, jgBStr, jgMiStr, comboStr)
                        bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                        diffuseScore(self, "main", "SecondaryText")
                    end
                end,
                RolloverUpdateCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "in" then
                        self:diffusealpha(buttonHoverAlpha)
                    else
                        self:diffusealpha(1)
                    end
                end,
                ClickCommand = function(self, params)
                    if params.update ~= "OnMouseDown" then return end
                    if self:IsInvisible() then return end
                    if params.event == "DeviceButton_left mouse button" then
                        if score ~= nil then
                            local url = "https://etternaonline.com/score/view/" .. score:GetScoreid() .. score:GetUserid()
                            GAMESTATE:ApplyGameCommand("urlnoexit," .. url)
                        end
                    end
                end,
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "WifePercent",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    txt:halign(1):valign(0)
                    bg:halign(1):valign(0)
                    self:x(actuals.ItemWidth - actuals.RightInfoRightAlignRightGap)
                    txt:zoom(wifePercentTextSize)
                    -- 5/6 the space allowed vs the date
                    -- the icons take up the other half probably
                    txt:maxwidth((actuals.ItemWidth - actuals.RightInfoLeftAlignLeftGap - actuals.RightInfoRightAlignRightGap) / 6 * 4 / wifePercentTextSize)
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    self:playcommand("SetScore")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        local ws = score:GetWifeScore()
                        local wifeStr = checkWifeStr(ws)
                        local grade = GetGradeFromPercent(score:GetWifeScore())
                        txt:settext(wifeStr)
                        txt:diffuse(colorByGrade(grade))
                        bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                    end
                end,
                RolloverUpdateCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "in" then
                        if score ~= nil and score:HasReplayData() then
                            TOOLTIP:SetText(translations["ShowOffsetPlot"])
                            TOOLTIP:Show()
                            self:diffusealpha(buttonHoverAlpha)
                        end
                    else
                        self:diffusealpha(1)
                        TOOLTIP:Hide()
                    end
                end,
                ClickCommand = function(self, params)
                    if params.update ~= "OnMouseDown" then return end
                    if self:IsInvisible() then return end
                    if params.event == "DeviceButton_left mouse button" then
                        if score ~= nil then
                            if score:HasReplayData() then
                                ms.ok("Loading offsets...")
                                DLMAN:RequestOnlineScoreReplayData(
                                    score,
                                    function()
                                        ms.ok("Loaded offsets")
                                        self:GetParent():GetParent():playcommand("DisplayOnlineOffsets", {
                                            score = score,
                                            index = i
                                        })
                                    end
                                )
                            end
                        end
                    end
                end,
            },
            LoadFont("Common Normal") .. {
                Name = "DateTime",
                InitCommand = function(self)
                    self:halign(1):valign(1)
                    self:xy(actuals.ItemWidth - actuals.RightInfoRightAlignRightGap, actuals.ItemHeight)
                    self:zoom(dateTextSize)
                    self:maxwidth((actuals.ItemWidth - actuals.RightInfoLeftAlignLeftGap - actuals.RightInfoRightAlignRightGap) / dateTextSize - textzoomFudge)
                    diffuseScore(self, "main", "SecondaryText")
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    diffuseScore(self, "main", "SecondaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        self:settext(score:GetDate())
                        diffuseScore(self, "main", "SecondaryText")
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
                                if scr:GetMusicWheel():SelectSong(GAMESTATE:GetCurrentSong()) then
                                    local success = SCREENMAN:GetTopScreen():PlayReplay(score)
                                    if success then
                                        SCREENMAN:set_input_redirected(PLAYER_1, false)
                                    end
                                end
                            end
                        end
                    )
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    self:diffusealpha(buttonHoverAlpha)
                    TOOLTIP:SetText(translations["ShowReplay"])
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

                    if SCREENMAN:GetTopScreen():GetMusicWheel():SelectSong(GAMESTATE:GetCurrentSong()) then
                        local success = SCREENMAN:GetTopScreen():ShowEvalScreenForScore(score)
                        if success then
                            SCREENMAN:set_input_redirected(PLAYER_1, false)
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
                    if localrateIndex == index then
                        local inc = 0
                        if params.event == "DeviceButton_left mouse button" then
                            inc = 1
                        elseif params.event == "DeviceButton_right mouse button" then
                            inc = -1
                        end
                        local max = #localrtTable[localrates[localrateIndex]]
                        localscoreIndex = localscoreIndex + inc
                        if localscoreIndex > max then localscoreIndex = 1 end
                        if localscoreIndex < 1 then localscoreIndex = max end
                    else
                        localrateIndex = index
                        localscoreIndex = 1
                    end
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
    local extrasizing = 75 / 1920 * SCREEN_WIDTH -- extra size added to the plot for padding stuff
    local graphbgalpha = 0.9
    t[#t+1] = Def.ActorFrame {
        Name = "OnlineOffsetPlot",
        InitCommand = function(self)
            self:diffusealpha(0)
            self:z(-200)
        end,
        DisplayOnlineOffsetsCommand = function(self, params)
            -- very very very roughly the remaining width of the screen opposite of the scores tab, but lowered to 95%
            local zoombias = (1 + (actuals.MainGraphicWidth + extrasizing) / (SCREEN_WIDTH - actuals.ItemWidth)) * 0.95
            -- get the x pos based on the wheel position
            local finalxPos = getWheelPosition() and ((-actuals.MainGraphicWidth - extrasizing/2) * zoombias) or (actuals.ItemWidth + extrasizing)
            self:finishtweening()
            self:diffusealpha(0)
            self:x(actuals.MainGraphicWidth)
            self:y((actuals.ItemAllottedSpace / (itemCount - 1)) * (params.index-1) + actuals.ItemUpperSpacing)
            self:zoom(0)
            self:zoomy(0)
            self:z(-200)
            self:decelerate(0.3)
            self:diffusealpha(1)
            self:xy(finalxPos, 0)
            self:z(10)
            self:zoom(zoombias)
            self:zoomy(zoombias)
        end,
        HidePlotCommand = function(self)
            self:finishtweening()
            self:accelerate(0.3)
            self:xy(actuals.MainGraphicWidth, 0)
            self:zoom(0):zoomy(0)
            self:z(-200)
            self:diffusealpha(0)
        end,

        UIElements.QuadButton(1, 1) .. {
            Name = "BG",
            InitCommand = function(self)
                self:valign(0):halign(0)
                self:xy(-extrasizing / 2, -extrasizing)
                self:zoomto(actuals.MainGraphicWidth + extrasizing, actuals.OffsetPlotHeight + extrasizing * 2)
                self:diffusealpha(graphbgalpha)
                registerActorToColorConfigElement(self, "main", "SecondaryBackground")
            end,
            MouseDownCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(graphbgalpha)
                self:GetParent():playcommand("HidePlot")
            end,
            MouseOverCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(buttonHoverAlpha * graphbgalpha)
            end,
            MouseOutCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(graphbgalpha)
            end,
        },
        LoadFont("Common Normal") .. {
            Name = "TopText",
            InitCommand = function(self)
                self:xy(actuals.MainGraphicWidth / 2, -extrasizing / 2)
                self:zoom(onlinePlotTextSize)
                self:maxwidth(((actuals.MainGraphicWidth)) / onlinePlotTextSize)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            DisplayOnlineOffsetsCommand = function(self, params)
                local score = params.score
                if score == nil then self:settext("") return end
                local ws = score:GetWifeScore()
                local wifeStr = checkWifeStr(ws)
                -- keeping these plots j4 because i dont want to complicate logic for stuff
                --local judgeSetting = (PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or table.find(ms.JudgeScalers, notShit.round(score:GetJudgeScale(), 2))) or GetTimingDifficulty()
                self:settextf("%s  |  %s: %s  |  %s", translations["ShowingJudge4Plot"], translations["ScoreBy"], score:GetName(), wifeStr)
            end,
        },
        LoadFont("Common Normal") .. {
            Name = "BottomText",
            InitCommand = function(self)
                self:xy(actuals.MainGraphicWidth / 2, actuals.OffsetPlotHeight + extrasizing / 2)
                self:zoom(onlinePlotTextSize)
                self:maxwidth(((actuals.MainGraphicWidth)) / onlinePlotTextSize)
                self:settext(translations["HowToCloseOffsetPlot"])
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
        },
        LoadActorWithParams("../../offsetplot.lua", {sizing = {Width = actuals.MainGraphicWidth, Height = actuals.OffsetPlotHeight}, textsize = 0.43}) .. {
            DisplayOnlineOffsetsCommand = function(self, params)
                local score = params.score
                if score == nil then return end
                local steps = GAMESTATE:GetCurrentSteps()
                -- keeping these plots j4 because i dont want to complicate logic for stuff
                --local judgeSetting = (PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or table.find(ms.JudgeScalers, notShit.round(score:GetJudgeScale(), 2))) or GetTimingDifficulty()
                local judgeSetting = 4
                if steps ~= nil then
                    if score:HasReplayData() then
                        local offsets = score:GetOffsetVector()
                        -- for online offset vectors a 180 offset is a miss
                        for i, o in ipairs(offsets) do
                            if o >= 180 then
                                offsets[i] = 1000
                            end
                        end
                        local tracks = score:GetTrackVector()
                        local types = score:GetTapNoteTypeVector()
                        local noterows = score:GetNoteRowVector()
                        local holds = score:GetHoldNoteVector()
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
        },
    }

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
                self:z(2)
                self:diffusealpha(1)
            else
                self:finishtweening()
                self:smooth(localPageAnimationSeconds)
                self:z(-2)
                self:diffusealpha(0)
            end
        end,

        LoadFont("Common Large") .. {
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
                        if isOver(self) then
                            self:diffusealpha(buttonHoverAlpha)
                            if WHEELDATA:GetCurrentSort() == 1 then
                                TOOLTIP:SetText(translations["UploadScorePack"])
                            else
                                TOOLTIP:SetText(translations["UploadScore"])
                            end
                            TOOLTIP:Show()
                        end
                    else
                        self:diffusealpha(0)
                        if isOver(self) then
                            TOOLTIP:Hide()
                        end
                    end
                end
            end,
            MouseOverCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(buttonHoverAlpha)
                if WHEELDATA:GetCurrentSort() == 1 then
                    TOOLTIP:SetText(translations["UploadScorePack"])
                else
                    TOOLTIP:SetText(translations["UploadScore"])
                end
                TOOLTIP:Show()
            end,
            MouseOutCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(1)
                TOOLTIP:Hide()
            end,
            MouseDownCommand = function(self, params)
                if self:IsInvisible() then return end
                -- holding shift in the group sort causes a pack score upload
                if INPUTFILTER:IsShiftPressed() and WHEELDATA:GetCurrentSort() == 1 then
                    DLMAN:UploadScoresForPack(GAMESTATE:GetCurrentSong():GetGroupName())
                    ms.ok("Uploading all scores for pack: "..GAMESTATE:GetCurrentSong():GetGroupName())
                else
                    if localscore ~= nil and localscore:HasReplayData() then
                        -- important to note this is actually for uploading replays.
                        -- misnomer. .. bad functionality? oh no
                        DLMAN:SendReplayDataForOldScore(localscore:GetScoreKey())
                        local steps = GAMESTATE:GetCurrentSteps()
                        ms.ok(string.format("Uploading Score (chart %s key %s)", steps:GetChartKey(), localscore:GetScoreKey()))
                    end
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
                        if isOver(self) then
                            self:diffusealpha(buttonHoverAlpha)
                            TOOLTIP:SetText(translations["ShowEvaluation"])
                            TOOLTIP:Show()
                        end
                    else
                        self:diffusealpha(0)
                        if isOver(self) then
                            TOOLTIP:Hide()
                        end
                    end
                end
            end,
            MouseOverCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(buttonHoverAlpha)
                TOOLTIP:SetText(translations["ShowEvaluation"])
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
                    if SCREENMAN:GetTopScreen():GetMusicWheel():SelectSong(GAMESTATE:GetCurrentSong()) then
                        local success = SCREENMAN:GetTopScreen():ShowEvalScreenForScore(localscore)
                        if success then
                            SCREENMAN:set_input_redirected(PLAYER_1, false)
                        end
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
                        if isOver(self) then
                            self:diffusealpha(buttonHoverAlpha)
                            TOOLTIP:SetText(translations["ShowReplay"])
                            TOOLTIP:Show()
                        end
                    else
                        self:diffusealpha(0)
                        if isOver(self) then
                            TOOLTIP:Hide()
                        end
                    end
                end
            end,
            MouseOverCommand = function(self)
                if self:IsInvisible() then return end
                self:diffusealpha(buttonHoverAlpha)
                TOOLTIP:SetText(translations["ShowReplay"])
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
                    if SCREENMAN:GetTopScreen():GetMusicWheel():SelectSong(GAMESTATE:GetCurrentSong()) then
                        local success = SCREENMAN:GetTopScreen():PlayReplay(localscore)
                        if success then
                            SCREENMAN:set_input_redirected(PLAYER_1, false)
                        end
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
                    local js = judge ~= 9 and judge or translations["JudgeJustice"]
                    local perc = checkWifeStr(wife)
                    self:ClearAttributes()
                    self:diffuse(COLORS:getMainColor("PrimaryText"))
                    self:diffusealpha(1)
                    self:settextf("%s | %s (%s | %s %s)", ssrstr, perc, wv, translations["JudgeDifficulty"], js)
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
                        self:settextf("%s: %s", translations["ComboBreakers"], mc)
                    else
                        self:settextf("%s: -", translations["ComboBreakers"])
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
                    self:settextf("%s: %d", translations["MaxCombo"], mc)
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
                    self:settextf("%s: %s", translations["DateAchieved"], d)
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
                    self:settextf("%s: %s", translations["ScoreModsUsed"], m)
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
            elseif #scores == 0 and steps and fetchingScores[steps:GetChartKey()] == true then
                self:diffusealpha(1)
                self:settext(translations["FetchingScores"])
            elseif #scores == 0 and steps and fetchingScores[steps:GetChartKey()] == false then
                self:diffusealpha(1)
                self:settext(translations["NoOnlineScoresRecorded"])
            elseif isLocal and localscore == nil then
                self:diffusealpha(1)
                self:settext(translations["NoLocalScoresRecorded"])
            else
                self:diffusealpha(0)
                self:settext("")
            end
        end
    }

    -- the sizing on this button is slightly too big but hey it can never get too big
    t[#t+1] = UIElements.TextButton(1, 1, "Common Normal") .. {
        Name = "PageText",
        InitCommand = function(self)
            local txt = self:GetChild("Text")
            local bg = self:GetChild("BG")
            txt:halign(1):valign(0)
            bg:halign(1):valign(0)
            self:xy(actuals.Width - actuals.PageTextRightGap, actuals.PageTextUpperGap)
            txt:zoom(pageTextSize)
            txt:maxwidth(actuals.Width / pageTextSize - textzoomFudge)
            bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
            registerActorToColorConfigElement(txt, "main", "PrimaryText")
        end,
        UpdateListCommand = function(self)
            local txt = self:GetChild("Text")
            local bg = self:GetChild("BG")

            -- nil scores = no scores
            if scores == nil then
                txt:settext("0-0/0")
                bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                return
            end

            if isLocal then
                if localrtTable ~= nil and localrates ~= nil and #localrates > 0 then
                    local mx = #localrtTable[localrates[localrateIndex]]
                    txt:settextf("%d/%d", localscoreIndex, mx)
                else
                    txt:settext("")
                end
            else
                local lb = clamp((page-1) * (itemCount) + 1, 0, #scores)
                local ub = clamp(page * itemCount, 0, #scores)
                txt:settextf("%d-%d/%d", lb, ub, #scores)
            end
            bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
        end,
        ClickCommand = function(self, params)
            if params.update ~= "OnMouseDown" then return end
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
                        localscoreIndex = localscoreIndex + dir
                        if localscoreIndex > max then localscoreIndex = 1 end
                        if localscoreIndex < 1 then localscoreIndex = max end
                        if localscoreIndex ~= beforeindex then
                            self:GetParent():playcommand("UpdateList")
                        end
                    end
                else
                    movePage(dir)
                end
            end
        end,
        RolloverUpdateCommand = function(self, params)
            if params.update == "in" then
                self:diffusealpha(buttonHoverAlpha)
            else
                self:diffusealpha(1)
            end
        end,
    }

    -- this list defines the appearance of the lower lip buttons
    -- the text can be anything
    -- preferably the defaults are the first position
    -- the inner lists have to be length 2
    local choiceNames = {
        {translations["ShowOnlineScores"], translations["ShowLocalScores"]},
        {translations["ShowTopScores"], translations["ShowAllScores"]},
        {translations["HideInvalidScores"], translations["ShowInvalidScores"]},
        {translations["CurrentRateOnly"], translations["AllRates"]},
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
