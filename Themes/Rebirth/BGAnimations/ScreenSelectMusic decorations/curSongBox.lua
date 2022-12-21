local displayScore
local lastHovered
local focused = true
local t = Def.ActorFrame {
    Name = "CurSongBoxFile",
    WheelSettledMessageCommand = function(self, params)
        -- update displayscore
        -- it sets to nil properly by itself
        displayScore = GetDisplayScore()

        lastHovered = params.hovered

        -- cascade visual update to everything
        self:playcommand("Set", {song = params.song, group = params.group, hovered = params.hovered, steps = params.steps})
    end,
    CurrentRateChangedMessageCommand = function(self)
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = GAMESTATE:GetCurrentSteps()})
    end,
    ChangedStepsMessageCommand = function(self, params)
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = params.steps})
    end,
    OptionUpdatedMessageCommand = function(self, params)
        if params and params.name == "Show Banners" then
            self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = GAMESTATE:GetCurrentSteps()})
        end
    end,
    GeneralTabSetMessageCommand = function(self)
        focused = true
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self)
        focused = false
    end,
    ChartPreviewToggleMessageCommand = function(self)
        focused = false
    end,
}

local ratios = {
    LeftGap = 1140 / 1920, -- distance from left side of screen to left side of frame
    TopGap = 109 / 1080, -- distance from top of screen to top of frame
    Height = 359 / 1080,
    Width = Var("widthRatio"), -- width taken from loading file default.lua
    BannerHeight = 243 / 1080,
    LowerLipHeight = 34 / 1080,
    LeftTextLeftGap = 10 / 1920,
    TextLowerGap1 = 8 / 1080, -- lowest text line, bottom frame to bottom letters
    TextLowerGap2 = 49 / 1080, -- these gaps are from bottom frame to bottom text
    TextLowerGap3 = 87 / 1080,
    ApproximateTextVerticalHeight = 25 / 1080, -- exactly what it says, this determines the max allowed height for text.

    RateTextLeftGap = 330 / 1920,
    BPMTextLeftGap = 210 / 1920,
    BPMNumberLeftGap = 265 / 1920, -- from right edge to right edge of numbers
    BPMWidth = 50 / 1920, -- from right edge of bpm number to right edge of bpm text
    LengthTextLeftGap = 10 / 1920,
    LengthNumberLeftGap = 110 / 1920, -- from right edge to right edge of numbers
    LengthWidth = 62 / 1920, -- from right edge of len number to right edge of len text

    -- deprecated values but still used for initialization at the very least
    -- these numbers are used for max width of the text
    -- after initialization, it is dependent on the actual size of the diff frame, so these numbers are not used
    -- DiffFrameRightGap IS USED for positioning the diff frame itself, do not remove
    DiffFrameLeftGap = 407 / 1920, -- left edge of frame to left edge of leftmost item (?)
    DiffFrameRightGap = 22 / 1920, -- from right edge of frame to right edge of rightmost item
}

local actuals = {
    LeftGap = ratios.LeftGap * SCREEN_WIDTH,
    TopGap = ratios.TopGap * SCREEN_HEIGHT,
    Height = ratios.Height * SCREEN_HEIGHT,
    Width = ratios.Width * SCREEN_WIDTH,
    BannerWidth = ratios.Width * SCREEN_WIDTH,
    BannerHeight = ratios.BannerHeight * SCREEN_HEIGHT,
    LowerLipHeight = ratios.LowerLipHeight * SCREEN_HEIGHT,
    LeftTextLeftGap = ratios.LeftTextLeftGap * SCREEN_WIDTH,
    TextLowerGap1 = ratios.TextLowerGap1 * SCREEN_HEIGHT,
    TextLowerGap2 = ratios.TextLowerGap2 * SCREEN_HEIGHT,
    TextLowerGap3 = ratios.TextLowerGap3 * SCREEN_HEIGHT,
    ApproximateTextVerticalHeight = ratios.ApproximateTextVerticalHeight * SCREEN_HEIGHT,
    RateTextLeftGap = ratios.RateTextLeftGap * SCREEN_WIDTH,
    BPMTextLeftGap = ratios.BPMTextLeftGap * SCREEN_WIDTH,
    BPMNumberLeftGap = ratios.BPMNumberLeftGap * SCREEN_WIDTH,
    BPMWidth = ratios.BPMWidth * SCREEN_WIDTH,
    LengthTextLeftGap = ratios.LengthTextLeftGap * SCREEN_WIDTH,
    LengthNumberLeftGap = ratios.LengthNumberLeftGap * SCREEN_WIDTH,
    LengthWidth = ratios.LengthWidth * SCREEN_WIDTH,

    DiffFrameLeftGap = ratios.DiffFrameLeftGap * SCREEN_WIDTH,
    DiffFrameRightGap = ratios.DiffFrameRightGap * SCREEN_WIDTH,
}

