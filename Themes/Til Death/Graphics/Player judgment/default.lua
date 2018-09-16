-- Removed all the protiming junk, it's obsoleted
local keymode = getCurrentKeyMode()
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local c
local values = {
	JudgeX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeX,
	JudgeY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeY,
	JudgeZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].JudgeZoom
}
-- CUZ WIDESCREEN DEFAULTS SCREAAAAAAAAAAAAAAAAAAAAAAAAAM -mina
if IsUsingWideScreen( ) then
	values.JudgeY = values.JudgeY - 5
	values.JudgeX = values.JudgeX + 5
end

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

local propsFunctions = {
	X = Actor.x,
	Y = Actor.y,
	Zoom = Actor.zoom
}

local movable = { 
	current = "",
	pressed = false,
	DeviceButton_1 = {
		name = "Judge",
		element = { },
		children = { "Judgment" },
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
			property = "X",
			inc = -5
		},
		DeviceButton_right = {
			property = "X",
			inc = 5
		},
	},
	DeviceButton_2 = {
		name = "Judge",
		element = { },
		children = { "Judgment" },
		properties = { "Zoom" },
		elementTree = "GameplaySizes",
		condition = true,
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		},
	},
}

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
			for _, attribute in ipairs(current.children) do
				propsFunctions[curKey.property](current.element[attribute], newVal)	
			end
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1))[current.elementTree][keymode][prop] = newVal
			playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
			playerConfig:save(pn_to_profile_slot(PLAYER_1))
		end
	end
	return false
end

local t = Def.ActorFrame {
	LoadActor(THEME:GetPathG("Judgment","Normal")) .. {
		Name="Judgment",
		InitCommand=function(self)
			self:pause():visible(false):xy(values.JudgeX,values.JudgeY):zoom(values.JudgeZoom)
		end,
		ResetCommand=function(self)
			self:finishtweening():stopeffect():visible(false)
		end,
	},
	
	InitCommand = function(self)
		c = self:GetChildren()
		movable.DeviceButton_1.element = c
		movable.DeviceButton_2.element = c
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


