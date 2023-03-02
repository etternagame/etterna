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

local translations = {
    GameName = THEME:GetString("Common", "Etterna"):upper(),
    UpdateAvailable = THEME:GetString("ScreenTitleMenu", "UpdateAvailable"),
    By = THEME:GetString("ScreenTitleMenu", "By"),
}

local nameTextSize = 0.9
local themenameTextSize = 0.8
local versionTextSize = 0.5
local versionTextSizeSmall = 0.25
local animationSeconds = 0.5 -- the intro animation
local updateDownloadIconSize = 30 / 1080 * SCREEN_HEIGHT

-- information for the update button
local latest = tonumber((DLMAN:GetLastVersion():gsub("[.]", "", 1)))
local current = tonumber((GAMESTATE:GetEtternaVersion():gsub("[.]", "", 1)))
if latest ~= nil and current ~= nil and latest > current then
    updateRequired = true
end

-- if you go to the help screen this puts you back on the main menu
SCUFF.helpmenuBackout = "ScreenTitleMenu"

-- for the secret jukebox button
local playingMusic = {}
local playingMusicCounter = 1

local buttonHoverAlpha = 0.6
local function hoverfunc(self)
    if self:IsInvisible() then return end
    if isOver(self) then
        self:diffusealpha(buttonHoverAlpha)
    else
        self:diffusealpha(1)
    end
end

local function clickDownload(self, params)
    if self:IsInvisible() then return end
    if not params or params.event ~= "DeviceButton_left mouse button" then return end
    GAMESTATE:ApplyGameCommand("urlnoexit,https://github.com/etternagame/etterna/releases;text,GitHub")
end

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
        Texture = THEME:GetPathG("", "title-solid"),
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(gradientwidth, gradientheight)
            registerActorToColorConfigElement(self, "title", "GradientColor1")
        end
    },
    Def.Sprite {
        Name = "LeftBGGradient",
        Texture = THEME:GetPathG("", "title-gradient"),
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(gradientwidth, gradientheight)
            registerActorToColorConfigElement(self, "title", "GradientColor2")
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
            Name = "LogoTriangle",
            Texture = THEME:GetPathG("", "Logo-Triangle"),
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(logoW, logoH)
                registerActorToColorConfigElement(self, "title", "LogoTriangle")
            end,
        },
        UIElements.SpriteButton(100, 1, THEME:GetPathG("", "Logo-E")) .. {
            Name = "Logo",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(logoW, logoH)
                registerActorToColorConfigElement(self, "title", "LogoE")
            end,
            MouseOverCommand = function(self)
                self:GetParent():GetChild("LogoTriangle"):diffusealpha(buttonHoverAlpha)
                self:diffusealpha(buttonHoverAlpha)
            end,
            MouseOutCommand = function(self)
                self:GetParent():GetChild("LogoTriangle"):diffusealpha(1)
                self:diffusealpha(1)
            end,
            MouseDownCommand = function(self, params)
                if params.event == "DeviceButton_left mouse button" then
                    local function startSong()
                        local sngs = SONGMAN:GetAllSongs()
                        if #sngs == 0 then ms.ok("No songs to play") return end

                        local s = sngs[math.random(#sngs)]
                        local p = s:GetMusicPath()
                        local l = s:MusicLengthSeconds()
                        local top = SCREENMAN:GetTopScreen()

                        local thisSong = playingMusicCounter
                        playingMusic[thisSong] = true

                        SOUND:StopMusic()
                        SOUND:PlayMusicPart(p, 0, l)
            
                        ms.ok("NOW PLAYING: "..s:GetMainTitle() .. " | LENGTH: "..SecondsToMMSS(l))
            
                        top:setTimeout(
                            function()
                                if not playingMusic[thisSong] then return end
                                playingMusicCounter = playingMusicCounter + 1
                                startSong()
                            end,
                            l
                        )
            
                    end
            
                    SCREENMAN:GetTopScreen():setTimeout(function()
                            playingMusic[playingMusicCounter] = false
                            playingMusicCounter = playingMusicCounter + 1
                            startSong()
                        end,
                    0.1)
                else
                    SOUND:StopMusic()
                    playingMusic = {}
                    playingMusicCounter = playingMusicCounter + 1
                    ms.ok("Stopped music")
                end
            end,
        },
        LoadFont("Menu Bold") .. {
            Name = "GameName",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:x(logoNameLeftGap + logoW)
                self:zoom(nameTextSize)
                self:maxwidth((separatorxpos - (logoNameLeftGap + logoW) - logoNameLeftGap) / nameTextSize)
                self:settext(translations["GameName"])
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
            InitCommand = function(self) -- happens first
                self:halign(0):valign(0)
                self:xy(versionNumberLeftGap, versionNumberUpperGap)
                self:zoom(versionTextSize)
                self:settext("V "..GAMESTATE:GetEtternaVersion())
                self:diffuse(COLORS:getTitleColor("PrimaryText"))
                self:diffusealpha(1)
            end
        },
        UIElements.TextToolTip(100, 1, "Menu Normal") .. {
            Name = "VersionUpdate",
            BeginCommand = function(self) -- happens second
                self:halign(0):valign(0)
                local vnc = self:GetParent():GetChild("VersionNumber")
                local bufferspace = 5 / 1920 * SCREEN_WIDTH
                self:xy(vnc:GetX() + vnc:GetZoomedWidth() + bufferspace, versionNumberUpperGap)
                self:zoom(versionTextSize)
                self:maxwidth(((gradientwidth - vnc:GetX() - vnc:GetZoomedWidth() - logoFrameLeftGap - separatorthickness) / versionTextSize))
                self:settextf("- %s (%s)", translations["UpdateAvailable"], DLMAN:GetLastVersion())
                self:diffuse(COLORS:getTitleColor("UpdateText"))
                self:diffusealpha(1)
                self:visible(false)

                if updateRequired then
                    self:visible(true)
                end
            end,
            MouseOutCommand = hoverfunc,
            MouseOverCommand = hoverfunc,
            MouseDownCommand = clickDownload,
        },
        UIElements.SpriteButton(100, 1, THEME:GetPathG("", "updatedownload")) .. {
            Name = "VersionUpdateDownload",
            OnCommand = function(self) -- happens third
                self:halign(0):valign(0)
                local vuc = self:GetParent():GetChild("VersionUpdate")
                local bufferspace = 5 / 1920 * SCREEN_WIDTH
                self:xy(vuc:GetX() + vuc:GetZoomedWidth() + bufferspace, versionNumberUpperGap)
                self:zoomto(updateDownloadIconSize, updateDownloadIconSize)
                self:diffuse(COLORS:getTitleColor("UpdateText"))
                self:diffusealpha(1)
                self:visible(false)

                if updateRequired then
                    self:visible(true)
                end
            end,
            MouseOutCommand = hoverfunc,
            MouseOverCommand = hoverfunc,
            MouseDownCommand = clickDownload,
        },

        LoadFont("Menu Normal") .. {
            Name = "ThemeVersionAndCredits",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(versionNumberLeftGap, themeVersionUpperGap)
                self:maxwidth((gradientwidth - versionNumberLeftGap - logoFrameLeftGap - separatorthickness) / versionTextSizeSmall)
                self:zoom(versionTextSizeSmall)
                self:settext("("..getThemeName().." v"..getThemeVersion().."@"..getThemeDate().." " .. translations["By"] .. " "..getThemeAuthor()..")")
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
            self:diffuse(color("#805faf"))
            registerActorToColorConfigElement(self, "title", "ItemTriangle")
        end
    }
}

return t
