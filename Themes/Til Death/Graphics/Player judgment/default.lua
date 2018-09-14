-- Removed all the protiming junk, it's obsoleted
local keymode = getCurrentKeyMode()
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local onePressed = false
local twoPressed = false
local changed = false
local c
local x = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeX
local y = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeY

-- CUZ WIDESCREEN DEFAULTS SCREAAAAAAAAAAAAAAAAAAAAAAAAAM -mina
if IsUsingWideScreen( ) then
	y = y - 5
	x = x + 5
end

local zoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].JudgeZoom

local JudgeCmds = {
	TapNoteScore_W1 = THEME:GetMetric( "Judgment", "JudgmentW1Command" ),
	TapNoteScore_W2 = THEME:GetMetric( "Judgment", "JudgmentW2Command" ),
	TapNoteScore_W3 = THEME:GetMetric( "Judgment", "JudgmentW3Command" ),
	TapNoteScore_W4 = THEME:GetMetric( "Judgment", "JudgmentW4Command" ),
	TapNoteScore_W5 = THEME:GetMetric( "Judgment", "JudgmentW5Command" ),
	TapNoteScore_Miss = THEME:GetMetric( "Judgment", "JudgmentMissCommand" ),
}

local TNSFrames = {
	TapNoteScore_W1 = 0;
	TapNoteScore_W2 = 1;
	TapNoteScore_W3 = 2;
	TapNoteScore_W4 = 3;
	TapNoteScore_W5 = 4;
	TapNoteScore_Miss = 5;
}

local function input(event)
	if event.DeviceInput.button == "DeviceButton_1" then
		onePressed = not (event.type == "InputEventType_Release")
	end
	if event.DeviceInput.button == "DeviceButton_2" then
		twoPressed = not (event.type == "InputEventType_Release")
	end
	if event.type ~= "InputEventType_Release" and onePressed then
		if event.DeviceInput.button == "DeviceButton_up" then
			y = y - 5
			c.Judgment:y(y)
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeY = y
			changed = true
		end
		if event.DeviceInput.button == "DeviceButton_down" then
			y = y + 5
			c.Judgment:y(y)
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeY = y
			changed = true
		end
		if event.DeviceInput.button == "DeviceButton_left" then
			x = x - 5
			c.Judgment:x(x)
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeX = x
			changed = true
		end
		if event.DeviceInput.button == "DeviceButton_right" then
			x = x + 5
			c.Judgment:x(x)
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeX = x
			changed = true
		end
		if changed then
			playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
			playerConfig:save(pn_to_profile_slot(PLAYER_1))
			changed = false
		end
	end
	if event.type ~= "InputEventType_Release" and twoPressed then
		if event.DeviceInput.button == "DeviceButton_up" then
			zoom = zoom + 0.01
			c.Judgment:zoom(zoom)
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].JudgeZoom	= zoom
		end
		if event.DeviceInput.button == "DeviceButton_down" then
			zoom = zoom - 0.01
			c.Judgment:zoom(zoom)
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].JudgeZoom	= zoom
		end
		if changed then
			playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
			playerConfig:save(pn_to_profile_slot(PLAYER_1))
			changed = false
		end
	end
	return false
end

local t = Def.ActorFrame {
	Def.Sprite {
		Texture = "../../../../"..getAssetPath("judgement"),
		Name="Judgment",
		InitCommand=function(self)
			self:pause():visible(false):xy(x,y):zoom(zoom)
		end,
		ResetCommand=function(self)
			self:finishtweening():stopeffect():visible(false)
		end,
	},
	
	InitCommand = function(self)
		c = self:GetChildren()
	end,
	OnCommand=function(self) 
		if(allowedCustomization) then
			SCREENMAN:GetTopScreen():AddInputCallback(input)
		end
	end,

	JudgmentMessageCommand=function(self, param)
		if param.HoldNoteScore then return end
		local iNumStates = c.Judgment:GetNumStates()
		local iFrame = TNSFrames[param.TapNoteScore]
		if not iFrame then return end
		
		self:playcommand("Reset")
		c.Judgment:visible(true)
		c.Judgment:setstate(iFrame)
		JudgeCmds[param.TapNoteScore](c.Judgment)
	end
}

if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgmentText then
	return t
end

return {}


