-- the exponential weighted moving average display. it displays the ewma as a number instead of like the error bar

local formatstr = THEME:GetString("ScreenGameplay", "EWMADisplayFormatStr")
local EWMATextSize = GAMEPLAY:getItemHeight("ewmaDisplayText")
local bgMargin = 4
local bgalpha = 0.4

local alph = 0.07 -- not opacity, math for ewma
local avg = 0
local lastavg = 0

return Def.ActorFrame {
    Name = "DisplayEWMA",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.DisplayEWMAX, MovableValues.DisplayEWMAY)
        self:zoom(MovableValues.DisplayEWMAZoom)
    end,
    JudgmentMessageCommand = function(self, params)
        -- should work fine only for judged taps, not misses or holds
        if not params.HoldNoteScore and params.Offset ~= nil and params.Offset < 1000 then
            avg = alph * params.Offset + (1 - alph) * lastavg
            lastavg = avg
            self:playcommand("UpdateEWMAText")
        end
    end,
    PracticeModeResetMessageCommand = function(self)
        avg = 0
        lastavg = 0
        self:playcommand("UpdateEWMAText")
    end,
    UpdateEWMATextCommand = function(self)
        local bg = self:GetChild("EWMABacking")
        local perc = self:GetChild("DisplayEWMA")

        if perc then
            if avg == 0 then
                perc:settextf(formatstr, 0)
            else
                perc:settextf(formatstr, avg)
            end
        end
        if bg and perc then
            bg:zoomto(perc:GetZoomedWidth() + bgMargin, perc:GetZoomedHeight() + bgMargin)
        end
    end,

    Def.Quad {
        Name = "EWMABacking",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:xy(bgMargin/2, -bgMargin/2)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryBackground")
            self:diffusealpha(bgalpha)
        end
    },
    LoadFont("Common Large") .. {
        Name = "DisplayEWMA",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:zoom(EWMATextSize)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryText")
            self:diffusealpha(1)
        end,
    }
}