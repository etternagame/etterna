local c
local enabledCombo = playerConfig:get_data().ComboText
local enabledLabel = playerConfig:get_data().ComboLabel

local function arbitraryComboZoom(value)
    c.Label:zoom(value)
    c.Number:zoom(value - 0.1)
end

local ShowComboAt = THEME:GetMetric("Combo", "ShowComboAt")
local labelColor = getComboColor("ComboLabel")
local mfcNumbers = getComboColor("MarvFullCombo")
local pfcNumbers = getComboColor("PerfFullCombo")
local fcNumbers = getComboColor("FullCombo")
local regNumbers = getComboColor("RegularCombo")

local translated_combo = "Combo"--THEME:GetString("ScreenGameplay", "ComboText")

local t = Def.ActorFrame {
    Name = "Combo",
    InitCommand = function(self)
        c = self:GetChildren()
        -- queued to execute slightly late
        self:queuecommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    OnCommand = function(self)
        if (allowedCustomization) then
            c.Number:visible(true)
            c.Number:settext(1000)
            c.Label:visible(enabledLabel)
            c.Label:settext(translated_combo)
        end
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.ComboX, MovableValues.ComboY)
        arbitraryComboZoom(MovableValues.ComboZoom)
    end,
    ComboCommand = function(self, param)
        local iCombo = param.Combo
        if not iCombo or iCombo < ShowComboAt then
            c.Number:visible(false)
            c.Label:visible(false)
            return
        end

        c.Number:visible(true)
        c.Number:settext(iCombo)
        c.Label:visible(enabledLabel)
        c.Label:settext(translated_combo)

        c.BG:x(-c.Number:GetZoomedWidth() - (enabledLabel and 24 or 4))
        c.BG:zoomto(c.Number:GetZoomedWidth() + c.Label:GetZoomedWidth() + (enabledLabel and 24 or 4), c.Label:GetZoomedHeight())

        -- FullCombo Rewards
        if param.FullComboW1 then
            c.Number:diffuse(mfcNumbers)
            c.Number:glowshift()
        elseif param.FullComboW2 then
            c.Number:diffuse(pfcNumbers)
            c.Number:glowshift()
        elseif param.FullComboW3 then
            c.Number:diffuse(fcNumbers)
            c.Number:stopeffect()
        elseif param.Combo then
            c.Number:diffuse(regNumbers)
            c.Number:stopeffect()
            c.Label:diffuse(labelColor)
            c.Label:diffusebottomedge(color("0.75,0.75,0.75,1"))
        else
            -- probably for if you want to fade out the combo after a miss
            c.Number:diffuse(color("#ff0000"))
            c.Number:stopeffect()
            c.Label:diffuse(Color("Red"))
            c.Label:diffusebottomedge(color("0.5,0,0,1"))
        end
    end,

    Def.Quad { -- not normally visible but acts as a way for customize gameplay to hook into the combo size
        Name = "BG",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:visible(false)
        end,
    },
    LoadFont("Combo", "numbers") .. {
        Name = "Number",
        InitCommand = function(self)
            if enabledLabel then
                self:halign(1):valign(1)
                self:x(-4)
                self:skewx(-0.125)
                self:visible(false)
            else
                self:halign(0.5):valign(1)
                self:x(-24)
                self:skewx(-0.125)
                self:visible(false)
            end
        end
    },
    LoadFont("Common Normal") .. {
        Name = "Label",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:diffusebottomedge(color("0.75,0.75,0.75,1"))
            self:visible(false)
        end
    },
}

if enabledCombo then
    return t
end

return Def.ActorFrame {}
