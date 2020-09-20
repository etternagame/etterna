local boxWidth = SCREEN_WIDTH / 2.5
local boxHeight = SCREEN_HEIGHT / 3

local dimAlpha = 0.6
local bgColor = color("0,0,0")
local boxColor = color("0,0,0")

return Def.ActorFrame {
    Name = "TextEntryUnderlayFile",
    InitCommand = function(self)
        self:diffusealpha(0)
    end,
    OnCommand = function(self)
        self:smooth(0.5)
        self:diffusealpha(1)
        local question = self:GetParent():GetChild("Question")
        if question then
            question:maxwidth(boxWidth / question:GetZoom())
        end
        local answer = self:GetParent():GetChild("Answer")
        if answer then
            answer:maxwidth(boxWidth / answer:GetZoom())
        end
    end,

    Def.Quad {
        Name = "DimBG",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
            self:diffuse(bgColor)
            self:diffusealpha(dimAlpha)
        end
    },
    Def.Quad {
        Name = "MainBG",
        InitCommand = function(self)
            self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y)
            self:zoomto(boxWidth, boxHeight)
            self:diffuse(boxColor)
            self:diffusealpha(1)
        end
    }
}