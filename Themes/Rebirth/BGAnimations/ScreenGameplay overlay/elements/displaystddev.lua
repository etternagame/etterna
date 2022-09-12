-- the standard deviation display. it displays the standard deviation.

local formatstr = THEME:GetString("ScreenGameplay", "StdDevDisplayFormatStr")
local stddevTextSize = GAMEPLAY:getItemHeight("stddevDisplayText")
local bgMargin = 4
local bgalpha = 0.4

-- this is welfords online algorithm for variance (we can get sd from variance)
-- it might not be necessary here, didnt really put a lot of thought into it
-- but its the same thing that is used for ReplaySnapshots
local runningmean = 0
local runningvariance = 0
local tapcount = 0

return Def.ActorFrame {
    Name = "DisplayStdDev",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.DisplayStdDevX, MovableValues.DisplayStdDevY)
        self:zoom(MovableValues.DisplayStdDevZoom)
    end,
    JudgmentMessageCommand = function(self, params)
        -- should work fine only for judged taps, not misses or holds
        if not params.HoldNoteScore and params.Offset ~= nil and params.Offset < 1000 then
            tapcount = tapcount + 1
            local delta = params.Offset - runningmean
            runningmean = runningmean + (delta / tapcount)
            local delta2 = params.Offset - runningmean
            runningvariance = runningvariance + (delta * delta2)
            self:playcommand("UpdateStdDevText")
        end
    end,
    PracticeModeResetMessageCommand = function(self)
        tapcount = 0
        runningmean = 0
        runningvariance = 0
        self:playcommand("UpdateStdDevText")
    end,
    UpdateStdDevTextCommand = function(self)
        local bg = self:GetChild("StdDevBacking")
        local perc = self:GetChild("DisplayStdDev")

        if perc then
            if tapcount == 0 or tapcount == 1 then
                perc:settextf(formatstr, 0)
            else
                perc:settextf(formatstr, math.sqrt(runningvariance / (tapcount - 1)))
            end
        end
        if bg and perc then
            bg:zoomto(perc:GetZoomedWidth() + bgMargin, perc:GetZoomedHeight() + bgMargin)
        end
    end,

    Def.Quad {
        Name = "StdDevBacking",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:xy(bgMargin/2, -bgMargin/2)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryBackground")
            self:diffusealpha(bgalpha)
        end
    },
    LoadFont("Common Large") .. {
        Name = "DisplayStdDev",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:zoom(stddevTextSize)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryText")
            self:diffusealpha(1)
        end,
    }
}