-- the mean display. it displays the mean

-- i dunno less copy paste whatever bro
local formatstr = THEME:GetString("ScreenGameplay", "MeanDisplayFormatStr")
local meanTextSize = GAMEPLAY:getItemHeight("meanDisplayText")
local bgMargin = 4
local bgalpha = 0.4

local curMeanSum = 0
local curMeanCount = 0

return Def.ActorFrame {
    Name = "DisplayMean",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.DisplayMeanX, MovableValues.DisplayMeanY)
        self:zoom(MovableValues.DisplayMeanZoom)
    end,
    JudgmentMessageCommand = function(self, params)
        -- should work fine only for judged taps, not misses or holds
        if not params.HoldNoteScore and params.Offset ~= nil and params.Offset < 1000 then
            curMeanSum = curMeanSum + params.Offset
            curMeanCount = curMeanCount + 1
            self:playcommand("UpdateMeanText")
        end
    end,
    PracticeModeResetMessageCommand = function(self)
        curMeanCount = 0
        curMeanSum = 0
        self:playcommand("UpdateMeanText")
    end,
    UpdateMeanTextCommand = function(self)
        local bg = self:GetChild("MeanBacking")
        local perc = self:GetChild("DisplayMean")

        if perc then
            if curMeanCount == 0 then
                perc:settextf(formatstr, 0)
            else
                perc:settextf(formatstr, curMeanSum / curMeanCount)
            end
        end
        if bg and perc then
            bg:zoomto(perc:GetZoomedWidth() + bgMargin, perc:GetZoomedHeight() + bgMargin)
        end
    end,

    Def.Quad {
        Name = "MeanBacking",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:xy(bgMargin/2, -bgMargin/2)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryBackground")
            self:diffusealpha(bgalpha)
        end
    },
    LoadFont("Common Large") .. {
        Name = "DisplayMean",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:zoom(meanTextSize)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryText")
            self:diffusealpha(1)
        end,
    }
}