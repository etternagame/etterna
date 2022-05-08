local t = Def.ActorFrame {}
-- Controls the overlay ScreenSystemLayer
-- meant more for critical messages
-- this is mostly identical to the Til Death instance of this file
-- but is not stored in fallback in this condition due to the possibility of not supporting some features of this overlay
-- for example, we may not want to display downloads on the overlay or something else

-- Text
t[#t + 1] = Def.ActorFrame {
    Def.Quad {
        InitCommand = function(self)
            self:zoomtowidth(SCREEN_WIDTH)
            self:zoomtoheight(30)
            self:halign(0):valign(0)
            self:y(SCREEN_TOP)
            self:diffuse(color("0,0,0,0"))
        end,
        OnCommand = function(self)
            self:finishtweening()
            self:diffusealpha(0.85)
        end,
        OffCommand = function(self)
            self:sleep(3):linear(0.5)
            self:diffusealpha(0)
        end
    },
    Def.BitmapText {
        Font = "Common Normal",
        Name = "Text",
        InitCommand = function(self)
            self:maxwidth(SCREEN_WIDTH * 0.8)
            self:halign(0):valign(0)
            self:xy(SCREEN_LEFT + 10, SCREEN_TOP + 10)
            self:diffusealpha(0)
        end,
        OnCommand = function(self)
            self:finishtweening()
            self:diffusealpha(1)
            self:zoom(0.5)
        end,
        OffCommand = function(self)
            self:sleep(3):linear(0.5)
            self:diffusealpha(0)
        end
    },
    SystemMessageMessageCommand = function(self, params)
        self:GetChild("Text"):settext(params.Message)
        self:playcommand("On")
        if params.NoAnimate then
            self:finishtweening()
        end
        self:playcommand("Off")
    end,
    HideSystemMessageMessageCommand = function(self)
        self:finishtweening()
    end
}

-- song reload
local www = 1366 * 0.8
local hhh = SCREEN_HEIGHT * 0.8
local rtzoom = 0.6

t[#t + 1] =
    Def.ActorFrame {
    DFRStartedMessageCommand = function(self)
        self:visible(true)
    end,
    DFRFinishedMessageCommand = function(self, params)
        self:visible(false)
    end,
    BeginCommand = function(self)
        self:visible(false)
        self:x(www / 8 + 10)
        self:y(SCREEN_BOTTOM - hhh / 8 - 70)
    end,
    Def.Quad {
        InitCommand = function(self)
            self:zoomto(www / 4, hhh / 4)
            self:diffuse(color("0.1,0.1,0.1,0.8"))
        end
    },
    Def.BitmapText {
        Font = "Common Normal",
        InitCommand = function(self)
            self:diffusealpha(0.9)
            self:zoom(rtzoom)
            self:settext("")
            self:maxwidth((www / 4 - 40) / rtzoom)
        end,
        DFRUpdateMessageCommand = function(self, params)
            self:settext(params.txt)
        end
    }
}

return t
