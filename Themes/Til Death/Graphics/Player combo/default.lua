local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local c
local enabledCombo = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ComboText
local CenterCombo = CenteredComboEnabled()
local CTEnabled = ComboTweensEnabled()


local function numberZoom()
    return math.max((MovableValues.ComboZoom) - 0.1, 0)
end

local function labelZoom()
    return math.max(MovableValues.ComboZoom, 0)
end

--[[
	-- old Pulse function from [Combo]:
	%function(self,param) self:stoptweening(); self:zoom(1.1*param.Zoom); self:linear(0.05); self:zoom(param.Zoom); end
]]
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

local function arbitraryComboX(value)
	c.Label:x(value)
	if not CenterCombo then
		c.Number:x(value - 4)
	else
		c.Number:x(value - 24)
	end
	c.Border:x(value)
  end

local function arbitraryComboZoom(value)
	c.Label:zoom(value)
	c.Number:zoom(value - 0.1)
	if allowedCustomization then
		c.Border:playcommand("ChangeWidth", {val = c.Number:GetZoomedWidth() + c.Label:GetZoomedWidth()})
		c.Border:playcommand("ChangeHeight", {val = c.Number:GetZoomedHeight()})
	end
end

local ShowComboAt = THEME:GetMetric("Combo", "ShowComboAt")
local labelColor = getComboColor("ComboLabel")
local mfcNumbers = getComboColor("Marv_FullCombo")
local pfcNumbers = getComboColor("Perf_FullCombo")
local fcNumbers = getComboColor("FullCombo")
local regNumbers = getComboColor("RegularCombo")

local translated_combo = THEME:GetString("ScreenGameplay", "ComboText")

local t = Def.ActorFrame {
	InitCommand = function(self)
		self:vertalign(bottom)
	end,
	LoadFont("Combo", "numbers") .. {
		Name = "Number",
		InitCommand = function(self)
			if not CenterCombo then
				self:halign(1):valign(1):skewx(-0.125)
				self:xy(MovableValues.ComboX - 4, MovableValues.ComboY)
				self:visible(false)
			else
				self:halign(0.5):valign(1):skewx(-0.125)
				self:xy(MovableValues.ComboX - 24, MovableValues.ComboY)
				self:visible(false)
			end
		end
	},
	LoadFont("Common Normal") .. {
		Name = "Label",
		InitCommand = function(self)
			self:halign(0):valign(1)
			self:xy(MovableValues.ComboX, MovableValues.ComboY)
			self:diffusebottomedge(color("0.75,0.75,0.75,1"))
			self:visible(false)
		end
	},
	InitCommand = function(self)
		c = self:GetChildren()
		if (allowedCustomization) then
			Movable.DeviceButton_3.element = c
			Movable.DeviceButton_4.element = c
			Movable.DeviceButton_3.condition = enabledCombo
			Movable.DeviceButton_4.condition = enabledCombo
			Movable.DeviceButton_3.Border = self:GetChild("Border")
			Movable.DeviceButton_3.DeviceButton_left.arbitraryFunction = arbitraryComboX
			Movable.DeviceButton_3.DeviceButton_right.arbitraryFunction = arbitraryComboX
			Movable.DeviceButton_4.DeviceButton_up.arbitraryFunction = arbitraryComboZoom
			Movable.DeviceButton_4.DeviceButton_down.arbitraryFunction = arbitraryComboZoom
		end
	end,
	OnCommand = function(self)
		if (allowedCustomization) then
			c.Number:visible(true)
			c.Number:settext(1000)
			c.Label:visible(not CenterCombo)
			c.Label:settext(translated_combo)

			Movable.DeviceButton_3.propertyOffsets = {self:GetTrueX() -6, self:GetTrueY()}	-- centered to screen/valigned
			setBorderAlignment(c.Border, 0.5, 1)
		end
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
		c.Label:visible(not CenterCombo)
		c.Label:settext(translated_combo)

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
			-- I actually don't know what this is.
			-- It's probably for if you want to fade out the combo after a miss.
			-- Oh well; Til death doesn't care.		-poco
			c.Number:diffuse(color("#ff0000"))
			c.Number:stopeffect()
			c.Label:diffuse(Color("Red"))
			c.Label:diffusebottomedge(color("0.5,0,0,1"))
		end

		--Animations
		if CTEnabled then
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
	MovableBorder(0, 0, 1, MovableValues.ComboX, MovableValues.ComboY),
}

if enabledCombo then
	return t
end

return Def.ActorFrame {}
