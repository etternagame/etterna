-- the wifepercent display. it displays the wifepercent

-- i dunno less copy paste whatever bro
local formatstr = "%05.2f%%"
local wifepercentTextSize = GAMEPLAY:getItemHeight("wifeDisplayText")
local bgMargin = 4
local bgalpha = 0.4

return Def.ActorFrame {
    Name = "DisplayPercent",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.DisplayPercentX, MovableValues.DisplayPercentY)
        self:zoom(MovableValues.DisplayPercentZoom)
    end,
    SpottedOffsetCommand = function(self, params)
        local bg = self:GetChild("PercentBacking")
        local perc = self:GetChild("DisplayPercent")

        -- kind of putting all the logic in one place here
        -- modify text then update bg for it
        if params ~= nil and params.wifePercent ~= nil then
            if perc then
                perc:settextf(formatstr, params.wifePercent)
            end
        else
            if perc then
                perc:settextf(formatstr, 0)
            end
        end
        if bg and perc then
            bg:zoomto(perc:GetZoomedWidth() + bgMargin, perc:GetZoomedHeight() + bgMargin)
        end
    end,

    Def.Quad {
        Name = "PercentBacking",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:xy(bgMargin/2,-bgMargin/2)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryBackground")
            self:diffusealpha(bgalpha)
        end
    },
    LoadFont("Common Large") .. {
        Name = "DisplayPercent",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:zoom(wifepercentTextSize)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryText")
            self:diffusealpha(1)
        end,
    }
}
