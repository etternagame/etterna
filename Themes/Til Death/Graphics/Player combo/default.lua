local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local c
local enabledCombo = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ComboText

local function arbitraryComboX(value) 
	c.Label:x(value) 
	c.Number:x(value - 4)
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

local t =
	Def.ActorFrame {
	InitCommand = function(self)
		self:vertalign(bottom)
	end,
	LoadFont("Combo", "numbers") ..
		{
			Name = "Number",
			InitCommand = function(self)
				self:xy(MovableValues.ComboX - 4, MovableValues.ComboY):halign(1):valign(1):skewx(-0.125):visible(
					false
				)
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "Label",
			InitCommand = function(self)
				self:xy(MovableValues.ComboX, MovableValues.ComboY):diffusebottomedge(color("0.75,0.75,0.75,1")):halign(0):valign(
					1
				):visible(false)
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
			c.Label:settext(translated_combo)
			c.Number:visible(true)
			c.Label:visible(true)
			c.Number:settext(1000)
			Movable.DeviceButton_3.propertyOffsets = {self:GetTrueX() -6, self:GetTrueY() + c.Number:GetHeight()*1.5}	-- centered to screen/valigned
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

		c.Label:settext(translated_combo)
		c.Number:visible(true)
		c.Label:visible(true)
		c.Number:settext(iCombo)

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
	end,
	MovableBorder(0, 0, 1, MovableValues.ComboX, MovableValues.ComboY),
}

if enabledCombo then
	return t
end

return Def.ActorFrame {}
