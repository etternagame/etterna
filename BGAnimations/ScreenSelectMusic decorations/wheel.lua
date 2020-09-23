local t = Def.ActorFrame {Name = "WheelFile"}

 -- 11 visible items (top is a group header)
 -- an unfortunate amount of code is reliant on the fact that there are 11 items
 -- but thankfully everything works fine if you change it
 -- ... the header wont look very good if you push it off the screen though
local numWheelItems = 11

local ratios = {
    LeftGap = 77 / 1920,
    UpperGap = 135 / 1080, -- distance from top of screen, not info frame
    LowerGap = 12 / 1080, -- expected, maybe unused
    Width = 883 / 1920,
    Height = 827 / 1080, -- does not include the header
    ItemHeight = 82 / 1080, -- 80 + 2 to account for half of the upper and lower item dividers
    ItemDividerThickness = 2 / 1080,
    ItemDividerLength = 600 / 1920,
    ItemGradeWidth = 163 / 1920,
    ItemGradeHeight = 23 / 1080, -- 23 + 2 for a 1px shadow on top and bottom
    ItemGradeLowerGap = 11 / 1080, -- gap between center of divider to inside of grade shadow
    ItemTextUpperGap = 20 / 1080, -- distance from top of item to center of title text
    ItemTextLowerGap = 18 / 1080, -- distance from center of divider to center of author text
    ItemTextCenterDistance = 40 / 1080, -- distance from lower (divider center) to center of subtitle
    BannerWidth = 265 / 1920,
    BannerItemGap = 18 / 1920, -- gap between banner and item text/dividers
    HeaderHeight = 105 / 1080,
    HeaderBannerWidth = 336 / 1920,
    HeaderTextUpperGap = 30 / 1080, -- distance from top edge to center of text
    HeaderTextLowerGap = 27 / 1080, -- distance from bottom edge to center of text
    HeaderTextLeftGap = 30 / 1920, -- distance from edge of banner to left of text

    -- controls the width of the mouse wheel scroll box, should be the same number as the general box X position
    -- (found in generalBox.lua)
    GeneralBoxLeftGap = 1056 / 1920, -- distance from left edge to the left edge of the general box

    ScrollBarWidth = 18 / 1920,
    ScrollBarHeight = 933 / 1080,
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
    ItemGradeWidth = ratios.ItemGradeWidth * SCREEN_WIDTH,
    ItemGradeHeight = ratios.ItemGradeHeight * SCREEN_HEIGHT,
    ItemGradeLowerGap = ratios.ItemGradeLowerGap * SCREEN_HEIGHT,
    ItemTextUpperGap = ratios.ItemTextUpperGap * SCREEN_HEIGHT,
    ItemTextLowerGap = ratios.ItemTextLowerGap * SCREEN_HEIGHT,
    ItemTextCenterDistance = ratios.ItemTextCenterDistance * SCREEN_HEIGHT,
    BannerWidth = ratios.BannerWidth * SCREEN_WIDTH,
    BannerItemGap = ratios.BannerItemGap * SCREEN_WIDTH,
    HeaderHeight = ratios.HeaderHeight * SCREEN_HEIGHT,
    HeaderBannerWidth = ratios.HeaderBannerWidth * SCREEN_WIDTH,
    HeaderTextUpperGap = ratios.HeaderTextUpperGap * SCREEN_HEIGHT,
    HeaderTextLowerGap = ratios.HeaderTextLowerGap * SCREEN_HEIGHT,
    HeaderTextLeftGap = ratios.HeaderTextLeftGap * SCREEN_WIDTH,
    GeneralBoxLeftGap = ratios.GeneralBoxLeftGap * SCREEN_WIDTH,
    ScrollBarWidth = ratios.ScrollBarWidth * SCREEN_WIDTH,
    ScrollBarHeight = ratios.ScrollBarHeight * SCREEN_HEIGHT,
}

local wheelItemTextSize = 0.62
local wheelItemTitleTextSize = 0.65
local wheelItemSubTitleTextSize = 0.62
local wheelItemArtistTextSize = 0.62
local wheelItemGroupTextSize = 0.75
local wheelItemGroupInfoTextSize = 0.62
local wheelHeaderTextSize = 1.2
local textzoomfudge = 5 -- used in maxwidth to allow for gaps when squishing text
local headerFudge = 5 -- used to make the header slightly bigger to account for ??? vertical gaps

