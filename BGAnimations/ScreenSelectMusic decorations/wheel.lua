local t = Def.ActorFrame {Name = "WheelFile"}

local numWheelItems = 10 -- 10 visible items

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
}

local wheelItemTextSize = 0.62
local wheelHeaderTextSize = 1.2
local textzoomfudge = 5 -- used in maxwidth to allow for gaps when squishing text
local headerFudge = 5 -- used to make the header slightly bigger to account for ??? vertical gaps

-- functionally create each item base because they are identical (BG and divider)
local function wheelItemBase()
    return Def.ActorFrame {
        Name = "WheelItemBase",
        Def.Quad {
            Name = "ItemBG",
            InitCommand = function(self)
                self:zoomto(actuals.Width, actuals.ItemHeight)
                self:diffuse(color("0,0,0,0.8"))
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

local function songBannerSetter(self, song)
    if song then
        local bnpath = song:GetBannerPath()
        if not bnpath then
            bnpath = THEME:GetPathG("Common", "fallback banner")
        end
        self:LoadBackground(bnpath)
    end
end

local function groupBannerSetter(self, group)
    local bnpath = SONGMAN:GetSongGroupBannerPath(group)
    if not bnpath or bnpath == "" then
        bnpath = THEME:GetPathG("Common", "fallback banner")
    end
    self:LoadBackground(bnpath)
end

t[#t+1] = Def.ActorFrame {
    Name = "WheelContainer",
    InitCommand = function(self)
        -- push from top left of screen, this position is CENTER of the wheel X/Y
        -- also for some odd reason we have to move down by half a wheelItem....
        self:xy(actuals.LeftGap + actuals.Width / 2, actuals.UpperGap + actuals.HeaderHeight + actuals.Height / 2 + actuals.ItemHeight / 2)
        SCREENMAN:set_input_redirected(PLAYER_1, true)
    end,
    OnCommand = function(self)
    end,

    -- because of the above, all of the X/Y positions are "relative" to center of the wheel
    -- ugh
    MusicWheel:new({
        count = numWheelItems,
        songActorBuilder = function() return Def.ActorFrame {
            wheelItemBase(),
            LoadFont("Common Normal") .. {
                Name = "Title",
                InitCommand = function(self)
                    self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                    self:y(-actuals.ItemHeight / 2 + actuals.ItemTextUpperGap)
                    self:strokecolor(color("1,1,1,1"))
                    self:zoom(wheelItemTextSize)
                    self:halign(0)
                    self:maxwidth(actuals.ItemDividerLength / wheelItemTextSize - textzoomfudge)
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
                    self:zoom(wheelItemTextSize)
                    self:halign(0)
                    self:maxwidth(actuals.ItemDividerLength / wheelItemTextSize - textzoomfudge)
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
                    self:zoom(wheelItemTextSize)
                    self:halign(0)
                    self:maxwidth(actuals.ItemDividerLength / wheelItemTextSize - textzoomfudge)
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
                end,
                BeginCommand = function(self)
                    self:GetParent().Banner = self
                end
            }
        }
        end,
        groupActorBuilder = function() return Def.ActorFrame {
            wheelItemBase(),
            LoadFont("Common Normal") .. {
                Name = "GroupName",
                InitCommand = function(self)
                    self:x(actuals.Width / 2 - actuals.ItemDividerLength)
                    self:y(-actuals.ItemHeight / 2 + actuals.ItemTextUpperGap)
                    self:zoom(wheelItemTextSize)
                    self:halign(0)
                    self:maxwidth(actuals.ItemDividerLength / wheelItemTextSize - textzoomfudge)
                end,
                BeginCommand = function(self)
                    self:GetParent().Title = self
                end
            },
            Def.Sprite {
                Name = "Banner",
                InitCommand = function(self)
                    -- y is already set: relative to "center"
                    self:x(-actuals.Width / 2):halign(0)
                    self:scaletoclipped(actuals.BannerWidth, actuals.ItemHeight)
                end,
                BeginCommand = function(self)
                    self:GetParent().Banner = self
                end
            }
        }
        end,
        songActorUpdater = function(songFrame, song)
            songFrame.Title:settext(song:GetDisplayMainTitle())
            songFrame.SubTitle:settext(song:GetDisplaySubTitle())
            songFrame.Artist:settext("~"..song:GetDisplayArtist())
            songBannerSetter(songFrame.Banner, song)
        end,
        groupActorUpdater = function(groupFrame, packName)
            groupFrame.Title:settext(packName)
            groupBannerSetter(groupFrame.Banner, packName)
        end,
        frameTransformer = function(frame, offsetFromCenter, index, total)
            frame:y(offsetFromCenter * actuals.ItemHeight)
        end
    }),
    Def.ActorFrame {
        Name = "WheelHeader",
        InitCommand = function(self)
            -- frame position is now center of header
            self:y(-actuals.Height / 2 - actuals.HeaderHeight / 2 - actuals.ItemHeight / 2)
        end,
        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:zoomto(actuals.Width, actuals.HeaderHeight + headerFudge)
                self:diffuse(color("0,0,0,0.8"))
            end
        },
        Def.Sprite {
            Name = "GroupBanner",
            InitCommand = function(self)
                self:x(-actuals.Width / 2):halign(0)
                self:scaletoclipped(actuals.HeaderBannerWidth, actuals.HeaderHeight)
                self:LoadBackground(THEME:GetPathG("Common", "fallback banner"))
            end
        },
        LoadFont("Common Normal") .. {
            Name = "GroupName",
            InitCommand = function(self)
                self:xy(-actuals.Width / 2 + actuals.HeaderBannerWidth + actuals.HeaderTextLeftGap, -actuals.HeaderHeight / 2 + actuals.HeaderTextUpperGap)
                self:halign(0)
                self:zoom(wheelHeaderTextSize)
                self:maxwidth((actuals.Width - actuals.HeaderBannerWidth - actuals.HeaderTextLeftGap) / wheelHeaderTextSize - textzoomfudge)
                self:settext("Placeholder Pack Name")
            end
        },
        LoadFont("Common Normal") .. {
            Name = "GroupInfo",
            InitCommand = function(self)
                self:xy(-actuals.Width / 2 + actuals.HeaderBannerWidth + actuals.HeaderTextLeftGap, actuals.HeaderHeight / 2 - actuals.HeaderTextLowerGap)
                self:halign(0)
                self:zoom(wheelHeaderTextSize)
                self:maxwidth((actuals.Width - actuals.HeaderBannerWidth - actuals.HeaderTextLeftGap) / wheelHeaderTextSize - textzoomfudge)
                self:settext("200 Files (Average MSD: 13.37)")
            end
        }
    }
}





return t