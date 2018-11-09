local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local c

local function arbitraryComboX(value)
	c.Label:x(value)
	c.Number:x(value - 4)
end

local function arbitraryComboZoom(value)
	c.Label:zoom(value)
	c.Number:zoom(value - 0.1)
end

local ShowComboAt = THEME:GetMetric("Combo", "ShowComboAt")

local t =
	Def.ActorFrame {
	InitCommand = function(self)
		self:vertalign(bottom)
	end,
	LoadFont("Combo", "numbers") ..
		{
			Name = "Number",
			InitCommand = function(self)
				self:xy(MovableValues.ComboX - 4, MovableValues.ComboY):zoom(MovableValues.ComboZoom - 0.1):halign(1):valign(1):skewx(-0.125):visible(
					false
				)
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "Label",
			InitCommand = function(self)
				self:xy(MovableValues.ComboX, MovableValues.ComboY):zoom(MovableValues.ComboZoom):diffusebottomedge(color("0.75,0.75,0.75,1")):halign(0):valign(
					1
				):visible(false)
			end
		},
	-- MovableBorder(0, 0, 1, MovableValues.ComboX, MovableValues.ComboY),
	InitCommand = function(self)
		c = self:GetChildren()
		if (allowedCustomization) then
			Movable.DeviceButton_3.element = c
			Movable.DeviceButton_4.element = c
			Movable.DeviceButton_3.condition = true
			Movable.DeviceButton_4.condition = true
			Movable.DeviceButton_4.Border = self:GetChild("Border")
			Movable.DeviceButton_3.DeviceButton_left.arbitraryFunction = arbitraryComboX
			Movable.DeviceButton_3.DeviceButton_right.arbitraryFunction = arbitraryComboX
			Movable.DeviceButton_4.DeviceButton_up.arbitraryFunction = arbitraryComboZoom
			Movable.DeviceButton_4.DeviceButton_down.arbitraryFunction = arbitraryComboZoom
		end
	end,
	ComboCommand = function(self, param)
		local iCombo = param.Combo
		if not iCombo or iCombo < ShowComboAt then
			c.Number:visible(false)
			c.Label:visible(false)
			return
		end

		c.Label:settext("COMBO")
		c.Number:visible(true)
		c.Label:visible(true)
		c.Number:settext(iCombo)

		-- FullCombo Rewards
		if param.FullComboW1 then
			c.Number:diffuse(color("#00aeef"))
			c.Number:glowshift()
		elseif param.FullComboW2 then
			c.Number:diffuse(color("#fff568"))
			c.Number:glowshift()
		elseif param.FullComboW3 then
			c.Number:diffuse(color("#a4ff00"))
			c.Number:stopeffect()
		elseif param.Combo then
			c.Number:diffuse(Color("White"))
			c.Number:stopeffect()
			c.Label:diffuse(Color("Blue"))
			c.Label:diffusebottomedge(color("0.75,0.75,0.75,1"))
		else
			c.Number:diffuse(color("#ff0000"))
			c.Number:stopeffect()
			c.Label:diffuse(Color("Red"))
			c.Label:diffusebottomedge(color("0.5,0,0,1"))
		end
	end
}

return t
