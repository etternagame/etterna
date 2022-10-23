 -- 11 visible items (top is a group header)
 -- an unfortunate amount of code is reliant on the fact that there are 11 items
 -- but thankfully everything works fine if you change it
 -- ... the header wont look very good if you push it off the screen though
 -- (retrospective comment: wtf i changed this to 14 and it still works)
local numWheelItems = 14

local ratios = {
    LeftGap = 16 / 1920,
    UpperGap = 219 / 1080, -- distance from top of screen, not info frame
    LowerGap = 0 / 1080, -- expected, maybe unused
    Width = 867 / 1920,
    Height = 861 / 1080, -- does not include the header
    ItemHeight = 86.5 / 1080, -- 85 + 2 to account for half of the upper and lower item dividers
    ItemDividerThickness = 2 / 1080,
    ItemDividerLength = 584 / 1920,
    ItemTextUpperGap = 20 / 1080, -- distance from top of item to center of title text
    ItemTextLowerGap = 18 / 1080, -- distance from center of divider to center of author text
    ItemTextCenterDistance = 40 / 1080, -- distance from lower (divider center) to center of subtitle
    ItemGradeTextRightGap = 24 / 1920, -- distance from right of item to right edge of text
    ItemGradeTextMaxWidth = 86 / 1920, -- approximation of width of the AAAAA grade
    ItemFavoriteIconRightGap = 18 / 1920, -- from right edge of banner to middle of favorite icon
    ItemFavoriteIconSize = 36 / 1080, -- width and height of the icon
    ItemPermamirrorIconRightGap = 40 / 1920, -- from right edge of banner to middle of favorite icon
    ItemPermamirrorIconSize = 39 / 1080, -- width and height of the icon
    BannerWidth = 265 / 1920,
    BannerItemGap = 18 / 1920, -- gap between banner and item text/dividers
    HeaderHeight = 110 / 1080,
    HeaderUpperGap = 109 / 1080, -- top of screen to top of frame (same as playerinfo height)
    wtffudge = 45 / 1080, -- this random number fixes the weird offset of the wheel vertically so that it fits perfectly with the header
    -- effective measurements for group specific information
    HeaderBannerWidth = 336 / 1920,
    HeaderText1UpperGap = 21 / 1080, -- distance from top edge to top text top edge
    HeaderText2UpperGap = 68 / 1080, -- distance from top edge to top edge of lower text
    HeaderTextLeftGap = 21 / 1920, -- distance from edge of banner to left of text
    -- effective measurements for the header lines when not in group specific info mode
    HeaderMText1UpperGap = 16 / 1080,
    HeaderMText2UpperGap = 47 / 1080,
    HeaderMText3UpperGap = 78 / 1080,
    HeaderMTextLeftGap = 0 / 1920, -- text width will be the same as the banner width
    HeaderGraphWidth = 573 / 1920, -- very approximate

    -- controls the width of the mouse wheel scroll box, should be the same number as the general box X position
    -- (found in generalBox.lua)
    GeneralBoxLeftGap = 1140 / 1920, -- distance from left edge to the left edge of the general box

    ScrollBarWidth = 18 / 1920,
    ScrollBarHeight = 971 / 1080,
}

local actuals = {
    LeftGap = ratios.LeftGap * SCREEN_WIDTH,
    UpperGap = ratios.UpperGap * SCREEN_HEIGHT,
    LowerGap = ratios.LowerGap * SCREEN_HEIGHT,
    Width = ratios.Width * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    ItemHeight = ratios.ItemHeight * SCREEN_HEIGHT,
    ItemDividerThickness = 2, -- maybe needs to be constant
    ItemDividerLength = ratios.ItemDividerLength * SCREEN_WIDTH,
    ItemTextUpperGap = ratios.ItemTextUpperGap * SCREEN_HEIGHT,
    ItemTextLowerGap = ratios.ItemTextLowerGap * SCREEN_HEIGHT,
    ItemTextCenterDistance = ratios.ItemTextCenterDistance * SCREEN_HEIGHT,
    ItemGradeTextRightGap = ratios.ItemGradeTextRightGap * SCREEN_WIDTH,
    ItemGradeTextMaxWidth = ratios.ItemGradeTextMaxWidth * SCREEN_WIDTH,
    ItemFavoriteIconRightGap = ratios.ItemFavoriteIconRightGap * SCREEN_WIDTH,
    ItemFavoriteIconSize = ratios.ItemFavoriteIconSize * SCREEN_HEIGHT,
    ItemPermamirrorIconRightGap = ratios.ItemPermamirrorIconRightGap * SCREEN_WIDTH,
    ItemPermamirrorIconSize = ratios.ItemPermamirrorIconSize * SCREEN_HEIGHT,
    BannerWidth = ratios.BannerWidth * SCREEN_WIDTH,
    BannerItemGap = ratios.BannerItemGap * SCREEN_WIDTH,
    HeaderHeight = ratios.HeaderHeight * SCREEN_HEIGHT,
    HeaderUpperGap = ratios.HeaderUpperGap * SCREEN_HEIGHT,
    wtffudge = ratios.wtffudge * SCREEN_HEIGHT,
    HeaderBannerWidth = ratios.HeaderBannerWidth * SCREEN_WIDTH,
    HeaderText1UpperGap = ratios.HeaderText1UpperGap * SCREEN_HEIGHT,
    HeaderText2UpperGap = ratios.HeaderText2UpperGap * SCREEN_HEIGHT,
    HeaderTextLeftGap = ratios.HeaderTextLeftGap * SCREEN_WIDTH,
    HeaderMText1UpperGap = ratios.HeaderMText1UpperGap * SCREEN_HEIGHT,
    HeaderMText2UpperGap = ratios.HeaderMText2UpperGap * SCREEN_HEIGHT,
    HeaderMText3UpperGap = ratios.HeaderMText3UpperGap * SCREEN_HEIGHT,
    HeaderMTextLeftGap = ratios.HeaderMTextLeftGap * SCREEN_WIDTH,
    HeaderGraphWidth = ratios.HeaderGraphWidth * SCREEN_WIDTH,
    GeneralBoxLeftGap = ratios.GeneralBoxLeftGap * SCREEN_WIDTH,
    ScrollBarWidth = ratios.ScrollBarWidth * SCREEN_WIDTH,
    ScrollBarHeight = ratios.ScrollBarHeight * SCREEN_HEIGHT,
}

local translations = {
    NumberOfSongs = THEME:GetString("ScreenSelectMusic Wheel", "NumberOfSongs"),
    AverageMSDShort = THEME:GetString("ScreenSelectMusic Wheel", "AverageMSDShort"),
    AverageMSDLong = THEME:GetString("ScreenSelectMusic Wheel", "AverageMSDLong"),
    PackClearedUsingDownrates = THEME:GetString("ScreenSelectMusic Wheel", "PackClearedUsingDownrates"),
    SessionTime = THEME:GetString("ScreenSelectMusic Wheel", "SessionTime"),
    SessionPlays = THEME:GetString("ScreenSelectMusic Wheel", "SessionPlays"),
    AverageAccuracy = THEME:GetString("ScreenSelectMusic Wheel", "AverageAccuracy"),
}

local wheelItemTextSize = 0.62
local wheelItemGradeTextSize = 1
local wheelItemTitleTextSize = 0.82
local wheelItemSubTitleTextSize = 0.62
local wheelItemArtistTextSize = 0.62
local wheelItemGroupTextSize = 0.82
local wheelItemGroupInfoTextSize = 0.62
local wheelHeaderTextSize = 1.2
local wheelHeaderMTextSize = 0.6
local textzoomfudge = 5 -- used in maxwidth to allow for gaps when squishing text

local graphLineColor = COLORS:getWheelColor("GraphLine")
local primaryTextColor = COLORS:getMainColor("PrimaryText")

