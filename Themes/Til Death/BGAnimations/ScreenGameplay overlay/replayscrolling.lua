local modifierPressed = false
local forward = true

local queuecommand = Actor.queuecommand

local scroller -- just an alias for the actor that runs the commands

local function input(event)
	--SCREENMAN:SystemMessage(event.DeviceInput.button)
	if event.DeviceInput.button == "DeviceButton_right ctrl" or event.DeviceInput.button == "DeviceButton_left ctrl" then
		modifierPressed = not (event.type == "InputEventType_Release")
	end
	if event.DeviceInput.button == "DeviceButton_right shift" or event.DeviceInput.button == "DeviceButton_left shift" then
		ratePressed = not (event.type == "InputEventType_Release")
	end
	if event.type ~= "InputEventType_Release" then
		if event.GameButton == "EffectUp" then
			forward = true
			if ratePressed then
				scroller:queuecommand("ReplayRate")
			else
				scroller:queuecommand("ReplayScroll")
			end
		elseif event.GameButton == "EffectDown" then
			forward = false
			if ratePressed then
				scroller:queuecommand("ReplayRate")
			else
				scroller:queuecommand("ReplayScroll")
			end
		end
	end
end

local function getNewSongPos()
	currentpos = SCREENMAN:GetTopScreen():GetSongPosition()
	newpos = currentpos + (modifierPressed and 0.1 or 5) * (forward and 1 or -1)
	SCREENMAN:SystemMessage(string.format("%f to %f", currentpos, newpos))
	return newpos
end

local function getNewRate()
	currentrate = getCurRateValue()
	newrate = currentrate + (modifierPressed and 0.05 or 0.1) * (forward and 1 or -1)
	SCREENMAN:SystemMessage(string.format("%f to %f", currentrate, newrate))
	return newrate
end



scroller = Def.ActorFrame {
	Name = "ScrollManager",
	OnCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback( input )
		scroller = self
	end,
	ReplayScrollCommand = function(self)
		newpos = getNewSongPos()
		SCREENMAN:GetTopScreen():SetReplayPosition( newpos )
	end,
	ReplayRateCommand = function(self)
		newrate = getNewRate()
		SCREENMAN:GetTopScreen():SetReplayRate( newrate )
	end
	

}
return scroller