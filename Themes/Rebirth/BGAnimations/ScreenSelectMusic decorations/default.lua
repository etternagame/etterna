local t = Def.ActorFrame {}

-- tracking the right frame - the box containing song info and the general tabs
-- this is not the right frame which pops up from off screen, that is the PlayerInfoFrame
local rightFrameVisible = true
local visibleX = 0
local hiddenX = SCREEN_WIDTH
local widthRatio = 780 / 1920
local widthActual = widthRatio * SCREEN_WIDTH

t[#t+1] = LoadActor("wheel")

t[#t+1] = Def.ActorFrame {
    Name = "RightFrame",
    InitCommand = function(self)
        self:playcommand("SetThePositionForThisFrameNothingElse")
    end,
    SetThePositionForThisFrameNothingElseCommand = function(self)
        if getWheelPosition() then
            visibleX = 0
            hiddenX = widthActual
        else
            visibleX = -SCREEN_WIDTH + widthActual
            hiddenX = -SCREEN_WIDTH - widthActual
        end
        if rightFrameVisible then
            self:x(visibleX)
        else
            self:x(hiddenX)
        end
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params ~= nil and params.tab ~= nil then
            SCUFF.generaltab = params.tab
        end
        if not rightFrameVisible then
            CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Main1")
            self:finishtweening()
            self:smooth(0.1)
            self:x(visibleX)
            rightFrameVisible = true
        end
        TOOLTIP:Hide()
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self)
        rightFrameVisible = false
        self:finishtweening()
        self:smooth(0.1)
        self:x(hiddenX)
        TOOLTIP:Hide()
    end,
    ChartPreviewToggleMessageCommand = function(self)
        rightFrameVisible = false
        self:finishtweening()
        self:smooth(0.1)
        self:x(hiddenX)
        TOOLTIP:Hide()
    end,
    UpdateWheelPositionCommand = function(self)
        self:playcommand("SetThePositionForThisFrameNothingElse")
    end,
    OptionUpdatedMessageCommand = function(self, params)
        if params and params.name == "Music Wheel Position" then
            self:GetParent():playcommand("UpdateWheelPosition")
        end
    end,

    LoadActorWithParams("curSongBox", {
        widthRatio = widthRatio,
    }),
    LoadActorWithParams("generalBox", {
        widthRatio = widthRatio,
    }),
}

t[#t+1] = LoadActor("_chartPreview.lua")

return t