-- check to see if a stepstype is countable for average diff reasons
local function countableStepsType(stepstype)
    local thelist = {
        stepstype_dance_single = true,
        stepstype_dance_solo = true,
    }
    return thelist[stepstype:lower()] ~= nil
end

local packCounts = SONGMAN:GetSongGroupNames()
local function packCounter()
    packCounts = {}
    for i, song in ipairs(SONGMAN:GetAllSongs()) do
        local pack = song:GetGroupName()
        local x = packCounts[pack]
        packCounts[pack] = x and x + 1 or 1
    end
end
packCounter()

local avgDiffByPack = {}
local function calcAverageDiffByPack()
    avgDiffByPack = {}
    for pack, _ in pairs(packCounts) do
        local chartcount = 0
        avgDiffByPack[pack] = 0
        for i, song in ipairs(SONGMAN:GetSongsInGroup(pack)) do
            for _, chart in ipairs(song:GetAllSteps()) do
                if countableStepsType(chart:GetStepsType()) then
                    chartcount = chartcount + 1
                    avgDiffByPack[pack] = avgDiffByPack[pack] + chart:GetMSD(1, 1)
                end
            end
        end
        if chartcount > 0 then
            avgDiffByPack[pack] = avgDiffByPack[pack] / chartcount
        end
    end
end
calcAverageDiffByPack()

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
            end,
            HeaderOnCommand = function(self, params)
                self:smooth(0.05)
                self:zoomto(actuals.Width, actuals.HeaderHeight + headerFudge)
                self:diffusealpha(0.8)
            end,
            HeaderOffCommand = function(self)
                self:smooth(0.05)
                self:zoomto(actuals.Width, actuals.ItemHeight)
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
            end,
            HeaderOnCommand = function(self)
                self:smooth(0.05)
                self:diffusealpha(0)
            end,
            HeaderOffCommand = function(self)
                self:smooth(0.05)
                self:diffusealpha(1)
            end
        },
    }
end

local function songBannerSetter(self, song)
    if song then
        local bnpath = song:GetBannerPath()
        if not bnpath then
            bnpath = THEME:GetPathG("Common", "fallback banner")
        end
        if self.bnpath ~= bnpath then
            self:Load(bnpath)
        end
        self.bnpath = bnpath
    end
end

local function groupBannerSetter(self, group)
    local bnpath = SONGMAN:GetSongGroupBannerPath(group)
    if not bnpath or bnpath == "" then
        bnpath = THEME:GetPathG("Common", "fallback banner")
    end
    if self.bnpath ~= bnpath then
        self:Load(bnpath)
    end
    self.bnpath = bnpath
end

local function songActorUpdater(songFrame, song)
    songFrame.Title:settext(song:GetDisplayMainTitle())
    songFrame.SubTitle:settext(song:GetDisplaySubTitle())
    songFrame.Artist:settext("~"..song:GetDisplayArtist())
    songBannerSetter(songFrame.Banner, song)
end

