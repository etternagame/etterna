local lastusedsong = nil
local t = Def.ActorFrame {
    Name = "ChartPreviewFile",
    InitCommand = function(self)
        -- hide chart preview to start
        SCUFF.preview.active = false
        self:diffusealpha(0)
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
    end,
    ChangedStepsMessageCommand = function(self, params)
        -- should trigger only if switching steps, not when switching songs
        self:playcommand("LoadNoteData", {song = GAMESTATE:GetCurrentSong(), steps = params.steps})
        lastusedsong = GAMESTATE:GetCurrentSong()
    end,
    ToggleChartPreviewCommand = function(self, params)
        if params ~= nil and params.active ~= nil then
            SCUFF.preview.active = params.active
        end

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
        else
            -- chart preview turning off
            self:diffusealpha(0)
            -- hide in case you are hovering the graph
            TOOLTIP:Hide()
        end
    end
}

local ratios = {
    DensityGraphHeight = 53 / 555,
    NoteFieldHeight = 502 / 555,
}

local actuals = {
    -- some actuals left out here and calculated below instead

}

-- scoping magic
do
    -- copying the provided ratios and actuals tables to have access to the sizing for the overall frame
    local rt = Var("ratios")
    for k,v in pairs(rt) do
        ratios[k] = v
    end
    local at = Var("actuals")
    for k,v in pairs(at) do
        actuals[k] = v
    end
end

-- expected actual values may not exist for whatever reason, so default to 0 instead of nil error
actuals.LowerLipHeight = actuals.LowerLipHeight or 0
actuals.Height = actuals.Height or 0
-- the ratio is the percentage of the area we will use, so multiply it by the raw (actual) given area
-- when placing, take into account each other (notefield is placed below the density graph)
actuals.DensityGraphHeight = ratios.DensityGraphHeight * (actuals.Height - actuals.LowerLipHeight)
actuals.NoteFieldHeight = ratios.NoteFieldHeight * (actuals.Height - actuals.LowerLipHeight)


-- relative to the leftmost part of the general box, this is the horizontal center of the notefield
actuals.VerticalDividerLeftGap = actuals.VerticalDividerLeftGap or 0
actuals.DividerThickness = actuals.DividerThickness or 0
local rightHalfXBegin = actuals.VerticalDividerLeftGap + actuals.DividerThickness
local notefieldXCenter = rightHalfXBegin + (actuals.Width - rightHalfXBegin) / 2
local notefieldYCenter = actuals.DensityGraphHeight + actuals.NoteFieldHeight / 2
local expectedGeneralReceptorHeight = 64 -- this number varies slightly but typically receptors are "64x64"
local aspectRatioProportion = (16/9) / (SCREEN_WIDTH / SCREEN_HEIGHT) -- this was designed for 16:9 so compensate
local notefieldZoomBaseline = 0.8 -- zoom for 4key width
local notefieldWidthBaseline = 256 -- 4key width
local notefieldYOffset = actuals.DensityGraphHeight + expectedGeneralReceptorHeight / 1080 * SCREEN_HEIGHT * notefieldZoomBaseline
local notefieldReverseAdd = actuals.NoteFieldHeight - notefieldYOffset
local notefieldLengthPixels = 300 -- this isnt a perfect number but it fits for our use and i dont know how to calculate it
local notefieldAllowBeyondReceptorPixels = 0 -- this shouldnt be changed
local notefieldYReversePixelsBase = 288 -- this is what it is in gameplay, but it needs to change if we mess with mini/zoom

local function getSizeForStyle()
    local style = GAMESTATE:GetCurrentStyle()
    if style == nil then return notefieldZoomBaseline, notefieldLengthPixels / notefieldZoomBaseline end

    local stylewidth = style:GetWidth()
    -- the assumption is that a width of notefieldWidthBaseline uses a zoom of notefieldZoomBaseline
    --  and notefieldLengthPixels is 300 for that baseline zoom
    -- find a zoom and pixel length that fits using math
    local pdiff = stylewidth / notefieldWidthBaseline
    local newzoom = notefieldZoomBaseline / pdiff / aspectRatioProportion
    local newlength = notefieldLengthPixels / newzoom
    -- taking new calculated reverse pixel offset and making it smaller by a bit
    local newreverse = notefieldYReversePixelsBase / newzoom
    local newreversediff = notefieldYReversePixelsBase - (notefieldYReversePixelsBase - newreverse) / 3

    return newzoom, newlength, newreversediff
end

t[#t+1] = UIElements.QuadButton(1, 1) .. {
    Name = "BG",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:xy(rightHalfXBegin, actuals.DensityGraphHeight)
        self:zoomto(actuals.Width - rightHalfXBegin, actuals.NoteFieldHeight + actuals.LowerLipHeight)
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
        -- THESE ARE LITERALLY RANDOM NUMBERS
        -- I DO NOT KNOW WHY THIS IS NECESSARY
        -- IT DOES NOT MAKE ANY SENSE
        if getWheelPosition() then
            ms.ok(self:GetZoom())
            self:x((rightHalfXBegin + 75) * self:GetZoom())
        else
            self:x(rightHalfXBegin + (actuals.Width - rightHalfXBegin) / 2)
        end
    end,
    LoadNoteDataCommand = function(self, params)
        local steps = params.steps
        if steps ~= nil then
            self:LoadNoteData(steps, true)
        else
            self:LoadDummyNoteData()
        end
        local z, l, r = getSizeForStyle()
        self:zoom(z)
        self:playcommand("SetPosition")
        self:SetConstantMini(ReceptorSizeToMini(z))
        -- when changing zoom of the notefield, the receptors change position just like the length needs to
        -- so need to move the notefield up or down to compensate for the change in zoom
        local compensation = -(actuals.NoteFieldHeight) * (notefieldZoomBaseline-z)/2
        -- running from what you fear brings you yet closer to that which you loathe
        -- magic numbers run the world
        compensation = compensation + (getPlayerOptions():UsingReverse() and 25 or 0)
        self:y(notefieldYCenter - 40 + compensation)
        self:UpdateDrawDistance(notefieldAllowBeyondReceptorPixels, l)
        self:UpdateYReversePixels(r)
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

t[#t+1] = LoadActorWithParams("../../chordDensityGraph.lua", {sizing = {
    Width = actuals.Width - rightHalfXBegin,
    Height = actuals.DensityGraphHeight,
    NPSThickness = 1.5,
    TextSize = 0.45,
}}) .. {
    InitCommand = function(self)
        self:x(rightHalfXBegin)
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

t[#t+1] = UIElements.QuadButton(1, 1) .. {
    Name = "ChoicesCover",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:xy(rightHalfXBegin, actuals.DensityGraphHeight + actuals.NoteFieldHeight)
        self:zoomto(actuals.Width - rightHalfXBegin, actuals.LowerLipHeight)
        self:diffuse(color("#000000"))
        self:draworder(2):visible(0)
    end,
}

return t
