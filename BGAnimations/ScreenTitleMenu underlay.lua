local t = Def.ActorFrame {Name = "UnderlayFile"}

t[#t+1] = Def.Quad {
    Name = "RightBG",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
        self:diffuse(color("#333333"))
    end
}

-- the grid image we are using is 200x200
-- determine how many images we need to load to cover the screen
local imageheight = 200 -- unused
local imagewidth = 200 -- unused
local adjustedsize = 200 / 1080 * SCREEN_HEIGHT

local verticalcount = math.ceil(SCREEN_HEIGHT / adjustedsize)
local horizontalcount = math.ceil(SCREEN_WIDTH / adjustedsize)

for i = 1,verticalcount do
    for j = 1, horizontalcount do
        t[#t+1] = Def.Sprite {
            Name = "BGCheckerboard_"..i.."_"..j,
            Texture = THEME:GetPathG("", "bg-pattern"),
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy((j-1) * adjustedsize, (i-1) * adjustedsize)
                self:zoomto(adjustedsize, adjustedsize)
            end
        }
    end
end




local gradientwidth = 1104 / 1920 * SCREEN_WIDTH
local gradientheight = SCREEN_HEIGHT
local separatorxpos = 814 / 1920 * SCREEN_WIDTH -- basically the top right edge of the gradient
local separatorthickness = 22 / 1920 * SCREEN_WIDTH -- very slightly fudged due to measuring a diagonal line
local separatorlength = math.sqrt(SCREEN_HEIGHT * SCREEN_HEIGHT + (gradientwidth - separatorxpos) * (gradientwidth - separatorxpos)) + 10 -- hypotenuse


t[#t+1] = Def.Sprite {
    Name = "LeftBG",
    Texture = THEME:GetPathG("", "title-gradient"),
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(gradientwidth, gradientheight)
    end
}

t[#t+1] = Def.Quad {
    Name = "SeparatorShadow",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(separatorthickness * 3, separatorlength)
        self:x(separatorxpos - separatorthickness)
        self:diffuse(color("0,0,0,1"))
        self:fadeleft(1)
        self:faderight(1)
        local ang = math.atan((gradientwidth - separatorxpos) / separatorlength)
        self:rotationz(-math.deg(ang))
    end
}

t[#t+1] = Def.Quad {
    Name = "Separator",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(separatorthickness, separatorlength)
        self:x(separatorxpos)
        local ang = math.atan((gradientwidth - separatorxpos) / separatorlength)
        self:rotationz(-math.deg(ang))
    end
}



local logoFrameUpperGap = 39 / 1080 * SCREEN_HEIGHT -- from top edge to logo
local logoFrameLeftGap = 61 / 1920 * SCREEN_WIDTH -- from left edge to logo
local logoNameLeftGap = 33 / 1920 * SCREEN_WIDTH -- from end of logo to left of text
local logoThemeNameLeftGap = 28 / 1920 * SCREEN_WIDTH -- from end of logo to left of text
local logoThemeNameUpperGap = 60 / 1080 * SCREEN_HEIGHT -- from top of name text to top of theme text
local logosourceHeight = 133
local logosourceWidth = 102
local logoratio = math.min(1920 / SCREEN_WIDTH, 1080 / SCREEN_HEIGHT)
local logoH, logoW = getHWKeepAspectRatio(logosourceHeight, logosourceWidth, logosourceWidth / logosourceWidth)

local nameTextSize = 1
local themenameTextSize = 1

t[#t+1] = Def.ActorFrame {
    Name = "LogoFrame",
    InitCommand = function(self)
        self:xy(logoFrameLeftGap, logoFrameUpperGap)
    end,

    Def.Sprite {
        Name = "Logo",
        Texture = THEME:GetPathG("", "Logo"),
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(logoW, logoH)
        end
    },
    LoadFont("Common Normal") .. {
        Name = "GameName",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:x(logoNameLeftGap + logoW)
            self:settext("ETTERNA")
        end
    },
    LoadFont("Common Normal") .. {
        Name = "ThemeName",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:xy(logoThemeNameLeftGap + logoW, logoThemeNameUpperGap)
            self:settext("ThemeName")
        end
    }
}


local selectorHeight = 37 / 1080 * SCREEN_HEIGHT
local selectorWidth = 574 / 1920 * SCREEN_WIDTH
local choiceTable = strsplit(THEME:GetMetric("ScreenTitleMenu", "ChoiceNames"), ",")

t[#t+1] = Def.ActorFrame {
    Name = "SelectionFrame",
    BeginCommand = function(self)
        -- i love hacks.
        -- this makes all positioning and everything relative to the scroller actorframe
        -- because we cant actually make the scroller we want
        -- so we do this way
        -- and this is the actorframe responsible for the highlight on the title screen choices.
        self:SetFakeParent(SCREENMAN:GetTopScreen():GetChild("Scroller"))
    end,
    MenuSelectionChangedMessageCommand = function(self)
        local i = self:GetFakeParent():GetDestinationItem() + 1
        local actorScroller = self:GetFakeParent():GetChild("ScrollChoice"..choiceTable[i])

        self:finishtweening()
        self:smooth(0.03)
        self:y(actorScroller:GetY())
    end,

    Def.Sprite {
        Name = "Fade",
        Texture = THEME:GetPathG("", "selectorFade"),
        BeginCommand = function(self)
            self:x(-selectorHeight)
            self:halign(0)
            self:zoomto(selectorWidth, selectorHeight)
        end
    },
    Def.Sprite {
        Name = "Triangle",
        Texture = THEME:GetPathG("", "Triangle"),
        BeginCommand = function(self)
            self:x(-selectorHeight)
            self:halign(0)
            self:zoomto(selectorHeight, selectorHeight)
        end
    }
}


return t