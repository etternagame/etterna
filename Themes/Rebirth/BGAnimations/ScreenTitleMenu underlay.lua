local t = Def.ActorFrame {
    Name = "UnderlayFile",
    BeginCommand = function(self)
        -- if theres no songs loaded send them to the bundle screen once
        if SONGMAN:GetNumSongGroups() == 0 and not SCUFF.visitedCoreBundleSelect then
            SCUFF.visitedCoreBundleSelect = true
            SCREENMAN:SetNewScreen("ScreenCoreBundleSelect")
        end
    end,
}

t[#t+1] = LoadActor(THEME:GetPathG("Title", "BG"))

local gradientwidth = 1104 / 1920 * SCREEN_WIDTH
local gradientheight = SCREEN_HEIGHT
local separatorxpos = 814 / 1920 * SCREEN_WIDTH -- basically the top right edge of the gradient
local separatorthickness = 22 / 1920 * SCREEN_WIDTH -- very slightly fudged due to measuring a diagonal line
local separatorlength = math.sqrt(SCREEN_HEIGHT * SCREEN_HEIGHT + (gradientwidth - separatorxpos) * (gradientwidth - separatorxpos)) + 10 -- hypotenuse

local logoFrameUpperGap = 39 / 1080 * SCREEN_HEIGHT -- from top edge to logo
local logoFrameLeftGap = 61 / 1920 * SCREEN_WIDTH -- from left edge to logo
local logoNameLeftGap = 33 / 1920 * SCREEN_WIDTH -- from end of logo to left of text
local logoThemeNameLeftGap = 33 / 1920 * SCREEN_WIDTH -- from end of logo to left of text
local logoThemeNameUpperGap = 67 / 1080 * SCREEN_HEIGHT -- from top of name text to top of theme text
local logosourceHeight = 133
local logosourceWidth = 102
local logoratio = math.min(1920 / SCREEN_WIDTH, 1080 / SCREEN_HEIGHT)
local logoH, logoW = getHWKeepAspectRatio(logosourceHeight, logosourceWidth, logosourceWidth / logosourceWidth)

local versionNumberLeftGap = 5 / 1920 * SCREEN_WIDTH
local versionNumberUpperGap = 980 / 1080 * SCREEN_HEIGHT
local themeVersionUpperGap = 1015 / 1080 * SCREEN_HEIGHT

local nameTextSize = 0.9
local themenameTextSize = 0.8
local versionTextSize = 0.5
local versionTextSizeSmall = 0.25
local animationSeconds = 0.5 -- the intro animation

t[#t+1] = Def.ActorFrame {
    Name = "LeftSide",
    InitCommand = function(self)
        self:x(-SCREEN_WIDTH)
    end,
    BeginCommand = function(self)
        self:smooth(animationSeconds)
        self:x(0)
    end,

    Def.Sprite {
        Name = "LeftBG",
        Texture = THEME:GetPathG("", "title-gradient"),
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(gradientwidth, gradientheight)
        end
    },
    Def.Quad {
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
    },
    Def.Quad {
        Name = "Separator",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(separatorthickness, separatorlength)
            self:x(separatorxpos)
            local ang = math.atan((gradientwidth - separatorxpos) / separatorlength)
            self:rotationz(-math.deg(ang))
            self:diffuse(COLORS:getTitleColor("Separator"))
            self:diffusealpha(1)
        end
    },

    Def.ActorFrame {
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
        LoadFont("Menu Bold") .. {
            Name = "GameName",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:x(logoNameLeftGap + logoW)
                self:zoom(nameTextSize)
                self:maxwidth((separatorxpos - (logoNameLeftGap + logoW) - logoNameLeftGap) / nameTextSize)
                self:settext("ETTERNA")
                self:diffuse(COLORS:getTitleColor("PrimaryText"))
                self:diffusealpha(1)
            end
        },
        LoadFont("Menu Normal") .. {
            Name = "ThemeName",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(logoThemeNameLeftGap + logoW, logoThemeNameUpperGap)
                self:zoom(themenameTextSize)
                self:maxwidth((separatorxpos - (logoNameLeftGap + logoW) - logoThemeNameLeftGap) / themenameTextSize)
                self:settext(getThemeName())
                self:diffuse(COLORS:getTitleColor("PrimaryText"))
                self:diffusealpha(1)
            end
        },
        LoadFont("Menu Normal") .. {
            Name = "VersionNumber",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(versionNumberLeftGap, versionNumberUpperGap)
                self:zoom(versionTextSize)
                self:settext("V "..GAMESTATE:GetEtternaVersion())
                self:diffuse(COLORS:getTitleColor("PrimaryText"))
                self:diffusealpha(1)
            end
        },
        LoadFont("Menu Normal") .. {
            Name = "ThemeVersionAndCredits",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(versionNumberLeftGap, themeVersionUpperGap)
                self:maxwidth((gradientwidth - versionNumberLeftGap - logoFrameLeftGap - separatorthickness) / versionTextSizeSmall)
                self:zoom(versionTextSizeSmall)
                self:settext("("..getThemeName().." v"..getThemeVersion().."@"..getThemeDate().." by "..getThemeAuthor()..")")
                self:diffuse(COLORS:getTitleColor("SecondaryText"))
                self:diffusealpha(1)
            end
        }
    }
}




local scrollerX = 99 / 1920 * SCREEN_WIDTH
local scrollerY = 440 / 1920 * SCREEN_HEIGHT
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
        local scr = SCREENMAN:GetTopScreen():GetChild("Scroller")
        self:SetFakeParent(scr)

        -- move choices on screen
        -- we set it to start off screen using the metrics
        scr:smooth(animationSeconds)
        scr:x(scrollerX)
        scr:y(scrollerY)
    end,
    MenuSelectionChangedMessageCommand = function(self)
        local i = self:GetFakeParent():GetDestinationItem() + 1
        local actorScroller = self:GetFakeParent():GetChild("ScrollChoice"..choiceTable[i])

        self:finishtweening()
        self:smooth(0.05)
        self:y(actorScroller:GetY())
    end,

    Def.Sprite {
        Name = "Fade",
        Texture = THEME:GetPathG("", "selectorFade"),
        BeginCommand = function(self)
            self:x(-selectorHeight)
            self:halign(0)
            self:zoomto(selectorWidth, selectorHeight)
            self:queuecommand("UpdateWidth")
        end,
        UpdateWidthCommand = function(self)
            -- the minimum width is going to be about 10% longer than the longest choice text
            local widest = selectorWidth
            for name, child in pairs(self:GetParent():GetFakeParent():GetChildren()) do
                local w = child:GetChild("ScrollerText"):GetZoomedWidth()
                if w > widest - (selectorHeight * 2) then
                    widest = w + selectorHeight * 2
                end
            end
            if widest ~= selectorWidth then
                widest = widest * 1.1
            end
            self:zoomto(widest, selectorHeight)
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