local function groupActorUpdater(groupFrame, packName, packCount, packAverageDiff)
    if packCount == nil then
        packCount = packCounts[packName] or 0
    end
    if packAverageDiff == nil then
        packAverageDiff = avgDiffByPack[packName] or 0
    end
    groupFrame.Title:settext(packName)
    groupFrame.GroupInfo:playcommand("Setinfo", {count = packCount, avg = packAverageDiff})
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
                self:strokecolor(color("1,1,1,1"))
                self:zoom(wheelItemTitleTextSize)
                self:halign(0)
                self:maxwidth(actuals.ItemDividerLength / wheelItemTitleTextSize - textzoomfudge)
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
                self:maxwidth(actuals.ItemDividerLength / wheelItemSubTitleTextSize - textzoomfudge)
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
                self:maxwidth(actuals.ItemDividerLength / wheelItemArtistTextSize - textzoomfudge)
            end,
            BeginCommand = function(self)
                self:GetParent().Artist = self
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
                self:maxwidth(actuals.ItemDividerLength / wheelItemGroupTextSize - textzoomfudge)
                -- we make the background of groups fully opaque to distinguish them from songs
                self:GetParent():GetChild("WheelItemBase"):GetChild("ItemBG"):diffusealpha(1)
            end,
            BeginCommand = function(self)
                self:GetParent().Title = self
            end,
            HeaderOnCommand = function(self)
                self:smooth(0.05)
                self:xy(-actuals.Width / 2 + actuals.HeaderBannerWidth + actuals.HeaderTextLeftGap, -actuals.HeaderHeight / 2 + actuals.HeaderTextUpperGap)
                self:zoom(wheelHeaderTextSize)
                self:maxwidth((actuals.Width - actuals.HeaderBannerWidth - actuals.HeaderTextLeftGap) / wheelHeaderTextSize - textzoomfudge)
            end,
            HeaderOffCommand = function(self)
                self:zoom(wheelItemGroupTextSize)
                self:maxwidth(actuals.ItemDividerLength / wheelItemGroupTextSize - textzoomfudge)
                self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                self:y(-actuals.ItemHeight / 2 + actuals.ItemTextUpperGap)
            end
        },
        LoadFont("Common Normal") .. {
            Name = "GroupInfo",
            InitCommand = function(self)
                self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                self:y(actuals.ItemHeight / 2 - actuals.ItemTextLowerGap)
                self:zoom(wheelItemGroupInfoTextSize)
                self:halign(0)
                self:maxwidth(actuals.ItemDividerLength / wheelItemGroupInfoTextSize - textzoomfudge)
                self.avg = 0
                self.count = 0
            end,
            BeginCommand = function(self)
                self:GetParent().GroupInfo = self
            end,
            SetinfoCommand = function(self, params)
                self.count = params.count
                self.avg = params.avg
                self:playcommand("UpdateText")
            end,
            UpdateTextCommand = function(self)
                if self:GetParent():GetParent().sticky then
                    self:settextf("%d Files (Average MSD: %5.2f)", self.count, self.avg)
                else
                    self:settextf("%d Files (Avg %5.2f)", self.count, self.avg)
                end
            end,
            HeaderOnCommand = function(self)
                self:playcommand("UpdateText")
                self:smooth(0.05)
                self:xy(-actuals.Width / 2 + actuals.HeaderBannerWidth + actuals.HeaderTextLeftGap, actuals.HeaderHeight / 2 - actuals.HeaderTextLowerGap)
                self:zoom(wheelHeaderTextSize)
                self:maxwidth((actuals.Width - actuals.HeaderBannerWidth - actuals.HeaderTextLeftGap) / wheelHeaderTextSize - textzoomfudge)
            end,
            HeaderOffCommand = function(self)
                self:playcommand("UpdateText")
                self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                self:y(actuals.ItemHeight / 2 - actuals.ItemTextLowerGap)
                self:zoom(wheelItemGroupInfoTextSize)
                self:maxwidth(actuals.ItemDividerLength / wheelItemGroupInfoTextSize - textzoomfudge)
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
            end,
            HeaderOnCommand = function(self)
                self:smooth(0.05)
                self:scaletoclipped(actuals.HeaderBannerWidth, actuals.HeaderHeight + headerFudge)
            end,
            HeaderOffCommand = function(self)
                self:smooth(0.05)
                self:scaletoclipped(actuals.BannerWidth, actuals.ItemHeight)
            end
        }
    }
end

local openedGroup = ""
local onAnAdventure = false -- true if scrolling on groups
local firstUpdate = true -- for not triggering the header switch on init

