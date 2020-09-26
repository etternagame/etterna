local focused = false
local t = Def.ActorFrame {
    Name = "ProfilePageFile",
    InitCommand = function(self)
        -- hide all general box tabs on startup
        self:diffusealpha(0)
    end,
    BeginCommand = function(self)
        SCOREMAN:SortSSRsForGame("Stream")
        self:playcommand("UpdateScores")
        self:playcommand("UpdateList")
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params and params.tab ~= nil then
            if params.tab == 6 then
                self:smooth(0.2)
                self:diffusealpha(1)
                focused = true
            else
                self:smooth(0.2)
                self:diffusealpha(0)
                focused = false
            end
        end
    end
}

local ratios = {
    BottomLipHeight = 43 / 1080, -- frame edge to lip edge

    MainIndicatorLeftGap = 12 / 1920, -- left edge to left edge of text, this is the online/local thing
    MainIndicatorUpperGap = 52 / 1080,
    ItemIndexMargin = 29 / 1920, -- left edge to center of the indices
    ItemSSRRightAlignLeftGap = 129 / 1920, -- left edge to right edge of SSR
    ItemSSRWidth = 70 / 1920, -- rough estimation
    ItemSongNameLeftGap = 146 / 1920, -- left edge to song name
    ItemSongNameWidth = 375 / 1920, -- rough estimation
    ItemRateRightAlignLeftGap = 569 / 1920, -- left edge to right edge of rate
    ItemRateWidth = 48 / 1920, -- rough estimaion, lines up with the song name width
    ItemPercentRightAlignLeftGap = 691 / 1920, -- left edge to right edge of percent
    ItemPercentWidth = 115 / 1920, -- slightly less than the distance to the rate
    ItemDiffRightAlignLeftGap = 743 / 1920, -- left edge to right edge of diff
    ItemDiffWidth = 50 / 1920, -- slightly less than the distance to the percent

    ItemListUpperGap = 87 / 1080, -- top of frame to top of first item
    ItemSpacing = 29 / 1080, -- from top of item to top of next item
    ItemAllottedSpace = 441 / 1080, -- from first item to top of bottom lip
}

