local ratios = {
    Width = 780 / 1920,
    Height = 971 / 1080,
    TopLipHeight = 44 / 1080,
    EdgePadding = 13 / 1920, -- distance from left and right edges for everything
    DividerThickness = 2 / 1080, -- consistently 2 pixels basically
    LowerLipHeight = 34 / 1080,
    DensityGraphHeight = 53 / 555,

    RateTextLeftGap = 330 / 1920,
    BPMTextLeftGap = 210 / 1920,
    BPMNumberLeftGap = 265 / 1920, -- from right edge to right edge of numbers
    BPMWidth = 50 / 1920, -- from right edge of bpm number to right edge of bpm text
    LengthTextLeftGap = 10 / 1920,
    LengthNumberLeftGap = 110 / 1920, -- from right edge to right edge of numbers
    LengthWidth = 62 / 1920, -- from right edge of len number to right edge of len text
}

local actuals = {
    Width = ratios.Width * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    TopLipHeight = ratios.TopLipHeight * SCREEN_HEIGHT,
    EdgePadding = ratios.EdgePadding * SCREEN_WIDTH,
    DividerThickness = ratios.DividerThickness * SCREEN_HEIGHT,
    LowerLipHeight = ratios.LowerLipHeight * SCREEN_HEIGHT,
    DensityGraphHeight = ratios.DensityGraphHeight * SCREEN_HEIGHT,

    RateTextLeftGap = ratios.RateTextLeftGap * SCREEN_WIDTH,
    BPMTextLeftGap = ratios.BPMTextLeftGap * SCREEN_WIDTH,
    BPMNumberLeftGap = ratios.BPMNumberLeftGap * SCREEN_WIDTH,
    BPMWidth = ratios.BPMWidth * SCREEN_WIDTH,
    LengthTextLeftGap = ratios.LengthTextLeftGap * SCREEN_WIDTH,
    LengthNumberLeftGap = ratios.LengthNumberLeftGap * SCREEN_WIDTH,
    LengthWidth = ratios.LengthWidth * SCREEN_WIDTH,
}

local translations = {
    CloseChartPreview = THEME:GetString("ChartPreview", "CloseChartPreview"),
    Length = THEME:GetString("ScreenSelectMusic CurSongBox", "Length"),
    BPM = THEME:GetString("ScreenSelectMusic CurSongBox", "BPM"),
}

local lastusedsong = nil
local visibleframeX = 0 -- this number is reset by SetPosition
local visibleframeY = SCREEN_HEIGHT - actuals.Height
local hiddenframeX = SCREEN_WIDTH
local animationSeconds = 0.1
local focused = false

local buttonAlpha = 0.1
local buttonTextSize = 0.95
local buttonHoverAlpha = 0.6
local textzoomFudge = 5
local textsize = 0.8