t[#t+1] = Def.ActorFrame {
    Name = "WheelContainer",
    InitCommand = function(self)
        -- push from top left of screen, this position is CENTER of the wheel X/Y
        -- also for some odd reason we have to move down by half a wheelItem....
        self:xy(actuals.LeftGap + actuals.Width / 2, actuals.UpperGap + actuals.HeaderHeight + actuals.Height / 2 + actuals.ItemHeight / 2)
        SCREENMAN:set_input_redirected(PLAYER_1, true)
    end,
    BeginCommand = function(self)
        -- hide the old musicwheel
        SCREENMAN:GetTopScreen():GetMusicWheel():visible(false)
    end,
    OpenedGroupMessageCommand = function(self, params)
        openedGroup = params.group
        onAnAdventure = false
    end,
    ClosedGroupMessageCommand = function(self)
        openedGroup = ""
        onAnAdventure = true
    end,
    ScrolledIntoGroupMessageCommand = function(self, params)
        onAnAdventure = false
    end,
    ScrolledOutOfGroupMessageCommand = function(self)
        onAnAdventure = true
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
                    self:y(-actuals.ItemHeight)
                    self:zoomto(actuals.Width - actuals.BannerWidth, actuals.ItemHeight)
                    self:diffusealpha(0.1)
                    self:diffuseramp()
                end
            }
        }
        end,
        songActorUpdater = songActorUpdater,
        groupActorUpdater = groupActorUpdater,
        frameTransformer = function(frame, offsetFromCenter, index, total)
            if index == 1 and openedGroup ~= nil then
                if openedGroup == frame:GetChild("GroupFrame").Title:GetText() then
                    if firstUpdate then
                        firstUpdate = false
                        frame:y(offsetFromCenter * actuals.ItemHeight)
                    elseif not frame.sticky and not onAnAdventure then
                        frame.sticky = true
                        frame:playcommand("HeaderOn", {offsetFromCenter = -math.ceil(numWheelItems / 2)})
                    elseif onAnAdventure then
                        frame:y(offsetFromCenter * actuals.ItemHeight)
                    end
                else
                    if frame.sticky then
                        frame.sticky = false
                        frame:finishtweening()
                        frame:smooth(0.05)
                        frame:playcommand("HeaderOff")
                    end
                    frame:y(offsetFromCenter * actuals.ItemHeight)
                end
            else
                frame:y(offsetFromCenter * actuals.ItemHeight)
            end
        end,
        frameBuilder = function()
            local f
            f = Def.ActorFrame {
                Name = "ItemFrame",
                InitCommand = function(self)
                    f.actor = self
                end,
                HeaderOnCommand = function(self, params)
                    -- if the opened group is not real, then stop
                    -- this happens on init basically
                    if openedGroup == "" then
                        return
                    end
                    self:finishtweening()
                    self:smooth(0.05)
                    self.g:visible(true)
                    self.s:visible(false)
                    self:y(params.offsetFromCenter * actuals.ItemHeight - (actuals.HeaderHeight - actuals.ItemHeight) + headerFudge)
                end,
                UIElements.QuadButton(1) .. {
                    Name = "WheelItemClickBox",
                    InitCommand = function(self)
                        self:diffusealpha(0)
                        self:zoomto(actuals.Width, actuals.ItemHeight)
                    end,
                    MouseDownCommand = function(self, params)
                        if not self:GetParent().sticky then
                            if params.event == "DeviceButton_left mouse button" then
                                local index = self:GetParent().index
                                local distance = math.floor(index - numWheelItems / 2)
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
                        else
                            -- this means we clicked the header
                            -- hidden feature: random song in group if doing that
                            local group = self:GetParent().g.Title:GetText()
                            if group == nil or group == "" then return end
                            local songs = SONGMAN:GetSongsInGroup(group)
                            if #songs == 0 then return end
                            local song = songs[math.random(#songs)]
                            SCREENMAN:GetTopScreen():GetChild("WheelFile"):playcommand("FindSong", {song = song})
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
                -- dont mess around with sticky'd frames
                if not frame.sticky then
                    local s = frame.s
                    s:visible(true)
                    local g = frame.g
                    g:visible(false)
                    songActorUpdater(s, songOrPack)
                end
            else
                -- This is a group
                -- dont mess with non sticky'd frames
                if not frame.sticky then
                    local s = frame.s
                    s:visible(false)
                    local g = (frame.g)
                    g:visible(true)
                    groupActorUpdater(g, songOrPack, packCounts[songOrPack], avgDiffByPack[songOrPack])
                end
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
            self:zoomto(actuals.GeneralBoxLeftGap, actuals.Height + actuals.HeaderHeight * 1.2)
        end,
        MouseScrollMessageCommand = function(self, params)
            if isOver(self) then
                if params.direction == "Up" then
                    self:GetParent():GetChild("Wheel"):playcommand("Move", {direction = -1})
                else
                    self:GetParent():GetChild("Wheel"):playcommand("Move", {direction = 1})
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
            self:y(-actuals.ItemHeight * numWheelItems / 2 - actuals.HeaderHeight - headerFudge)
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
    }
}


return t
