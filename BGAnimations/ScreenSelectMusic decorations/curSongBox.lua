local t = Def.ActorFrame {Name = "CurSongBoxFile"}

local ratios = {
    LeftGap = 1055 / 1920, -- distance from left side of screen to left side of frame
    TopGap = 135 / 1080, -- distance from top of screen to top of frame
    Height = 345 / 1080,
    Width = 777 / 1920,
    BannerHeight = 243 / 1080,
    LowerLipHeight = 34 / 1080,
    LeftTextLeftGap = 10 / 1920,
    DiffItemWidth = 60 / 1920,
    DiffItemHeight = 40 / 1080,
    DiffFrameLeftGap = 429 / 1920,
    DiffFrameRightGap = 11 / 1920,
    TextLowerGap1 = 6 / 1080, -- subtracting 2 here because thats about how much letters go "down"
    TextLowerGap2 = 39 / 1080, -- these gaps are from bottom frame to bottom text
    TextLowerGap3 = 75 / 1080,
    BPMTextRightGap = 62 / 1920,
    BPMNumberRightGap = 12 / 1920, -- from right edge to right edge of numbers
    BPMWidth = 50 / 1920, -- from right edge of bpm number to right edge of bpm text
    LengthTextRightGap = 197 / 1920,
    LengthNumberRightGap = 135 / 1920, -- from right edge to right edge of numbers
    LengthWidth = 62 / 1920, -- from right edge of len number to right edge of len text
}

local actuals = {
    LeftGap = ratios.LeftGap * SCREEN_WIDTH,
    TopGap = ratios.TopGap * SCREEN_HEIGHT,
    Height = ratios.Height * SCREEN_HEIGHT,
    Width = ratios.Width * SCREEN_WIDTH,
    BannerHeight = ratios.BannerHeight * SCREEN_HEIGHT,
    LowerLipHeight = ratios.LowerLipHeight * SCREEN_HEIGHT,
    LeftTextLeftGap = ratios.LeftTextLeftGap * SCREEN_WIDTH,
    DiffItemWidth = ratios.DiffItemWidth * SCREEN_WIDTH,
    DiffItemHeight = ratios.DiffItemHeight * SCREEN_HEIGHT,
    DiffFrameLeftGap = ratios.DiffFrameLeftGap * SCREEN_WIDTH,
    DiffFrameRightGap = ratios.DiffFrameRightGap * SCREEN_WIDTH,
    TextLowerGap1 = ratios.TextLowerGap1 * SCREEN_HEIGHT,
    TextLowerGap2 = ratios.TextLowerGap2 * SCREEN_HEIGHT,
    TextLowerGap3 = ratios.TextLowerGap3 * SCREEN_HEIGHT,
    BPMTextRightGap = ratios.BPMTextRightGap * SCREEN_WIDTH,
    BPMNumberRightGap = ratios.BPMNumberRightGap * SCREEN_WIDTH,
    BPMWidth = ratios.BPMWidth * SCREEN_WIDTH,
    LengthTextRightGap = ratios.LengthTextRightGap * SCREEN_WIDTH,
    LengthNumberRightGap = ratios.LengthNumberRightGap * SCREEN_WIDTH,
    LengthWidth = ratios.LengthWidth * SCREEN_WIDTH,
}

local textsize = 0.8
local textzoomFudge = 5

t[#t+1] = Def.ActorFrame {
    Name = "Frame",
    InitCommand = function(self)
        self:xy(actuals.LeftGap, actuals.TopGap)
    end,

    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(actuals.Width, actuals.Height)
            self:diffuse(color("0,0,0,0.6"))
        end
    },
    Def.Quad {
        Name = "LowerLip",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:y(actuals.Height)
            self:zoomto(actuals.Width, actuals.LowerLipHeight)
            self:diffuse(color("0,0,0,0.6"))
        end
    },
    Def.Sprite {
        Name = "Banner",
        Texture = THEME:GetPathG("Common", "fallback banner"),
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(actuals.Width, actuals.BannerHeight)
        end
    },
    LoadFont("Common Normal") .. {
        Name = "TitleAuthor",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:xy(actuals.LeftTextLeftGap, actuals.Height - actuals.TextLowerGap3)
            self:zoom(textsize)
            self:maxwidth((actuals.DiffFrameLeftGap - actuals.LeftTextLeftGap) / textsize - textzoomFudge)
            self:settext("Song Title - Song Author")
        end
    },
    LoadFont("Common Normal") .. {
        Name = "SubTitle",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:xy(actuals.LeftTextLeftGap, actuals.Height - actuals.TextLowerGap2)
            self:zoom(textsize)
            self:maxwidth((actuals.DiffFrameLeftGap - actuals.LeftTextLeftGap) / textsize - textzoomFudge)
            self:settext("Song SubTitle (1995)")
        end
    },
    LoadFont("Common Normal") .. {
        Name = "Rate",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:xy(actuals.LeftTextLeftGap, actuals.Height - actuals.TextLowerGap1)
            self:zoom(textsize)
            self:maxwidth((actuals.DiffFrameLeftGap - actuals.LeftTextLeftGap) / textsize - textzoomFudge)
            self:settext("6.9x")
        end
    },
    
    LoadFont("Common Normal") .. {
        Name = "LengthText",
        InitCommand = function(self)
            self:halign(1):valign(1)
            self:xy(actuals.Width - actuals.LengthTextRightGap, actuals.Height - actuals.TextLowerGap1)
            self:zoom(textsize)
            self:settext("LENGTH")
        end
    },
    LoadFont("Common Normal") .. {
        Name = "LengthNumbers",
        InitCommand = function(self)
            self:halign(1):valign(1)
            self:xy(actuals.Width - actuals.LengthNumberRightGap, actuals.Height - actuals.TextLowerGap1)
            self:zoom(textsize)
            self:maxwidth(actuals.LengthWidth / textsize - textzoomFudge)
            self:settext("55:55")
        end
    },
    
    LoadFont("Common Normal") .. {
        Name = "BPMText",
        InitCommand = function(self)
            self:halign(1):valign(1)
            self:xy(actuals.Width - actuals.BPMTextRightGap, actuals.Height - actuals.TextLowerGap1)
            self:zoom(textsize)
            self:settext("BPM")
        end
    },
    Def.BPMDisplay {
        File = THEME:GetPathF("Common", "Normal"),
        Name = "BPMDisplay",
        InitCommand = function(self)
            self:halign(1):valign(1)
            self:xy(actuals.Width - actuals.BPMNumberRightGap, actuals.Height - actuals.TextLowerGap1)
            self:zoom(textsize)
            self:maxwidth(actuals.BPMWidth / textsize - textzoomFudge)
        end
    }

}


return t