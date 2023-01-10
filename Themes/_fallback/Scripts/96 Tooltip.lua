--- Mouse Tooltips
-- originally created by ca25nada/Prim
-- minor modifications by poco0317
-- @module 96_Tooltip

-- Singleton for accessing the tooltip actor.
local textScale = 0.5
local height = 10
local boxBorder = 3
local screenBorder = 10
local sideswapped = false
local tooltipOffSetX = 10
local tooltipOffSetY = 10
local cursorSize = 5

TOOLTIP = {
    Actor = nil,
    Pointer = nil,
    ClickWave = nil,
}

-- Returns the actor used for the tooltip.
function TOOLTIP.New(self)
    local t = Def.ActorFrame{
        InitCommand = function(self)
            self:visible(false)
            TOOLTIP.Actor = self
        end,
        OnCommand = function(self)
        end,
        ScreenChangedMessageCommand = function(self)
            self:visible(false)
        end,
        OffCommand = function(self)
            self:visible(false)
        end
    }

    t[#t+1] = Def.Quad{
        Name = "Box",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:diffuse(color("#000000")):diffusealpha(0.7)
        end,
    }

    t[#t+1] = LoadFont("Common Large")..{
        Name = "Text",
        InitCommand = function(self)
            self:zoom(textScale)
            self:halign(0):valign(0)
            self:xy(boxBorder / textScale, boxBorder / textScale)
        end,
    }

    local p = Def.Quad {
        InitCommand = function(self)
            self:rotationz(45)
            self:zoomto(cursorSize, cursorSize)
            self:visible(false)
            TOOLTIP.Pointer = self

        end,
        OnCommand = function(self)
        end,
        BeginCommand = function(self)
            self:visible(false)
        end
    }

    local w = LoadActor(THEME:GetPathG("", "_circle")) .. {
        Name = "CursorClick",
        InitCommand = function(self)
            self:diffusealpha(0)
            TOOLTIP.ClickWave = self

        end,
        OnCommand = function(self)
        end,
        MouseClickReleaseMessageCommand = function(self, params)
            if params.button == "DeviceButton_left mouse button" then
                self:finishtweening()
                self:xy(INPUTFILTER:GetMouseX(), INPUTFILTER:GetMouseY())
                self:diffusealpha(1)
                self:zoom(0)
                self:decelerate(0.5)
                self:diffusealpha(0)
                self:zoom(1)
            end
        end,
    }

    return t, p, w
end

function TOOLTIP.SetText(self, text)
    self.Actor:GetChild("Text"):settext(text)
    local height = self.Actor:GetChild("Text"):GetHeight() * textScale
    local width = self.Actor:GetChild("Text"):GetWidth() * textScale

    self.Actor:GetChild("Box"):zoomto(width + boxBorder * 2 / textScale, height + boxBorder * 2 / textScale)
end

function TOOLTIP.Show(self)
    self.Actor:visible(true)
end

function TOOLTIP.Hide(self)
    self.Actor:visible(false)
end

function TOOLTIP.ShowPointer(self)
    self.Pointer:visible(true)
end

function TOOLTIP.HidePointer(self)
    self.Pointer:visible(false)
end

function TOOLTIP.SetPosition(self, x, y)
    local height = (self.Actor:GetChild("Text"):GetHeight() * textScale) + boxBorder * 2 / textScale
    local width = (self.Actor:GetChild("Text"):GetWidth() * textScale) + boxBorder * 2 / textScale

    self.Pointer:xy(x, y)

    if sideswapped then
        self.Actor:xy(
            clamp(x - tooltipOffSetX, screenBorder + width, SCREEN_WIDTH - screenBorder), 
            clamp(y + tooltipOffSetY, screenBorder, SCREEN_HEIGHT - screenBorder - height)
        )
        self.Actor:GetChild("Box"):halign(1)
        self.Actor:GetChild("Text"):halign(1):x(-boxBorder / textScale)
    else
        self.Actor:xy(
            clamp(x + tooltipOffSetX, screenBorder, SCREEN_WIDTH - screenBorder - width), 
            clamp(y + tooltipOffSetY, screenBorder, SCREEN_HEIGHT - screenBorder - height)
        )
        self.Actor:GetChild("Box"):halign(0)
        self.Actor:GetChild("Text"):halign(0):x(boxBorder / textScale)
    end

    -- if the mouse ends up on top of the tooltip we may lose visibility on things
    -- so swap its horizontal position
    if isOver(self.Actor:GetChild("Box")) then
        sideswapped = not sideswapped
    end
end