-- require that the banner ratio is 3.2 for consistency
local nonstandardBannerSizing = false
do
    local rat = actuals.BannerWidth / actuals.BannerHeight
    if rat ~= 3.2 then
        local possibleHeight = actuals.BannerWidth / 3.2
        if possibleHeight > actuals.BannerHeight then
            -- height stays, width moves
            actuals.BannerWidth = actuals.BannerHeight * 3.2
        else
            -- width stays, height moves
            actuals.BannerHeight = actuals.BannerWidth / 3.2
        end
        -- this will produce a visible gap but the ratio will stay the same
        nonstandardBannerSizing = true
    end
    actuals.BannerAreaHeight = ratios.BannerHeight * SCREEN_HEIGHT
end

local translations = {
    Length = THEME:GetString("ScreenSelectMusic CurSongBox", "Length"),
    BPM = THEME:GetString("ScreenSelectMusic CurSongBox", "BPM"),
}

local textsize = 0.8
local textzoomFudge = 5

local buttonHoverAlpha = 0.8

t[#t+1] = Def.ActorFrame {
    Name = "Frame",
    InitCommand = function(self)
        self:xy(actuals.LeftGap, actuals.TopGap)
    end,
    BeginCommand = function(self)
        local snm = SCREENMAN:GetTopScreen():GetName()
        local anm = self:GetName()
        CONTEXTMAN:RegisterToContextSet(snm, "Main1", anm)

        -- the math with the logic inline will make increments be 0.1x
        -- holding Select will do 0.05x increments
        local selectPressed = false
        SCREENMAN:GetTopScreen():AddInputCallback(function(event)
            -- require context is set and the general box is set to anything but the Scores tab
            if not CONTEXTMAN:CheckContextSet(snm, "Main1") or SCUFF.generaltab == SCUFF.scoretabindex then 
                selectPressed = false
                return
            end
            if event.type == "InputEventType_FirstPress" then
                if event.button == "EffectUp" then
                    changeMusicRate(1, selectPressed)
                elseif event.button == "EffectDown" then
                    changeMusicRate(-1, selectPressed)
                elseif event.button == "Select" then
                    selectPressed = true
                end
            elseif event.type == "InputEventType_Release" then
                if event.button == "Select" then
                    selectPressed = false
                end
            end
        end)
    end,

    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(actuals.Width, actuals.Height)
            self:diffusealpha(0.83)
            registerActorToColorConfigElement(self, "main", "PrimaryBackground")
        end
    },
    --[[
    Def.Quad {
        Name = "LowerLip",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:y(actuals.Height)
            self:zoomto(actuals.Width, actuals.LowerLipHeight)
            self:diffuse(color("#111111"))
            self:diffusealpha(0.6)
        end
    },
    ]]
    UIElements.QuadButton(1, 1) .. {
        Name = "BannerAreaButton",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(actuals.Width, actuals.BannerAreaHeight)
            self:diffusealpha(0)
            self.banner = function(self) return self:GetParent():GetChild("Banner") end
        end,
        MouseDownCommand = function(self, params)
            -- clicking the banner will toggle chart preview
            -- tree:
            -- self - frame - cursongbox.lua - rightframe
            --      rightframe owns generalbox - owns general owns chart preview
            -- this should work based on the actor tree that exists
            -- if it fails, probably nothing was there to receive the message or the tree is bad
            if SCUFF.generaltab == SCUFF.generaltabindex and focused and params.event == "DeviceButton_left mouse button" then
                SCUFF.preview.active = not SCUFF.preview.active
                MESSAGEMAN:Broadcast("ChartPreviewToggle")
            elseif params.event == "DeviceButton_right mouse button" then
                local top = SCREENMAN:GetTopScreen()
                if top.PauseSampleMusic then
                    top:PauseSampleMusic()
                end
            end
        end,
        MouseOverCommand = function(self)
            if SCUFF.generaltab ~= SCUFF.generaltabindex then return end
            self:banner():diffusealpha(buttonHoverAlpha)
        end,
        MouseOutCommand = function(self)
            -- unhover state
            self:banner():diffusealpha(1)
        end,
        GeneralTabSetMessageCommand = function(self, params)
            -- prevent "stuck" hovered state
            if SCUFF.generaltab ~= SCUFF.generaltabindex or params ~= nil and params.tab ~= SCUFF.generaltabindex then
                self:banner():diffusealpha(1)
            else
                -- hover if already hovered
                if isOver(self) then
                    self:banner():diffusealpha(buttonHoverAlpha)
                end
            end
        end,
    },
    Def.Sprite {
        Name = "Banner",
        InitCommand = function(self)
            self:halign(0):valign(0)
            if nonstandardBannerSizing then
                -- when the banner has been resized to an unexpected size, to fit the 3.2 ratio, reposition it
                -- this movement centers it in the area provided
                self:xy((actuals.Width - actuals.BannerWidth) / 2, (actuals.BannerAreaHeight - actuals.BannerHeight) / 2)
            end
            self:scaletoclipped(actuals.BannerWidth, actuals.BannerHeight)
            self:SetDecodeMovie(useVideoBanners())
        end,
        SetCommand = function(self, params)
            self:finishtweening()
            self:smooth(0.05)
            self:diffusealpha(1)
            if params.song then
                local bnpath = params.song:GetBannerPath()
                if not showBanners() then
                    self:visible(false)
                elseif not bnpath then
                    bnpath = THEME:GetPathG("Common", "fallback banner")
                    self:visible(false)
                else
                    self:visible(true)
                end
                self:LoadBackground(bnpath)
            else
                local bnpath = WHEELDATA:GetFolderBanner(params.hovered)
                if not showBanners() then
                    self:visible(false)
                elseif not bnpath or bnpath == "" then
                    bnpath = THEME:GetPathG("Common", "fallback banner")
                    self:visible(false)
                else
                    self:visible(true)
                end
                self:LoadBackground(bnpath)
            end
            -- handles group banners or missing backgrounds
            -- logic in the bg handles whether or not we successfully loaded a banner here
            if params.song == nil or params.song:GetBackgroundPath() == nil then
                MESSAGEMAN:Broadcast("SetAverageColor", {actor=self})
            end
        end,
        OptionUpdatedMessageCommand = function(self, params)
            if params and params.name == "Video Banners" then
                self:SetDecodeMovie(useVideoBanners())
            end
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "TitleAuthor",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:xy(actuals.LeftTextLeftGap, actuals.Height - actuals.TextLowerGap3)
            self:zoom(textsize)
            self:maxwidth((actuals.DiffFrameLeftGap - actuals.LeftTextLeftGap) / textsize - textzoomFudge)
            self:maxheight(actuals.ApproximateTextVerticalHeight / textsize)
            self:settext("Song Title - Song Author")
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        SetCommand = function(self, params)
            if params.song then
                local title = params.song:GetDisplayMainTitle()
                local artist = params.song:GetDisplayArtist()
                self:settextf("%s - %s", title, artist)
            else
                self:settext("")
            end
        end,
        DisplayLanguageChangedMessageCommand = function(self)
            self:playcommand("Set", {song = GAMESTATE:GetCurrentSong()})
        end
    },
    LoadFont("Common Normal") .. {
        Name = "SubTitle",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:xy(actuals.LeftTextLeftGap, actuals.Height - actuals.TextLowerGap2)
            self:zoom(textsize)
            self:maxwidth((actuals.DiffFrameLeftGap - actuals.LeftTextLeftGap) / textsize - textzoomFudge)
            self:maxheight(actuals.ApproximateTextVerticalHeight / textsize)
            self:settext("Song SubTitle (1995)")
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end,
        SetCommand = function(self, params)
            if params.song then
                self:settext(params.song:GetDisplaySubTitle())
            else
                self:settext("")
            end
        end,
        DisplayLanguageChangedMessageCommand = function(self)
            self:playcommand("Set", {song = GAMESTATE:GetCurrentSong()})
        end
    },
    UIElements.TextButton(1, 1, "Common Normal") .. {
        Name = "Rate",
        InitCommand = function(self)
            self:xy(actuals.RateTextLeftGap, actuals.Height - actuals.TextLowerGap1)
            local txt = self:GetChild("Text")
            local bg = self:GetChild("BG")

            txt:halign(0):valign(1)
            txt:zoom(textsize)
            txt:maxwidth((actuals.Width - actuals.RateTextLeftGap) / textsize - textzoomFudge)
            registerActorToColorConfigElement(txt, "main", "PrimaryText")
            bg:halign(0):valign(1)
            bg:zoomy(actuals.LowerLipHeight)
            bg:y(actuals.TextLowerGap1)
        end,
        SetCommand = function(self, params)
            local txt = self:GetChild("Text")
            local bg = self:GetChild("BG")
            local str = string.format("%.2f", getCurRateValue()) .. "x"
            txt:settext(str)
            bg:zoomx(txt:GetZoomedWidth())
        end,
        ClickCommand = function(self, params)
            if self:IsInvisible() then return end
            if params.update == "OnMouseDown" then
                if params.event == "DeviceButton_left mouse button" then
                    changeMusicRate(1, true)
                elseif params.event == "DeviceButton_right mouse button" then
                    changeMusicRate(-1, true)
                end
            end
        end,
        RolloverUpdateCommand = function(self, params)
            if self:IsInvisible() then return end
            if params.update == 'in' then
                self:diffusealpha(buttonHoverAlpha)
            else
                self:diffusealpha(1)
            end
        end,
        MouseScrollMessageCommand = function(self, params)
            if self:IsInvisible() then return end
            if isOver(self:GetChild("BG")) then
                if params.direction == "Up" then
                    changeMusicRate(1, true)
                elseif params.direction == "Down" then
                    changeMusicRate(-1, true)
                end
            end
        end
    },
    
    LoadFont("Common Normal") .. {
        Name = "LengthText",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:xy(actuals.LengthTextLeftGap, actuals.Height - actuals.TextLowerGap1)
            self:zoom(textsize)
            self:maxwidth((actuals.LengthNumberLeftGap - actuals.LeftTextLeftGap) / textsize - textzoomFudge)
            self:settext(translations["Length"])
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end
    },
    LoadFont("Common Normal") .. {
        Name = "LengthNumbers",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:xy(actuals.LengthNumberLeftGap, actuals.Height - actuals.TextLowerGap1)
            self:zoom(textsize)
            self:maxwidth((actuals.BPMTextLeftGap - actuals.LengthNumberLeftGap) / textsize - textzoomFudge)
            self:settext("55:55")
        end,
        SetCommand = function(self, params)
            if params.steps then
                local len = GetPlayableTime()
                self:settext(SecondsToMMSS(len))
                self:diffuse(colorByMusicLength(len))
            else
                self:settext("--:--")
                self:diffuse(color("1,1,1,1"))
            end
        end
    },
    
    LoadFont("Common Normal") .. {
        Name = "BPMText",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:xy(actuals.BPMTextLeftGap, actuals.Height - actuals.TextLowerGap1)
            self:zoom(textsize)
            self:maxwidth((actuals.BPMNumberLeftGap - actuals.BPMTextLeftGap) / textsize - textzoomFudge)
            self:settext(translations["BPM"])
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end
    },
    Def.BPMDisplay {
        File = THEME:GetPathF("Common", "Normal"),
        Name = "BPMDisplay",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:xy(actuals.BPMNumberLeftGap, actuals.Height - actuals.TextLowerGap1)
            self:zoom(textsize)
            self:maxwidth(actuals.BPMWidth / textsize - textzoomFudge)
        end,
        SetCommand = function(self, params)
            if params.steps then
                self:visible(true)
                self:SetFromSteps(params.steps)
            else
                self:visible(false)
            end
        end
    },
    LoadActorWithParams("stepsdisplay", {ratios = ratios, actuals = actuals})

}


return t