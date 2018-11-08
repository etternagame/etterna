local keymode = getCurrentKeyMode()
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay

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
	JudgeCounterY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeCounterY
}

local WIDESCREENWHY = -5
local WIDESCREENWHX = -5

local Round = notShit.round
local Floor = notShit.floor
local queuecommand = Actor.queuecommand
local playcommand = Actor.queuecommand
local settext = BitmapText.settext
local Broadcast = MessageManager.Broadcast

if IsUsingWideScreen() then
	values.MiniProgressBarY = values.MiniProgressBarY + WIDESCREENWHY
	values.MiniProgressBarX = values.MiniProgressBarX - WIDESCREENWHX
	values.TargetTrackerY = values.TargetTrackerY + WIDESCREENWHY
	values.TargetTrackerX = values.TargetTrackerX - WIDESCREENWHX
end

local usingReverse = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():UsingReverse()
local enabledErrorBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ErrorBar
local enabledMiniBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).MiniProgressBar
local enabledFullBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).FullProgressBar
local enabledTargetTracker = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetTracker
local enabledDisplayPercent = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).DisplayPercent
local enabledJudgeCounter = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgeCounter
local leaderboardEnabled = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).leaderboardEnabled and DLMAN:IsLoggedIn()
local isReplay = GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerController() == "PlayerController_Replay"

local propsFunctions = {
	X = Actor.x,
	Y = Actor.y,
	Zoom = Actor.zoom,
	Height = Actor.zoomtoheight,
	Width = Actor.zoomtowidth,
	AddX = Actor.addx,
	AddY = Actor.addy
}

Movable = {
	message = {},
	current = "None",
	pressed = false,
	DeviceButton_1 = {
		name = "Judge",
		textHeader = "Judgment Label Position:",
		properties = {"X", "Y"},
		elementTree = "GameplayXYCoordinates",
		external = true,
		condition = true
	},
	DeviceButton_2 = {
		name = "Judge",
		textHeader = "Judgment Label Size:",
		properties = {"Zoom"},
		elementTree = "GameplaySizes",
		external = true,
		condition = true
	},
	DeviceButton_3 = {
		name = "Combo",
		textHeader = "Combo Position:",
		properties = {"X", "Y"},
		elementTree = "GameplayXYCoordinates",
		external = true,
		condition = true
	},
	DeviceButton_4 = {
		name = "Combo",
		textHeader = "Combo Size:",
		properties = {"Zoom"},
		elementTree = "GameplaySizes",
		external = true,
		condition = true
	},
	DeviceButton_5 = {
		name = "ErrorBar",
		textHeader = "Error Bar Position:",
		element = {}, -- initialized later
		properties = {"X", "Y"},
		children = {"Center", "WeightedBar", "Border"},
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
		}
	},
	DeviceButton_6 = {
		name = "ErrorBar",
		textHeader = "Error Bar Size:",
		element = {},
		properties = {"Width", "Height"},
		children = {"Center", "WeightedBar"},
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
			property = "Width",
			inc = -10
		},
		DeviceButton_right = {
			property = "Width",
			inc = 10
		}
	},
	DeviceButton_7 = {
		name = "TargetTracker",
		textHeader = "Goal Tracker Position:",
		element = {},
		properties = {"X", "Y"},
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
		}
	},
	DeviceButton_8 = {
		name = "TargetTracker",
		textHeader = "Goal Tracker Size:",
		element = {},
		properties = {"Zoom"},
		elementTree = "GameplaySizes",
		condition = enabledTargetTracker,
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		}
	},
	DeviceButton_9 = {
		name = "FullProgressBar",
		textHeader = "Full Progress Bar Position:",
		element = {},
		properties = {"X", "Y"},
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
		}
	},
	DeviceButton_0 = {
		name = "FullProgressBar",
		textHeader = "Full Progress Bar Size:",
		element = {},
		properties = {"Width", "Height"},
		elementTree = "GameplaySizes",
		condition = enabledFullBar,
		noBorder = true,
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
		}
	},
	DeviceButton_q = {
		name = "MiniProgressBar",
		textHeader = "Mini Progress Bar Position:",
		element = {},
		properties = {"X", "Y"},
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
		}
	},
	DeviceButton_w = {
		name = "DisplayPercent",
		textHeader = "Current Percent Position:",
		element = {},
		properties = {"X", "Y"},
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
		}
	},
	DeviceButton_e = {
		name = "DisplayPercent",
		textHeader = "Current Percent Size:",
		element = {},
		properties = {"Zoom"},
		elementTree = "GameplaySizes",
		condition = enabledDisplayPercent,
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		}
	},
	DeviceButton_r = {
		name = "Notefield",
		textHeader = "Notefield Position:",
		element = {},
		properties = {"X", "Y"},
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
		}
	},
	DeviceButton_t = {
		name = "Notefield",
		textHeader = "Notefield Size:",
		element = {},
		elementList = true, -- god bless the notefield
		properties = {"Width", "Height"},
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
		}
	},
	DeviceButton_p = {
		name = "JudgeCounter",
		textHeader = "Judge Counter Position:",
		element = {},
		properties = {"X", "Y"},
		elementTree = "GameplayXYCoordinates",
		condition = enabledJudgeCounter,
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
	DeviceButton_a = {
		name = "Leaderboard",
		textHeader = "Leaderboard Position:",
		properties = {"X", "Y"},
		elementTree = "GameplayXYCoordinates",
		external = true,
		condition = leaderboardEnabled
	},
	DeviceButton_s = {
		name = "Leaderboard",
		textHeader = "Leaderboard Size:",
		properties = {"Width", "Height"},
		elementTree = "GameplaySizes",
		external = true,
		condition = leaderboardEnabled
	},
	DeviceButton_d = {
		name = "Leaderboard",
		textHeader = "Leaderboard Spacing:",
		properties = {"Spacing"},
		elementTree = "GameplaySizes",
		external = true,
		condition = leaderboardEnabled
	},
	DeviceButton_f = {
		name = "ReplayButtons",
		textHeader = "Replay Buttons Position:",
		properties = {"X", "Y"},
		elementTree = "GameplayXYCoordinates",
		external = true,
		condition = isReplay
	},
	DeviceButton_g = {
		name = "ReplayButtons",
		textHeader = "Replay Buttons Size:",
		properties = {"Zoom"},
		elementTree = "GameplaySizes",
		external = true,
		condition = isReplay
	},
	DeviceButton_h = {
		name = "ReplayButtons",
		textHeader = "Replay Buttons Spacing:",
		properties = {"Spacing"},
		elementTree = "GameplaySizes",
		external = true,
		condition = isReplay
	},
}

