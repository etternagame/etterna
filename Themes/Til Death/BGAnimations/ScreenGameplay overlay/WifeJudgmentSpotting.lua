--[[ 
	Basically rewriting the c++ code to not be total shit so this can also not be total shit.
]]
local keymode = getCurrentKeyMode()
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local jcKeys = tableKeys(colorConfig:get_data().judgment)
local jcT = {}										-- A "T" following a variable name will designate an object of type table.

for i=1, #jcKeys do
	jcT[jcKeys[i]] = byJudgment(jcKeys[i])
end

local jdgT = {										-- Table of judgments for the judgecounter 
	"TapNoteScore_W1",
	"TapNoteScore_W2",
	"TapNoteScore_W3",
	"TapNoteScore_W4",
	"TapNoteScore_W5",
	"TapNoteScore_Miss",
	"HoldNoteScore_Held",
	"HoldNoteScore_LetGo",
}

local dvCur																	
local jdgCur																-- Note: only for judgments with OFFSETS, might reorganize a bit later
local positive = getMainColor("positive")
local highlight = getMainColor("highlight")
local negative = getMainColor('negative')

-- We can also pull in some localized aliases for workhorse functions for a modest speed increase
local Round = notShit.round
local Floor = notShit.floor
local diffusealpha = Actor.diffusealpha
local diffuse = Actor.diffuse
local finishtweening = Actor.finishtweening
local linear = Actor.linear
local x = Actor.x
local y = Actor.y
local addx = Actor.addx
local addy = Actor.addy
local Zoomtoheight = Actor.zoomtoheight
local Zoomtowidth = Actor.zoomtowidth
local Zoomm = Actor.zoom
local queuecommand = Actor.queuecommand
local playcommand = Actor.queuecommand
local settext = BitmapText.settext
local Broadcast = MessageManager.Broadcast

-- Screenwide params
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
isCentered = PREFSMAN:GetPreference("Center1Player")
local CenterX = SCREEN_CENTER_X
local mpOffset = 0
if not isCentered then
	CenterX = THEME:GetMetric("ScreenGameplay",string.format("PlayerP1%sX",ToEnumShortString(GAMESTATE:GetCurrentStyle():GetStyleType())))
	mpOffset = SCREEN_CENTER_X
end
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--

local screen 			-- the screen after it is loaded
local messageBox		-- the message box from when you try to move something

local WIDESCREENWHY = -5
local WIDESCREENWHX = -5

local values = {
	ErrorBarX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ErrorBarX,
	ErrorBarY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ErrorBarY,
	ErrorBarWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].ErrorBarWidth,
	ErrorBarHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].ErrorBarHeight,
	TargetTrackerX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].TargetTrackerX,
	TargetTrackerY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].TargetTrackerY,
	TargetTrackerZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].TargetTrackerZoom,
	FullProgressBarX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].FullProgressBarX,
	FullProgressBarY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].FullProgressBarY,
	FullProgressBarWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].FullProgressBarWidth,
	FullProgressBarHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].FullProgressBarHeight,
	MiniProgressBarX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].MiniProgressBarX,
	MiniProgressBarY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].MiniProgressBarY,
	DisplayPercentX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].DisplayPercentX,
	DisplayPercentY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].DisplayPercentY,
	DisplayPercentZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].DisplayPercentZoom,
	NotefieldX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].NotefieldX,
	NotefieldY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].NotefieldY,
	NotefieldWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].NotefieldWidth,
	NotefieldHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].NotefieldHeight,
	JudgeCounterX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeCounterX,
	JudgeCounterY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeCounterY,
}

--error bar things
local errorBarFrameWidth = capWideScale(get43size(values.ErrorBarWidth),values.ErrorBarWidth)
local wscale = errorBarFrameWidth/180

--differential tracker things
local targetTrackerMode = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetTrackerMode

-- CUZ WIDESCREEN DEFAULTS SCREAAAAAAAAAAAAAAAAAAAAAAAAAM -mina
if IsUsingWideScreen( ) then
	values.MiniProgressBarY = values.MiniProgressBarY + WIDESCREENWHY
	values.MiniProgressBarX = values.MiniProgressBarX - WIDESCREENWHX
	values.TargetTrackerY = values.TargetTrackerY + WIDESCREENWHY
	values.TargetTrackerX = values.TargetTrackerX - WIDESCREENWHX
