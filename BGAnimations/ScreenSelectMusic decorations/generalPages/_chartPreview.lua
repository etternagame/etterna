local t = Def.ActorFrame {
    Name = "ChartPreviewFile",
    WheelSettledMessageCommand = function(self, params)
        -- should trigger if wheel stops moving
       self:playcommand("LoadNoteData", {song = params.song, steps = params.steps})
    end,
    ChangedStepsMessageCommand = function(self, params)
        -- should trigger only if switching steps, not when switching songs
        self:playcommand("LoadNoteData", {song = GAMESTATE:GetCurrentSong(), steps = params.steps})
    end,
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
actuals.DensityGraphHeight = ratios.DensityGraphHeight * (actuals.Height - actuals.LowerLipHeight)
actuals.NoteFieldHeight = ratios.NoteFieldHeight * (actuals.Height - actuals.LowerLipHeight)


-- relative to the leftmost part of the general box, this is the horizontal center of the notefield
actuals.VerticalDividerLeftGap = actuals.VerticalDividerLeftGap or 0
actuals.DividerThickness = actuals.DividerThickness or 0
local rightHalfXBegin = actuals.VerticalDividerLeftGap + actuals.DividerThickness
local notefieldXCenter = rightHalfXBegin + (actuals.Width - rightHalfXBegin) / 2
local expectedGeneralReceptorHeight = 64 -- this number varies slightly but typically receptors are "64x64"
local notefieldZoom = 0.5
local notefieldYOffset = actuals.DensityGraphHeight + expectedGeneralReceptorHeight * notefieldZoom
local notefieldReverseAdd = actuals.NoteFieldHeight - notefieldYOffset

t[#t+1] = UIElements.QuadButton(1, 1) .. {
    Name = "BG",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:xy(rightHalfXBegin, actuals.DensityGraphHeight)
        self:zoomto(actuals.Width - rightHalfXBegin, actuals.NoteFieldHeight)
        self:diffuse(color("#000000"))
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
    DrawDistanceBeforeTargetsPixels = 600,
    DrawDistanceAfterTargetsPixels = 0,
    YReverseOffsetPixels = -expectedGeneralReceptorHeight,

    InitCommand = function(self)
        self:x(notefieldXCenter)
        self:zoom(notefieldZoom):draworder(90)
        self:playcommand("UpdateReverseNoteFieldPosition")
    end,
    BeginCommand = function(self)
        -- we need to redo the draw order for the notefield and graph
        -- the notefield ends up being on top of everything in the actorframe otherwise
        self:draworder(1)
        self:GetParent():GetChild("ChordDensityGraphFile"):draworder(2)
        self:GetParent():SortByDrawOrder()
    end,
    UpdateReverseNoteFieldPositionCommand = function(self)
        local rev = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():UsingReverse()
        if rev then
            self:y(notefieldYOffset + notefieldReverseAdd)
        else
            self:y(notefieldYOffset)
        end
    end,
    LoadNoteDataCommand = function(self, params)
        local steps = params.steps
        if steps ~= nil then
            self:LoadNoteData(steps)
        else
            self:LoadDummyNoteData()
        end
    end
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

return t