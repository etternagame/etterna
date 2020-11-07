local t = Def.ActorFrame {Name = "WheelFile"}

 -- 11 visible items (top is a group header)
 -- an unfortunate amount of code is reliant on the fact that there are 11 items
 -- but thankfully everything works fine if you change it
 -- ... the header wont look very good if you push it off the screen though
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
    if min == max then
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

    for i, s in ipairs(scoresThisSession) do
        local w = s:GetWifeScore() * 100
        local x = graphXPos(i, graphWidth)
        local y = graphYPos(w, actuals.HeaderHeight / 8 * 6)
        createVertices(v, x, y, color("1,1,1,1"))
    end

    return v
end

accThisSession = calcAverageWifePercentThisSession()
calculateGraphBounds()
-----


-- functionally create each item base because they are identical (BG and divider)
local function wheelItemBase()
    return Def.ActorFrame {
        Name = "WheelItemBase",

        Def.Quad {
            Name = "ItemBG",
            InitCommand = function(self)
                self:zoomto(actuals.Width, actuals.ItemHeight)
                self:diffuse(color("#111111"))
                self:diffusealpha(0.6)
            end
        },
        Def.Quad { 
            Name = "Divider",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(actuals.ItemDividerLength, actuals.ItemDividerThickness)
                self:xy(actuals.Width / 2 - actuals.ItemDividerLength, -actuals.ItemHeight/2)
                self:diffuse(color("0.6,0.6,0.6,1"))
            end
        },
    }
end

-- responsible for setting song banner for wheelitem updates
local function songBannerSetter(self, song)
    if song then
        local bnpath = song:GetBannerPath()
        -- we load the fallback banner but for aesthetic purpose at the moment, invisible
        if not bnpath then
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
local function groupBannerSetter(self, group)
    local bnpath = SONGMAN:GetSongGroupBannerPath(group)
    -- we load the fallback banner but for aesthetic purpose at the moment, invisible
    if not bnpath or bnpath == "" then
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
local function songActorUpdater(songFrame, song)
    songFrame.Title:settext(song:GetDisplayMainTitle())
    songFrame.SubTitle:settext(song:GetDisplaySubTitle())
    songFrame.Artist:settext("~"..song:GetDisplayArtist())
    songFrame.Grade:playcommand("SetGrade", {grade = song:GetHighestGrade()})
    songBannerSetter(songFrame.Banner, song)
end