end

--receptor/Notefield things
local Notefield
local noteColumns
local usingReverse

--guess checking if things are enabled before changing them is good for not having a log full of errors
local enabledErrorBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ErrorBar
local enabledMiniBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).MiniProgressBar
local enabledFullBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).FullProgressBar
local enabledTargetTracker = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetTracker
local enabledDisplayPercent = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).DisplayPercent
local enabledJudgeCounter = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgeCounter

-- restart button (MOVED OUT OF THEME IN FAVOR OF REMAPPING)
--[[
local function froot(loop)
	if loop.DeviceInput.button == "DeviceButton_`" then
		SCREENMAN:GetTopScreen():SetPrevScreenName("ScreenStageInformation"):begin_backing_out()
	end
end
]]
--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
												**Main listener that moves and resizes things**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

]]
local function arbitraryErrorBarValue(value)
	errorBarFrameWidth = capWideScale(get43size(value),value)
	wscale = errorBarFrameWidth/180
end 

local propsFunctions = {
	X = x,
	Y = y,
	Zoom = Zoomm,
	Height = Zoomtoheight,
	Width = Zoomtowidth,
	AddX = addx,
	AddY = addy
}

local movable = { 
	current = "",
	pressed = false,
	DeviceButton_1 = {
		name = "Judge",
		textHeader = "Judgment Label Position:",
		properties = { "X", "Y" },
		elementTree = "GameplayXYCoordinates",
		external = true,
		condition = true
	},
	DeviceButton_2 = {
		name = "Judge",
		textHeader = "Judgment Label Size:",
		properties = { "Zoom" },
		elementTree = "GameplaySizes",
		external = true,
		condition = true
	},
	DeviceButton_3 = {
		name = "Combo",
		textHeader = "Combo Position:",
		properties = { "X", "Y" },
		elementTree = "GameplayXYCoordinates",
		external = true,
		condition = true
	},
	DeviceButton_4 = {
		name = "Combo",
		textHeader = "Combo Size:",
		properties = { "Zoom" },
		elementTree = "GameplaySizes",
		external = true,
		condition = true
	},
	DeviceButton_5 = {
		name = "ErrorBar",
		textHeader = "Error Bar Position:",
		element = { }, -- initialized later
		properties = { "X", "Y" },
		children = { "Center", "WeightedBar" },
		elementTree = "GameplayXYCoordinates",
		condition = enabledErrorBar ~= 0,
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
	DeviceButton_6 = {
		name = "ErrorBar",
		textHeader = "Error Bar Size:",
		element = { },
		properties = { "Width", "Height" },
		children = { "Center", "WeightedBar" },
		elementTree = "GameplaySizes",
		condition = enabledErrorBar ~= 0,
		DeviceButton_up = {
			property = "Height",
			inc = 1
		},
		DeviceButton_down = {
			property = "Height",
			inc = -1
		},
		DeviceButton_left = {
			arbitraryFunction = arbitraryErrorBarValue,
			property = "Width",
			inc = -10
		},
		DeviceButton_right = {
			arbitraryFunction = arbitraryErrorBarValue,
			property = "Width",
			inc = 10
		},
	},
	DeviceButton_7 = {
		name = "TargetTracker",
		textHeader = "Goal Tracker Position:",
		element = { },
		properties = { "X", "Y" },
		-- no children so the changes are applied to the element itself
		elementTree = "GameplayXYCoordinates",
		condition = enabledTargetTracker,
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
	DeviceButton_8 = {
		name = "TargetTracker",
		textHeader = "Goal Tracker Size:",
		element = { },
		properties = { "Zoom" },
		elementTree = "GameplaySizes",
		condition = enabledTargetTracker,
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		},
	},
	DeviceButton_9 = {
		name = "FullProgressBar",
		textHeader = "Full Progress Bar Position:",
		element = { },
		properties = { "X", "Y" },
		elementTree = "GameplayXYCoordinates",
		condition = enabledFullBar,
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
			inc = -5
		},
		DeviceButton_right = {
			property = "X",
			inc = 5
		},
	},
	DeviceButton_0 = {
		name = "FullProgressBar",
		textHeader = "Full Progress Bar Size:",
		element = { },
		properties = { "Width", "Height" },
		elementTree = "GameplaySizes",
		condition = enabledFullBar,
		DeviceButton_up = {
			property = "Height",
			inc = 0.1
		},
		DeviceButton_down = {
			property = "Height",
			inc = -0.1
		},
		DeviceButton_left = {
			property = "Width",
			inc = -0.01
		},
		DeviceButton_right = {
			property = "Width",
			inc = 0.01
		},
	},
	DeviceButton_q = {
		name = "MiniProgressBar",
		textHeader = "Mini Progress Bar Position:",
		element = { },
		properties = { "X", "Y" },
		elementTree = "GameplayXYCoordinates",
		condition = enabledMiniBar,
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
	DeviceButton_w = {
		name = "DisplayPercent",
		textHeader = "Current Percent Position:",
		element = { },
		properties = { "X", "Y" },
		elementTree = "GameplayXYCoordinates",
		condition = enabledDisplayPercent,
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
	DeviceButton_e = {
		name = "DisplayPercent",
		textHeader = "Current Percent Size:",
		element = { },
		properties = { "Zoom" },
		elementTree = "GameplaySizes",
		condition = enabledDisplayPercent,
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		},
	},
	DeviceButton_r = {
		name = "Notefield",
		textHeader = "Notefield Position:",
		element = { },
		properties = { "X", "Y" },
		elementTree = "GameplayXYCoordinates",
		condition = true,
		DeviceButton_up = {
			notefieldY = true,
			property = "AddY",
			inc = -3
		},
		DeviceButton_down = {
			notefieldY = true,
			property = "AddY",
			inc = 3
		},
		DeviceButton_left = {
			property = "AddX",
			inc = -3
		},
		DeviceButton_right = {
			property = "AddX",
			inc = 3
		},
	},
	DeviceButton_t = {
		name = "Notefield",
		textHeader = "Notefield Size:",
		element = { },
		elementList = true, -- god bless the notefield
		properties = { "Width", "Height" },
		elementTree = "GameplaySizes",
		condition = true,
		DeviceButton_up = {
			property = "Height",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Height",
			inc = -0.01
		},
		DeviceButton_left = {
			property = "Width",
			inc = -0.01
		},
		DeviceButton_right = {
			property = "Width",
			inc = 0.01
		},
	},
	DeviceButton_p = {
		name = "JudgeCounter",
		textHeader = "Judge Counter Position:",
		element = { },
		properties = { "X", "Y" },
		elementTree = "GameplayXYCoordinates",
		condition = enabledJudgeCounter,
		DeviceButton_up = {
			property = "AddY",
			inc = -3
		},
		DeviceButton_down = {
			property = "AddY",
			inc = 3
		},
		DeviceButton_left = {
			property = "AddX",
			inc = -3
		},
		DeviceButton_right = {	
			property = "AddX",
			inc = 3
		},
	},
}

local function input(event)
	if getAutoplay() ~= 0 then
		local button = event.DeviceInput.button
		local notReleased = not (event.type == "InputEventType_Release")
		if movable[button] and movable[button].condition then
			movable.pressed = notReleased
			movable.current = button
			local text = {
				movable[button].textHeader
			}
			for _, prop in ipairs(movable[button].properties) do
				local fullProp = movable[button].name .. prop
				if movable[button].external then
					text[#text+1] = prop .. ": " .. playerConfig:get_data(pn_to_profile_slot(PLAYER_1))[movable[button].elementTree][keymode][fullProp]
				else
					text[#text+1] = prop .. ": " .. values[fullProp]
				end
			end
			messageBox:GetChild("message"):settext(table.concat(text, "\n"))
			messageBox:GetChild("message"):visible(notReleased)
		end

		local current = movable[movable.current]
		if movable.pressed and current[button] and current.condition and notReleased and current.external == nil then
			local curKey = current[button]
			local prop = current.name .. string.gsub(curKey.property, "Add", "")
			local newVal = values[prop] + (curKey.inc * ((curKey.notefieldY and not usingReverse) and -1 or 1))
			values[prop] = newVal
			if curKey.arbitraryFunction then
				curKey.arbitraryFunction(newVal)
			elseif current.elementList then
				for _, elem in ipairs(current.element) do
					propsFunctions[curKey.property](elem, newVal)
				end
			elseif current.children then
				for _, attribute in ipairs(current.children) do
					propsFunctions[curKey.property](current.element[attribute], newVal)
				end
			elseif curKey.property == "AddX" or curKey.property == "AddY" then
				propsFunctions[curKey.property](current.element, curKey.inc)
			else
				propsFunctions[curKey.property](current.element, newVal)
			end
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1))[current.elementTree][keymode][prop] = newVal
			playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
			playerConfig:save(pn_to_profile_slot(PLAYER_1))
		end
	end
	return false
end


--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
								     **Wife deviance tracker. Basically half the point of the theme.**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	For every doot there is an equal and opposite scoot.
]]

