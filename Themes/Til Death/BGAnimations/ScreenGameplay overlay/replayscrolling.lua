local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay

local buttons = {}
local bobo
local function spaceButtons(value)
	for i, b in ipairs(buttons) do
		b:addy((i-1) * value)
	end
	bobo:playcommand("ChangeHeight", {val = buttons[#buttons]:GetChild("Label"):GetY() +  buttons[#buttons]:GetChild("Label"):GetHeight()/2})
end

local modifierPressed = false
local forward = true

local scroller  -- just an alias for the actor that runs the commands

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
	Name = "ReplayButtons",
	InitCommand = function(self)
		self:xy(MovableValues.ReplayButtonsX, MovableValues.ReplayButtonsY)
		if allowedCustomization then
			bobo = self:GetChild("Border")
			Movable.DeviceButton_f.element = self
			Movable.DeviceButton_f.condition = true
			--Movable.DeviceButton_g.element = self
			--Movable.DeviceButton_g.condition = true
			--Movable.DeviceButton_g.Border = bobo
			spaceButtons(MovableValues.ReplayButtonsSpacing)
			bobo:playcommand("ChangeWidth", {val = buttons[1]:GetChild("Label"):GetWidth()})
			--bobo:playcommand("ChangeZoom", {val = MovableValues.ReplayButtonsZoom}) 
			setBorderAlignment(bobo, 0.5, 0)
		end
	end,
	OnCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
		scroller = self
	end,
	ReplayScrollCommand = function(self)
		newpos = getNewSongPos()
		SCREENMAN:GetTopScreen():SetSongPosition(newpos)
	end,
	ReplayRateCommand = function(self)
		newrate = getNewRate()
		givenrate = SCREENMAN:GetTopScreen():SetRate(newrate)
		if givenrate ~= nil then
			realnewrate = notShit.round(givenrate, 3)
		--SCREENMAN:SystemMessage(string.format("Set rate to %f", realnewrate))
		end
	end,
	ReplayPauseToggleCommand = function(self)
		SCREENMAN:GetTopScreen():TogglePause()
	end,
	ReplayBookmarkSetCommand = function(self)
		position = SCREENMAN:GetTopScreen():GetSongPosition()
		SCREENMAN:GetTopScreen():SetBookmark(position)
	end,
	ReplayBookmarkGotoCommand = function(self)
		SCREENMAN:GetTopScreen():JumpToBookmark()
	end,
}
local span = 50
local x = -1 * span

local translated_info = {
	Pause = THEME:GetString("ScreenGameplay", "ButtonPause"),
	FastForward = THEME:GetString("ScreenGameplay", "ButtonFastForward"),
	Rewind = THEME:GetString("ScreenGameplay", "ButtonRewind"),
	Play = THEME:GetString("ScreenGameplay", "ButtonPlay")
}

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
		y = x,
		onInit = function(self)
			buttons[#buttons+1] = self.actor
		end
	}
end
scroller[#scroller + 1] = MovableBorder(0, 0, 1, 0, 0)
scroller[#scroller + 1] =
	Widg.Container {
	name = "ReplayButtons",
	onInit = function(self)
		if allowedCustomization then
			Movable.DeviceButton_h.condition = true
			Movable.DeviceButton_h.DeviceButton_up.arbitraryFunction = spaceButtons
			Movable.DeviceButton_h.DeviceButton_down.arbitraryFunction = spaceButtons
		end
	end,
	content = {
		button(
			translated_info["Pause"],
			function(self)
				SCREENMAN:GetTopScreen():TogglePause()
				local paused = GAMESTATE:IsPaused()
				self.label.actor:settext(paused and translated_info["Play"] or translated_info["Pause"])
			end
		),
		button(
			translated_info["FastForward"],
			function()
				SCREENMAN:GetTopScreen():SetSongPosition(SCREENMAN:GetTopScreen():GetSongPosition() + 5)
			end
		),
		button(
			translated_info["Rewind"],
			function()
				SCREENMAN:GetTopScreen():SetSongPosition(SCREENMAN:GetTopScreen():GetSongPosition() - 5)
			end
		),
	}
}
return scroller