local t = Def.ActorFrame {
    Name = "ChartPreviewFile",
    InitCommand = function(self)
        -- hide chart preview to start
        SCUFF.preview.active = false
        self:playcommand("SetPosition")
        self:y(visibleframeY)
        self:diffusealpha(0)
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self, params)
        -- if we ever get this message we need to hide the frame and just exit.
        focused = false
        SCUFF.preview.active = false
        self:finishtweening()
        self:smooth(animationSeconds)
        self:diffusealpha(0)
        self:x(hiddenframeX)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        -- if we ever get this message we need to hide the frame and just exit.
        focused = false
        SCUFF.preview.active = false
        self:finishtweening()
        self:smooth(animationSeconds)
        self:diffusealpha(0)
        self:x(hiddenframeX)
    end,
    ChartPreviewToggleMessageCommand = function(self, params)
        if not focused then
            self:diffusealpha(1)
            self:finishtweening()
            self:sleep(0.01)
            self:queuecommand("FinishFocusing")
            self:smooth(animationSeconds)
            self:x(visibleframeX)
        else
            -- if toggled twice, send back to the general tab
            self:sleep(0.02):queuecommand("ExitPreview")
        end
    end,
    ExitPreviewCommand = function(self)
        MESSAGEMAN:Broadcast("GeneralTabSet", {tab = SCUFF.generaltabindex})
    end,
    FinishFocusingCommand = function(self)
        focused = true

        if SCUFF.preview.active then
            -- chart preview turning on
            if not SCUFF.preview.resetmusic and lastusedsong ~= nil then
                local top = SCREENMAN:GetTopScreen()
                if top.PlayCurrentSongSampleMusic then
                    -- reset music, force start, force full length
                    SCUFF.preview.resetmusic = true
                    SOUND:StopMusic()
                    top:PlayCurrentSongSampleMusic(true, true)
                end
            end
            self:diffusealpha(1)
            CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Main1")
        else
            -- chart preview turning off
            self:diffusealpha(0)
            -- hide in case you are hovering the graph
            TOOLTIP:Hide()
        end
    end,
    SetPositionCommand = function(self)
        if getWheelPosition() then
            visibleframeX = SCREEN_WIDTH
            hiddenframeX = SCREEN_WIDTH + actuals.Width
        else
            visibleframeX = actuals.Width
            hiddenframeX = -actuals.Width
        end
        if focused then
            self:x(visibleframeX)
        else
            self:x(hiddenframeX)
        end
    end,
    UpdateWheelPositionCommand = function(self)
        self:playcommand("SetPosition")
    end,
    OptionUpdatedMessageCommand = function(self, params)
        if params and params.name == "Music Wheel Position" then
            self:playcommand("UpdateWheelPosition")
        end
    end,
    WheelSettledMessageCommand = function(self, params)
        -- should trigger if wheel stops moving
        self:playcommand("LoadNoteData", {song = params.song, steps = params.steps})
        lastusedsong = params.song

       SCUFF.preview.resetmusic = false
       if lastusedsong ~= nil and SCUFF.preview.active then
            local top = SCREENMAN:GetTopScreen()
            if top.PlayCurrentSongSampleMusic then
                -- reset music, force start, force full length
                SCUFF.preview.resetmusic = true
                SOUND:StopMusic()
                top:PlayCurrentSongSampleMusic(true, true)
            end
       end
       self:playcommand("Set", {song = params.song, group = params.group, hovered = params.hovered, steps = params.steps})
    end,
    ChangedStepsMessageCommand = function(self, params)
        -- should trigger only if switching steps, not when switching songs
        self:playcommand("LoadNoteData", {song = GAMESTATE:GetCurrentSong(), steps = params.steps})
        lastusedsong = GAMESTATE:GetCurrentSong()
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastusedsong, steps = params.steps})
    end,
    CurrentRateChangedMessageCommand = function(self)
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastusedsong, steps = GAMESTATE:GetCurrentSteps()})
    end,
}

local notefieldYCenter = (actuals.Height - actuals.DensityGraphHeight) / 2 -- this is technically wrong BUT WORKS
local aspectRatioProportion = (16/9) / (SCREEN_WIDTH / SCREEN_HEIGHT) -- this was designed for 16:9 so compensate
local notefieldZoomBaseline = 1 -- zoom for 4key width
local notefieldWidthBaseline = 256 -- 4key width
local notefieldLengthPixels = actuals.Height - actuals.DensityGraphHeight - 15 -- this isnt a perfect number but it fits for our use and i dont know how to calculate it
local notefieldAllowBeyondReceptorPixels = 0 -- this shouldnt be changed
local notefieldYReversePixelsBase = 0 -- this means when reverse is toggled the notefield moves itself. dont let it do that

-- this function makes no sense btw. none of the math is actually based on anything
-- find a replacement if you are going to mess with it
local function getSizeForStyle()
    local style = GAMESTATE:GetCurrentStyle()
    if style == nil then return notefieldZoomBaseline, notefieldLengthPixels / notefieldZoomBaseline end

    local stylewidth = style:GetWidth()
    -- the assumption is that a width of notefieldWidthBaseline uses a zoom of notefieldZoomBaseline
    --  and notefieldLengthPixels is 300 for that baseline zoom
    -- find a zoom and pixel length that fits using math
    local pdiff = stylewidth / notefieldWidthBaseline

    -- i replaced these with the defaults because the area given was so big
    -- unreplace them if for some reason this becomes a problem
    local newzoom = 1--notefieldZoomBaseline / pdiff / aspectRatioProportion
    local newlength = notefieldLengthPixels-- / newzoom

    return newzoom, newlength
