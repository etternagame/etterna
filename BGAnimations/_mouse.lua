local screenName = ...
local topScreen

assert(type(screenName) == "string", "Screen Name must be specified when loading _mouse.lua")
BUTTON:ResetButtonTable(screenName)

local t = Def.ActorFrame{
    OnCommand = function(self)
        topScreen = SCREENMAN:GetTopScreen()
        topScreen:AddInputCallback(BUTTON.InputCallback)
        TOOLTIP:ShowPointer()
    end,
    OffCommand = function(self)
        BUTTON:ResetButtonTable(screenName)
        TOOLTIP:Hide()
    end,
    CancelCommand = function(self)
        self:playcommand("Off")
    end
}

return t