-- updates all information for a group wheelitem
local function groupActorUpdater(groupFrame, packName)
    local packCount = WHEELDATA:GetFolderCount(packName)
    local packAverageDiff = WHEELDATA:GetFolderAverageDifficulty(packName)
    local clearstats = WHEELDATA:GetFolderClearStats(packName)

    groupFrame.Title:settext(packName)
    groupFrame.GroupInfo:playcommand("SetInfo", {count = packCount, avg = packAverageDiff[1]})
    groupFrame.ClearStats:playcommand("SetInfo", {stats = clearstats})
    groupBannerSetter(groupFrame.Banner, packName)
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
                self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                self:y(-actuals.ItemHeight / 2 + actuals.ItemTextUpperGap)
                self:strokecolor(color("0.6,0.6,0.6,0.75"))
                self:zoom(wheelItemTitleTextSize)
                self:halign(0)
                self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemTitleTextSize - textzoomfudge)
                self:maxheight(actuals.ItemHeight / 3 / wheelItemTitleTextSize)
            end,
            BeginCommand = function(self)
                self:GetParent().Title = self
            end
        },
        LoadFont("Common Normal") .. {
            Name = "SubTitle",
            InitCommand = function(self)
                self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                self:y(actuals.ItemHeight / 2 - actuals.ItemTextCenterDistance)
                self:zoom(wheelItemSubTitleTextSize)
                self:halign(0)
                self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemSubTitleTextSize - textzoomfudge)
                self:maxheight(actuals.ItemHeight / 3 / wheelItemSubTitleTextSize)
            end,
            BeginCommand = function(self)
                self:GetParent().SubTitle = self
            end
        },
        LoadFont("Common Normal") .. {
            Name = "Artist",
            InitCommand = function(self)
                self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                self:y(actuals.ItemHeight / 2 - actuals.ItemTextLowerGap)
                self:zoom(wheelItemArtistTextSize)
                self:halign(0)
                self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemArtistTextSize - textzoomfudge)
                self:maxheight(actuals.ItemHeight / 3 / wheelItemArtistTextSize)
            end,
            BeginCommand = function(self)
                self:GetParent().Artist = self
            end
        },
        LoadFont("Common Normal") .. {
            Name = "Grade",
            InitCommand = function(self)
                self:halign(1)
                self:x(actuals.Width / 2 - actuals.ItemGradeTextRightGap)
                self:zoom(wheelItemGradeTextSize)
                self:maxwidth(actuals.ItemGradeTextMaxWidth / wheelItemGradeTextSize)
            end,
            BeginCommand = function(self)
                self:GetParent().Grade = self
            end,
            SetGradeCommand = function(self, params)
                if params.grade and params.grade ~= "Grade_Invalid" then
                    self:settext(THEME:GetString("Grade", params.grade:sub(#"Grade_T")))
                else
                    self:settext("")
                end
            end
        },
        Def.Sprite {
            Name = "Banner",
            InitCommand = function(self)
                -- y is already set: relative to "center"
                self:x(-actuals.Width / 2):halign(0)
                self:scaletoclipped(actuals.BannerWidth, actuals.ItemHeight)
                -- dont play movies because they lag the wheel so much like wow please dont ever use those (for now)
                self:SetDecodeMovie(false)
            end,
            BeginCommand = function(self)
                self:GetParent().Banner = self
            end
        }
    }
end

-- see songActorBuilder comment
local function groupActorBuilder()
    return Def.ActorFrame {
        Name = "GroupFrame",
        wheelItemBase(),
        LoadFont("Common Normal") .. {
            Name = "GroupName",
            InitCommand = function(self)
                self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                self:y(-actuals.ItemHeight / 2 + actuals.ItemTextUpperGap)
                self:zoom(wheelItemGroupTextSize)
                self:halign(0)
                self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemGroupTextSize - textzoomfudge)
                -- we make the background of groups fully opaque to distinguish them from songs
                self:GetParent():GetChild("WheelItemBase"):GetChild("ItemBG"):diffusealpha(1)
            end,
            BeginCommand = function(self)
                self:GetParent().Title = self
            end
        },
        LoadFont("Common Normal") .. {
            Name = "GroupInfo",
            InitCommand = function(self)
                self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                self:y(actuals.ItemHeight / 2 - actuals.ItemTextLowerGap)
                self:zoom(wheelItemGroupInfoTextSize)
                self:halign(0)
                self:maxwidth((actuals.ItemDividerLength - actuals.ItemGradeTextMaxWidth - actuals.ItemGradeTextRightGap) / wheelItemGroupInfoTextSize - textzoomfudge)
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
                self:settextf("%d Songs (Avg %5.2f)", self.count, self.avg)
            end
        },
        LoadFont("Common Normal") .. {
            Name = "ClearStats",
            InitCommand = function(self)
                self:halign(1)
                self:x(actuals.Width / 2 - actuals.ItemGradeTextRightGap)
                self:zoom(wheelItemGradeTextSize)
                self:maxwidth(actuals.ItemGradeTextMaxWidth / wheelItemGradeTextSize)
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
                    lstr = THEME:GetString("Grade", self.lamp:sub(#"Grade_T"))
                end
                self:settext(lstr)
            end
        },
        Def.Sprite {
            Name = "Banner",
            InitCommand = function(self)
                -- y is already set: relative to "center"
                self:x(-actuals.Width / 2):halign(0)
                self:scaletoclipped(actuals.BannerWidth, actuals.ItemHeight)
                -- dont play movies because they lag the wheel so much like wow please dont ever use those (for now)
                self:SetDecodeMovie(false)
            end,
            BeginCommand = function(self)
                self:GetParent().Banner = self
            end
        }
    }
end

local openedGroup = ""

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
                    self:x(actuals.BannerWidth / 2)
                    self:zoomto(actuals.Width - actuals.BannerWidth, actuals.ItemHeight)
                    self:diffusealpha(0.1)
                    self:diffuseramp()
                    self:effectclock("beat")
                end
            }
        }
        end,
        songActorUpdater = songActorUpdater,
        groupActorUpdater = groupActorUpdater,
        frameTransformer = function(frame, offsetFromCenter, index, total)
            -- this stuff makes the x position of the item go way off screen for the end indices
            -- should induce less of a feeling of items materializing from nothing
            local bias = -actuals.Width * 3
            local ofc = math.ceil(total / 2) + offsetFromCenter
            -- the power of 50 and the rounding here are kind of specific for our application
            -- if you mess with overall parameters to the wheel size or count, you will want to mess with this
            -- maybe
            local result = math.round(math.pow(ofc / ((total - 2) / 2) - (((total + 2) / 2) / ((total - 2) / 2)), 50), 2)
            local xp = bias * result
            frame:xy(xp, offsetFromCenter * actuals.ItemHeight)
        end,
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
        frameUpdater = function(frame, songOrPack)
            if songOrPack.GetAllSteps then
                -- This is a song
                local s = frame.s
                s:visible(true)
                local g = frame.g
                g:visible(false)
                songActorUpdater(s, songOrPack)
            else
                -- This is a group
                local s = frame.s
                s:visible(false)
                local g = (frame.g)
                g:visible(true)
                groupActorUpdater(g, songOrPack)
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
            self:x(-actuals.LeftGap - actuals.Width / 2)
            self:zoomto(actuals.GeneralBoxLeftGap, actuals.Height + actuals.HeaderHeight * 2.45)
        end,
        MouseScrollMessageCommand = function(self, params)
            if isOver(self) then
                if params.direction == "Up" then
                    self:GetParent():GetChild("Wheel"):playcommand("Move", {direction = -1})
                else
                    self:GetParent():GetChild("Wheel"):playcommand("Move", {direction = 1})
                end
            end
        end,
        MouseClickPressMessageCommand = function(self, params)
            if params ~= nil and params.button ~= nil then
                if params.button == "DeviceButton_right mouse button" then
                    if isOver(self) then
                        SCREENMAN:GetTopScreen():PausePreviewNoteField()
                    end
                end
            end
        end
    },

    Def.ActorFrame {
        Name = "ScrollBar",
        InitCommand = function(self)
            self:x(-actuals.LeftGap / 2 - actuals.Width / 2)
            -- places the frame at the top of the wheel
            -- positions will be relative to that
            self:y(-actuals.ItemHeight * numWheelItems / 2 + actuals.ItemHeight * 1.242)
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
                if params.event == "DeviceButton_left mouse button" then
                    local max = self:GetZoomedHeight()
                    local dist = params.MouseY
                    self:GetParent():GetParent():GetChild("Wheel"):playcommand("Move", {percent = dist / max})
                end
            end
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

    UIElements.QuadButton(1) .. {
        Name = "BG",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(actuals.Width, actuals.HeaderHeight)
            self:diffuse(color("#111111"))
            self:diffusealpha(0.6)
        end,
        MouseDownCommand = function(self, params)
            if params.event == "DeviceButton_left mouse button" then
                if not self:GetParent():GetChild("GroupPage"):IsInvisible() then
                    -- left clicking the group header gives a random song in the group
                    -- for some reason this breaks you out of groups into others
                    -- i believe this is either a race condition (???) or duplicate songs being found in other groups
                    local song = WHEELDATA:GetRandomSongInFolder(openedGroup)
                    self:GetParent():GetParent():GetChild("WheelContainer"):playcommand("FindSong", {song = song})
                elseif not self:GetParent():GetChild("MiscPage"):IsInvisible() then
                    -- left clicking the normal header gives a random group (???)
                    local group = WHEELDATA:GetRandomFolder()
                    self:GetParent():GetParent():GetChild("WheelContainer"):playcommand("FindGroup", {group = group})
                end
            end
        end
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
                -- dont play movies because they lag the wheel so much like wow please dont ever use those (for now)
                self:SetDecodeMovie(false)
            end,
            SetCommand = function(self)
                local bnpath = SONGMAN:GetSongGroupBannerPath(openedGroup)
                if not bnpath or bnpath == "" then
                    bnpath = THEME:GetPathG("Common", "fallback banner")
                    self:visible(false)
                else
                    self:visible(true)
                end
                self:Load(bnpath)
            end
        },
        LoadFont("Common Normal") .. {
            Name = "GroupTitle",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.HeaderBannerWidth + actuals.HeaderTextLeftGap, actuals.HeaderText1UpperGap)
                self:zoom(wheelHeaderTextSize)
                self:maxwidth((actuals.Width - actuals.HeaderTextLeftGap * 2 - actuals.HeaderBannerWidth) / wheelHeaderTextSize)
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
            end,
            SetCommand = function(self)
                local files = WHEELDATA:GetFolderCount(openedGroup)
                local avg = WHEELDATA:GetFolderAverageDifficulty(openedGroup)[1]
                self:settextf("%d Songs (Average MSD: %5.2f)", files, avg)
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
            end,
            SetCommand = function(self)
                local sesstime = GAMESTATE:GetSessionTime()
                self:settextf("Session Time: %s", SecondsToHHMMSS(sesstime))
            end
        },
        LoadFont("Common Normal") .. {
            Name = "SessionPlays",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.HeaderMTextLeftGap, actuals.HeaderMText2UpperGap)
                self:zoom(wheelHeaderMTextSize)
                self:maxwidth((actuals.BannerWidth - actuals.HeaderMTextLeftGap) / wheelHeaderMTextSize)
            end,
            SetCommand = function(self)
                self:settextf("Session Plays: %d", playsThisSession)
            end
        },
        LoadFont("Common Normal") .. {
            Name = "AverageAccuracy",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.HeaderMTextLeftGap, actuals.HeaderMText3UpperGap)
                self:zoom(wheelHeaderMTextSize)
                self:maxwidth((actuals.BannerWidth - actuals.HeaderMTextLeftGap) / wheelHeaderMTextSize)
            end,
            SetCommand = function(self)
                self:settextf("Average Accuracy: %5.2f%%", accThisSession)
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
                end,
            },
            Def.Quad {
                Name = "XAxisLine",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:x(actuals.HeaderMTextLeftGap)
                    self:y(actuals.HeaderHeight - actuals.HeaderHeight / 8)
                    self:zoomto(graphWidth, actuals.ItemDividerThickness)
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
                end,
            },

            Def.ActorMultiVertex {
                Name = "Line",
                InitCommand = function(self)
                    self:x(actuals.HeaderMTextLeftGap)
                    self:y(actuals.HeaderHeight / 8 * 7)
                    local v = generateRecentWifeScoreGraph()
                    self:SetVertices(v)
                    self:SetDrawState {Mode = "DrawMode_LineStrip", First = 1, Num = #v}
                end
            }
        }
    }
}

return t
