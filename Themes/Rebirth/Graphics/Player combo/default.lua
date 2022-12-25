local c
local enabledCombo = playerConfig:get_data().ComboText
local enabledLabel = playerConfig:get_data().ComboLabel
local enableGlow = playerConfig:get_data().ComboGlow
local animateCombo = playerConfig:get_data().ComboTweens

local function numberZoom()
    return math.max((MovableValues.ComboZoom * 1.25) - 0.1, 0)
end

local function labelZoom()
    return math.max(MovableValues.ComboZoom * 1.25, 0)
end

local ShowComboAt = THEME:GetMetric("Combo", "ShowComboAt")
local labelColor = getComboColor("ComboLabel")
local mfcNumbers = getComboColor("MarvFullCombo")
local pfcNumbers = getComboColor("PerfFullCombo")
local fcNumbers = getComboColor("FullCombo")
local regNumbers = getComboColor("RegularCombo")

local Pulse = function(self, param)
	self:stoptweening()
	self:zoom(1.125 * param.Zoom * numberZoom())
	self:linear(0.05)
	self:zoom(param.Zoom * numberZoom())
end

local PulseLabel = function(self, param)
	self:stoptweening()
	self:zoom(1.125 * param.LabelZoom * labelZoom())
	self:linear(0.05)
	self:zoom(param.LabelZoom * labelZoom())
end

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
        c.Label:zoom(labelZoom())
        c.Number:zoom(numberZoom())
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
            if enableGlow then
                c.Number:glowshift()
            end
        elseif param.FullComboW2 then
            c.Number:diffuse(pfcNumbers)
            if enableGlow then
                c.Number:glowshift()
            end
        elseif param.FullComboW3 then
            c.Number:diffuse(fcNumbers)
            if enableGlow then
                c.Number:stopeffect()
            end
        elseif param.Combo then
            c.Number:diffuse(regNumbers)
            if enableGlow then
                c.Number:stopeffect()
            end
            c.Label:diffuse(labelColor)
            c.Label:diffusebottomedge(color("0.75,0.75,0.75,1"))
        else
            -- probably for if you want to fade out the combo after a miss
            c.Number:diffuse(color("#ff0000"))
            c.Number:stopeffect()
            c.Label:diffuse(Color("Red"))
            c.Label:diffusebottomedge(color("0.5,0,0,1"))
        end

		if animateCombo then
            local lb = 0.9
            local ub = 1.1
            local maxcombo = 100
            param.LabelZoom = scale( iCombo, 0, maxcombo, lb, ub )
            param.LabelZoom = clamp( param.LabelZoom, lb, ub )
            param.Zoom = scale( iCombo, 0, maxcombo, lb, ub )
            param.Zoom = clamp( param.Zoom, lb, ub )
			Pulse(c.Number, param)
            PulseLabel(c.Label, param)
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
                self:xy(-4, -0.5)
                self:skewx(-0.125)
                self:visible(false)
            else
                self:halign(0.5):valign(1)
                self:xy(-24, -0.5)
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
