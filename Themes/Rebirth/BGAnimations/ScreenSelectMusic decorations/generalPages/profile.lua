local focused = false
local t = Def.ActorFrame {
    Name = "ProfilePageFile",
    InitCommand = function(self)
        -- hide all general box tabs on startup
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params and params.tab ~= nil then
            if params.tab == SCUFF.profiletabindex then
                self:z(200)
                self:smooth(0.2)
                self:diffusealpha(1)
                focused = true
            else
                self:z(-100)
                self:smooth(0.2)
                self:diffusealpha(0)
                focused = false
            end
        end
    end
}

local ratios = {
    UpperLipHeight = 43 / 1080, -- frame edge to lip edge
    LipSeparatorThickness = 2 / 1080,

    MainIndicatorLeftGap = 12 / 1920, -- left edge to left edge of text, this is the online/local thing
    MainIndicatorUpperGap = 48 / 1080,
    PageTextRightGap = 33 / 1920, -- right of frame, right of text
    ItemIndexMargin = 29 / 1920, -- left edge to center of the indices
    ItemSSRRightAlignLeftGap = 129 / 1920, -- left edge to right edge of SSR
    ItemSSRWidth = 70 / 1920, -- rough estimation
    ItemSongNameLeftGap = 146 / 1920, -- left edge to song name
    ItemSongNameWidth = 375 / 1920, -- rough estimation
    ItemRateRightAlignLeftGap = 569 / 1920, -- left edge to right edge of rate
    ItemRateWidth = 48 / 1920, -- rough estimaion, lines up with the song name width
    ItemPercentRightAlignLeftGap = 691 / 1920, -- left edge to right edge of percent
    ItemPercentWidth = 115 / 1920, -- slightly less than the distance to the rate
    ItemDiffRightAlignLeftGap = 748 / 1920, -- left edge to right edge of diff
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
    RightSmallTextUpperGap = 124 / 1080, -- top of frame to top of stream skillset
    RightSmallTextAllottedSpace = 410 / 1080, -- the reference doesnt show overall so we have to position based on this to be dynamic
}