end

t[#t+1] = UIElements.QuadButton(1, 1) .. {
    Name = "BG",
    InitCommand = function(self)
        self:halign(1):valign(0)
        self:zoomto(actuals.Width, actuals.Height)
        registerActorToColorConfigElement(self, "chartPreview", "Background")
    end,
    MouseDownCommand = function(self, params)
        local top = SCREENMAN:GetTopScreen()
        if params.event ~= "DeviceButton_left mouse button" then
            if top.PauseSampleMusic then
                top:PauseSampleMusic()
            end
        end
    end
}

t[#t+1] = UIElements.TextButton(2, 2, "Common Normal") .. {
    Name = "CloseChartPreview",
    InitCommand = function(self)
        self.txt = self:GetChild("Text")
        self.bg = self:GetChild("BG")
        self.txt:valign(0)
        self.txt:settext(translations["CloseChartPreview"])
        self.txt:zoom(buttonTextSize)
        self.bg:x(-self.txt:GetZoomedWidth() * 0.025)
        self.bg:y(self.txt:GetZoomedHeight() / 2)
        self.bg:zoomto(self.txt:GetZoomedWidth() * 1.05, self.txt:GetZoomedHeight() * 1.3)
        self.bg:diffusealpha(buttonAlpha)
        self:y(actuals.DensityGraphHeight + actuals.EdgePadding)

        self.alphaDeterminingFunction = function(self)
            if isOver(self.bg) then
                self:diffusealpha(buttonHoverAlpha)
            else
                self:diffusealpha(1)
            end
        end
    end,
    SetPositionCommand = function(self)
        if getWheelPosition() then
            self.bg:halign(0)
            self.txt:halign(0)
            self:x(-actuals.Width + actuals.EdgePadding)
        else
            self.bg:halign(1)
            self.txt:halign(1)
            self:x(-actuals.EdgePadding)
        end
    end,
    RolloverUpdateCommand = function(self, params)
        self:alphaDeterminingFunction()
    end,
    ClickCommand = function(self, params)
        if params.update == "OnMouseDown" then
            self:GetParent():playcommand("ExitPreview")
            self:alphaDeterminingFunction()
        end
    end,
}

