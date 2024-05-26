local screenName = Var("LoadingScreen") or ...
local topScreen

assert(type(screenName) == "string", "Screen Name was missing when loading _mouse.lua")
BUTTON:ResetButtonTable(screenName)


-- this file actually only just resets the mouse button functionality for the current screen
-- it also controls the visibility of the mouse cursor
-- if this file is loaded at all, it immediately checks to see if making the cursor visible is needed
-- the visible mouse is in Tooltip.lua, part of the TOOLTIP table


-- this determines whether or not mouse functionality is fully disabled until the mouse actually moves
-- to prevent moving elements from triggering hovers when the mouse didnt even move
local waitForMouseMovement = Var("waitForMouseMovement") or false
BUTTON:WaitForMouseMovement(waitForMouseMovement) -- this must be called after ResetButtonTable

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