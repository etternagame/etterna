local keymode = getCurrentKeyMode()
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local values = {
	ReplayButtonsX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ReplayButtonsX,
	ReplayButtonsY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ReplayButtonsY,
	ReplayButtonsSpacing = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].ReplayButtonsSpacing,
	ReplayButtonsZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].ReplayButtonsZoom
}

local buttons = {}
local function spaceButtons(value)
	for i, b in ipairs(buttons) do
		b:addy((i-1) * value)
	end
end

local propsFunctions = {
	X = Actor.x,
	Y = Actor.y,
	Zoom = Actor.zoom
}

local modifierPressed = false
local forward = true

local scroller  -- just an alias for the actor that runs the commands

local movable = {
	current = "",
	pressed = false,
	DeviceButton_f = {
		name = "ReplayButtons",
		element = {},
		elementTree = "GameplayXYCoordinates",
		condition = true,
		DeviceButton_up = {
			property = "Y",
			inc = -3
		},
		DeviceButton_down = {
			property = "Y",
			inc = 3
		},
		DeviceButton_left = {
			property = "X",
			inc = -3
		},
		DeviceButton_right = {
			property = "X",
			inc = 3
		}
	},
	DeviceButton_g = {
		name = "ReplayButtons",
		element = {},
		elementTree = "GameplaySizes",
		condition = true,
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		}
	},
	DeviceButton_h = {
		name = "ReplayButtons",
		elementTree = "GameplaySizes",
		condition = true,
		DeviceButton_up = {
			arbitraryFunction = spaceButtons,
			property = "Spacing",
			inc = -0.5
		},
		DeviceButton_down = {
			arbitraryFunction = spaceButtons,
			property = "Spacing",
			inc = 0.5
		},
	},
}

local function movableInput(event)
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
			curKey.arbitraryFunction(curKey.inc)
		else
			propsFunctions[curKey.property](current.element, newVal)
		end
		playerConfig:get_data(pn_to_profile_slot(PLAYER_1))[current.elementTree][keymode][prop] = newVal
		playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
		playerConfig:save(pn_to_profile_slot(PLAYER_1))
	end
	return false
end

local function input(event)
	--SCREENMAN:SystemMessage(event.DeviceInput.button)
	if event.DeviceInput.button == "DeviceButton_right ctrl" or event.DeviceInput.button == "DeviceButton_left ctrl" then
		modifierPressed = not (event.type == "InputEventType_Release")
	end
	if event.DeviceInput.button == "DeviceButton_right shift" or event.DeviceInput.button == "DeviceButton_left shift" then
		ratePressed = not (event.type == "InputEventType_Release")
	end
	if event.DeviceInput.button == "DeviceButton_right alt" or event.DeviceInput.button == "DeviceButton_left alt" then
		bookmarkPressed = not (event.type == "InputEventType_Release")
	end
	if event.type ~= "InputEventType_Release" then
		if event.GameButton == "EffectUp" then
			if bookmarkPressed then
				scroller:queuecommand("ReplayBookmarkSet")
				return 0
			end
			forward = true
			if ratePressed then
				scroller:queuecommand("ReplayRate")
			else
				scroller:queuecommand("ReplayScroll")
			end
		elseif event.GameButton == "EffectDown" then
			if bookmarkPressed then
				scroller:queuecommand("ReplayBookmarkGoto")
				return 0
			end
			forward = false
			if ratePressed then
				scroller:queuecommand("ReplayRate")
			else
				scroller:queuecommand("ReplayScroll")
			end
		elseif event.GameButton == "Coin" then
			scroller:queuecommand("ReplayPauseToggle")
		end
	end
end

local function getNewSongPos()
	currentpos = SCREENMAN:GetTopScreen():GetSongPosition()
	newpos = currentpos + (modifierPressed and 0.1 or 5) * (forward and 1 or -1)
	--SCREENMAN:SystemMessage(string.format("%f to %f", currentpos, newpos))
	return newpos
end

local function getNewRate()
	currentrate = GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate()
	newrate = currentrate + (modifierPressed and 0.05 or 0.1) * (forward and 1 or -1)
	--SCREENMAN:SystemMessage(string.format("%f to %f", currentrate, newrate))
	return newrate
end

scroller =
	Def.ActorFrame {
	Name = "ScrollManager",
	OnCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
		if allowedCustomization then
			SCREENMAN:GetTopScreen():AddInputCallback(movableInput)
		end
		scroller = self
	end,
	ReplayScrollCommand = function(self)
		newpos = getNewSongPos()
		SCREENMAN:GetTopScreen():SetReplayPosition(newpos)
	end,
	ReplayRateCommand = function(self)
		newrate = getNewRate()
		givenrate = SCREENMAN:GetTopScreen():SetReplayRate(newrate)
		if givenrate ~= nil then
			realnewrate = notShit.round(givenrate, 3)
		--SCREENMAN:SystemMessage(string.format("Set rate to %f", realnewrate))
		end
	end,
	ReplayPauseToggleCommand = function(self)
		SCREENMAN:GetTopScreen():ToggleReplayPause()
	end,
	ReplayBookmarkSetCommand = function(self)
		position = SCREENMAN:GetTopScreen():GetSongPosition()
		SCREENMAN:GetTopScreen():SetReplayBookmark(position)
	end,
	ReplayBookmarkGotoCommand = function(self)
		SCREENMAN:GetTopScreen():JumpToReplayBookmark()
	end
}
local span = 50
local x = -1 * span
local function button(txt, click)
	x = x + span
	return Widg.Button {
		text = txt,
		width = 60,
		height = 30,
		halign = 1,
		bgColor = getMainColor("highlight"),
		highlight = {color = getMainColor("positive")},
		border = {color = getMainColor("highlight"), width = 2},
		onClick = click,
		y = x + 50,
		onInit = function(self)
			buttons[#buttons+1] = self
		end
	}
end

scroller[#scroller + 1] =
	Widg.Container {
	x = values.ReplayButtonsX,
	y = values.ReplayButtonsY,
	onInit = function(self)
		movable.DeviceButton_f.element = self
		movable.DeviceButton_g.element = self
		self:zoom(values.ReplayButtonsZoom)
		spaceButtons(values.ReplayButtonsSpacing)
	end,
	content = {
		button(
			"Pause",
			function(self)
				SCREENMAN:GetTopScreen():ToggleReplayPause()
				local paused = GAMESTATE:IsPaused()
				self.label.actor:settext(paused and "Play" or "Pause")
			end
		),
		button(
			"Fast Forward",
			function()
				SCREENMAN:GetTopScreen():SetReplayPosition(SCREENMAN:GetTopScreen():GetSongPosition() + 5)
			end
		),
		button(
			"Rewind",
			function()
				SCREENMAN:GetTopScreen():SetReplayPosition(SCREENMAN:GetTopScreen():GetSongPosition() - 5)
			end
		)
	}
}
return scroller
