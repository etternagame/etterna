--file containing stuff for cursors.
--this should only be loaded by screen overlays,
--otherwise the inputcallback function won't be able to find the actors.

local screenName = Var("LoadingScreen") or ...
local topScreen
BUTTON:ResetButtonTable(screenName)

local function UpdateLoop()
    local mouseX = INPUTFILTER:GetMouseX()
    local mouseY = INPUTFILTER:GetMouseY()
    TOOLTIP:SetPosition(mouseX, mouseY)
    BUTTON:UpdateMouseState()

    return false
end

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
		self:SetUpdateFunction(UpdateLoop)
        self:SetUpdateFunctionInterval(1 / DISPLAY:GetDisplayRefreshRate())
        topScreen = SCREENMAN:GetTopScreen()
        topScreen:AddInputCallback(BUTTON.InputCallback)
        TOOLTIP:SetTextSize(0.35)
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
    end,
    ReloadedScriptsMessageCommand = function(self)
        cursorCheck()
    end,
}

local tooltip, pointer, clickwave = TOOLTIP:New()
t[#t+1] = tooltip
t[#t+1] = pointer
t[#t+1] = clickwave


return t