local t = Def.ActorFrame{										
	Name = "WifePerch",
	OnCommand=function()
		-- Discord thingies
		local largeImageTooltip = GetPlayerOrMachineProfile(PLAYER_1):GetDisplayName() .. ": " .. string.format("%5.2f", GetPlayerOrMachineProfile(PLAYER_1):GetPlayerRating())
		local detail = GAMESTATE:GetCurrentSong():GetDisplayMainTitle() .. " " .. string.gsub(getCurRateDisplayString(), "Music", "") .. " [" .. GAMESTATE:GetCurrentSong():GetGroupName() .. "]"
		-- truncated to 128 characters(discord hard limit)
		detail = #detail < 128 and detail or string.sub(detail, 1, 124) .. "..."
		local state = "MSD: " .. string.format("%05.2f", GAMESTATE:GetCurrentSteps(PLAYER_1):GetMSD(getCurRateValue(),1))
		local endTime = os.time() + GetPlayableTime()
		GAMESTATE:UpdateDiscordPresence(largeImageTooltip, detail, state, endTime)

		--[[if SCREENMAN:GetTopScreen():GetName() == "ScreenGameplay" then
			SCREENMAN:GetTopScreen():AddInputCallback(froot)
		end]]
		if (allowedCustomization) then
			SCREENMAN:GetTopScreen():AddInputCallback(input)
		end
		screen = SCREENMAN:GetTopScreen()
		usingReverse = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():UsingReverse()
		Notefield = screen:GetChild("PlayerP1"):GetChild("NoteField")
		Notefield:addy(values.NotefieldY * (usingReverse and 1 or -1))
		Notefield:addx(values.NotefieldX)
		movable.DeviceButton_r.element = Notefield
		movable.DeviceButton_t.element = Notefield
		noteColumns = Notefield:get_column_actors()
		movable.DeviceButton_t.element = noteColumns
		for i, actor in ipairs(noteColumns) do
			actor:zoomtowidth(values.NotefieldWidth)
			actor:zoomtoheight(values.NotefieldHeight)
		end
	end,
	JudgmentMessageCommand=function(self, msg)
		if msg.Offset ~= nil then
			dvCur = msg.Offset 
			jdgCur = msg.Judgment
			Broadcast(MESSAGEMAN, "SpottedOffset")
		end
	end,
}

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
																	**LaneCover**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Old scwh lanecover back for now. Equivalent to "screencutting" on ffr; essentially hides notes for a fixed distance before they appear