local function updatetext(button)
	local text = {Movable[button].textHeader}
	for _, prop in ipairs(Movable[button].properties) do
		local fullProp = Movable[button].name .. prop
		if Movable[button].external then
			text[#text + 1] =
				prop ..
				": " .. playerConfig:get_data(pn_to_profile_slot(PLAYER_1))[Movable[button].elementTree][keymode][fullProp]
		else
			text[#text + 1] = prop .. ": " .. values[fullProp]
		end
	end
	Movable.message:settext(table.concat(text, "\n"))
	Movable.message:visible(Movable.pressed)
end

function MovableInput(event)
	if getAutoplay() ~= 0 then
		-- this will eat any other mouse input than a right click (toggle)
		-- so we don't have to worry about anything weird happening with the ersatz inputs -mina
		if event.DeviceInput.is_mouse then	
			if event.DeviceInput.button == "DeviceButton_right mouse button" then
				Movable.current = "None"
				Movable.pressed = false
				Movable.message:visible(Movable.pressed)
			end
			return 
		end

		local button = event.DeviceInput.button	
		event.hellothisismouse = event.hellothisismouse and true or false -- so that's why bools kept getting set to nil -mina
		local notReleased = not (event.type == "InputEventType_Release")
		-- changed to toggle rather than hold down -mina
		if (Movable[button] and Movable[button].condition and notReleased) or event.hellothisismouse then
			Movable.pressed = not Movable.pressed or event.hellothisismouse	-- this stuff is getting pretty hacky now -mina
			if Movable.current ~= event.DeviceInput.button and not event.hellothisismouse then
				Movable.pressed = true	-- allow toggling using the kb to directly move to a different key rather than forcing an untoggle first -mina
			end
			Movable.current = button
			if not Movable.pressed then 
				Movable.current = "None"
			end
			updatetext(button)	-- this will only update the text when the toggles occur
		end
		
		local current = Movable[Movable.current]

		-- left/right move along the x axis and up/down along the y; set them directly here -mina
		if event.hellothisismouse then
			if event.axis == "x" then
				button = "DeviceButton_left"
			else
				button = "DeviceButton_up"
			end
			Movable.pressed = true	-- we need to do this or the mouse input facsimile will toggle on when moving x, and off when moving y
		end
		
		if Movable.pressed and current[button] and current.condition and notReleased and current.external == nil then
			local curKey = current[button]
			local keyProperty = curKey.property
			local prop = current.name .. string.gsub(keyProperty, "Add", "")
			local newVal

			-- directly set newval if we're using the mouse -mina
			if event.hellothisismouse then
				newVal = event.val
			else
				newVal = values[prop] + (curKey.inc * ((curKey.notefieldY and not usingReverse) and -1 or 1))
			end
			
			values[prop] = newVal
			if curKey.arbitraryFunction then
				curKey.arbitraryFunction(newVal)
			elseif current.children then
				for _, attribute in ipairs(current.children) do
					propsFunctions[curKey.property](current.element[attribute], newVal)
				end
			elseif current.elementList then
				for _, elem in ipairs(current.element) do
					propsFunctions[keyProperty](elem, newVal)
				end
			elseif keyProperty == "AddX" or keyProperty == "AddY" then
				propsFunctions[keyProperty](current.element, curKey.inc)
			else
				propsFunctions[keyProperty](current.element, newVal)
			end

			if not current.noBorder then
				local border = Movable[Movable.current]["Border"]
				if keyProperty == "Height" or keyProperty == "Width" or keyProperty == "Zoom" then
					border:playcommand("Change" .. keyProperty, {val = newVal} )
				end
			end

			if not event.hellothisismouse then
				updatetext(Movable.current)	-- updates text when keyboard movements are made (mouse already updated)
			end
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1))[current.elementTree][keymode][prop] = newVal
			playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
			playerConfig:save(pn_to_profile_slot(PLAYER_1))
		end
	end
	return false