-----
-- header related things
local headerTransitionSeconds = 0.2
local graphBoundTextSize = 0.4
local graphBoundOffset = 10 / 1080 * SCREEN_HEIGHT -- offset the graph bounds diagonally by this much for alignment reasons
local graphWidth = actuals.ItemDividerLength - actuals.ItemGradeTextRightGap / 2
local playsThisSession = SCOREMAN:GetNumScoresThisSession()
local scoresThisSession = SCOREMAN:GetScoresThisSession()
local accThisSession = 0
-- the vertical bounds for the graph
-- need to keep them respectable ... dont allow below 50 or above 100
local graphUpperBound = 100
local graphLowerBound = 50
local hoverAlpha = 0.9

-- calculate average wife percent for scores set this session
-- ignores negative percents
local function calcAverageWifePercentThisSession()
    local sum = 0
    for i, s in ipairs(scoresThisSession) do
        sum = sum + clamp(s:GetWifeScore() * 100, 0, 100)
    end

    -- prevent division by 0
    if playsThisSession == 0 then
        return 0
    else
        return sum / playsThisSession
    end
end

-- figure out the graph bounds in a very slightly more intelligent way than normal
local function calculateGraphBounds()
    local max = -10000
    local min = 10000

    local sum = 0
    local sd = 0
    local mean = 0
    for _, s in ipairs(scoresThisSession) do
        local w = clamp(s:GetWifeScore() * 100, 0, 100)
        if w > max then
            max = w
        end
        if w < min then
            min = w
        end
        sum = sum + w
    end

    -- prevent division by 0
    if playsThisSession == 0 then
        mean = 85
        sd = 15
        min = 0
        max = 100
    else
        mean = sum / playsThisSession
        -- 2nd pass for sd
        for _, s in ipairs(scoresThisSession) do
            local w = clamp(s:GetWifeScore() * 100, 0, 100)
            sd = sd + (w - mean) ^ 2
        end
        sd = math.sqrt(sd / playsThisSession)
    end

    max = clamp(mean + sd, min, 100)
    min = clamp(mean - sd / 2, 0, max)

    -- probably possible if your only score is outside the 0-100 range somehow
    -- allow impossible bounds here (negative and +100%)
    if min == max and playsThisSession > 1 then
        max = max + 2.5
        min = min - 2.5
    end
    graphUpperBound = max
    graphLowerBound = min
end

-- convert x value to x position in graph
local function graphXPos(x, width)
    -- dont hit the edge
    local buffer = 0.01
    local minX = width * buffer
    local maxX = width * (1 - buffer)

    local count = playsThisSession
    -- the left end of segments should be where the points go
    -- for 1, this is the middle
    -- 2 points makes 3 segments
    -- ...etc
    local xsegmentsize = (width - buffer * 2) / (count + 1)

    -- remember offset by minX and then push it over by the segmentsize
    return minX + xsegmentsize * x
end

-- convert y value to y position in graph
local function graphYPos(y, height)
    -- dont quite allow hitting the edges
    local buffer = 1
    local minY = graphLowerBound - buffer
    local maxY = graphUpperBound + buffer
    y = clamp(y, graphLowerBound, graphUpperBound)

    local percentage = (y - minY) / (maxY - minY)

    -- negate the output, the graph line position is relative to bottom left corner of graph
    -- negative numbers go upward on the screen
    return -1 * percentage * height
end

