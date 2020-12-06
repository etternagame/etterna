local focused = false
local t = Def.ActorFrame {
    Name = "ProfilePageFile",
    InitCommand = function(self)
        -- hide all general box tabs on startup
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params and params.tab ~= nil then
            if params.tab == 6 then
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
    end
}

local ratios = {
    UpperLipHeight = 43 / 1080, -- frame edge to lip edge

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
    ItemAllottedSpace = 470 / 1080, -- from first item to top of bottom lip

    -- Overall tab is like a subtab, partially separate measurements
    InfoUpperMargin = 87 / 1080, -- this appears to be the gap for everything from top of frame to top of everything
    AvatarLeftGap = 19 / 1920, -- left edge to avatar
    AvatarSize = 109 / 1080, -- height based for squareness
    -- positions are relative to the avatar to work with all sizes (complexification)
    NameInfoLeftMargin = 8 / 1920, -- avatar to left edge of text
    NameInfoLargeLineSpacing = 32 / 1080, -- top of top line to top of next line
    NameInfoSmallerUpperGap = 150 / 1080, -- top of frame to top of third line of text
    NameInfoSmallerLineSpacing = 26 / 1080, -- top of top line to top of next line
    RightTextLeftGap = 414 / 1920, -- left frame edge to left edge of rightmost text
    RightTextSlightOffset = 1 / 1920, -- really?
    RightSmallTextUpperGap = 119 / 1080, -- top of frame to top of stream skillset
    RightSmallTextAllottedSpace = 410 / 1080, -- the reference doesnt show overall so we have to position based on this to be dynamic
}

