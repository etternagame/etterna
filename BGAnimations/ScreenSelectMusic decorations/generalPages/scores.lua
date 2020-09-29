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
        self:playcommand("UpdateScores")
        self:playcommand("UpdateList")
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params and params.tab ~= nil then
            if params.tab == 2 then
                self:z(2)
                self:smooth(0.2)
                self:diffusealpha(1)
                focused = true
            else
                self:z(-1)
                self:smooth(0.2)
                self:diffusealpha(0)
                focused = false
            end
        end
    end,
    CurrentRateChangedMessageCommand = function(self)
        self:playcommand("UpdateScores")
        self:playcommand("UpdateList")
    end,
    ChangedStepsMessageCommand = function(self, params)
        self:playcommand("UpdateScores")
        self:playcommand("UpdateList")
    end
}

-- this can be nil if no scores exist in the profile
local mostRecentScore = SCOREMAN:GetMostRecentScore()

local ratios = {
    LowerLipHeight = 43 / 1080,
    ItemUpperSpacing = 68 / 1080, -- top of frame to top of text, to push all the items down

    PageTextRightGap = 33 / 1920, -- right of frame, right of text
    PageTextUpperGap = 505 / 1080, -- top of frame, top of text

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
}

local actuals = {
    LowerLipHeight = ratios.LowerLipHeight * SCREEN_HEIGHT,
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
    Name = "LowerLip",
    InitCommand = function(self)
        self:halign(0):valign(1)
        self:y(actuals.Height)
        self:zoomto(actuals.Width, actuals.LowerLipHeight)
        self:diffuse(color("#111111"))
        self:diffusealpha(0.6)
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

-- how many to display
local itemCount = 7
local scoreListAnimationSeconds = 0.05

local itemIndexSize = 0.95
local ssrTextSize = 0.9
local rateTextSize = 0.92
local nameTextSize = 0.9
local judgmentTextSize = 0.6
local wifePercentTextSize = 0.92
local dateTextSize = 0.6
local pageTextSize = 0.9
local textzoomFudge = 5

local buttonHoverAlpha = 0.6


-- functionally create the score list
-- this is basically a slimmed version of the Evaluation Scoreboard
function createList()
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

    local scores = {}

    -- to prevent bombing the server repeatedly with leaderboard requests
    -- chartkeys to booleans
    local alreadyRequestedLeaderboard = {}

    local t = Def.ActorFrame {
        Name = "ScoreListFrame",
        BeginCommand = function(self)
            scorelistframe = self
        end,
        UpdateScoresCommand = function(self)
            page = 1
            -- no steps, no scores.
            local steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
            if steps == nil then
                scores = {}
                return
            end

            if isLocal then
                local scoresByRate = getRateTable(getScoresByKey(PLAYER_1))
                if scoresByRate == nil then
                    scores = {}
                    return
                end

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
                    scores = scoresByRate[getRateDisplayString2(getCurRateString())] or {}
                end
            else
                -- operate with dlman scores
                -- ... everything here is determined by internal bools set by the toggle buttons
                scores = DLMAN:GetChartLeaderBoard(steps:GetChartKey(), dlmanScoreboardCountryFilter)

                -- this is the initial request for the leaderboard which should only end up running once
                -- and when it finishes, it loops back through this command
                -- on the second passthrough, the leaderboard is hopefully filled out
                if #scores == 0 then
                    if steps then
                        local kee = steps:GetChartKey()
                        if not alreadyRequestedLeaderboard[kee] then
                            alreadyRequestedLeaderboard[kee] = true
                            DLMAN:RequestChartLeaderBoardFromOnline(
                                steps:GetChartKey(),
                                function(leaderboard)
                                    -- disallow replacing the leaderboard if the request doesnt match the current steps
                                    local s = GAMESTATE:GetCurrentSteps(PLAYER_1)
                                    if s and s:GetChartKey() == kee then
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
            maxPage = math.ceil(#scores / itemCount)

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
                allRates = not allRates
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
                score = scores[scoreIndex]
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
                        self:diffuse(byMSD(ssr))
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
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local n = score:GetName()
                        if n == "" then
                            if isLocal then
                                n = "You"
                            end
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
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local wifeStr = string.format("%05.2f%%", notShit.floor(score:GetWifeScore() * 10000) / 100)
                        local grade = GetGradeFromPercent(score:GetWifeScore())
                        self:settext(wifeStr)
                        self:diffuse(getGradeColor(grade))
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
                    if isLocal then
                        SCREENMAN:GetTopScreen():PlayReplay(score)
                    else
                        local sng = GAMESTATE:GetCurrentSteps(PLAYER_1)
                        DLMAN:RequestOnlineScoreReplayData(
							score,
                            function()
                                local scr = SCREENMAN:GetTopScreen()
                                local sng2 = GAMESTATE:GetCurrentSteps(PLAYER_1)
                                if sng and sng2 and sng:GetChartKey() == sng2:GetChartKey() then
                                    SCREENMAN:GetTopScreen():PlayReplay(score)
                                end
							end
						)
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
            },
            UIElements.SpriteButton(1, 1, THEME:GetPathG("", "showEval")) .. {
                Name = "ShowEval",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.RightInfoLeftAlignLeftGap + actuals.TrophySize * 1.2)
                    self:zoomto(actuals.TrophySize, actuals.TrophySize)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        -- can't view online eval screens
                        if score:HasReplayData() and isLocal then
                            self:diffusealpha(1)
                        else
                            self:diffusealpha(0)
                        end
                    end
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end

                    SCREENMAN:GetTopScreen():ShowEvalScreenForScore(score)
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end

                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end

                    self:diffusealpha(1)
                end
            },
        }
    end

    for i = 1, itemCount do
        t[#t+1] = createItem(i)
    end

    t[#t+1] = Def.Quad {
        Name = "MouseWheelRegion",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:diffusealpha(0)
            self:zoomto(actuals.Width, actuals.Height)
        end,
        MouseScrollMessageCommand = function(self, params)
            if isOver(self) and focused then
                if params.direction == "Up" then
                    movePage(-1)
                else
                    movePage(1)
                end
            end
        end
    }

    t[#t+1] = LoadFont("Common Normal") .. {
        Name = "PageText",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:xy(actuals.Width - actuals.PageTextRightGap, actuals.PageTextUpperGap)
            self:zoom(pageTextSize)
            self:maxwidth(actuals.Width / pageTextSize - textzoomFudge)
        end,
        UpdateListCommand = function(self)
            local lb = (page-1) * (itemCount) + 1
            if lb > #scores then
                lb = #scores
            end
            local ub  = page * itemCount
            if ub > #scores then
                ub = #scores
            end
            self:settextf("%d-%d/%d", lb, ub, #scores)
        end
    }

    -- this list defines the appearance of the lower lip buttons
    -- the text can be anything
    -- preferably the defaults are the first position
    -- the inner lists have to be length 2
    local choiceNames = {
        {"Local", "Online"},
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
            return (not allRates and isLocal) or (DLMAN:GetCurrentRateFilter() and not isLocal)
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

    -- which choices should only appear if logged in
    local choiceOnlineOnly = {
        true,   -- local/online
        true,   -- top/all scores
        true,   -- toggle invalid scores
        false,  -- current/all rates
    }

    local choiceTextSize = 0.8

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
                    bg:zoomto(actuals.Width / #choiceNames, actuals.LowerLipHeight)
                    if choiceOnlineOnly[i] and not DLMAN:IsLoggedIn() then
                        self:diffusealpha(0)
                    end
                end,
                UpdateToggleStatusCommand = function(self)
                    -- for online only elements, hide if not online
                    if choiceOnlineOnly[i] and not DLMAN:IsLoggedIn() then
                        self:diffusealpha(0)
                        return
                    end
                    -- update things
                    --if choiceFunctions[i] then
                    --    choiceFunctions[i](self)
                    --end

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
            }
        end
        local t = Def.ActorFrame {
            Name = "Choices",
            InitCommand = function(self)
                self:y(actuals.Height - actuals.LowerLipHeight / 2)
            end,
            UpdateButtonsMessageCommand = function(self)
                allScores = not DLMAN:GetTopScoresOnlyFilter()
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