on screen so you can adjust the time arrows display on screen without modifying their spacing from each other. 
]]	

if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).LaneCover then
	t[#t+1] = LoadActor("lanecover")
end
	
--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 					    	**Player Target Differential: Ghost target rewrite, average score gone for now**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Point differential to AA.
]]

-- Mostly clientside now. We set our desired target goal and listen to the results rather than calculating ourselves.
local target = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetGoal
GAMESTATE:GetPlayerState(PLAYER_1):SetTargetGoal(target/100)

-- We can save space by wrapping the personal best and set percent trackers into one function, however
-- this would make the actor needlessly cumbersome and unnecessarily punish those who don't use the
-- personal best tracker (although everything is efficient enough now it probably wouldn't matter)

-- moved it for better manipulation
local d = Def.ActorFrame{
}

if targetTrackerMode == 0 then
	d[#d+1] = LoadFont("Common Normal")..{
		Name = "PercentDifferential",
		InitCommand=function(self)
			movable.DeviceButton_7.element = self
			movable.DeviceButton_8.element = self
			self:xy(values.TargetTrackerX,values.TargetTrackerY):zoom(values.TargetTrackerZoom):halign(0):valign(1)
		end,
		JudgmentMessageCommand=function(self,msg)
			local tDiff = msg.WifeDifferential
			if tDiff >= 0 then 											
				diffuse(self,positive)
			else
				diffuse(self,negative)
			end
			self:settextf("%5.2f (%5.2f%%)", tDiff, target)
		end
	}
	else
	d[#d+1] = LoadFont("Common Normal")..{
		Name = "PBDifferential",
		InitCommand=function(self)
			movable.DeviceButton_7.element = self
			movable.DeviceButton_8.element = self
			self:xy(values.TargetTrackerX,values.TargetTrackerY):zoom(values.TargetTrackerZoom):halign(0):valign(1)
		end,
		JudgmentMessageCommand=function(self,msg)
			local tDiff = msg.WifePBDifferential
			if tDiff then
				local pbtarget = msg.WifePBGoal
				if tDiff >= 0 then
					diffuse(self,color("#00ff00"))
				else
					diffuse(self,negative)
				end
				self:settextf("%5.2f (%5.2f%%)", tDiff, pbtarget*100)
			else
				tDiff = msg.WifeDifferential
				if tDiff >= 0 then 											
					diffuse(self,positive)
				else
					diffuse(self,negative)
				end
				self:settextf("%5.2f (%5.2f%%)", tDiff, target)
			end
		end
	}
end

if enabledTargetTracker then
	t[#t+1] = d
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 					    									**Display Percent**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Displays the current percent for the score.
]]

