local t = Def.ActorFrame {
    Name = "ScoresPageFile",
    WheelSettledMessageCommand = function(self, params)
        -- cascade visual update to everything
        self:playcommand("Set", {song = params.song, group = params.group, hovered = params.hovered, steps = params.steps})
    end
}

local ratios = {
    LowerLipHeight = 43 / 1080,

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

    TrophySize = 21 / 1920, -- height and width of the icon
    PlaySize = 19 / 1920,
}

local actuals = {
    LowerLipHeight = ratios.LowerLipHeight * SCREEN_HEIGHT,
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

-- functionally create the score list
-- this is basically a slimmed version of the Evaluation Scoreboard
function createList()
    local page = 1
    local maxPage = 1

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

        MESSAGEMAN:Broadcast("MovedPage")
    end

    -- yes, we do have a country filter
    -- we don't tell anyone
    -- the Global country just shows everyone
    local dlmanScoreboardCountryFilter = "Global"

    local scores = {}

    -- to prevent bombing the server repeatedly with leaderboard requests
    local alreadyRequestedLeaderboard = false

    local t = Def.ActorFrame {
        Name = "ScoreListFrame",
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
                    scores = scoresByRate[getCurrentRateValue()] or {}
                end
            else
                local steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
                -- operate with dlman scores
                -- ... everything here is determined by internal bools set by the toggle buttons
                scores = DLMAN:GetChartLeaderBoard(steps:GetChartKey(), dlmanScoreboardCountryFilter)

                -- this is the initial request for the leaderboard which should only end up running once
                -- and when it finishes, it loops back through this command
                -- on the second passthrough, the leaderboard is hopefully filled out
                if #scores == 0 then
                    if steps then
                        if not alreadyRequestedLeaderboard then
                            alreadyRequestedLeaderboard = true
                            DLMAN:RequestChartLeaderBoardFromOnline(
                                steps:GetChartKey(),
                                function(leaderboard)
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
            maxPage = math.ceil(#scores / itemCount)

            for i = 1, itemCount do
                local index = (page - 1) * itemCount + i
                self:GetChild("ScoreItem_"..i):playcommand("SetScore", {scoreIndex = index})
            end
            self:GetParent():playcommand("UpdateButtons")
        end,
        MovedPageMessageCommand = function(self)
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


    function createItem(i)

    end

    -- this list defines the appearance of the lower lip buttons
    -- the text can be anything
    -- preferably the defaults are the first position
    -- the inner lists have to be length 2
    local choiceNames = {
        {"Local", "Online"},
        {"Top Scores", "All Scores"},
        {"", ""},
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

        function() return true end, -- empty space

        function() -- invalid score toggle
            -- true means Invalid csores are hidden
            return not DLMAN:GetCCFilter()
        end,

        function() -- current/all rates
            return (allRates and isLocal) or (not DLMAN:GetCurrentRateFilter() and not isLocal)
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

        -- empty space
        nil,

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
        false,  -- empty space
        true,   -- toggle invalid scores
        false,  -- current/all rates
    }

    local choiceTextSize = 0.8
    local textzoomFudge = 5

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
                    if choiceOnlineOnly[i] and not DLMAN:IsLoggedIn() then
                        self:diffusealpha(0)
                        return
                    end
                    -- update things
                    if choiceFunctions[i] then
                        choiceFunctions[i](self)
                    end

                    -- have to separate things because order of execution gets wacky
                    self:playcommand("UpdateText")
                end,
                UpdateTextCommand = function(self)
                    local txt = self:GetChild("Text")
                    nameIndex = (choiceTextIndexGetters[i]() and 1 or 2)
                    txt:settext(choiceNames[i][nameIndex])
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