end


-- more assumptions here about hierarchy, alignment
-- halign(1):valign(0) is for hal == 1 and halign(0):valign(1) otherwise, yes i know this could be done better/automatically -mina
function setBordersForText(self, hal)
	self:GetParent():GetChild("Border"):playcommand("ChangeWidth", {val = self:GetZoomedWidth()})
	self:GetParent():GetChild("Border"):playcommand("ChangeHeight", {val = self:GetZoomedHeight()})
	self:GetParent():GetChild("Border"):playcommand("ChangeZoom", {val = self:GetParent():GetZoom()})
	if hal == 1 then
		self:xy(self:GetZoomedWidth()/2, -self:GetZoomedHeight()/2*1.04 )
	else 
		self:xy(-self:GetZoomedWidth()/2, self:GetZoomedHeight()/2*1.04 )
	end
end

-- this is supreme lazy -mina
local function elementtobutton(name)
	for k,v in pairs(Movable) do
		if type(v) == 'table' and v.name == name and v.properties[1] == "X" then
			return k
		end
	end
end

local function bordermousereact(self)
	self:queuecommand("mousereact")
end

local function movewhendragged(self)
	-- this is a somewhat dangerous hierarchical assumption but it should help us get organied in the short term -mina
	local b = elementtobutton(self:GetParent():GetParent():GetName())
	if isOver(self) or (Movable.pressed and Movable.current == b) then
		if Movable.pressed and Movable.current == b then
			self:GetParent():diffusealpha(0.75)	-- this is active
		else
			self:GetParent():diffusealpha(0.35)	-- this has been moused over
		end

		-- second half of the expr stops elements from being activated if you mouse over them while moving something else
		if INPUTFILTER:IsBeingPressed("Mouse 0", "Mouse") and (Movable.current == b or Movable.current == "None") then
			local nx = Round(INPUTFILTER:GetMouseX())
			local ny = Round(INPUTFILTER:GetMouseY())
			input({DeviceInput = {button = b}, hellothisismouse = true, axis = "x", val = nx})
			input({DeviceInput = {button = b}, hellothisismouse = true, axis = "y", val = ny})
		end
	elseif Movable.pressed then 
		self:GetParent():diffusealpha(0.35)		-- something is active, but not this
	else
		self:GetParent():diffusealpha(0.1)		-- nothing is active and this is not moused over
	end
end

-- border function in use -mina
function MovableBorder(width, height, bw, x, y)
	if not allowedCustomization then return end	-- we don't want to be loading all this garbage if we aren't in customization
	return Def.ActorFrame {
		Name = "Border",
		InitCommand=function(self)
			self:xy(x,y):diffusealpha(0)
			self:SetUpdateFunction(bordermousereact)
		end,
		ChangeWidthCommand=function(self, params)
			self:GetChild("xbar"):zoomx(params.val)
			self:GetChild("showybox"):zoomx(params.val)
			self:GetChild("hideybox"):zoomx(params.val-2*bw)
		end,
		ChangeHeightCommand=function(self, params)
			self:GetChild("ybar"):zoomy(params.val)
			self:GetChild("showybox"):zoomy(params.val)
			self:GetChild("hideybox"):zoomy(params.val-2*bw)
		end,
		ChangeZoomCommand=function(self,params)
			local wot = self:GetZoom()/(1/params.val)
			self:zoom(1/params.val)
			self:playcommand("ChangeWidth", {val = self:GetChild("showybox"):GetZoomX() * wot})
			self:playcommand("ChangeHeight", {val = self:GetChild("showybox"):GetZoomY() * wot})
		end,
		Def.Quad {
			Name = "xbar",
			InitCommand=function(self)
				self:zoomto(width,bw):diffusealpha(0.5)	-- did not realize this was multiplicative with parent's value -mina
			end
		},
		Def.Quad {
			Name = "ybar",
			InitCommand=function(self)
				self:zoomto(bw,height):diffusealpha(0.5)
			end
		},
		Def.Quad {
			Name = "hideybox",
			InitCommand=function(self)
				self:zoomto(width-2*bw, height-2*bw):MaskSource(true)
			end
		},
		Def.Quad {
			Name = "showybox",
			InitCommand=function(self)
				self:zoomto(width,height):MaskDest()
			end,
			mousereactCommand=function(self)
				movewhendragged(self)	-- this quad owns the mouse movement function -mina
			end
		},
	}
end