-- generate vertices for 1 dot in the graph
local function createVertices(vt, x, y, c)
    vt[#vt + 1] = {{x, y, 0}, c}
end

-- generate the vertices to put into the ActorFrameTexture for the MiscPage graph
local function generateRecentWifeScoreGraph()
    local v = {}
    -- update color if it happened to update before now
    graphLineColor = COLORS:getWheelColor("GraphLine")

    for i, s in ipairs(scoresThisSession) do
        local w = s:GetWifeScore() * 100
        local x = graphXPos(i, graphWidth)
        local y = graphYPos(w, actuals.HeaderHeight / 8 * 6)
        createVertices(v, x, y, graphLineColor)
    end

    return v
end

accThisSession = calcAverageWifePercentThisSession()
calculateGraphBounds()
-----

-- the currently opened group (not necessarily the one hovered)
local openedGroup = ""
-- the hovered item (can be a group or a song)
-- if a song: has GetDisplayMainTitle
local hoveredItem = nil

-- wheel horizontal movement animation speed
local animationSeconds = 0.1
local wheelVisibleX = 0
local wheelHiddenX = -actuals.Width
local visible = true
local t = Def.ActorFrame {
    Name = "WheelFile",
    InitCommand = function(self)
        self:playcommand("SetThePositionForThisFrameNothingElse")
    end,
    HideWheelMessageCommand = function(self)
        if not visible then return end
        visible = false
        self:finishtweening()
        self:smooth(animationSeconds)
        self:diffusealpha(0)
        self:x(wheelHiddenX)
    end,
    ShowWheelMessageCommand = function(self)
        if visible then return end
        visible = true
        self:finishtweening()
        self:smooth(animationSeconds)
        self:diffusealpha(1)
        self:x(wheelVisibleX)
    end,
    ShowSettingsAltMessageCommand = function(self, params)
        if params and params.name then
            self:playcommand("HideWheel")
        else
            self:playcommand("ShowWheel")
        end
    end,
    OptionUpdatedMessageCommand = function(self, params)
        if params and params.name == "Music Wheel Banners" or params.name == "Show Banners" then
            self:playcommand("UpdateWheelBanners")
        end
    end,
    SetThePositionForThisFrameNothingElseCommand = function(self)
        if getWheelPosition() then
            wheelVisibleX = 0
            wheelHiddenX = -actuals.Width
        else
            wheelVisibleX = SCREEN_WIDTH - actuals.Width - actuals.LeftGap - actuals.ScrollBarWidth
            wheelHiddenX = SCREEN_WIDTH
        end
        if visible then
            self:x(wheelVisibleX)
        else
            self:x(wheelHiddenX)
        end
    end,
    UpdateWheelPositionCommand = function(self)
        self:playcommand("SetThePositionForThisFrameNothingElse")
    end,
    WheelSettledMessageCommand = function(self, params)
        -- just here to update hovered item for controlling the reload stuff
        if params then
            hoveredItem = params.hovered
        end
    end,
    ReloadWheelCommand = function(self)
        -- reloads the wheel data and places you back on the item you were on
        -- updates any changed filters
        local group = openedGroup
        local chartkey = nil
        if hoveredItem.GetDisplayMainTitle then
            chartkey = GAMESTATE:GetCurrentSteps():GetChartKey()
        else
            group = hoveredItem
        end
        WHEELDATA:ReloadWheelData()
        self:playcommand("ReloadFilteredSongs", {
            group = group,
            chartkey = chartkey,
        })
    end,
    ApplyFilterCommand = function(self)
        -- reloads the wheel data and places you on the first good match based on the search

        -- figure out if we entered the name of a group
        -- local searchterm = WHEELDATA:GetSearch().Title
        local findGroup = false
        --[[
        if searchterm ~= nil and searchterm ~= "" then
            -- WHEELDATA:ReloadWheelData() -- this is needed at least once before now
            findGroup = WHEELDATA:FindIndexOfFolder(searchterm:lower()) ~= -1
        end
        ]]

        if findGroup then
            WHEELDATA:ReloadWheelData()

            -- entered an exact group, go to that group
            self:playcommand("FindGroup", {
                group = searchterm:lower()
            })
        else
            if WHEELDATA:IsSearchFilterEmpty() then
                -- no search term? either search was removed or only a tag was applied.
                -- in that case, make an attempt to keep the current position
                -- if the current file got filtered out by a new tag, you are sent to position 1
                self:playcommand("ReloadWheel")
            else
                WHEELDATA:ReloadWheelData()
                local exactMatchSong = WHEELDATA:FindExactSearchMatchSong()
                if exactMatchSong == nil then
                    exactMatchSong = WHEELDATA:FindTheOnlySearchResult()
                end
                if exactMatchSong ~= nil then
                    -- there is an exact match maybe we want to go to
                    self:playcommand("FindSong", {
                        song = exactMatchSong,
                    })
                else
                    -- there isnt a precise match, just reset the wheel
                    self:playcommand("ReloadFilteredSongs")
                end
            end
        end
    end,
    DFRFinishedMessageCommand = function(self, params)
        if params and params.newsongs then
            if params.newsongs > 0 then
                self:playcommand("ReloadWheel")
                ms.ok(params.newsongs .. " new songs loaded")
            else
                ms.ok("No new songs loaded")
            end
        end
    end,
    ReloadedCurrentPackMessageCommand = function(self)
        self:playcommand("ReloadWheel")
    end,
    ReloadedCurrentSongMessageCommand = function(self)
        self:playcommand("ReloadWheel")
    end,
}


-- functionally create each item base because they are identical (BG and divider)
local function wheelItemBase()
    return Def.ActorFrame {
        Name = "WheelItemBase",

        Def.Quad {
            Name = "ItemBG",
            InitCommand = function(self)
                self:zoomto(actuals.Width, actuals.ItemHeight)
                -- coloring is somehow handled by the song/group stuff
            end,
        },
        Def.Quad {
            Name = "Divider",
            InitCommand = function(self)
                self:valign(0)
                self:halign(0)
                self:playcommand("SetPosition")
                self:diffusealpha(1)
                registerActorToColorConfigElement(self, "musicWheel", "ItemDivider")
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    if useWheelBanners() then
                        self:zoomto(actuals.ItemDividerLength, actuals.ItemDividerThickness)
                        self:xy(actuals.Width / 2 - actuals.ItemDividerLength, -actuals.ItemHeight/2)
                    else
                        self:zoomto(actuals.Width, actuals.ItemDividerThickness)
                        self:xy(actuals.Width / 2 - actuals.Width, -actuals.ItemHeight/2)
                    end
                else
                    if useWheelBanners() then
                        self:zoomto(actuals.ItemDividerLength, actuals.ItemDividerThickness)
                        self:xy(-actuals.Width / 2, -actuals.ItemHeight/2)
                    else
                        self:zoomto(actuals.Width, actuals.ItemDividerThickness)
                        self:xy(-actuals.Width / 2, -actuals.ItemHeight/2)
                    end
                end
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
            UpdateWheelBannersCommand = function(self)
                self:playcommand("SetPosition")
            end,
        },
    }
end

-- responsible for setting song banner for wheelitem updates
local function songBannerSetter(self, song, isCurrentItem)
    if not useWheelBanners() then
        self:visible(false)
        return
    end

    if isCurrentItem and useVideoBanners() then
        self:SetDecodeMovie(true)
    else
        self:SetDecodeMovie(false)
    end

    if song then
        local bnpath = song:GetBannerPath()
        -- we load the fallback banner but for aesthetic purpose at the moment, invisible
        if not showBanners() then
            self:visible(false)
        elseif not bnpath then
            bnpath = THEME:GetPathG("Common", "fallback banner")
            self:visible(false)
        else
            self:visible(true)
        end
        if self.bnpath ~= bnpath then
            self:Load(bnpath)
        end
        self.bnpath = bnpath
    end
end

-- responsible for setting group banner for wheelitem updates
local function groupBannerSetter(self, group, isCurrentItem)
    if not useWheelBanners() then
        self:visible(false)
        return
    end

    if isCurrentItem and useVideoBanners() then
        self:SetDecodeMovie(true)
    else
        self:SetDecodeMovie(false)
    end

    local bnpath = WHEELDATA:GetFolderBanner(group)
    -- we load the fallback banner but for aesthetic purpose at the moment, invisible
    if not showBanners() then
        self:visible(false)
    elseif not bnpath or bnpath == "" then
        bnpath = THEME:GetPathG("Common", "fallback banner")
        self:visible(false)
    else
        self:visible(true)
    end
    if self.bnpath ~= bnpath then
        self:Load(bnpath)
    end
    self.bnpath = bnpath
end

-- updates all information for a song wheelitem
local function songActorUpdater(songFrame, song, isCurrentItem)
    songFrame.Title:settext(song:GetDisplayMainTitle())
    songFrame.SubTitle:settext(song:GetDisplaySubTitle())
    songFrame.Artist:settext("~"..song:GetDisplayArtist())
    songFrame.Grade:playcommand("SetGrade", {grade = song:GetHighestGrade()})
    songFrame.Favorited:diffusealpha(song:IsFavorited() and 1 or 0)
    songFrame.Permamirror:diffusealpha(song:IsPermaMirror() and 1 or 0)
    songBannerSetter(songFrame.Banner, song, isCurrentItem)
end

-- updates all information for a group wheelitem
local function groupActorUpdater(groupFrame, packName, isCurrentItem)
    local packCount = WHEELDATA:GetFolderCount(packName)
    local packAverageDiff = WHEELDATA:GetFolderAverageDifficulty(packName)
    local clearstats = WHEELDATA:GetFolderClearStats(packName)

    groupFrame.Title:settext(packName)
    groupFrame.GroupInfo:playcommand("SetInfo", {count = packCount, avg = packAverageDiff[1]})
    groupFrame.ClearStats:playcommand("SetInfo", {stats = clearstats})
    groupFrame.ScoreStats:playcommand("SetInfo", {stats = clearstats, count = packCount})
    groupBannerSetter(groupFrame.Banner, packName, isCurrentItem)
end

-- to offer control of the actors specifically to us instead of the scripts
-- we have make separate local functions for the song/group builders
local function songActorBuilder()
    return Def.ActorFrame {
        Name = "SongFrame",
        wheelItemBase(),
        LoadFont("Common Normal") .. {
            Name = "Title",
            InitCommand = function(self)
                self:playcommand("SetPosition")
                self:zoom(wheelItemTitleTextSize)
                self:halign(0)
                self:maxheight(actuals.ItemHeight / 3 / wheelItemTitleTextSize)
                self:diffusealpha(1)
                registerActorToColorConfigElement(self, "main", "PrimaryText", 0.65)
                -- hack to color the ItemBG later
                local itembg = self:GetParent():GetChild("WheelItemBase"):GetChild("ItemBG")
                itembg:diffusealpha(0.6)
                registerActorToColorConfigElement(itembg, "musicWheel", "SongBackground")
            end,
            BeginCommand = function(self)
                self:GetParent().Title = self
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    if useWheelBanners() then
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                        self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemTitleTextSize - textzoomfudge)
                    else
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength - actuals.BannerWidth)
                        self:maxwidth(((actuals.ItemDividerLength + actuals.BannerWidth) - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemTitleTextSize - textzoomfudge)
                    end
                else
                    self:x((-actuals.Width / 2) + actuals.ItemGradeTextMaxWidth + actuals.ItemGradeTextRightGap)
                    if useWheelBanners() then
                        self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemTitleTextSize - textzoomfudge)
                    else
                        self:maxwidth(((actuals.ItemDividerLength + actuals.BannerWidth) - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemTitleTextSize - textzoomfudge)
                    end
                end
                self:y(-actuals.ItemHeight / 2 + actuals.ItemTextUpperGap)
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
            UpdateWheelBannersCommand = function(self)
                self:playcommand("SetPosition")
            end,
        },
        LoadFont("Common Normal") .. {
            Name = "SubTitle",
            InitCommand = function(self)
                self:playcommand("SetPosition")
                self:zoom(wheelItemSubTitleTextSize)
                self:halign(0)
                self:maxheight(actuals.ItemHeight / 3 / wheelItemSubTitleTextSize)
                self:diffusealpha(1)
                registerActorToColorConfigElement(self, "main", "SecondaryText")
            end,
            BeginCommand = function(self)
                self:GetParent().SubTitle = self
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    if useWheelBanners() then
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                        self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemSubTitleTextSize - textzoomfudge)
                    else
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength - actuals.BannerWidth)
                        self:maxwidth(((actuals.ItemDividerLength + actuals.BannerWidth) - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemSubTitleTextSize - textzoomfudge)
                    end
                else
                    self:x((-actuals.Width / 2) + actuals.ItemGradeTextMaxWidth + actuals.ItemGradeTextRightGap)
                    if useWheelBanners() then
                        self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemSubTitleTextSize - textzoomfudge)
                    else
                        self:maxwidth(((actuals.ItemDividerLength + actuals.BannerWidth) - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemSubTitleTextSize - textzoomfudge)
                    end
                end
                self:y(actuals.ItemHeight / 2 - actuals.ItemTextCenterDistance)
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
            UpdateWheelBannersCommand = function(self)
                self:playcommand("SetPosition")
            end,
        },
        LoadFont("Common Normal") .. {
            Name = "Artist",
            InitCommand = function(self)
                self:playcommand("SetPosition")
                self:zoom(wheelItemArtistTextSize)
                self:maxheight(actuals.ItemHeight / 3 / wheelItemArtistTextSize)
                self:diffusealpha(1)
                registerActorToColorConfigElement(self, "main", "SecondaryText")
            end,
            BeginCommand = function(self)
                self:GetParent().Artist = self
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    self:halign(0)
                    if useWheelBanners() then
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                        self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemArtistTextSize - textzoomfudge)
                    else
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength - actuals.BannerWidth)
                        self:maxwidth(((actuals.ItemDividerLength + actuals.BannerWidth) - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemArtistTextSize - textzoomfudge)
                    end
                else
                    self:halign(1)
                    if useWheelBanners() then
                        self:x(actuals.Width / 2 - actuals.BannerWidth - actuals.ItemGradeTextRightGap)
                        self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap * 2) / wheelItemArtistTextSize - textzoomfudge)
                    else
                        self:x(actuals.Width / 2 - actuals.ItemGradeTextRightGap)
                        self:maxwidth(((actuals.ItemDividerLength + actuals.BannerWidth) - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemArtistTextSize - textzoomfudge)
                    end
                end
                self:y(actuals.ItemHeight / 2 - actuals.ItemTextLowerGap)
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
            UpdateWheelBannersCommand = function(self)
                self:playcommand("SetPosition")
            end,
        },
        LoadFont("Common Normal") .. {
            Name = "Grade",
            InitCommand = function(self)
                self:playcommand("SetPosition")
                self:zoom(wheelItemGradeTextSize)
            end,
            BeginCommand = function(self)
                self:GetParent().Grade = self
            end,
            SetGradeCommand = function(self, params)
                if params.grade and params.grade ~= "Grade_Invalid" then
                    self:settext(THEME:GetString("Grade", params.grade:sub(#"Grade_T")))
                    self:diffuse(colorByGrade(params.grade))
                else
                    self:settext("")
                end
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    self:halign(1)
                    self:x(actuals.Width / 2 - actuals.ItemGradeTextRightGap)
                    self:maxwidth(actuals.ItemGradeTextMaxWidth / wheelItemGradeTextSize)
                else
                    self:halign(0.5)
                    self:x(-actuals.Width / 2 + (actuals.ItemGradeTextMaxWidth + actuals.ItemGradeTextRightGap) / 2)
                    self:maxwidth((actuals.ItemGradeTextMaxWidth - (actuals.ItemGradeTextRightGap)) / wheelItemGradeTextSize)
                end
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
        },
        Def.Sprite {
            Name = "Banner",
            InitCommand = function(self)
                -- y is already set: relative to "center"
                self:playcommand("SetPosition")
                self:scaletoclipped(actuals.BannerWidth, actuals.ItemHeight)
                -- dont play movies because they lag the wheel so much like wow please dont ever use those (for now)
                self:SetDecodeMovie(false)
            end,
            BeginCommand = function(self)
                self:GetParent().Banner = self
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    self:x(-actuals.Width / 2):halign(0)
                else
                    self:x(actuals.Width / 2):halign(1)
                end
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
        },
        Def.Sprite {
            Name = "FavoriteIcon",
            Texture = THEME:GetPathG("", "round_star"),
            InitCommand = function(self)
                -- same y line as the artist text
                self:y(actuals.ItemHeight / 2 - actuals.ItemTextLowerGap)
                self:playcommand("SetPosition")
                self:zoomto(actuals.ItemFavoriteIconSize, actuals.ItemFavoriteIconSize)
                self:diffusealpha(0)
                registerActorToColorConfigElement(self, "musicWheel", "Favorite")
                self:wag()
            end,
            BeginCommand = function(self)
                self:GetParent().Favorited = self
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    self:x(actuals.Width / 2)
                    --[[ old position was on the banner
                    if useWheelBanners() then
                        self:x(-actuals.Width / 2 + actuals.BannerWidth - actuals.ItemFavoriteIconRightGap)
                    else
                        self:x(actuals.Width / 2)
                    end
                    ]]
                else
                    self:x(-actuals.Width / 2)
                    --[[ old position was on the banner
                    if useWheelBanners() then
                        self:x(actuals.Width / 2 - actuals.BannerWidth + actuals.ItemFavoriteIconRightGap)
                    else
                        self:x(-actuals.Width / 2)
                    end
                    ]]
                end
            end,
            UpdateWheelBannersCommand = function(self)
                self:playcommand("SetPosition")
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
        },
        Def.Sprite {
            Name = "PermamirrorIcon",
            Texture = THEME:GetPathG("", "mirror"),
            InitCommand = function(self)
                -- same y line as the artist text
                self:y(actuals.ItemHeight / 2 - actuals.ItemTextLowerGap)
                self:playcommand("SetPosition")
                self:zoomto(actuals.ItemPermamirrorIconSize, actuals.ItemPermamirrorIconSize)
                self:diffusealpha(0)
                registerActorToColorConfigElement(self, "musicWheel", "Permamirror")
                self:wag()
            end,
            BeginCommand = function(self)
                self:GetParent().Permamirror = self
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    self:x(actuals.Width / 2 - actuals.ItemPermamirrorIconRightGap)
                    --[[ old position was on the banner
                    if useWheelBanners() then
                        self:x(-actuals.Width / 2 + actuals.BannerWidth - actuals.ItemPermamirrorIconRightGap)
                    else
                        self:x(actuals.Width / 2 - actuals.ItemPermamirrorIconRightGap)
                    end
                    ]]
                else
                    self:x(-actuals.Width / 2 + actuals.ItemPermamirrorIconRightGap)
                    --[[ old position was on the banner
                    if useWheelBanners() then
                        self:x(actuals.Width / 2 - actuals.BannerWidth + actuals.ItemPermamirrorIconRightGap)
                    else
                        self:x(-actuals.Width / 2 + actuals.ItemPermamirrorIconRightGap)
                    end
                    ]]
                end
            end,
            UpdateWheelBannersCommand = function(self)
                self:playcommand("SetPosition")
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
        }
    }
end

-- generates the clear stats bar for each group
local function scoreStatsFrame()

    -- list of grades to consider (midgrades will be converted appropriately based on the table below)
    local gradesToUse = {
        "Grade_Tier01", -- AAAAA
        "Grade_Tier04", -- AAAA
        "Grade_Tier07", -- AAA
        "Grade_Tier10", -- AA
        "Grade_Tier13", -- A
        "Grade_Tier14", -- B
        "Grade_Tier15", -- C
        "Grade_Tier20", -- mysterious Clear grade (only referred to by WHEELDATA)
    }

    -- lists of grades that are equivalent to the current grade
    -- all keys of this should match the above table
    local expandedGrades = {
        Grade_Tier01 = {"Grade_Tier01"},
        Grade_Tier04 = {"Grade_Tier02", "Grade_Tier03", "Grade_Tier04"},
        Grade_Tier07 = {"Grade_Tier05", "Grade_Tier06", "Grade_Tier07"},
        Grade_Tier10 = {"Grade_Tier08", "Grade_Tier09", "Grade_Tier10"},
        Grade_Tier13 = {"Grade_Tier11", "Grade_Tier12", "Grade_Tier13"},
        Grade_Tier14 = {"Grade_Tier14"},
        Grade_Tier15 = {"Grade_Tier15"},
        Grade_Tier20 = {"Grade_Tier20"},
    }

    -- determines the size of the outline quad
    local function framelength() return useWheelBanners() and actuals.ItemDividerLength * (3/4) or actuals.Width * (3/4) end
    local frameheight = 11 / 1080 * SCREEN_HEIGHT
    -- determines how much to shave off to make the size fit
    local outlineThickness = 2 / 1080 * SCREEN_HEIGHT
    local function maxbarlength() return framelength() - outlineThickness * 2 end
    local barheight = frameheight - outlineThickness * 2

    -- a colorful bar for each grade to represent
    local function makeBar(i)
        local grade = gradesToUse[i]
        return Def.Quad {
            Name = "Bar_"..grade,
            InitCommand = function(self)
                self:halign(0)
                self:zoomto(0, barheight)
                self:diffusealpha(1)
                if grade ~= "Grade_Tier20" then
                    registerActorToColorConfigElement(self, "grades", grade)
                else
                    registerActorToColorConfigElement(self, "clearType", "Clear")
                end
            end,
        }
    end

    local t = Def.ActorFrame {
        Name = "ScoreStatsFrame",
        SetInfoCommand = function(self, params)
            if params == nil and self.storedparams ~= nil then
                params = self.storedparams
            end

            if params ~= nil and params.stats ~= nil then
                -- if there are no scores in this pack, dont show the bar
                if params.stats.totalScores == 0 then
                    self:diffusealpha(0)
                else
                    self:diffusealpha(1)
                end

                self.storedparams = params

                local barcounts = {}
                -- determine how many scores count for each bar
                for gkey, gradeTable in pairs(expandedGrades) do
                    barcounts[gkey] = 0
                    for _, grade in ipairs(gradeTable) do
                        local c = params.stats.clearPerGrade[grade]
                        if c ~= nil then
                            barcounts[gkey] = barcounts[gkey] + c
                        end
                    end
                end
                -- update each bar
                local runningsum = 0
                for i = #gradesToUse, 1, -1 do
                    local grade = gradesToUse[i]
                    local child = self:GetChild("Bar_"..grade)
                    local percentSoFar = runningsum / params.count
                    local percentForThisBar = (barcounts[grade] or 0) / params.count
                    runningsum = runningsum + (barcounts[grade] or 0)
                    child:x(outlineThickness + maxbarlength() * percentSoFar)
                    child:zoomx(maxbarlength() * percentForThisBar)
                end
            end
        end,
        UpdateWheelBannersCommand = function(self)
            self:playcommand("SetInfo")
        end,
        UpdateWheelPositionCommand = function(self)
            self:playcommand("SetInfo")
        end,
        Def.Quad {
            Name = "Outline",
            InitCommand = function(self)
                self:halign(0)
                self:playcommand("UpdateWheelBanners")
            end,
            UpdateWheelBannersCommand = function(self)
                self:zoomto(framelength(), frameheight)
            end,
        },
        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:halign(0)
                self:x(outlineThickness)
                self:playcommand("UpdateWheelBanners")
                self:diffuse(color("0,0,0,1"))
            end,
            UpdateWheelBannersCommand = function(self)
                self:zoomto(maxbarlength(), barheight)
            end,
        }
    }

    for i = 1, #gradesToUse do
        t[#t+1] = makeBar(i)
    end

    return t
end

-- return a transformer function for the x position of the wheel items
-- changes parameters based on the getWheelPosition result
local function getFrameTransformer()
    local direction = getWheelPosition() and 1 or -1

    return function(frame, offsetFromCenter, index, total)
        -- this stuff makes the x position of the item go way off screen for the end indices
        -- should induce less of a feeling of items materializing from nothing
        local bias = -actuals.Width * 3
        local ofc = math.ceil(total / 2) + offsetFromCenter
        -- the power of 50 and the rounding here are kind of specific for our application
        -- if you mess with overall parameters to the wheel size or count, you will want to mess with this
        -- maybe
        local result = math.round(math.pow(ofc / ((total - 2) / 2) - (((total + 2) / 2) / ((total - 2) / 2)), 50), 2)
        local xp = bias * result * direction
        frame:xy(xp, offsetFromCenter * actuals.ItemHeight)
    end
end

-- see songActorBuilder comment
local function groupActorBuilder()
    return Def.ActorFrame {
        Name = "GroupFrame",
        wheelItemBase(),
        LoadFont("Common Normal") .. {
            Name = "GroupName",
            InitCommand = function(self)
                self:playcommand("SetPosition")
                self:zoom(wheelItemGroupTextSize)
                self:halign(0)
                self:diffusealpha(1)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
                -- we make the background of groups fully opaque to distinguish them from songs
                local itembg = self:GetParent():GetChild("WheelItemBase"):GetChild("ItemBG")
                itembg:diffusealpha(1)
                registerActorToColorConfigElement(itembg, "musicWheel", "FolderBackground")
            end,
            BeginCommand = function(self)
                self:GetParent().Title = self
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    if useWheelBanners() then
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                        self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemGroupTextSize - textzoomfudge)
                    else
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength - actuals.BannerWidth)
                        self:maxwidth(((actuals.ItemDividerLength + actuals.BannerWidth) - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemGroupTextSize - textzoomfudge)
                    end
                else
                    self:x((-actuals.Width / 2) + actuals.ItemGradeTextMaxWidth + actuals.ItemGradeTextRightGap)
                    if useWheelBanners() then
                        self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemGroupTextSize - textzoomfudge)
                    else
                        self:maxwidth(((actuals.ItemDividerLength + actuals.BannerWidth) - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemGroupTextSize - textzoomfudge)
                    end
                end
                self:y(-actuals.ItemHeight / 2 + actuals.ItemTextUpperGap)
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
            UpdateWheelBannersCommand = function(self)
                self:playcommand("SetPosition")
            end,
        },
        LoadFont("Common Normal") .. {
            Name = "GroupInfo",
            InitCommand = function(self)
                self:playcommand("SetPosition")
                self:zoom(wheelItemGroupInfoTextSize)
                self:diffusealpha(1)
                registerActorToColorConfigElement(self, "main", "SecondaryText")
                self.avg = 0
                self.count = 0
            end,
            BeginCommand = function(self)
                self:GetParent().GroupInfo = self
            end,
            SetInfoCommand = function(self, params)
                self.count = params.count
                self.avg = params.avg
                self:playcommand("UpdateText")
            end,
            UpdateTextCommand = function(self)
                self:visible(not WHEELDATA:inSortModeMenu())
                self:settextf("%d %s (%s %5.2f)", self.count, translations["NumberOfSongs"], translations["AverageMSDShort"], self.avg)
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    self:halign(0)
                    if useWheelBanners() then
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                        self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemGroupInfoTextSize - textzoomfudge)
                    else
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength - actuals.BannerWidth)
                        self:maxwidth(((actuals.ItemDividerLength + actuals.BannerWidth) - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemGroupInfoTextSize - textzoomfudge)
                    end
                else
                    self:halign(1)
                    if useWheelBanners() then
                        self:x(actuals.Width / 2 - actuals.BannerWidth - actuals.ItemGradeTextRightGap)
                        self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap * 2) / wheelItemGroupInfoTextSize - textzoomfudge)
                    else
                        self:x(actuals.Width / 2 - actuals.ItemGradeTextRightGap)
                        self:maxwidth(((actuals.ItemDividerLength + actuals.BannerWidth) - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemGroupInfoTextSize - textzoomfudge)
                    end
                end
                self:y(actuals.ItemHeight / 2 - actuals.ItemTextLowerGap)
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
            UpdateWheelBannersCommand = function(self)
                self:playcommand("SetPosition")
            end,
        },
        LoadFont("Common Normal") .. {
            Name = "ClearStats",
            InitCommand = function(self)
                self:playcommand("SetPosition")
                self:zoom(wheelItemGradeTextSize)
                self.lamp = nil
                self.scores = 0
            end,
            BeginCommand = function(self)
                self:GetParent().ClearStats = self
            end,
            SetInfoCommand = function(self, params)
                self.lamp = params.stats.lamp
                self.scores = params.stats.totalScores
                self:playcommand("UpdateText")
            end,
            UpdateTextCommand = function(self)
                local lstr = ""
                if self.lamp ~= nil then
                    if self.lamp ~= "Grade_Tier20" then
                        lstr = THEME:GetString("Grade", self.lamp:sub(#"Grade_T"))
                        self:diffuse(colorByGrade(self.lamp))
                    else
                        lstr = translations["PackClearedUsingDownrates"]
                        -- color for a clear
                        self:diffuse(colorByClearType("Clear"))
                    end
                end
                self:visible(not WHEELDATA:inSortModeMenu())
                self:settext(lstr)
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    self:halign(1)
                    self:x(actuals.Width / 2 - actuals.ItemGradeTextRightGap)
                    self:maxwidth(actuals.ItemGradeTextMaxWidth / wheelItemGradeTextSize)
                else
                    self:halign(0.5)
                    self:x(-actuals.Width / 2 + (actuals.ItemGradeTextMaxWidth + actuals.ItemGradeTextRightGap) / 2)
                    self:maxwidth((actuals.ItemGradeTextMaxWidth - (actuals.ItemGradeTextRightGap)) / wheelItemGradeTextSize)
                end
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
        },
        scoreStatsFrame() .. {
            InitCommand = function(self)
                self:playcommand("UpdateWheelBanners")
                self:y(actuals.ItemHeight / 30)
            end,
            BeginCommand = function(self)
                self:GetParent().ScoreStats = self
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    if useWheelBanners() then
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                    else
                        self:x(actuals.Width / 2 - actuals.ItemDividerLength - actuals.BannerWidth)
                    end
                else
                    self:x((-actuals.Width / 2) + actuals.ItemGradeTextMaxWidth + actuals.ItemGradeTextRightGap)
                end
            end,
            UpdateWheelBannersCommand = function(self)
                self:playcommand("SetPosition")
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
        },
        Def.Sprite {
            Name = "Banner",
            InitCommand = function(self)
                -- y is already set: relative to "center"
                self:playcommand("SetPosition")
                self:scaletoclipped(actuals.BannerWidth, actuals.ItemHeight)
                -- dont play movies because they lag the wheel so much like wow please dont ever use those (for now)
                self:SetDecodeMovie(false)
            end,
            BeginCommand = function(self)
                self:GetParent().Banner = self
            end,
            SetPositionCommand = function(self)
                if getWheelPosition() then
                    self:x(-actuals.Width / 2):halign(0)
                else
                    self:x(actuals.Width / 2):halign(1)
                end
            end,
            UpdateWheelPositionCommand = function(self)
                self:playcommand("SetPosition")
            end,
        }
    }
end

t[#t+1] = Def.ActorFrame {
    Name = "WheelContainer",
    InitCommand = function(self)
        -- push from top left of screen, this position is CENTER of the wheel X/Y
        -- also for some odd reason we have to move down by half a wheelItem....
        self:xy(actuals.LeftGap + actuals.Width / 2, actuals.UpperGap + actuals.Height / 2 - actuals.ItemHeight + actuals.wtffudge)
        SCREENMAN:set_input_redirected(PLAYER_1, true)
    end,
    BeginCommand = function(self)
        -- hide the old musicwheel
        SCREENMAN:GetTopScreen():GetMusicWheel():visible(false)
    end,
    OpenedGroupMessageCommand = function(self, params)
        openedGroup = params.group
    end,
    ClosedGroupMessageCommand = function(self)
        openedGroup = ""
    end,
    UpdateWheelBannersCommand = function(self)
        self:playcommand("UpdateWheel")
    end,
    UpdateWheelPositionCommand = function(self)
        self:playcommand("SetFrameTransformer", {f = getFrameTransformer()})
    end,


    -- because of the above, all of the X/Y positions are "relative" to center of the wheel
    -- ugh
    MusicWheel:new({
        count = numWheelItems,
        startOnPreferred = true,
        songActorBuilder = songActorBuilder,
        groupActorBuilder = groupActorBuilder,
        highlightBuilder = function() return Def.ActorFrame {
            Name = "HighlightFrame",
            Def.Quad {
                Name = "Highlight",
                InitCommand = function(self)
                    -- the highlighter should not cover the banner
                    -- move it by half the size and make it that much smaller
                    self:playcommand("SetPosition")
                    self:diffusealpha(0.2)
                    self:diffuseramp()
                    self:effectclock("beat")
                    registerActorToColorConfigElementForDiffuseRamp(self, "musicWheel", "HighlightColor", 0.5, 0.8)
                end,
                SetPositionCommand = function(self)
                    if getWheelPosition() then
                        if useWheelBanners() then
                            self:x(actuals.BannerWidth / 2)
                            self:zoomto(actuals.Width - actuals.BannerWidth, actuals.ItemHeight)
                        else
                            self:x(0)
                            self:zoomto(actuals.Width, actuals.ItemHeight)
                        end
                    else
                        if useWheelBanners() then
                            self:x(-actuals.BannerWidth / 2)
                            self:zoomto(actuals.Width - actuals.BannerWidth, actuals.ItemHeight)
                        else
                            self:x(0)
                            self:zoomto(actuals.Width, actuals.ItemHeight)
                        end
                    end
                end,
                UpdateWheelPositionCommand = function(self)
                    self:playcommand("SetPosition")
                end,
                UpdateWheelBannersCommand = function(self)
                    self:playcommand("SetPosition")
                end,
            }
        }
        end,
        songActorUpdater = songActorUpdater,
        groupActorUpdater = groupActorUpdater,
        frameTransformer = getFrameTransformer(),
        frameBuilder = function()
            local f
            f = Def.ActorFrame {
                Name = "ItemFrame",
                InitCommand = function(self)
                    f.actor = self
                end,
                UIElements.QuadButton(1) .. {
                    Name = "WheelItemClickBox",
                    InitCommand = function(self)
                        self:diffusealpha(0)
                        self:zoomto(actuals.Width, actuals.ItemHeight)
                    end,
                    MouseDownCommand = function(self, params)
                        if not visible then return end
                        if params.event == "DeviceButton_left mouse button" then
                            local index = self:GetParent().index
                            -- subtract 1 here BASED ON numWheelItems
                            -- ... i know its dumb but it works for the params i set myself
                            -- if you mess with numWheelItems YOU NEED TO MAKE SURE THIS WORKS
                            local distance = math.floor(index - numWheelItems / 2) - 1
                            local wheel = self:GetParent():GetParent()
                            if distance ~= 0 then
                                -- clicked a nearby item
                                wheel:playcommand("Move", {direction = distance})
                                wheel:playcommand("OpenIfGroup")
                            else
                                -- clicked the current item
                                wheel:playcommand("SelectCurrent")
                            end
                        end
                    end
                },

                groupActorBuilder() .. {
                    BeginCommand = function(self)
                        f.actor.g = self
                    end
                },
                songActorBuilder() .. {
                    BeginCommand = function(self)
                        f.actor.s = self
                    end
                }
            }
            return f
        end,
        frameUpdater = function(frame, songOrPack, offset, isCurrentItem)
            if songOrPack.GetAllSteps then
                -- This is a song
                local s = frame.s
                s:visible(true)
                local g = frame.g
                g:visible(false)
                songActorUpdater(s, songOrPack, isCurrentItem)
            else
                -- This is a group
                local s = frame.s
                s:visible(false)
                local g = (frame.g)
                g:visible(true)
                groupActorUpdater(g, songOrPack, isCurrentItem)
            end
        end
    }),

    Def.Quad {
        Name = "MouseWheelRegion",
        InitCommand = function(self)
            self:diffusealpha(0)
            -- the sizing here should make everything left of the wheel a mousewheel region
            -- and also just a bit above and below it
            -- and also the empty region to the right
            -- the wheel positioning is not as clear as it could be
            self:halign(0)
            self:y(-(actuals.HeaderHeight + actuals.ItemHeight) / 2)
            self:playcommand("SetPosition")
            self:zoomto(actuals.GeneralBoxLeftGap, actuals.Height + actuals.HeaderHeight * 2.45)
        end,
        SetPositionCommand = function(self)
            if getWheelPosition() then
                self:halign(0)
                self:x(-actuals.LeftGap - actuals.Width / 2)
            else
                self:halign(1)
                self:x(actuals.LeftGap + actuals.Width / 2)
            end
        end,
        UpdateWheelPositionCommand = function(self)
            self:playcommand("SetPosition")
        end,
        MouseScrollMessageCommand = function(self, params)
            if isOver(self) and visible then
                if params.direction == "Up" then
                    self:GetParent():GetChild("Wheel"):playcommand("Move", {direction = -1})
                else
                    self:GetParent():GetChild("Wheel"):playcommand("Move", {direction = 1})
                end
            end
        end,
        MouseClickPressMessageCommand = function(self, params)
            if params ~= nil and params.button ~= nil and visible then
                if params.button == "DeviceButton_right mouse button" then
                    if isOver(self) then
                        SCREENMAN:GetTopScreen():PauseSampleMusic()
                    end
                end
            end
        end
    },

    Def.ActorFrame {
        Name = "ScrollBar",
        InitCommand = function(self)
            self:playcommand("SetWPosition")
            -- places the frame at the top of the wheel
            -- positions will be relative to that
            self:y(-actuals.ItemHeight * numWheelItems / 2 + actuals.ItemHeight * 1.242)
        end,
        SetWPositionCommand = function(self)
            if getWheelPosition() then
                self:x(-actuals.LeftGap / 2 - actuals.Width / 2)
            else
                self:x(actuals.Width / 2 + actuals.ScrollBarWidth / 2)
            end
        end,
        UpdateWheelPositionCommand = function(self)
            self:playcommand("SetWPosition")
        end,

        Def.Sprite {
            Name = "BG",
            Texture = THEME:GetPathG("", "roundedCapsBar"),
            InitCommand = function(self)
                self:valign(0)
                self:diffuse(color("0,0,0"))
                self:diffusealpha(0.6)
                self:zoomto(actuals.ScrollBarWidth, actuals.ScrollBarHeight)
            end,
        },
        UIElements.QuadButton(1) .. {
            Name = "ClickBox",
            InitCommand = function(self)
                self:valign(0)
                self:diffusealpha(0)
                self:zoomto(actuals.ScrollBarWidth * 2.5, actuals.ScrollBarHeight)
            end,
            MouseDownCommand = function(self, params)
                if not visible then return end
                if params.event == "DeviceButton_left mouse button" then
                    local max = self:GetZoomedHeight()
                    local dist = params.MouseY
                    self:GetParent():GetParent():GetChild("Wheel"):playcommand("Move", {percent = dist / max})
                end
            end,
            MouseDragCommand = function(self, params)
                if not visible then return end
                if params.event == "DeviceButton_left mouse button" then
                    local max = self:GetZoomedHeight()
                    local dist = params.MouseY
                    self:GetParent():GetParent():GetChild("Wheel"):playcommand("Move", {percent = dist / max})
                end
            end,
        },
        Def.Sprite {
            Name = "Position",
            Texture = THEME:GetPathG("", "marker"),
            InitCommand = function(self)
                self:zoomto(actuals.ScrollBarWidth, actuals.ScrollBarWidth)
            end,
            SetPositionCommand = function(self, params)
                -- something really quirky here is that last element of the wheel is considered the first
                -- its a side effect of the currentSelection index being moved in the wheel so that the
                --      highlight is centered.
                -- this is a bad thing, but at least thats the explanation
                local maxY = self:GetParent():GetChild("BG"):GetZoomedHeight()
                local dist = params.index / params.maxIndex * maxY
                self:finishtweening()
                self:linear(0.05)
                self:y(dist)
            end,
            WheelIndexChangedMessageCommand = function(self, params)
                self:playcommand("SetPosition", params)
            end,
            WheelSettledMessageCommand = function(self, params)
                self:playcommand("SetPosition", params)
            end,
            ModifiedGroupsMessageCommand = function(self, params)
                self:playcommand("SetPosition", params)
            end,
        }
    },
}

t[#t+1] = Def.ActorFrame {
    Name = "WheelHeader",
    InitCommand = function(self)
        self:xy(actuals.LeftGap,actuals.HeaderUpperGap)
    end,
    ClosedGroupMessageCommand = function(self)
        self:playcommand("ScrolledOutOfGroup")
    end,
    ScrolledIntoGroupMessageCommand = function(self)
        self:GetChild("MiscPage"):playcommand("Out")
        self:GetChild("GroupPage"):playcommand("In")
    end,
    ScrolledOutOfGroupMessageCommand = function(self)
        self:GetChild("GroupPage"):playcommand("Out")
        self:GetChild("MiscPage"):playcommand("In")
    end,
    OnCommand = function(self)
        if openedGroup == nil or openedGroup == "" then
            self:GetChild("GroupPage"):playcommand("Out")
            self:GetChild("MiscPage"):playcommand("In")
        end
    end,

    UIElements.QuadButton(1) .. {
        Name = "BG",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(actuals.Width, actuals.HeaderHeight)
            self:diffusealpha(0.6)
            registerActorToColorConfigElement(self, "musicWheel", "HeaderBackground")
        end,
        MouseDownCommand = function(self, params)
            if not visible then return end
            if params.event == "DeviceButton_left mouse button" then
                if not self:GetParent():GetChild("GroupPage"):IsInvisible() then
                    -- left clicking the group header gives a random song in the group
                    local song = WHEELDATA:GetRandomSongInFolder(openedGroup)
                    self:GetParent():GetParent():GetChild("WheelContainer"):playcommand("FindSong", {song = song, group = openedGroup})
                elseif not self:GetParent():GetChild("MiscPage"):IsInvisible() then
                    -- left clicking the normal header gives a random group (???)
                    local group = WHEELDATA:GetRandomFolder()
                    self:GetParent():GetParent():GetChild("WheelContainer"):playcommand("FindGroup", {group = group})
                end
            end
        end,
        MouseOverCommand = function(self)
            self:GetParent():diffusealpha(hoverAlpha)
            MESSAGEMAN:Broadcast("HoverWheelHeader", {on = true})
        end,
        MouseOutCommand = function(self)
            self:GetParent():diffusealpha(1)
            MESSAGEMAN:Broadcast("HoverWheelHeader", {off = true})
        end,
    },
    Def.ActorFrame {
        Name = "GroupPage",
        InitCommand = function(self)
            self:diffusealpha(0)
        end,
        InCommand = function(self)
            self:finishtweening()
            self:smooth(headerTransitionSeconds)
            self:diffusealpha(1)
            self:playcommand("Set")
        end,
        OutCommand = function(self)
            self:finishtweening()
            self:smooth(headerTransitionSeconds)
            self:diffusealpha(0)
        end,

        Def.Sprite {
            Name = "Banner",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:scaletoclipped(actuals.HeaderBannerWidth, actuals.HeaderHeight)
                self:SetDecodeMovie(useVideoBanners())
            end,
            SetCommand = function(self)
                local bnpath = WHEELDATA:GetFolderBanner(openedGroup)
                if not showBanners() then
                    self:visible(false)
                elseif not bnpath or bnpath == "" then
                    bnpath = THEME:GetPathG("Common", "fallback banner")
                    self:visible(false)
                else
                    self:visible(true)
                end
                self:Load(bnpath)
            end,
            OptionUpdatedMessageCommand = function(self, params)
                if params and params.name == "Video Banners" then
                    self:SetDecodeMovie(useVideoBanners())
                elseif params and params.name == "Show Banners" then
                    self:playcommand("Set")
                end
            end,
        },
        LoadFont("Common Normal") .. {
            Name = "GroupTitle",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.HeaderBannerWidth + actuals.HeaderTextLeftGap, actuals.HeaderText1UpperGap)
                self:zoom(wheelHeaderTextSize)
                self:maxwidth((actuals.Width - actuals.HeaderTextLeftGap * 2 - actuals.HeaderBannerWidth) / wheelHeaderTextSize)
                self:diffusealpha(1)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            SetCommand = function(self)
                self:settext(openedGroup)
            end
        },
        LoadFont("Common Normal") .. {
            Name = "GroupInfo",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.HeaderBannerWidth + actuals.HeaderTextLeftGap, actuals.HeaderText2UpperGap)
                self:zoom(wheelHeaderTextSize)
                self:maxwidth((actuals.Width - actuals.HeaderTextLeftGap * 2 - actuals.HeaderBannerWidth) / wheelHeaderTextSize)
                self:diffusealpha(1)
                registerActorToColorConfigElement(self, "main", "SecondaryText")
            end,
            SetCommand = function(self)
                local files = WHEELDATA:GetFolderCount(openedGroup)
                local avg = WHEELDATA:GetFolderAverageDifficulty(openedGroup)[1]
                self:settextf("%d %s (%s: %5.2f)", files, translations["NumberOfSongs"], translations["AverageMSDLong"], avg)
            end
        }
    },
    Def.ActorFrame {
        Name = "MiscPage",
        InitCommand = function(self)
            self:diffusealpha(0)
            self:SetUpdateFunction(function(self)
                if not self:IsInvisible() then
                    self:GetChild("SessionTime"):playcommand("Set")
                end
            end)
            self:SetUpdateFunctionInterval(0.5)
        end,
        InCommand = function(self)
            self:finishtweening()
            self:smooth(headerTransitionSeconds)
            self:diffusealpha(1)
            self:playcommand("Set")
        end,
        OutCommand = function(self)
            self:finishtweening()
            self:smooth(headerTransitionSeconds)
            self:diffusealpha(0)
        end,

        LoadFont("Common Normal") .. {
            Name = "SessionTime",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.HeaderMTextLeftGap, actuals.HeaderMText1UpperGap)
                self:zoom(wheelHeaderMTextSize)
                self:maxwidth((actuals.BannerWidth - actuals.HeaderMTextLeftGap) / wheelHeaderMTextSize)
                self:diffusealpha(1)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            SetCommand = function(self)
                local sesstime = GAMESTATE:GetSessionTime()
                self:settextf("%s: %s", translations["SessionTime"], SecondsToHHMMSS(sesstime))
            end
        },
        LoadFont("Common Normal") .. {
            Name = "SessionPlays",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.HeaderMTextLeftGap, actuals.HeaderMText2UpperGap)
                self:zoom(wheelHeaderMTextSize)
                self:maxwidth((actuals.BannerWidth - actuals.HeaderMTextLeftGap) / wheelHeaderMTextSize)
                self:diffusealpha(1)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            SetCommand = function(self)
                self:settextf("%s: %d", translations["SessionPlays"], playsThisSession)
            end
        },
        LoadFont("Common Normal") .. {
            Name = "AverageAccuracy",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.HeaderMTextLeftGap, actuals.HeaderMText3UpperGap)
                self:zoom(wheelHeaderMTextSize)
                self:maxwidth((actuals.BannerWidth - actuals.HeaderMTextLeftGap) / wheelHeaderMTextSize)
                self:diffusealpha(1)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end,
            SetCommand = function(self)
                self:settextf("%s: %5.2f%%", translations["AverageAccuracy"], accThisSession)
            end
        },
        Def.ActorFrame {
            Name = "Graph",
            InitCommand = function(self)
                -- the graph is placed relative to the top of the header and left aligned the same as the wheel items left alignment
                self:x(actuals.Width - actuals.ItemDividerLength)
            end,
            -- the "vertical space" of the graph is essentially 6/8 of the height of the header
            -- 1/8 from the top and 1/8 from the bottom
            -- the "width" of the graph is (the width - banner width - distance from left banner - distance from right)
            -- not a clear number

            Def.Quad {
                Name = "YAxisLine",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(actuals.HeaderMTextLeftGap)
                    self:y(actuals.HeaderHeight / 8)
                    self:zoomto(actuals.ItemDividerThickness, actuals.HeaderHeight / 8 * 6)
                    self:diffusealpha(1)
                    registerActorToColorConfigElement(self, "musicWheel", "GraphLine")
                end,
            },
            Def.Quad {
                Name = "XAxisLine",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(actuals.HeaderMTextLeftGap)
                    self:y(actuals.HeaderHeight - actuals.HeaderHeight / 8)
                    self:zoomto(graphWidth, actuals.ItemDividerThickness)
                    self:diffusealpha(1)
                    registerActorToColorConfigElement(self, "musicWheel", "GraphLine")
                end,
            },
            LoadFont("Common Normal") .. {
                Name = "YMax",
                InitCommand = function(self)
                    self:halign(1)
                    self:x(actuals.HeaderMTextLeftGap - graphBoundOffset / 2)
                    self:y(actuals.HeaderHeight / 8 - graphBoundOffset)
                    self:rotationz(-45)
                    self:zoom(graphBoundTextSize)
                    self:settextf("%d%%", notShit.round(graphUpperBound))
                    self:diffusealpha(1)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
            },
            LoadFont("Common Normal") .. {
                Name = "YMin",
                InitCommand = function(self)
                    self:halign(1)
                    self:x(actuals.HeaderMTextLeftGap - graphBoundOffset / 2)
                    self:y(actuals.HeaderHeight / 8 * 7 - graphBoundOffset)
                    self:rotationz(-45)
                    self:zoom(graphBoundTextSize)
                    self:settextf("%d%%", notShit.round(graphLowerBound))
                    self:diffusealpha(1)
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
            },

            Def.ActorMultiVertex {
                Name = "Line",
                InitCommand = function(self)
                    self:x(actuals.HeaderMTextLeftGap)
                    self:y(actuals.HeaderHeight / 8 * 7)
                    self:playcommand("SetGraph")
                end,
                SetGraphCommand = function(self)
                    local v = generateRecentWifeScoreGraph()
                    if #v > 1 then
                        self:SetVertices(v)
                        self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #v}
                    else
                        self:SetVertices({})
                        self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = 0}
                    end
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    self:playcommand("SetGraph")
                end,
            }
        }
    }
}

return t
