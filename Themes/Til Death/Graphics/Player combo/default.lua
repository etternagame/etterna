local keymode = getCurrentKeyMode()
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local c
local values = {
	ComboX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ComboX,
	ComboY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ComboY,
	ComboZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].ComboZoom
}

local function arbitraryComboX(value)
	c.Label:x(value)
	c.Number:x(value-4)
end 

local function arbitraryComboZoom(value)
	c.Label:zoom(value)
	c.Number:zoom(value - 0.1)
end

local propsFunctions = {
	Y = Actor.y,
	Zoom = Actor.zoom
}

local movable = { 
	current = "",
	pressed = false,
	DeviceButton_3 = {
		name = "Combo",
		element = { },
		children = { "Label", "Number" },
		properties = { "X", "Y" },
		elementTree = "GameplayXYCoordinates",
		condition = true,
		DeviceButton_up = {
			property = "Y",
			inc = -5
		},
		DeviceButton_down = {
			property = "Y",
			inc = 5
		},
		DeviceButton_left = {
			arbitraryFunction = arbitraryComboX,
			property = "X",
			inc = -5
		},
		DeviceButton_right = {
			arbitraryFunction = arbitraryComboX,
			property = "X",
			inc = 5
		},
	},
	DeviceButton_4 = {
		name = "Combo",
		element = { },
		children = { "Label", "Number" },
		properties = { "Zoom" },
		elementTree = "GameplaySizes",
		condition = true,
		DeviceButton_up = {
			arbitraryFunction = arbitraryComboZoom,
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			arbitraryFunction = arbitraryComboZoom,
			property = "Zoom",
			inc = -0.01
		},
	},
}

local ShowComboAt = THEME:GetMetric("Combo", "ShowComboAt");

local function input(event)
	if getAutoplay() ~= 0 then
		local button = event.DeviceInput.button
		local notReleased = not (event.type == "InputEventType_Release")
		if movable[button] then
			movable.pressed = notReleased
			movable.current = button
		end

		local current = movable[movable.current]
		if movable.pressed and current[button] and current.condition and notReleased then
			local curKey = current[button]
			local prop = current.name .. curKey.property
			local newVal = values[prop] + curKey.inc
			values[prop] = newVal
			if curKey.arbitraryFunction then
				curKey.arbitraryFunction(newVal)
			else
				for _, attribute in ipairs(current.children) do
					propsFunctions[curKey.property](current.element[attribute], newVal)	
				end
			end
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1))[current.elementTree][keymode][prop] = newVal
			playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
			playerConfig:save(pn_to_profile_slot(PLAYER_1))
		end
	end
	return false
end

local t = Def.ActorFrame {
	InitCommand=function(self)
		self:vertalign(bottom)
	end,
	LoadFont( "Combo", "numbers" ) .. {
		Name="Number",
		InitCommand=function(self)
			self:xy(values.ComboX-4,values.ComboY):zoom(values.ComboZoom - 0.1):halign(1):valign(1):skewx(-0.125):visible(false)
		end,
	},
	LoadFont("Common Normal") .. {
		Name="Label",
		InitCommand=function(self)
			self:xy(values.ComboX,values.ComboY):zoom(values.ComboZoom):diffusebottomedge(color("0.75,0.75,0.75,1")):halign(0):valign(1):visible(false)
		end,
	},
	InitCommand = function(self)
		c = self:GetChildren()
		movable.DeviceButton_3.element = c
		movable.DeviceButton_4.element = c
	end,
	OnCommand=function(self) 
		if(allowedCustomization) then
			SCREENMAN:GetTopScreen():AddInputCallback(input)
		end
	end,
	ComboCommand=function(self, param)
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