local actuals = {
    BottomLipHeight = ratios.BottomLipHeight * SCREEN_HEIGHT,
    MainIndicatorLeftGap = ratios.MainIndicatorLeftGap * SCREEN_WIDTH,
    MainIndicatorUpperGap = ratios.MainIndicatorUpperGap * SCREEN_HEIGHT,
    ItemIndexMargin = ratios.ItemIndexMargin * SCREEN_WIDTH,
    ItemSSRRightAlignLeftGap = ratios.ItemSSRRightAlignLeftGap * SCREEN_WIDTH,
    ItemSSRWidth = ratios.ItemSSRWidth * SCREEN_WIDTH,
    ItemSongNameLeftGap = ratios.ItemSongNameLeftGap * SCREEN_WIDTH,
    ItemSongNameWidth = ratios.ItemSongNameWidth * SCREEN_WIDTH,
    ItemRateRightAlignLeftGap = ratios.ItemRateRightAlignLeftGap * SCREEN_WIDTH,
    ItemRateWidth = ratios.ItemRateWidth * SCREEN_WIDTH,
    ItemPercentRightAlignLeftGap = ratios.ItemPercentRightAlignLeftGap * SCREEN_WIDTH,
    ItemPercentWidth = ratios.ItemPercentWidth * SCREEN_WIDTH,
    ItemDiffRightAlignLeftGap = ratios.ItemDiffRightAlignLeftGap * SCREEN_WIDTH,
    ItemDiffWidth = ratios.ItemDiffWidth * SCREEN_WIDTH,
    ItemListUpperGap = ratios.ItemListUpperGap * SCREEN_HEIGHT,
    ItemSpacing = ratios.ItemSpacing * SCREEN_HEIGHT,
    ItemAllottedSpace = ratios.ItemAllottedSpace * SCREEN_HEIGHT,
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

-- the page names in the order they go
-- it happens to literally just be the skillsets including Overall
local choiceNames = ms.SkillSets

local itemCount = 15
local scoreListAnimationSeconds = 0.05
-- we can potentially display every score on the profile
-- there isn't a good reason to do that or not to do that
-- so let's just default to about twice as many as we are used to
local upperBoundOfScoreCount = itemCount * 40

local pageTextSize = 1
local itemIndexSize = 1
local ssrTextSize = 1
local nameTextSize = 1
local rateTextSize = 1
local percentTextSize = 1
local diffTextSize = 1

local choiceTextSize = 0.8
local buttonHoverAlpha = 0.6
local buttonActiveStrokeColor = color("0.85,0.85,0.85,0.8")
local textzoomFudge = 5

local function createChoices()
    local selectedIndex = 1
    local function createChoice(i)
        return UIElements.TextButton(1, 1, "Common Normal") .. {
            Name = "ButtonTab_"..choiceNames[i],
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
                txt:settext(choiceNames[i])
                bg:zoomto(actuals.Width / #choiceNames, actuals.UpperLipHeight)
            end,
            UpdateSelectedIndexCommand = function(self)
                local txt = self:GetChild("Text")
                if selectedIndex == i then
                    txt:strokecolor(buttonActiveStrokeColor)
                else
                    txt:strokecolor(color("0,0,0,0"))
                end
            end,
            ClickCommand = function(self, params)
                if self:IsInvisible() then return end
                if params.update == "OnMouseDown" then
                    selectedIndex = i
                    if i == 1 then
                        -- overall does something different
                    else
                        -- sort by skillset
                    end
                    self:GetParent():playcommand("UpdateSelectedIndex")
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
            self:y(actuals.Height - actuals.BottomLipHeight / 2)
            self:playcommand("UpdateSelectedIndex")
        end
    }
    for i = 1, #choiceNames do
        t[#t+1] = createChoice(i)
    end
    return t
end

-- functionally create the score list
-- this is basically a slimmed version of the Scores Tab
local function createList()
    local page = 1
    local maxPage = 1
    local scorelistframe = nil
    local isLocal = true
    local chosenSkillset = "Stream"

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

    local scores = {}

    local t = Def.ActorFrame {
        Name = "ScoreListFrame",
        BeginCommand = function(self)
            scorelistframe = self
        end,
        UpdateScoresCommand = function(self)
            page = 1

            if isLocal then
                scores = {}
                local sortedScore = SCOREMAN:GetTopSSRHighScoreForGame(1, chosenSkillset)
                while sortedScore ~= nil and #scores < upperBoundOfScoreCount do
                    scores[#scores+1] = sortedScore
                    sortedScore = SCOREMAN:GetTopSSRHighScoreForGame(#scores + 1, chosenSkillset)
                end
            else
                -- operate with dlman scores
                scores = {}

            end
        end,
        UpdateListCommand = function(self)
            maxPage = math.ceil(#scores / itemCount)

            for i = 1, itemCount do
                local index = (page - 1) * itemCount + i
                self:GetChild("ScoreItem_"..i):playcommand("SetScore", {scoreIndex = index})
            end
        end,
        MovedPageCommand = function(self)
            self:playcommand("UpdateList")
        end
    }

    local function createItem(i)
        local score = nil
        local scoreIndex = i

        return Def.ActorFrame {
            Name = "ScoreItem_"..i,
            InitCommand = function(self)
                self:y((actuals.ItemAllottedSpace / (itemCount)) * (i-1) + actuals.ItemListUpperGap)
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
                    self:valign(0)
                    self:x(actuals.ItemIndexMargin)
                    self:zoom(itemIndexSize)
                    self:maxwidth((actuals.ItemSSRRightAlignLeftGap - actuals.ItemSSRWidth) / itemIndexSize - textzoomFudge)
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
                    self:x(actuals.ItemSSRRightAlignLeftGap)
                    self:zoom(ssrTextSize)
                    self:maxwidth(actuals.ItemSSRWidth / ssrTextSize - textzoomFudge)
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
                Name = "Name",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(actuals.ItemSongNameLeftGap)
                    self:zoom(nameTextSize)
                    self:maxwidth(actuals.ItemSongNameWidth / nameTextSize - textzoomFudge)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local song = SONGMAN:GetSongByChartKey(score:GetChartKey())
                        if song then
                            self:settext(song:GetDisplayMainTitle())
                        else
                            self:settext("")
                        end
                    end
                end,
                DisplayLanguageChangedMessageCommand = function(self)
                    self:playcommand("SetScore")
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Rate",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.ItemRateRightAlignLeftGap)
                    self:zoom(rateTextSize)
                    self:maxwidth(actuals.ItemRateWidth / rateTextSize - textzoomFudge)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local ratestring = string.format("%.2f", score:GetMusicRate()):gsub("%.?0+$", "") .. "x"
                        self:settext(ratestring)
                    end
                end
            },
            LoadFont("Common Normal") .. { 
                Name = "WifePercent",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.ItemPercentRightAlignLeftGap)
                    self:zoom(percentTextSize)
                    self:maxwidth(actuals.ItemPercentWidth / percentTextSize - textzoomFudge)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        self:settextf("%5.2f%%", score:GetWifeScore() * 100)
                    end
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Difficulty",
                InitCommand = function(self)
                    self:halign(1):valign(0)
                    self:x(actuals.ItemDiffRightAlignLeftGap)
                    self:zoom(diffTextSize)
                    self:maxwidth(actuals.ItemDiffWidth / diffTextSize - textzoomFudge)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local steps = SONGMAN:GetStepsByChartKey(score:GetChartKey())
                        if steps then
                            self:settext(getShortDifficulty(steps:GetDifficulty()))
                        else
                            self:settext("")
                        end
                    end
                end
            }
            
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
            self:xy(actuals.Width, actuals.MainIndicatorUpperGap)
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

    return t
end

t[#t+1] = Def.Quad {
    Name = "BottomLip",
    InitCommand = function(self)
        self:y(actuals.Height)
        self:halign(0):valign(1)
        self:zoomto(actuals.Width, actuals.BottomLipHeight)
        self:diffuse(color("#111111"))
        self:diffusealpha(0.6)
    end
}

t[#t+1] = createChoices()

t[#t+1] = createList()

return t