local actuals = {
    UpperLipHeight = ratios.UpperLipHeight * SCREEN_HEIGHT,
    LipSeparatorThickness = ratios.LipSeparatorThickness * SCREEN_HEIGHT,
    MainIndicatorLeftGap = ratios.MainIndicatorLeftGap * SCREEN_WIDTH,
    MainIndicatorUpperGap = ratios.MainIndicatorUpperGap * SCREEN_HEIGHT,
    PageTextRightGap = ratios.PageTextRightGap * SCREEN_WIDTH,
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

local translations = {
    SongsLoaded = THEME:GetString("ScreenSelectMusic Profile", "SongsLoaded"),
    PacksLoaded = THEME:GetString("ScreenSelectMusic Profile", "PacksLoaded"),
    CountVisible = THEME:GetString("ScreenSelectMusic Profile", "CountVisible"),
    JudgeDifficulty = THEME:GetString("ScreenSelectMusic Profile", "JudgeDifficulty"),
    Top3PlayedSkillsets = THEME:GetString("ScreenSelectMusic Profile", "Top3PlayedSkillsets"),
    UploadAllScores = THEME:GetString("ScreenSelectMusic Profile", "UploadAllScores"),
    ValidateAllScores = THEME:GetString("ScreenSelectMusic Profile", "ValidateAllScores"),
    Plays = THEME:GetString("ScreenSelectMusic Profile", "Plays"),
    ArrowsSmashed = THEME:GetString("ScreenSelectMusic Profile", "ArrowsSmashed"),
    Playtime = THEME:GetString("ScreenSelectMusic Profile", "Playtime"),
    ScoreStats = THEME:GetString("ScreenSelectMusic Profile", "ScoreStats"),
    PackLamps = THEME:GetString("ScreenSelectMusic Profile", "PackLamps"),
    ScoreUploadMayBeSlow = THEME:GetString("ScreenSelectMusic Profile", "ScoreUploadMayBeSlow"),
    OnlineServer = THEME:GetString("ScreenSelectMusic Profile", "OnlineServer"),
    PlayerRatings = THEME:GetString("ScreenSelectMusic Profile", "PlayerRatings"),
    OnlineSlashOffline = THEME:GetString("ScreenSelectMusic Profile", "OnlineSlashOffline"),
    PlayerStats = THEME:GetString("ScreenSelectMusic Profile", "PlayerStats"),
    ViewRecentScores = THEME:GetString("ScreenSelectMusic Profile", "ViewRecentScores"),
    ShowingLocalScores = THEME:GetString("ScreenSelectMusic Profile", "ShowingLocalScores"),
    ShowingOnlineScores = THEME:GetString("ScreenSelectMusic Profile", "ShowingOnlineScores"),
    SetPlayerName = THEME:GetString("ScreenSelectMusic Profile", "SetPlayerName"),
}

-- the page names in the order they go
-- it happens to literally just be the skillsets including Overall
local choiceNames = ms.SkillSets

local itemCount = 15
local maxPages = 40
local scoreListAnimationSeconds = 0.03
local textListAnimationSeconds = 0.03
-- we can potentially display every score on the profile
-- there isn't a good reason to do that or not to do that
-- so let's just default to about twice as many as we are used to
local upperBoundOfScoreCount = itemCount * maxPages

local indicatorTextSize = 0.7
local pageTextSize = 0.7
local itemIndexSize = 0.9
local ssrTextSize = 0.9
local nameTextSize = 0.9
local dateTextSize = 0.7
local rateTextSize = 0.9
local percentTextSize = 0.9
local diffTextSize = 0.9

-- overall page text sizes
local largelineTextSize = 0.8
local smalllineTextSize = 0.6
local rightSmallTextSize = 0.7

local choiceTextSize = 0.7
local buttonHoverAlpha = 0.6
local textzoomFudge = 5
-- movement on the Z axis to fix button related issues
-- this value doesnt change anything as long as it is smaller than 1
-- basically z movement is necessary to tell which buttons are on top even if they are invisible
-- invisible buttons on top of other buttons will make the bottom buttons inaccessible
local overallPageZBump = 0.1
-- this upwards bump fixes font related positioning
-- the font has a baseline which pushes it downward by some bit
-- this corrects the bg so that the hover is not wrong as a result
local scoreitembgbump = 1

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
                txt:settext(ms.SkillSetsTranslatedByName[choiceNames[i]])
                registerActorToColorConfigElement(txt, "main", "PrimaryText")
                bg:zoomto(actuals.Width / #choiceNames, actuals.LowerLipHeight)
            end,
            UpdateSelectedIndexCommand = function(self, params)
                local txt = self:GetChild("Text")
                if params ~= nil and params.index ~= nil then
                    selectedIndex = params.index
                end
                if selectedIndex == i then
                    txt:strokecolor(Brightness(COLORS:getMainColor("PrimaryText"), 0.75))
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

            -- if the index -1 is given, this is the recentscores index
            if params ~= nil and params.index == -1 then
                isLocal = true
                SCOREMAN:SortRecentScoresForGame()
                chosenSkillset = "Recent"
                scores = {}
                local sortedScore = SCOREMAN:GetRecentScoreForGame(1)
                while sortedScore ~= nil and #scores < upperBoundOfScoreCount do
                    scores[#scores+1] = sortedScore
                    sortedScore = SCOREMAN:GetRecentScoreForGame(#scores + 1)
                end
                return
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
                if DLMAN:IsLoggedIn() and chosenSkillset ~= "Recent" then
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
                    self:x(actuals.ItemSSRRightAlignLeftGap)
                    self:zoom(ssrTextSize)
                    self:maxwidth(actuals.ItemSSRWidth / ssrTextSize - textzoomFudge)
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local ssr = 0
                        if isLocal then
                            if chosenSkillset == "Recent" then
                                ssr = score:GetSkillsetSSR("Overall")
                            else
                                ssr = score:GetSkillsetSSR(chosenSkillset)
                            end
                        else
                            ssr = score.ssr
                        end
                        self:settextf("%05.2f", ssr)
                        self:diffuse(colorByMSD(ssr))
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
                    bg:y(-scoreitembgbump)

                    txt:zoom(nameTextSize)
                    txt:maxwidth(actuals.ItemSongNameWidth / nameTextSize - textzoomFudge)
                    txt:maxheight(actuals.ItemAllottedSpace / itemCount / nameTextSize)
                    registerActorToColorConfigElement(txt, "main", "PrimaryText")
                    bg:zoomy(actuals.ItemAllottedSpace / itemCount)
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    self:playcommand("SetScore")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")

                        if isLocal then
                            local song = SONGMAN:GetSongByChartKey(score:GetChartKey())
                            if song then
                                txt:settext(song:GetDisplayMainTitle())
                                if score:GetEtternaValid() then
                                    txt:diffuse(COLORS:getMainColor("PrimaryText"))
                                else
                                    txt:diffuse(COLORS:getColor("generalBox", "InvalidScore"))
                                end
                            else
                                txt:settext("")
                            end
                        else
                            txt:settext(score.songName)
                            txt:diffuse(COLORS:getMainColor("PrimaryText"))
                        end
                        -- for recent scores, cut the width in half to make room for the date
                        if chosenSkillset == "Recent" then
                            txt:maxwidth(actuals.ItemSongNameWidth / 3 * 2 / nameTextSize - textzoomFudge)
                        else
                            txt:maxwidth(actuals.ItemSongNameWidth / nameTextSize - textzoomFudge)
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
                        if params.event == "DeviceButton_left mouse button" then
                            -- find song on click (even if filtered)
                            local w = SCREENMAN:GetTopScreen():GetChild("WheelFile")
                            if w ~= nil then
                                local ck
                                if isLocal then
                                    ck = score:GetChartKey()
                                else
                                    ck = score.chartkey
                                end
                                if ck ~= nil then
                                    w:playcommand("FindSong", {chartkey = ck})
                                end
                            end
                        elseif params.event == "DeviceButton_right mouse button" and isLocal then
                            if score ~= nil then
                                score:ToggleEtternaValidation()
                                if score:GetEtternaValid() then
                                    ms.ok("Score validated")
                                else
                                    ms.ok("Score invalidated")
                                end
                                STATSMAN:UpdatePlayerRating()
                                self:playcommand("SetScore")
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
                Name = "Date",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(actuals.ItemSongNameLeftGap + actuals.ItemSongNameWidth / 3 * 2)
                    self:zoom(dateTextSize)
                    self:maxwidth(actuals.ItemSongNameWidth / 3 / dateTextSize - textzoomFudge)
                    self:settext("")
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        if chosenSkillset == "Recent" then
                            self:diffusealpha(1)
                            local d = score:GetDate()
                            if d ~= nil then
                                self:settext(d)
                            else
                                self:settext("")
                            end
                        else
                            self:diffusealpha(0)
                        end
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
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
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
                ColorConfigUpdatedMessageCommand = function(self)
                    self:playcommand("SetScore")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        if isLocal then
                            local w = score:GetWifeScore()
                            local wifestr = checkWifeStr(w)
                            self:settext(wifestr)
                            self:diffuse(colorByGrade(score:GetWifeGrade()))
                        else
                            local w = score.wife
                            local wifestr = checkWifeStr(w)
                            self:settext(wifestr)
                            self:diffuse(colorByGrade(score.grade))
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
                ColorConfigUpdatedMessageCommand = function(self)
                    self:playcommand("SetScore")
                end,
                SetScoreCommand = function(self)
                    if score ~= nil then
                        if isLocal then
                            local steps = SONGMAN:GetStepsByChartKey(score:GetChartKey())
                            if steps then
                                self:settext(getShortDifficulty(steps:GetDifficulty()))
                                self:diffuse(colorByDifficulty(steps:GetDifficulty()))
                            else
                                self:settext("")
                            end
                        else
                            local diff = score.difficulty
                            self:settext(getShortDifficulty(diff))
                            self:diffuse(colorByDifficulty(diff))
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
        -- Left refers to the stat lines constantly on the left
        -- Right refers to the stat lines in the alt page on the right
        local smallTextFunctions = {
            Left = {
                -- songs loaded
                function(self)
                    local count = SONGMAN:GetNumSongs()
                    self:settextf("%d %s", count, translations["SongsLoaded"])
                end,
                -- song packs installed (and filtered)
                function(self)
                    local packcount = SONGMAN:GetNumSongGroups()
                    local groupcount = #WHEELDATA:GetAllGroups()
                    if packcount ~= groupcount then
                        self:settextf("%d %s (%d %s)", packcount, translations["PacksLoaded"], groupcount, translations["CountVisible"])
                    else
                        self:settextf("%d %s", packcount, translations["PacksLoaded"])
                    end
                end,
                -- current judge
                function(self)
                    local judge = GetTimingDifficulty()
                    self:settextf("%s: %d", translations["JudgeDifficulty"], judge)
                end,
                -- a line break
                function(self)
                end,
                -- top skillset plays header
                function(self)
                    self:settext(translations["Top3PlayedSkillsets"])
                end,
                -- top played skillset
                function(self)
                    local count, name = getSkillsetPlaysByPosition(1)
                    self:settextf("%s (%d)", ms.SkillSetsTranslatedByName[name], count)
                end,
                -- 2nd top played skillset
                function(self)
                    local count, name = getSkillsetPlaysByPosition(2)
                    self:settextf("%s (%d)", ms.SkillSetsTranslatedByName[name], count)
                end,
                -- 3rd top played skillset
                function(self)
                    local count, name = getSkillsetPlaysByPosition(3)
                    self:settextf("%s (%d)", ms.SkillSetsTranslatedByName[name], count)
                end,
                -- blank
                function(self)
                end,
                -- Upload all scores button
                function(self)
                    if DLMAN:IsLoggedIn() then
                        self:settext(translations["UploadAllScores"]) 
                    else
                        self:settext("")
                    end
                end,
                -- Validate all scores button
                function(self)
                    self:settext(translations["ValidateAllScores"])
                end,
            },
            Right = {
                -- playcount
                function(self)
                    local pcount = SCOREMAN:GetTotalNumberOfScores()
                    self:settextf("%d %s", pcount, translations["Plays"])
                end,
                -- arrow count
                function(self)
                    local parrows = profile:GetTotalTapsAndHolds()
                    -- shorten if over 1 million since the number is kind of long
                    local nstr = shortenIfOver1Mil(parrows)
                    self:settextf("%s %s", nstr, translations["ArrowsSmashed"])
                end,
                -- playtime (overall, not gameplay)
                function(self)
                    local ptime = profile:GetTotalSessionSeconds()
                    self:settextf("%s %s", SecondsToHHMMSS(ptime), translations["Playtime"])
                end,
                -- empty padding
                function(self)
                end,
                function(self)
                end,
                -- score stats
                function(self)
                    self:settextf("%s:", translations["ScoreStats"])
                end,
                -- AAAAA count
                function(self)
                    local quints = WHEELDATA:GetTotalClearsByGrade("Grade_Tier01")
                    self:settextf("%ss: %d", getGradeStrings("Grade_Tier01"), quints)
                end,
                -- AAAA count
                function(self)
                    local scores = WHEELDATA:GetTotalClearsByGrade("Grade_Tier02") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier03") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier04")
                    self:settextf("%ss: %d", getGradeStrings("Grade_Tier04"), scores)
                end,
                -- AAA count
                function(self)
                    local scores = WHEELDATA:GetTotalClearsByGrade("Grade_Tier05") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier06") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier07")
                    self:settextf("%ss: %d", getGradeStrings("Grade_Tier07"), scores)
                end,
                -- AA count
                function(self)
                    local scores = WHEELDATA:GetTotalClearsByGrade("Grade_Tier08") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier09") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier10")
                    self:settextf("%ss: %d", getGradeStrings("Grade_Tier10"), scores)
                end,
                -- lamp count
                function(self)
                    local lamps = WHEELDATA:GetTotalLampCount()
                    self:settextf("%s: %d", translations["PackLamps"], lamps)
                end,

                -- at the cost of your fps to make the game look better
                -- these are here for space padding
                -- feel free to replace or remove them at will
                function(self)
                end,
                function(self)
                end,
                function(self)
                end,
                function(self)
                end,
                function(self)
                end,
            },
        }

        -- these functions run immediately after init if they exist
        -- they should have access to the top screen
        -- Left refers to the stat lines constantly on the left
        -- Right refers to the stat lines in the alt page on the right
        local smallTextInitFunctions = {
            Left = {
                -- [index] = function(self) end,
                [5] = function(self) -- hack to make this text slightly bigger
                    self:zoom(smalllineTextSize + 0.1)
                end,
            },
            Right = {
                -- [index] = function(self) end,
                [5] = function(self) -- hack to make this text slightly bigger
                    self:zoom(smalllineTextSize + 0.1)
                end,
            },
        }

        -- these functions run as you mouse over the text
        -- each subentry is 3 functions accepting no params, only self
        -- first function is onHover (mouse on)
        -- second function is onUnHover (mouse out)
        -- third function is onClick (mouse down)
        -- invisible checks are not necessary
        -- Left refers to the stat lines constantly on the left
        -- Right refers to the stat lines in the alt page on the right
        -- the indices of the subtable elements correspond to the function index implicitly defined in the tables above
        local smallTextHoverFunctions = {
            Left = {
                -- [index] = { function(self) end, function(self) end }
                [10] = {
                    function(self)
                        if DLMAN:IsLoggedIn() then
                            self:diffusealpha(buttonHoverAlpha)
                            TOOLTIP:SetText(translations["ScoreUploadMayBeSlow"])
                            TOOLTIP:Show()
                        end
                    end,
                    function(self)
                        self:diffusealpha(1)
                        TOOLTIP:Hide()
                    end,
                    function(self, params)
                        if DLMAN:IsLoggedIn() then
                            if params.event == "DeviceButton_left mouse button" then
                                DLMAN:UploadAllScores()
                            end
                        end
                    end,
                },
                [11] = {
                    function(self)
                        self:diffusealpha(buttonHoverAlpha)
                    end,
                    function(self)
                        self:diffusealpha(1)
                    end,
                    function(self, params)
                        if profile then
                            profile:UnInvalidateAllScores()
                            STATSMAN:UpdatePlayerRating()
                        end
                    end,
                },
            },
            Right = {
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
        }

        local function leftTextSmall(i)
            local cHover = nil
            local cUnHover = nil
            local cClick = nil
            -- set the mouse commands if they exist
            if smallTextHoverFunctions.Left[i] ~= nil then
                cHover = function(self, params)
                    if self:IsInvisible() then return end
                    smallTextHoverFunctions.Left[i][1](self)
                end
                cUnHover = function(self, params)
                    if self:IsInvisible() then return end
                    smallTextHoverFunctions.Left[i][2](self)
                end
                cClick = function(self, params)
                    if self:IsInvisible() then return end
                    smallTextHoverFunctions.Left[i][3](self, params)
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
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                    self:queuecommand("Startup")
                end,
                SetCommand = smallTextFunctions.Left[i],
                StartupCommand = smallTextInitFunctions.Left[i],
                MouseOverCommand = cHover,
                MouseOutCommand = cUnHover,
                MouseDownCommand = cClick,
                UpdateLoginStatusCommand = function(self)
                    self:playcommand("Set")
                    if cHover ~= nil then
                        if isOver(self) then self:playcommand("MouseOver") end
                    end
                    if cUnHover ~= nil then
                        if not isOver(self) then self:playcommand("MouseOut") end
                    end
                end,
            }
        end

        -- generalized function for the right column of text
        local function rightTextSmall(i, maxIndex)
            return UIElements.TextToolTip(1, 1, "Common Normal") .. {
                InitCommand = function(self)
                    -- do not override this stuff probably
                    self:halign(0):valign(0)
                    self:x(actuals.RightTextLeftGap)
                    self:y(actuals.RightSmallTextUpperGap + (actuals.RightSmallTextAllottedSpace / (maxIndex)) * (i-1))
                    self:zoom(rightSmallTextSize)
                    self:maxwidth((actuals.Width - actuals.RightTextLeftGap) / rightSmallTextSize - textzoomFudge)
                    self:settext("")
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                    self:playcommand("SubInit")
                end,
                SubInitCommand = function(self)
                    -- implement this within rightTextSkillsets or rightTextStats
                    -- it is run at init and should only be run at init (as the last thing)
                    -- (this is different from the StartupCommand, although it is the same exact concept)
                end,
                SetCommand = function(self)
                    -- implement this within rightTextSkillsets or rightTextStats
                end,
            }
        end

        -- right column text for skillsets
        local function rightTextSkillsets(i, maxIndex)
            local skillsetIndex = math.ceil(i/2)
            local skillset = ms.SkillSets[skillsetIndex]
            return rightTextSmall(i, maxIndex) .. {
                Name = "RightTextSkillsets_"..i,
                SubInitCommand = function(self)
                    -- for rating indices, give a tiny right shift
                    if i % 2 == 0 then
                        self:addx(actuals.RightTextSlightOffset)
                    end
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
                            self:diffuse(colorByMSD(orating))
                        else
                            local rating = profile:GetPlayerSkillsetRating(skillset)
                            self:settextf("%5.2f", rating)
                            self:diffuse(colorByMSD(rating))
                        end
                    else
                        self:settextf("%s:", ms.SkillSetsTranslatedByName[skillset])
                    end
                end,
                UpdateLoginStatusCommand = function(self)
                    self:playcommand("Set")
                end,
                PlayerRatingUpdatedMessageCommand = function(self)
                    self:playcommand("Set")
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    if i % 2 ~= 0 then return end -- cant click non numbers
                    if i <= 2 then return end -- cant click the overall skillset
                    self:diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    if i % 2 ~= 0 then return end
                    self:diffusealpha(1)
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.event == "DeviceButton_left mouse button" and i % 2 == 0 then
                        -- change chosen skillset and regrab all scores
                        -- but know that overall does something different
                        -- this many parents should lead to the t of this .lua
                        self:GetParent():GetParent():GetParent():GetParent():playcommand("UpdateScores", {index = i/2})
                        self:GetParent():GetParent():GetParent():GetParent():playcommand("UpdateSelectedIndex", {index = i/2})
                        self:GetParent():GetParent():GetParent():GetParent():playcommand("UpdateList")
                        self:diffusealpha(1)
                    end
                end,
            }
        end

        -- right column text for stats
        local function rightTextStats(i, maxIndex)
            local cHover = nil
            local cUnHover = nil
            -- set the hover commands if they exist
            if smallTextHoverFunctions.Right[i] ~= nil then
                cHover = function(self, params)
                    if self:IsInvisible() then return end
                    smallTextHoverFunctions.Right[i][1](self)
                end
                cUnHover = function(self, params)
                    if self:IsInvisible() then return end
                    smallTextHoverFunctions.Right[i][2](self)
                end
            end

            return rightTextSmall(i, maxIndex) .. {
                Name = "RightTextStats_"..i,
                SetCommand = smallTextFunctions.Right[i],
                StartupCommand = smallTextInitFunctions.Right[i],
                MouseOverCommand = cHover,
                MouseOutCommand = cUnHover,
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
                AvatarChangedMessageCommand = function(self)
                    self:playcommand("Set")
                end,
                SetCommand = function(self)
                    self:Load(getAvatarPath(PLAYER_1))
                    self:zoomto(actuals.AvatarSize, actuals.AvatarSize)
                end
            },
            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "NameRank",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(textmarginX, actuals.InfoUpperMargin)
                    self:zoom(largelineTextSize)
                    self:maxwidth((actuals.RightTextLeftGap - actuals.AvatarLeftGap - actuals.AvatarSize - actuals.NameInfoLeftMargin) / largelineTextSize - textzoomFudge)
                    self:settext("")
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                SetCommand = function(self)
                    if DLMAN:IsLoggedIn() then
                        local rank = DLMAN:GetSkillsetRank("Overall")
                        self:settextf("%s (#%d)", pname, rank)
                    else
                        self:settext(pname)
                    end
                    if self:IsInvisible() then return end
                    if isOver(self) then
                        TOOLTIP:SetText(translations["SetPlayerName"])
                        TOOLTIP:Show()
                    end
                end,
                ProfileRenamedMessageCommand = function(self)
                    pname = profile:GetDisplayName()
                    self:playcommand("Set")
                end,
                MouseOverCommand = function(self)
                    self:diffusealpha(buttonHoverAlpha)
                    if self:IsInvisible() then return end
                    TOOLTIP:SetText(translations["SetPlayerName"])
                    TOOLTIP:Show()
                end,
                MouseOutCommand = function(self)
                    self:diffusealpha(1)
                    TOOLTIP:Hide()
                end,
                MouseDownCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.event == "DeviceButton_left mouse button" then
                        TOOLTIP:Hide()
                        renameProfileDialogue(profile)
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
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                SetCommand = function(self)
                    if DLMAN:IsLoggedIn() then
                        self:settextf("(%s: %s)", translations["OnlineServer"], DLMAN:GetUsername())
                    else
                        self:settext("")
                    end
                end,
                UpdateLoginStatusCommand = function(self)
                    self:playcommand("Set")
                end
            },
            UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "PlayerRatingsTitle",
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
                    self:xy(actuals.RightTextLeftGap, actuals.InfoUpperMargin)

                    txt:halign(0):valign(0)
                    bg:halign(0):valign(0)
                    txt:zoom(largelineTextSize)
                    txt:maxwidth((actuals.Width - actuals.RightTextLeftGap) / largelineTextSize - textzoomFudge)
                    registerActorToColorConfigElement(txt, "main", "PrimaryText")

                    -- this is the ultimate fudge value
                    -- meant to be the approximate size of the text vertically but a lot smaller
                    bg:y(-actuals.NameInfoLargeLineSpacing / 3)
    
                    self.ratings = true
                    self:queuecommand("UpdateToggle")
                end,
                UpdateToggleCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
        
                    if self.ratings then
                        if DLMAN:IsLoggedIn() then
                            txt:settextf("%s (%s):", translations["PlayerRatings"], translations["OnlineSlashOffline"])
                        else
                            txt:settextf("%s:", translations["PlayerRatings"])
                        end
                    else
                        txt:settextf("%s:", translations["PlayerStats"])
                    end
        
                    bg:zoomto(txt:GetZoomedWidth(), actuals.NameInfoLargeLineSpacing + textzoomFudge)
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "OnMouseDown" then
                        self.ratings = not self.ratings
                        if self.ratings then
                            self:GetParent():GetChild("RightSmallTextFrame"):playcommand("ShowRatings")
                        else
                            self:GetParent():GetChild("RightSmallTextFrame"):playcommand("ShowStats")
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
                    self:playcommand("UpdateToggle")
                end
            },
            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "RecentScores",
                InitCommand = function(self)
                    self:halign(0):valign(1)
                    self:x(actuals.AvatarLeftGap)
                    self:y(actuals.Height - actuals.InfoUpperMargin)
                    self:zoom(largelineTextSize)
                    self:maxwidth((actuals.Width - actuals.AvatarLeftGap - actuals.RightTextLeftGap) / largelineTextSize - textzoomFudge)
                    self:settext(translations["ViewRecentScores"])
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
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
                    if self:IsInvisible() then return end
                    if params.event == "DeviceButton_left mouse button" then
                        self:diffusealpha(1)
                        self:GetParent():GetParent():playcommand("UpdateScores", {index = -1})
                        self:GetParent():GetParent():playcommand("UpdateSelectedIndex")
                        self:GetParent():GetParent():playcommand("UpdateList")
                    end
                end
            }
        }

        local function leftSmallTextContainer()
            -- just a container for control purposes
            local t = Def.ActorFrame {
                Name = "LeftSmallTextFrame",
            }
            for i = 1, #smallTextFunctions.Left do
                t[#t+1] = leftTextSmall(i)
            end
            return t
        end
        t[#t+1] = leftSmallTextContainer()

        local function rightSmallTextContainer()
            local skillsetTextCount = #ms.SkillSets * 2
            local statTextCount = #smallTextFunctions.Right

            -- just a container for control purposes
            local t = Def.ActorFrame {
                Name = "RightSmallTextFrame",
                BeginCommand = function(self)
                    self:playcommand("ShowRatings")
                end,
                ShowRatingsCommand = function(self)
                    for i = 1, skillsetTextCount do
                        local c = self:GetChild("RightTextSkillsets_"..i)
                        c:finishtweening()
                        c:z(5)
                        c:smooth(textListAnimationSeconds * i)
                        c:diffusealpha(1)
                    end
                    for i = 1, statTextCount do
                        local c = self:GetChild("RightTextStats_"..i)
                        c:finishtweening()
                        c:z(-5)
                        c:smooth(textListAnimationSeconds)
                        c:diffusealpha(0)
                    end
                end,
                ShowStatsCommand = function(self)
                    for i = 1, skillsetTextCount do
                        local c = self:GetChild("RightTextSkillsets_"..i)
                        c:finishtweening()
                        c:z(-5)
                        c:smooth(textListAnimationSeconds)
                        c:diffusealpha(0)
                    end
                    for i = 1, statTextCount do
                        local c = self:GetChild("RightTextStats_"..i)
                        c:finishtweening()
                        c:z(5)
                        c:smooth(textListAnimationSeconds * i)
                        c:diffusealpha(1)
                    end
                end,
            }
            for i = 1, skillsetTextCount do
                t[#t+1] = rightTextSkillsets(i, skillsetTextCount)
            end
            for i = 1, statTextCount do
                t[#t+1] = rightTextStats(i, statTextCount)
            end
            return t
        end
        t[#t+1] = rightSmallTextContainer()

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
            registerActorToColorConfigElement(txt, "main", "PrimaryText")
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
                txt:settext(translations["ShowingLocalScores"])
            else
                txt:settext(translations["ShowingOnlineScores"])
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
            self:xy(actuals.Width - actuals.PageTextRightGap, actuals.MainIndicatorUpperGap)
            self:zoom(pageTextSize)
            -- oddly precise max width but this should fit with the original size
            self:maxwidth(actuals.Width * 0.14 / pageTextSize - textzoomFudge)
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        UpdateListCommand = function(self)
            local lb = clamp((page-1) * (itemCount) + 1, 0, #scores)
            local ub = clamp(page * itemCount, 0, #scores)
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

t[#t+1] = createChoices()

t[#t+1] = createList()

return t