local actuals = {
    UpperLipHeight = ratios.UpperLipHeight * SCREEN_HEIGHT,
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
    InfoUpperMargin = ratios.InfoUpperMargin * SCREEN_HEIGHT,
    AvatarLeftGap = ratios.AvatarLeftGap * SCREEN_WIDTH,
    AvatarSize = ratios.AvatarSize * SCREEN_HEIGHT,
    NameInfoLeftMargin = ratios.NameInfoLeftMargin * SCREEN_WIDTH,
    NameInfoLargeLineSpacing = ratios.NameInfoLargeLineSpacing * SCREEN_HEIGHT,
    NameInfoSmallerUpperGap = ratios.NameInfoSmallerUpperGap * SCREEN_HEIGHT,
    NameInfoSmallerLineSpacing = ratios.NameInfoSmallerLineSpacing * SCREEN_HEIGHT,
    RightTextLeftGap = ratios.RightTextLeftGap * SCREEN_WIDTH,
    RightTextSlightOffset = ratios.RightTextSlightOffset * SCREEN_WIDTH,
    RightSmallTextUpperGap = ratios.RightSmallTextUpperGap * SCREEN_HEIGHT,
    RightSmallTextAllottedSpace = ratios.RightSmallTextAllottedSpace * SCREEN_HEIGHT,
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
local scoreListAnimationSeconds = 0.03
-- we can potentially display every score on the profile
-- there isn't a good reason to do that or not to do that
-- so let's just default to about twice as many as we are used to
local upperBoundOfScoreCount = itemCount * 40

local indicatorTextSize = 0.7
local pageTextSize = 0.7
local itemIndexSize = 0.9
local ssrTextSize = 0.9
local nameTextSize = 0.9
local rateTextSize = 0.9
local percentTextSize = 0.9
local diffTextSize = 0.9

-- overall page text sizes
local largelineTextSize = 0.8
local smalllineTextSize = 0.6
local skillsetTextSize = 0.7

local choiceTextSize = 0.8
local buttonHoverAlpha = 0.6
local buttonActiveStrokeColor = color("0.85,0.85,0.85,0.8")
local textzoomFudge = 5
-- movement on the Z axis to fix button related issues
-- this value doesnt change anything as long as it is smaller than 1
-- basically z movement is necessary to tell which buttons are on top even if they are invisible
-- invisible buttons on top of other buttons will make the bottom buttons inaccessible
local overallPageZBump = 0.1
-- upward vertical distance to move the button bg to fix positional related issues with the button bg quad relative to the text
-- if we used a sane baseline for text then this wouldnt be an issue
-- ... too late to fix this.
local scoreitembgbump = 3

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
                bg:zoomto(actuals.Width / #choiceNames, actuals.LowerLipHeight)
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
                    -- change chosen skillset and regrab all scores
                    -- but know that overall does something different
                    self:GetParent():GetParent():playcommand("UpdateScores", {index = i})
                    self:GetParent():playcommand("UpdateSelectedIndex")
                    self:GetParent():GetParent():playcommand("UpdateList")
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
            self:y(actuals.UpperLipHeight / 2)
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
    local chosenSkillset = ms.SkillSets[1] -- Overall default

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
        UpdateScoresCommand = function(self, params)
            page = 1

            -- if an index is given, we are switching the chosenSkillset
            if params ~= nil and params.index ~= nil and ms.SkillSets[params.index] ~= nil then
                chosenSkillset = ms.SkillSets[params.index]
                if chosenSkillset ~= "Overall" then
                    -- sort all top scores by skillset
                    SCOREMAN:SortSSRsForGame(ms.SkillSets[params.index])
                else
                    -- for overall, we dont want to sort
                    -- we are showing a special page instead
                    -- UpdateList (UpdateListCommand) should handle that
                    scores = {}
                    return
                end
            end

            if isLocal then
                -- repopulate the scores list with the internally sorted score list
                scores = {}
                local sortedScore = SCOREMAN:GetTopSSRHighScoreForGame(1, chosenSkillset)
                while sortedScore ~= nil and #scores < upperBoundOfScoreCount do
                    scores[#scores+1] = sortedScore
                    sortedScore = SCOREMAN:GetTopSSRHighScoreForGame(#scores + 1, chosenSkillset)
                end
            else
                -- operate with dlman scores
                scores = {}
                local sortedScore = DLMAN:GetTopSkillsetScore(1, chosenSkillset)
                while sortedScore ~= nil and #scores < upperBoundOfScoreCount do
                    scores[#scores+1] = sortedScore
                    sortedScore = DLMAN:GetTopSkillsetScore(#scores + 1, chosenSkillset)
                end
            end
        end,
        UpdateListCommand = function(self)
            maxPage = math.ceil(#scores / itemCount)

            for i = 1, itemCount do
                local index = (page - 1) * itemCount + i
                self:GetChild("ScoreItem_"..i):playcommand("SetScore", {scoreIndex = index})
            end
            if chosenSkillset == "Overall" then
                self:GetChild("OverallPage"):smooth(0.2):diffusealpha(1):z(overallPageZBump)
                self:GetChild("OnlineOfflineToggle"):diffusealpha(0)
                self:GetChild("PageText"):diffusealpha(0)
            else
                self:GetChild("OverallPage"):smooth(0.2):diffusealpha(0):z(-overallPageZBump)
                if DLMAN:IsLoggedIn() then
                    self:GetChild("OnlineOfflineToggle"):smooth(0.2):diffusealpha(1)
                end
                self:GetChild("PageText"):smooth(0.2):diffusealpha(1)
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
                        local ssr = 0
                        if isLocal then
                            ssr = score:GetSkillsetSSR(chosenSkillset)
                        else
                            ssr = score.ssr
                        end
                        self:settextf("%05.2f", ssr)
                        self:diffuse(byMSD(ssr))
                    end
                end
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    self:x(actuals.ItemSongNameLeftGap)
                    
                    txt:halign(0):valign(0)
                    bg:halign(0):valign(0)
                    -- this upwards bump fixes font related positioning
                    -- the font has a baseline which pushes it downward by some bit
                    -- this corrects the bg so that the hover is not wrong as a result
                    bg:y(-scoreitembgbump)

                    txt:zoom(nameTextSize)
                    txt:maxwidth(actuals.ItemSongNameWidth / nameTextSize - textzoomFudge)
                    bg:zoomy(actuals.ItemAllottedSpace / itemCount)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")

                        if isLocal then
                            local song = SONGMAN:GetSongByChartKey(score:GetChartKey())
                            if song then
                                txt:settext(song:GetDisplayMainTitle())
                            else
                                txt:settext("")
                            end
                        else
                            txt:settext(score.songName)
                        end
                        bg:zoomx(txt:GetZoomedWidth())

                        -- if mouse is currently hovering
                        if isOver(bg) then
                            self:diffusealpha(buttonHoverAlpha)
                        else
                            self:diffusealpha(1)
                        end
                    end
                end,
                DisplayLanguageChangedMessageCommand = function(self)
                    self:playcommand("SetScore")
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "OnMouseDown" then
                        -- find song on click (even if filtered)
                        local w = SCREENMAN:GetTopScreen():GetChild("WheelFile")
                        if w ~= nil then
                            local ck
                            if isLocal then
                                ck =  score:GetChartKey()
                            else
                                ck = score.chartkey
                            end
                            if ck ~= nil then
                                w:playcommand("FindSong", {chartkey = ck})
                            end
                        end
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
                        local rate = 0
                        if isLocal then
                            rate = score:GetMusicRate()
                        else
                            rate = score.rate
                        end
                        local ratestring = string.format("%.2f", rate):gsub("%.?0+$", "") .. "x"
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
                        if isLocal then
                            self:settextf("%5.2f%%", score:GetWifeScore() * 100)
                            self:diffuse(byGrade(score:GetWifeGrade()))
                        else
                            self:settextf("%5.2f%%", score.wife * 100)
                            self:diffuse(byGrade(score.grade))
                        end
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
                        if isLocal then
                            local steps = SONGMAN:GetStepsByChartKey(score:GetChartKey())
                            if steps then
                                self:settext(getShortDifficulty(steps:GetDifficulty()))
                                self:diffuse(byDifficulty(steps:GetDifficulty()))
                            else
                                self:settext("")
                            end
                        else
                            local diff = score.difficulty
                            self:settext(getShortDifficulty(diff))
                            self:diffuse(byDifficulty(diff))
                        end
                    end
                end
            }
            
        }
    end

    -- this generates the overall page
    -- it is within the createList function because it is really part of the skillset list
    -- but overall just needs its own special behavior
    local function overallPage()
        local textmarginX = actuals.AvatarLeftGap + actuals.AvatarSize + actuals.NameInfoLeftMargin

        local profile = GetPlayerOrMachineProfile(PLAYER_1)
        local pname = profile:GetDisplayName()

        -- list of skillsets mapped to number of plays
        -- sorted immediately after being emplaced here
        local playsbyskillset = SCOREMAN:GetPlaycountPerSkillset(profile)
        local playsbyskillsetSorted = {}

        -- sort and replace the skillsetplays table
        local function sortPlaysBySkillset()
            local t = {}
            for i,v in ipairs(playsbyskillset) do
                t[i] = {v, ms.SkillSets[i]}
            end

            table.sort(
                t,
                function(a,b)
                    return a[1] > b[1]
                end
            )
            return t
        end
        playsbyskillsetSorted = sortPlaysBySkillset()

        -- returns the skillset and count of the skillset in the desired position
        local function getSkillsetPlaysByPosition(position)
            position = clamp(position, 1, #ms.SkillSets)
            return playsbyskillsetSorted[position][1], playsbyskillsetSorted[position][2]
        end

        -- a list of functions which basically determine the behavior of each small text line in the overall page
        -- self refers to the text actor
        -- the amount of functions listed here determines how many small lines appear
        local smallTextFunctions = {
            -- playcount
            function(self)
                local pcount = SCOREMAN:GetTotalNumberOfScores()
                self:settextf("%d plays", pcount)
            end,
            -- arrow count
            function(self)
                local parrows = profile:GetTotalTapsAndHolds()
                -- shorten if over 1 million since the number is kind of long
                local nstr = shortenIfOver1Mil(parrows)
                self:settextf("%s arrows smashed", nstr)
            end,
            -- songs loaded
            function(self)
                local count = SONGMAN:GetNumSongs()
                self:settextf("%d songs loaded", count)
            end,
            -- song packs installed (and filtered)
            function(self)
                local packcount = SONGMAN:GetNumSongGroups()
                local groupcount = #WHEELDATA:GetAllGroups()
                if packcount ~= groupcount then
                    self:settextf("%d packs loaded (%d visible)", packcount, groupcount)
                else
                    self:settextf("%d packs loaded", packcount)
                end
            end,
            -- playtime (overall, not gameplay)
            function(self)
                local ptime = profile:GetTotalSessionSeconds()
                self:settextf("%s playtime", SecondsToHHMMSS(ptime))
            end,
            -- current judge
            function(self)
                local judge = GetTimingDifficulty()
                self:settextf("Judge: %d", judge)
            end,
            -- a line break
            function(self)
            end,
            -- top skillset plays header
            function(self)
                self:settext("Top 3 Played Skillsets")
            end,
            -- top played skillset
            function(self)
                local count, name = getSkillsetPlaysByPosition(1)
                self:settextf("  %s (%d)", name, count)
            end,
            -- 2nd top played skillset
            function(self)
                local count, name = getSkillsetPlaysByPosition(2)
                self:settextf("  %s (%d)", name, count)
            end,
            -- 3rd top played skillset
            function(self)
                local count, name = getSkillsetPlaysByPosition(3)
                self:settextf("  %s (%d)", name, count)
            end,
        }

        -- these functions run immediately after init if they exist
        -- they should have access to the top screen
        local smallTextInitFunctions = {
            -- [index] = function(self) end,
        }

        -- these functions run as you mouse over the text
        -- each entry is 2 functions accepting no params, only self
        -- first function is onHover (mouse on)
        -- second function is onUnHover (mouse out)
        -- invisible checks are not necessary
        -- the index corresponds to the function index implicitly defined in the tables above
        local smallTextHoverFunctions = {
            -- [index] = { function(self) end, function(self) end }
            [2] = {
                function(self)
                    -- for these functions i could just be changing the text itself
                    -- but to keep it consistent with how i did the other thing
                    -- im going to use tooltips here
                    TOOLTIP:SetText(profile:GetTotalTapsAndHolds())
                    TOOLTIP:Show()
                end,
                function(self)
                    TOOLTIP:Hide()
                end,
            }
        }

        local function leftTextSmall(i)
            local cHover = nil
            local cUnHover = nil
            -- set the hover commands if they exist
            if smallTextHoverFunctions[i] ~= nil then
                cHover = function(self, params)
                    if self:IsInvisible() then return end
                    smallTextHoverFunctions[i][1](self)
                end
                cUnHover = function(self, params)
                    if self:IsInvisible() then return end
                    smallTextHoverFunctions[i][2](self)
                end
            end

            return UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "LeftSmallText_"..i,
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(textmarginX, actuals.NameInfoSmallerUpperGap + (i-1) * actuals.NameInfoSmallerLineSpacing)
                    self:zoom(smalllineTextSize)
                    self:maxwidth((actuals.RightTextLeftGap - actuals.AvatarLeftGap - actuals.AvatarSize - actuals.NameInfoLeftMargin) / smalllineTextSize - textzoomFudge)
                    self:settext("")
                    self:queuecommand("Startup")
                end,
                SetCommand = smallTextFunctions[i],
                StartupCommand = smallTextInitFunctions[i],
                MouseOverCommand = cHover,
                MouseOutCommand = cUnHover,
            }
        end

        local function rightTextSmall(i)
            local skillsetIndex = math.ceil(i/2)
            local skillset = ms.SkillSets[skillsetIndex]

            return LoadFont("Common Normal") .. {
                Name = "RightText_"..i,
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(actuals.RightTextLeftGap)
                    -- for rating indices, give a tiny right shift
                    if i % 2 == 0 then
                        self:addx(actuals.RightTextSlightOffset)
                    end
                    self:y(actuals.RightSmallTextUpperGap + (actuals.RightSmallTextAllottedSpace / (#ms.SkillSets * 2)) * (i-1))
                    self:zoom(skillsetTextSize)
                    self:maxwidth((actuals.Width - actuals.RightTextLeftGap) / skillsetTextSize - textzoomFudge)
                    self:settext("")
                end,
                SetCommand = function(self)
                    -- even indices are the ratings
                    -- odd indices are the titles
                    if i % 2 == 0 then
                        if DLMAN:IsLoggedIn() then
                            local lrating = profile:GetPlayerSkillsetRating(skillset)
                            local orating = DLMAN:GetSkillsetRating(skillset)
                            local rank = DLMAN:GetSkillsetRank(skillset)
                            self:settextf("%5.2f (#%d) / %5.2f", orating, rank, lrating)
                            self:diffuse(byMSD(orating))
                        else
                            local rating = profile:GetPlayerSkillsetRating(skillset)
                            self:settextf("%5.2f", rating)
                            self:diffuse(byMSD(rating))
                        end
                    else
                        self:settextf("%s:", skillset)
                    end
                end,
                UpdateLoginStatusCommand = function(self)
                    self:playcommand("Set")
                end
            }
        end

        local t = Def.ActorFrame {
            Name = "OverallPage",
            BeginCommand = function(self)
                self:z(overallPageZBump)
                self:playcommand("Set")
            end,

            Def.Sprite {
                Name = "Avatar",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(actuals.AvatarLeftGap, actuals.InfoUpperMargin)
                end,
                SetCommand = function(self)
                    self:Load(getAvatarPath(PLAYER_1))
                    self:zoomto(actuals.AvatarSize, actuals.AvatarSize)
                end
            },
            LoadFont("Common Normal") .. {
                Name = "NameRank",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(textmarginX, actuals.InfoUpperMargin)
                    self:zoom(largelineTextSize)
                    self:maxwidth((actuals.RightTextLeftGap - actuals.AvatarLeftGap - actuals.AvatarSize - actuals.NameInfoLeftMargin) / largelineTextSize - textzoomFudge)
                    self:settext("")
                end,
                SetCommand = function(self)
                    if DLMAN:IsLoggedIn() then
                        local rank = DLMAN:GetSkillsetRank("Overall")
                        self:settextf("%s (#%d)", pname, rank)
                    else
                        self:settext(pname)
                    end
                end,
                UpdateLoginStatusCommand = function(self)
                    self:playcommand("Set")
                end
            },
            LoadFont("Common Normal") .. {
                Name = "LoggedInAs",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(textmarginX, actuals.InfoUpperMargin + actuals.NameInfoLargeLineSpacing)
                    self:zoom(largelineTextSize)
                    self:maxwidth((actuals.RightTextLeftGap - actuals.AvatarLeftGap - actuals.AvatarSize - actuals.NameInfoLeftMargin) / largelineTextSize - textzoomFudge)
                    self:settext("")
                end,
                SetCommand = function(self)
                    if DLMAN:IsLoggedIn() then
                        self:settextf("(EO: %s)", DLMAN:GetUsername())
                    else
                        self:settext("")
                    end
                end,
                UpdateLoginStatusCommand = function(self)
                    self:playcommand("Set")
                end
            },
            LoadFont("Common Normal") .. {
                Name = "PlayerRatingsTitle",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(actuals.RightTextLeftGap, actuals.InfoUpperMargin)
                    self:zoom(largelineTextSize)
                    self:maxwidth((actuals.Width - actuals.RightTextLeftGap) / largelineTextSize - textzoomFudge)
                    self:settext("")
                end,
                SetCommand = function(self)
                    if DLMAN:IsLoggedIn() then
                        self:settext("Player Ratings (Online/Offline):")
                    else
                        self:settext("Player Ratings:")
                    end
                end,
                UpdateLoginStatusCommand = function(self)
                    self:playcommand("Set")
                end
            }
        }

        for i = 1, #smallTextFunctions do
            t[#t+1] = leftTextSmall(i)
        end

        for i = 1, (#ms.SkillSets * 2) do
            t[#t+1] = rightTextSmall(i)
        end

        return t
    end

    for i = 1, itemCount do
        t[#t+1] = createItem(i)
    end

    t[#t+1] = overallPage()

    t[#t+1] = Def.Quad {
        Name = "MouseWheelRegion",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:diffusealpha(0)
            self:zoomto(actuals.Width, actuals.Height)
        end,
        MouseScrollMessageCommand = function(self, params)
            if isOver(self) and focused and chosenSkillset ~= "Overall" then
                if params.direction == "Up" then
                    movePage(-1)
                else
                    movePage(1)
                end
            end
        end
    }

    t[#t+1] = UIElements.TextButton(1, 1, "Common Normal") .. {
        Name = "OnlineOfflineToggle",
        InitCommand = function(self)
            local txt = self:GetChild("Text")
            local bg = self:GetChild("BG")

            self:x(actuals.MainIndicatorLeftGap)
            self:y(actuals.MainIndicatorUpperGap)
            txt:halign(0):valign(0)
            txt:zoom(indicatorTextSize)
            txt:maxwidth((actuals.ItemSongNameWidth + actuals.ItemSongNameLeftGap) / indicatorTextSize - textzoomFudge)
            txt:settext("")
            bg:halign(0)
            -- really really bad approximation in conjunction with the zoomto height below
            -- there must be a better way (dont suggest txt:GetZoomedHeight())
            bg:y(actuals.ItemSpacing / 2.4)
            self:queuecommand("UpdateToggle")
        end,
        UpdateToggleCommand = function(self)
            local txt = self:GetChild("Text")
            local bg = self:GetChild("BG")

            if chosenSkillset == "Overall" or not DLMAN:IsLoggedIn() then
                self:diffusealpha(0)
            else
                self:diffusealpha(1)
            end

            if isLocal then
                txt:settext("Showing Local")
            else
                txt:settext("Showing Online")
            end

            -- itemspacing is probably a good approximation for this button height
            bg:zoomto(txt:GetZoomedWidth(), actuals.ItemSpacing + textzoomFudge)
        end,
        ClickCommand = function(self, params)
            if self:IsInvisible() then return end
            if params.update == "OnMouseDown" then
                if DLMAN:IsLoggedIn() then
                    isLocal = not isLocal
                    self:GetParent():playcommand("UpdateScores")
                    self:GetParent():playcommand("UpdateList")
                else
                    -- to avoid being stuck in online mode when somehow not logged in
                    if not isLocal then
                        isLocal = true
                        self:GetParent():playcommand("UpdateScores")
                        self:GetParent():playcommand("UpdateList")
                    end
                end
                self:playcommand("UpdateToggle")
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
        UpdateLoginStatusCommand = function(self)
            -- this logic should handle case where you are logged out while in the online portion of the menu
            -- will force an exit
            local loggedIn = DLMAN:IsLoggedIn()
            if not isLocal and not loggedIn then
                self:playcommand("Click", {update = "OnMouseDown"})
                isLocal = true
            else
                self:playcommand("UpdateToggle")
            end
        end
    }

    t[#t+1] = LoadFont("Common Normal") .. {
        Name = "PageText",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:xy(actuals.ItemDiffRightAlignLeftGap, actuals.MainIndicatorUpperGap)
            self:zoom(pageTextSize)
            self:maxwidth((actuals.ItemDiffRightAlignLeftGap - actuals.ItemRateRightAlignLeftGap) / pageTextSize - textzoomFudge)
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
    Name = "UpperLip",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.UpperLipHeight)
        self:diffuse(color("#111111"))
        self:diffusealpha(0.6)
    end
}

t[#t+1] = createChoices()

t[#t+1] = createList()

return t