local cp = Def.ActorFrame{
	InitCommand = function(self)
		movable.DeviceButton_w.element = self
		movable.DeviceButton_e.element = self
		self:zoom(values.DisplayPercentZoom):addx(values.DisplayPercentX):addy(values.DisplayPercentY)
	end,
	Def.Quad{
		InitCommand=function(self)
			self:xy(60 + mpOffset,(SCREEN_HEIGHT*0.62)-90):zoomto( 60, 13):diffuse(color("0,0,0,0.4")):horizalign(left):vertalign(top)
		end	
	},
	-- Displays your current percentage score
	LoadFont("Common Large")..{											
		Name = "DisplayPercent",
		InitCommand=function(self)
			self:xy(115 + mpOffset,220):zoom(0.3):halign(1):valign(1)
		end,
		OnCommand=function(self)
			self:settextf("%05.2f%%", 0)
		end,
		JudgmentMessageCommand=function(self,msg)
			self:settextf("%05.2f%%", Floor(msg.WifePercent*100)/100)
		end
	},
}

if enabledDisplayPercent then
	t[#t+1] = cp
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
											    	**Player judgment counter (aka pa counter)**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Counts judgments.
--]]

-- User Parameters
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local frameX = 60 + mpOffset						 -- X position of the frame
local frameY = (SCREEN_HEIGHT*0.62)-90 				 -- Y Position of the frame
local spacing = 10									 -- Spacing between the judgetypes
local frameWidth = 60								 -- Width of the Frame
local frameHeight = ((#jdgT-1)*spacing)	- 8			 -- Height of the Frame
local judgeFontSize = 0.40							 -- Font sizes for different text elements 
local countFontSize = 0.35
local gradeFontSize = 0.45
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--

local jdgCounts = {}								 -- Child references for the judge counter

local j = Def.ActorFrame{
	InitCommand=function(self)
		movable.DeviceButton_p.element = self
		self:addx(values.JudgeCounterX):addy(values.JudgeCounterY)
	end,
	OnCommand=function(self)
		for i=1,#jdgT do
			jdgCounts[jdgT[i]] = self:GetChild(jdgT[i])
		end
	end,
	JudgmentMessageCommand=function(self, msg)
		if jdgCounts[msg.Judgment] then
			settext(jdgCounts[msg.Judgment],msg.Val)
		end
	end																		
}

 local function makeJudgeText(judge,index)		-- Makes text
 	return LoadFont("Common normal")..{
 		InitCommand=function(self)
 			self:xy(frameX+5,frameY+7+(index*spacing)):zoom(judgeFontSize):halign(0)
 		end,
 		OnCommand=function(self)
 			settext(self,getShortJudgeStrings(judge))
 			diffuse(self,jcT[judge])
 		end
 	}
 end
 
 local function makeJudgeCount(judge,index)		-- Makes county things for taps....
 	return LoadFont("Common Normal")..{
 		Name = judge,
		InitCommand=function(self)
			self:xy(frameWidth+frameX-5,frameY+7+(index*spacing)):zoom(countFontSize):horizalign(right):settext(0)
		end}
 end


-- Background
j[#j+1] = Def.Quad{InitCommand=function(self)
	self:xy(frameX,frameY+13):zoomto(frameWidth,frameHeight+18):diffuse(color("0,0,0,0.4")):horizalign(left):vertalign(top)
end}

-- Build judgeboard
for i=1,#jdgT do
	j[#j+1] = makeJudgeText(jdgT[i],i)
	j[#j+1] = makeJudgeCount(jdgT[i],i)
end

-- Now add the completed judgment table to the primary actor frame t if enabled
if enabledJudgeCounter then
	t[#t+1] = j
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														    	**Player ErrorBar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Visual display of deviance values. 
--]]

-- User Parameters
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local barcount = 30 									-- Number of bars. Older bars will refresh if judgments/barDuration exceeds this value. You don't need more than 40.
local barWidth = 2										-- Width of the ticks.
local barDuration = 0.75 								-- Time duration in seconds before the ticks fade out. Doesn't need to be higher than 1. Maybe if you have 300 bars I guess.
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local currentbar = 1 									-- so we know which error bar we need to update
local ingots = {}										-- references to the error bars
local alpha = 0.07;										-- ewma alpha
local avg;
local lastAvg;

-- Makes the error bars. They position themselves relative to the center of the screen based on your dv and diffuse to your judgement value before disappating or refreshing
-- Should eventually be handled by the game itself to optimize performance
function smeltErrorBar(index)
	return Def.Quad{
		Name = index,
		InitCommand=function(self)
			self:xy(values.ErrorBarX,values.ErrorBarY):zoomto(barWidth,values.ErrorBarHeight):diffusealpha(0)
		end,
		UpdateErrorBarCommand=function(self)						-- probably a more efficient way to achieve this effect, should test stuff later
			finishtweening(self)									-- note: it really looks like shit without the fade out 
			diffusealpha(self,1)
			diffuse(self,jcT[jdgCur])
			x(self,values.ErrorBarX+dvCur*wscale)
			y(self,values.ErrorBarY)
			Zoomtoheight(self, values.ErrorBarHeight)
			linear(self,barDuration)
			diffusealpha(self,0)
		end
	}
end

local e = Def.ActorFrame{
	InitCommand = function(self)
		movable.DeviceButton_5.element = self:GetChildren()
		movable.DeviceButton_6.element = self:GetChildren()
		if enabledErrorBar == 1 then
			for i=1,barcount do											-- basically the equivalent of using GetChildren() if it returned unnamed children numerically indexed
				ingots[#ingots+1] = self:GetChild(i)
			end
		else
			avg = 0;
			lastAvg = 0;
		end
	end,
	SpottedOffsetMessageCommand=function(self)
		if enabledErrorBar == 1 then
			currentbar = ((currentbar)%barcount) + 1
			playcommand(ingots[currentbar],"UpdateErrorBar")			-- Update the next bar in the queue
		end
	end,
	DootCommand=function(self)
		self:RemoveChild("DestroyMe")
		self:RemoveChild("DestroyMe2")
	end,
	Def.Quad{
		Name = "WeightedBar",
		InitCommand=function(self)
			if enabledErrorBar == 2 then
				self:xy(values.ErrorBarX,values.ErrorBarY):zoomto(barWidth,values.ErrorBarHeight):diffusealpha(1):diffuse(getMainColor('enabled'))
			else
				self:visible(false)
			end
		end,
		SpottedOffsetMessageCommand=function(self)
			if enabledErrorBar == 2 then
				avg = alpha * dvCur + (1 - alpha) * lastAvg
				lastAvg = avg
				self:x(values.ErrorBarX+avg*wscale)
			end
		end
	},
	Def.Quad {
		Name = "Center",
		InitCommand=function(self)
			self:diffuse(getMainColor('highlight')):xy(values.ErrorBarX,values.ErrorBarY):zoomto(2,values.ErrorBarHeight)
		end	
	},
	-- Indicates which side is which (early/late) These should be destroyed after the song starts.
	LoadFont("Common Normal") .. {
		Name = "DestroyMe",
		InitCommand=function(self)
			self:xy(values.ErrorBarX+errorBarFrameWidth/4,values.ErrorBarY):zoom(0.35)
		end,
		BeginCommand=function(self)
			self:settext("Late"):diffusealpha(0):smooth(0.5):diffusealpha(0.5):sleep(1.5):smooth(0.5):diffusealpha(0)
		end,
	},
	LoadFont("Common Normal") .. {
		Name = "DestroyMe2",
		InitCommand=function(self)
			self:xy(values.ErrorBarX-errorBarFrameWidth/4,values.ErrorBarY):zoom(0.35)
		end,
		BeginCommand=function(self)
			self:settext("Early"):diffusealpha(0):smooth(0.5):diffusealpha(0.5):sleep(1.5):smooth(0.5):diffusealpha(0):queuecommand("Doot")
		end,
		DootCommand=function(self)
			self:GetParent():queuecommand("Doot")
		end
	}
}


-- Initialize bars
if enabledErrorBar == 1 then
	for i=1,barcount do
		e[#e+1] = smeltErrorBar(i)
	end
end

-- Add the completed errorbar frame to the primary actor frame t if enabled
if enabledErrorBar ~= 0 then
	t[#t+1] = e
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															   **Player Info**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Avatar and such, now you can turn it off. Planning to have player mods etc exported similarly to the nowplaying, and an avatar only option
]]
if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).PlayerInfo then
	t[#t+1] = LoadActor("playerinfo")
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														       **Full Progressbar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Song Completion Meter that doesn't eat 100 fps. Courtesy of simply love. Decided to make the full progress bar and mini progress bar
separate entities. So you can have both, or one or the other, or neither. 
]]
 
-- User params
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local width = SCREEN_WIDTH/2-100
local height = 10
local alpha = 0.7
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--

local p = Def.ActorFrame{
	InitCommand = function(self)
		self:xy(values.FullProgressBarX,values.FullProgressBarY)
		self:zoomto(values.FullProgressBarWidth,values.FullProgressBarHeight)
		movable.DeviceButton_9.element = self
		movable.DeviceButton_0.element = self
	end,
	Def.Quad{
		InitCommand=function(self)
			self:zoomto(width,height):diffuse(color("#666666")):diffusealpha(alpha)
		end,
	},
	Def.SongMeterDisplay{
		InitCommand=function(self)
			self:SetUpdateRate(0.5)
		end,
		StreamWidth=width,
		Stream=Def.Quad{InitCommand=function(self)
			self:zoomy(height):diffuse(getMainColor("highlight"))
		end}
	},
	LoadFont("Common Normal")..{																		-- title
		InitCommand=function(self)
			self:zoom(0.35):maxwidth(width*2)
		end,
		BeginCommand=function(self)
			self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
		end,
		DoneLoadingNextSongMessageCommand=function(self)
			self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
		end	
	},
	LoadFont("Common Normal")..{																		-- total time
		InitCommand=function(self)
			self:x(width/2):zoom(0.35):maxwidth(width*2):halign(1)
		end,
		BeginCommand=function(self)
			local ttime = GetPlayableTime()
			settext(self,SecondsToMMSS(ttime))
			diffuse(self, byMusicLength(ttime))
		end,
		DoneLoadingNextSongMessageCommand=function(self)
			local ttime = GetPlayableTime()
			settext(self,SecondsToMMSS(ttime))
			diffuse(self, byMusicLength(ttime))
		end
	}
}

if enabledFullBar then
	t[#t+1] = p
end



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														      **Mini Progressbar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Song Completion Meter that doesn't eat 100 fps. Courtesy of simply love. Decided to make the full progress bar and mini progress bar
separate entities. So you can have both, or one or the other, or neither. 
]]

local width = 34
local height = 4
local alpha = 0.3

local mb = Def.ActorFrame{
	InitCommand = function(self)
		self:xy(values.MiniProgressBarX,values.MiniProgressBarY)
		movable.DeviceButton_q.element = self
	end,
	Def.Quad{
		InitCommand=function(self)
			self:zoomto(width,height):diffuse(color("#666666")):diffusealpha(alpha)
		end,
	},
	Def.Quad{
		InitCommand=function(self)
			self:x(1+width/2):zoomto(1,height):diffuse(color("#555555"))
		end,
	},
	Def.SongMeterDisplay{
		InitCommand=function(self)
			self:SetUpdateRate(0.5)
		end,
		StreamWidth=width,
		Stream=Def.Quad{
			InitCommand=function(self)
				self:zoomy(height):diffuse(getMainColor("highlight"))
			end,
		}
	}
}

if enabledMiniBar then
	t[#t+1] = mb
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														    	**Music Rate Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
]]

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=function(self)
		self:xy(SCREEN_CENTER_X,SCREEN_BOTTOM-10):zoom(0.35):settext(getCurRateDisplayString())
	end,
	DoneLoadingNextSongMessageCommand=function(self)
		self:settext(getCurRateDisplayString())
	end	
}

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														    	**BPM Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Better optimized frame update bpm display. 
]]

