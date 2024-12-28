local function UpdateLoop()
    local mouseX = INPUTFILTER:GetMouseX()
    local mouseY = INPUTFILTER:GetMouseY()
    TOOLTIP:SetPosition(mouseX, mouseY)
    BUTTON:UpdateMouseState()
end

local t = Def.ActorFrame {
    InitCommand = function(self)
        self:SetUpdateFunction(UpdateLoop)
        self:SetUpdateFunctionInterval(0.01)
        TOOLTIP:SetTextSize(0.5)
    end
}

local tooltip, pointer, clickwave = TOOLTIP:New()
t[#t+1] = tooltip
t[#t+1] = pointer
t[#t+1] = clickwave


return t;