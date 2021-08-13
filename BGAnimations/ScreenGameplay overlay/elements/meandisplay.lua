-- the mean display. it displays the mean

-- i dunno less copy paste whatever bro
local formatstr = "%5.2fms"
local meanTextSize = GAMEPLAY:getItemHeight("meanDisplayText")

local curMeanSum = 0
local curMeanCount = 0

return Def.ActorFrame {
    Name = "DisplayMean",
    InitCommand = function(self)
        self:xy(MovableValues.DisplayMeanX, MovableValues.DisplayMeanY)
        self:zoom(MovableValues.DisplayMeanZoom)
    end,
    JudgmentMessageCommand = function(self, params)
        -- should work fine only for judged taps, not misses or holds
        if not params.HoldNoteScore and params.Offset ~= nil then
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
            bg:zoomto(perc:GetZoomedWidth() + 4, perc:GetZoomedHeight() + 4)
        end
    end,

    Def.Quad {
        Name = "MeanBacking",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:diffuse(color("0,0,0,0.4"))
        end
    },
    LoadFont("Common Large") .. {
        Name = "DisplayMean",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:zoom(meanTextSize)
            -- maybe we want to set the text color here
        end,
    }
}