t[#t+1] = Def.ActorFrame {
    Name = "CopyPastedCurSongBoxLine",
    InitCommand = function(self)
        self:x(-actuals.Width + actuals.EdgePadding)
        self:y(actuals.Height - actuals.EdgePadding)
    end,

    LoadFont("Common Normal") .. {
        Name = "MSDLabel",
        InitCommand = function(self)
            self:x(actuals.LengthTextLeftGap)
            self:y(-actuals.LowerLipHeight - actuals.EdgePadding)
            self:halign(0)
            self:zoom(textsize)
            self:maxwidth((actuals.Width) / textsize - textzoomFudge)
            self:settext("MSD")
        end,
    },
    Def.RollingNumbers {
        Name = "MSD",
        Font = "Common Normal",
        BeginCommand = function(self)
            self:y(-actuals.LowerLipHeight - actuals.EdgePadding)
            self:halign(0)
            self:zoom(textsize)
            self:x(actuals.EdgePadding + self:GetParent():GetChild("MSDLabel"):GetZoomedWidth())
            self:maxwidth((actuals.Width - self:GetParent():GetChild("MSDLabel"):GetZoomedWidth()) / textsize - textzoomFudge)
            self:Load("RollingNumbers2Decimal")
        end,
        SetCommand = function(self, params)
            if params.steps then
                local meter = params.steps:GetMSD(getCurRateValue(), 1)
                self:targetnumber(meter)
                self:diffuse(colorByMSD(meter))
            else
                self:targetnumber(0)
                self:diffuse(color("1,1,1,1"))
            end
        end
    },

    UIElements.TextButton(1, 1, "Common Normal") .. {
        Name = "Rate",
        InitCommand = function(self)
            self:x(actuals.RateTextLeftGap)
            local txt = self:GetChild("Text")
            local bg = self:GetChild("BG")

            txt:halign(0):valign(1)
            txt:zoom(textsize)
            txt:maxwidth((actuals.Width - actuals.RateTextLeftGap) / textsize - textzoomFudge)
            registerActorToColorConfigElement(txt, "main", "PrimaryText")
            bg:halign(0):valign(1)
            bg:zoomy(actuals.LowerLipHeight)
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
            self:x(actuals.LengthTextLeftGap)
            self:zoom(textsize)
            self:maxwidth((actuals.LengthNumberLeftGap) / textsize - textzoomFudge)
            self:settext(translations["Length"])
            registerActorToColorConfigElement(self, "main", "PrimaryText")
        end
    },
    LoadFont("Common Normal") .. {
        Name = "LengthNumbers",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:x(actuals.LengthNumberLeftGap)
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
            self:x(actuals.BPMTextLeftGap)
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
            self:x(actuals.BPMNumberLeftGap)
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
}

t[#t+1] = Def.NoteFieldPreview {
    Name = "NoteField",
    DrawDistanceBeforeTargetsPixels = notefieldLengthPixels / notefieldZoomBaseline,
    DrawDistanceAfterTargetsPixels = notefieldAllowBeyondReceptorPixels, -- notes disappear at the receptor

    InitCommand = function(self)
        self:playcommand("SetPosition")
        self:y(notefieldYCenter)
        self:zoom(notefieldZoomBaseline)
        -- make mods work
        self:SetFollowPlayerOptions(true)
        self:SetUpdateFunction(function(self)
            ArrowEffects.Update()
        end)
    end,
    BeginCommand = function(self)
        -- we need to redo the draw order for the notefield and graph
        -- the notefield ends up being on top of everything in the actorframe otherwise
        self:draworder(1)
        self:GetParent():GetChild("ChordDensityGraphFile"):draworder(2)
        self:GetParent():SortByDrawOrder()
    end,
    SetPositionCommand = function(self)
        if getWheelPosition() then
            self:x(-actuals.Width / 2)
        else
            self:x(-actuals.Width / 2)
        end
    end,
    LoadNoteDataCommand = function(self, params)
        local steps = params.steps
        if steps ~= nil then
            self:LoadNoteData(steps, true)
        else
            self:LoadDummyNoteData()
        end
        local z, l = getSizeForStyle()
        self:zoom(z)
        self:playcommand("SetPosition")
        self:SetConstantMini(ReceptorSizeToMini(z))
        -- running from what you fear brings you yet closer to that which you loathe
        -- magic numbers rule the world. never forget it.
        local compensation = getPlayerOptions():UsingReverse() and (notefieldYCenter * z) or (-notefieldYCenter/2 * z)
        self:y(notefieldYCenter + compensation)
        self:UpdateDrawDistance(notefieldAllowBeyondReceptorPixels, l)
        self:UpdateYReversePixels(notefieldYReversePixelsBase)
    end,
    OptionUpdatedMessageCommand = function(self, params)
        if params ~= nil then
            -- listen for the notedata modifying mods being toggled and apply their changes immediately
            local options = {
                Mirror = true,
                Turn = true,
                ["Pattern Transform"] = true,
                ["Hold Transform"] = true,
                Remove = true,
                Insert = true,
                Mines = true,
                ["Scroll Direction"] = true,
            }
            if options[params.name] ~= nil then
                self:playcommand("LoadNoteData", {steps = GAMESTATE:GetCurrentSteps()})
            end

            if params.name == "Music Wheel Position" then
                self:playcommand("SetPosition")
            end
        end
    end,
}

t[#t+1] = LoadActorWithParams("../chordDensityGraph.lua", {sizing = {
    Width = actuals.Width,
    Height = actuals.DensityGraphHeight,
    NPSThickness = 2,
    TextSize = 0.65,
}}) .. {
    InitCommand = function(self)
        self:x(-actuals.Width)
    end,
    LoadNoteDataCommand = function(self, params)
        local steps = params.steps
        if steps ~= nil then
            self:playcommand("LoadDensityGraph", {steps = steps, song = params.song})
        else
            self:playcommand("LoadDensityGraph", {steps = steps, song = params.song})
        end
    end
}

return t
