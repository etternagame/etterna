local screenName = Var("LoadingScreen") or ...
local topScreen

assert(type(screenName) == "string", "Screen Name was missing when loading _mouse.lua")
BUTTON:ResetButtonTable(screenName)

local function cursorCheck()
    -- show cursor if in fullscreen
    if not PREFSMAN:GetPreference("Windowed") and not PREFSMAN:GetPreference("FullscreenIsBorderlessWindow") then
        TOOLTIP:ShowPointer()
    else
        TOOLTIP:HidePointer()
    end
end

local t = Def.ActorFrame{
    OnCommand = function(self)
        topScreen = SCREENMAN:GetTopScreen()
        topScreen:AddInputCallback(BUTTON.InputCallback)
        cursorCheck()
    end,
    OffCommand = function(self)
        BUTTON:ResetButtonTable(screenName)
        TOOLTIP:Hide()
    end,
    CancelCommand = function(self)
        self:playcommand("Off")
    end,
    WindowedChangedMessageCommand = function(self)
        cursorCheck()
    end
}

return t