local BPM
local a = GAMESTATE:GetPlayerState(PLAYER_1):GetSongPosition()	
local r = GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate() * 60
local GetBPS = SongPosition.GetCurBPS

local function UpdateBPM(self)
	local bpm = GetBPS(a) * r
	settext(BPM,Round(bpm,2))
end

t[#t+1] = Def.ActorFrame{
	InitCommand=function(self)
		BPM = self:GetChild("BPM")
		if #GAMESTATE:GetCurrentSong():GetTimingData():GetBPMs() > 1 then			-- dont bother updating for single bpm files
			self:SetUpdateFunction(UpdateBPM)
			self:SetUpdateRate(0.5)
		else
			BPM:settextf("%5.2f",GetBPS(a) * r) -- i wasn't thinking when i did this, we don't need to avoid formatting for performance because we only call this once -mina
		end
	end,
	LoadFont("Common Normal")..{
		Name="BPM",
		InitCommand=function(self)
			self:x(SCREEN_CENTER_X):y(SCREEN_BOTTOM-20):halign(0.5):zoom(0.40)
		end	
	},
	DoneLoadingNextSongMessageCommand=function(self)
		self:queuecommand("Init")
	end	
}



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															**Combo Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

]]

local x = 0
local y = 60

-- CUZ WIDESCREEN DEFAULTS SCREAAAAAAAAAAAAAAAAAAAAAAAAAM -mina
if IsUsingWideScreen( ) then
	y = y - WIDESCREENWHY
	x = x + WIDESCREENWHX
end

--This just initializes the initial point or not idk not needed to mess with this any more
function ComboTransformCommand( self, params )
	self:x( x )
	self:y( y )
end




--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														  **Judgment Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	moving here eventually
]]



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															 **NPS Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	re-enabling the old nps calc/graph for now 
]]

t[#t+1] = LoadActor("npscalc")



--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															  **NPS graph**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	ditto
]]

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
													**Message boxes for moving things**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	offset window esque boxes so its more intuitive to use the moving feature
]]
if (allowedCustomization) then
	t[#t+1] = Def.ActorFrame{
		InitCommand=function(self)
			messageBox = self
		end,
		Def.BitmapText{
			Name= "message", Font= "Common Normal", 
			InitCommand= function(self)
				self:horizalign(left):vertalign(top)
					:shadowlength(2):xy(10, 20):zoom(.5):visible(false)
			end,
		},
		Def.BitmapText{
			Name= "Instructions", Font= "Common Normal",
			InitCommand= function(self)
				self:horizalign(left):vertalign(top)
					:xy(SCREEN_WIDTH - 240, 100):zoom(.5):visible(true)
			end,
			OnCommand=function(self)
				local text= {
					"Enable AutoplayCPU with shift+f8\n",
					"Hold the following and press the arrow",
					"keys to alter the associated element\n",
					"1: Judgement Text Position",
					"2: Judgement Text Size",
					"3: Combo Text Position",
					"4: Combo Text Size",
					"5: Error Bar Text Position",
					"6: Error Bar Text Size",
					"7: Target Tracker Text Position",
					"8: Target Tracker Text Size",
					"9: Full Progress Bar Position",
					"0: Full Progress Bar Size",
					"q: Mini Progress Bar Position",
					"w: Display Percent Text Position",
					"e: Display Percent Text Size",
					"r: Notefield Position",
					"t: Notefield Size",
					"y: NPS Display Text Position",
					"u: NPS Display Text Size",
					"i: NPS Graph Position",
					"o: NPS Graph Size",
					"p: Judge Counter Position",
				}
				if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).LaneCover ~= 0 then
					table.insert(text, "/: Lane Cover Height")
				end
				self:settext(table.concat(text, "\n"))
			end
		},
	}
end

return t
