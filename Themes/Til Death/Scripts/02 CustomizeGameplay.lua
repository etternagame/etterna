local keymode
local allowedCustomization
local usingReverse

MovableValues = {}

local function loadValuesTable()
	allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
	usingReverse = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():UsingReverse()
	MovableValues.JudgeX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeX
	MovableValues.JudgeY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeY
	MovableValues.JudgeZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].JudgeZoom
	MovableValues.ComboX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ComboX
	MovableValues.ComboY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ComboY
	MovableValues.ComboZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].ComboZoom
	MovableValues.ErrorBarX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ErrorBarX
	MovableValues.ErrorBarY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ErrorBarY
	MovableValues.ErrorBarWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].ErrorBarWidth
	MovableValues.ErrorBarHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].ErrorBarHeight
	MovableValues.TargetTrackerX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].TargetTrackerX
	MovableValues.TargetTrackerY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].TargetTrackerY
	MovableValues.TargetTrackerZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].TargetTrackerZoom
	MovableValues.FullProgressBarX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].FullProgressBarX
	MovableValues.FullProgressBarY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].FullProgressBarY
	MovableValues.FullProgressBarWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].FullProgressBarWidth
	MovableValues.FullProgressBarHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].FullProgressBarHeight
	MovableValues.MiniProgressBarX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].MiniProgressBarX
	MovableValues.MiniProgressBarY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].MiniProgressBarY
	MovableValues.DisplayPercentX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].DisplayPercentX
	MovableValues.DisplayPercentY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].DisplayPercentY
	MovableValues.DisplayPercentZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].DisplayPercentZoom
	MovableValues.NotefieldX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].NotefieldX
	MovableValues.NotefieldY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].NotefieldY
	MovableValues.NotefieldWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].NotefieldWidth
	MovableValues.NotefieldHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].NotefieldHeight
	MovableValues.JudgeCounterX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeCounterX
	MovableValues.JudgeCounterY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].JudgeCounterY
	MovableValues.ReplayButtonsX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ReplayButtonsX
	MovableValues.ReplayButtonsY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].ReplayButtonsY
	MovableValues.ReplayButtonsSpacing = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].ReplayButtonsSpacing
	MovableValues.ReplayButtonsZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].ReplayButtonsZoom
	MovableValues.NPSGraphX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].NPSGraphX
	MovableValues.NPSGraphY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].NPSGraphY
	MovableValues.NPSGraphWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].NPSGraphWidth
	MovableValues.NPSGraphHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].NPSGraphHeight
	MovableValues.NPSDisplayX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].NPSDisplayX
	MovableValues.NPSDisplayY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].NPSDisplayY
	MovableValues.NPSDisplayZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].NPSDisplayZoom
	MovableValues.LeaderboardX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].LeaderboardX
	MovableValues.LeaderboardY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].LeaderboardY
	MovableValues.LeaderboardSpacing = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].LeaderboardSpacing
	MovableValues.LeaderboardWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].LeaderboardWidth
	MovableValues.LeaderboardHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].LeaderboardHeight
	MovableValues.LifeP1X = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].LifeP1X
	MovableValues.LifeP1Y = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].LifeP1Y
	MovableValues.LifeP1Rotation = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].LifeP1Rotation
	MovableValues.LifeP1Width = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].LifeP1Width
	MovableValues.LifeP1Height = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].LifeP1Height
	MovableValues.PracticeCDGraphX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].PracticeCDGraphX
	MovableValues.PracticeCDGraphY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].PracticeCDGraphY
	MovableValues.PracticeCDGraphHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].PracticeCDGraphHeight
	MovableValues.PracticeCDGraphWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].PracticeCDGraphWidth
	MovableValues.BPMTextX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].BPMTextX
	MovableValues.BPMTextY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].BPMTextY
	MovableValues.BPMTextZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].BPMTextZoom
	MovableValues.MusicRateX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].MusicRateX
	MovableValues.MusicRateY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].MusicRateY
	MovableValues.MusicRateZoom = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].MusicRateZoom

end

function setMovableKeymode(key)
	keymode = key
	loadValuesTable()
end

local Round = notShit.round
local Floor = notShit.floor
local queuecommand = Actor.queuecommand
local playcommand = Actor.queuecommand
local settext = BitmapText.settext

local propsFunctions = {
	X = Actor.x,
	Y = Actor.y,
	Zoom = Actor.zoom,
	Height = Actor.zoomtoheight,
	Width = Actor.zoomtowidth,
	AddX = Actor.addx,
	AddY = Actor.addy,
	Rotation = Actor.rotationz,
}

Movable = {
	message = {},
	current = "None",
	pressed = false,
	DeviceButton_1 = {
		name = "Judge",
		textHeader = "Judgment Label Position:",
		element = {},
		children = {"Judgment", "Border"},
		properties = {"X", "Y"},
		propertyOffsets = nil,	-- manual offsets for stuff hardcoded to be relative to center and maybe other things (init in wifejudgmentspotting)
		elementTree = "GameplayXYCoordinates",
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
	DeviceButton_2 = {	-- note: there's almost certainly an update function associated with this that is doing things we aren't aware of
		name = "Judge",
		textHeader = "Judgment Label Size:",
		element = {},
		children = {"Judgment"},
		properties = {"Zoom"},
		elementTree = "GameplaySizes",
		noBorder = true,
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		}
	},
	DeviceButton_3 = {
		name = "Combo",
		textHeader = "Combo Position:",
		element = {},
		children = {"Label", "Number", "Border"},
		properties = {"X", "Y"},
		propertyOffsets = nil,
		elementTree = "GameplayXYCoordinates",
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
	DeviceButton_4 = {	-- combo and label are 2 text objects, 1 right aligned and 1 left, this makes border resizing desync from the text sometimes
		name = "Combo",	-- i really dont want to deal with this right now -mina
		textHeader = "Combo Size:",
		element = {},
		children = {"Label", "Number"},
		properties = {"Zoom"},
		elementTree = "GameplaySizes",
		noBorder = true,
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		}
	},
	DeviceButton_5 = {
		name = "ErrorBar",
		textHeader = "Error Bar Position:",
		element = {}, -- initialized later
		properties = {"X", "Y"},
		children = {"Center", "WeightedBar", "Border"},
		elementTree = "GameplayXYCoordinates",
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
		noBorder = true,
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
		noBorder = true,
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
	DeviceButton_y = {
		name = "NPSDisplay",
		textHeader = "NPS Display Position:",
		element = {},
		properties = {"X", "Y"},
		elementTree = "GameplayXYCoordinates",
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
	DeviceButton_u = {
		name = "NPSDisplay",
		textHeader = "NPS Display Size:",
		element = {},
		properties = {"Zoom"},
		elementTree = "GameplaySizes",
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		}
	},
	DeviceButton_i = {
		name = "NPSGraph",
		textHeader = "NPS Graph Position:",
		element = {},
		properties = {"X", "Y"},
		elementTree = "GameplayXYCoordinates",
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
	DeviceButton_o = {
		name = "NPSGraph",
		textHeader = "NPS Graph Size:",
		element = {},
		properties = {"Width", "Height"},
		noBorder = true,
		elementTree = "GameplaySizes",
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
		element = {},
		elementTree = "GameplayXYCoordinates",
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
	DeviceButton_s = {
		name = "Leaderboard",
		textHeader = "Leaderboard Size:",
		properties = {"Width", "Height"},
		element = {},
		elementTree = "GameplaySizes",
		noBorder = true,
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
	DeviceButton_d = {
		name = "Leaderboard",
		textHeader = "Leaderboard Spacing:",
		properties = {"Spacing"},
		elementTree = "GameplaySizes",
		DeviceButton_up = {
			arbitraryInc = true,
			property = "Spacing",
			inc = -0.3
		},
		DeviceButton_down = {
			arbitraryInc = true,
			property = "Spacing",
			inc = 0.3
		},
	},
	DeviceButton_f = {
		name = "ReplayButtons",
		textHeader = "Replay Buttons Position:",
		element = {},
		properties = {"X", "Y"},
		elementTree = "GameplayXYCoordinates",
		condition = false,
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
	},--[[
	DeviceButton_g = {
		name = "ReplayButtons",
		textHeader = "Replay Buttons Size:",
		element = {},
		noBorder = true,
		properties = {"Zoom"},
		elementTree = "GameplaySizes",
		condition = false,
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		}
	},]]
	DeviceButton_h = {
		name = "ReplayButtons",
		textHeader = "Replay Buttons Spacing:",
		properties = {"Spacing"},
		elementTree = "GameplaySizes",
		condition = false,
		DeviceButton_up = {
			arbitraryInc = true,
			property = "Spacing",
			inc = -0.5
		},
		DeviceButton_down = {
			arbitraryInc = true,
			property = "Spacing",
			inc = 0.5
		},
	},
	DeviceButton_j = {
		name = "LifeP1",
		textHeader = "Lifebar Position:",
		element = {},
		properties = {"X", "Y"},
		-- propertyOffsets = {"178", "10"},
		elementTree = "GameplayXYCoordinates",
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
	DeviceButton_k = {
		name = "LifeP1",
		textHeader = "Lifebar Size:",
		properties = {"Width", "Height"},
		element = {},
		elementTree = "GameplaySizes",
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
	DeviceButton_l = {
		name = "LifeP1",
		textHeader = "Lifebar Rotation:",
		properties = {"Rotation"},
		element = {},
		elementTree = "GameplayXYCoordinates",
		DeviceButton_up = {
			property = "Rotation",
			inc = -1
		},
		DeviceButton_down = {
			property = "Rotation",
			inc = 1
		},
	},
	DeviceButton_z = {
		name = "PracticeCDGraph",
		textHeader = "Chord Density Graph Position:",
		properties = {"X","Y"},
		element = {},
		elementTree = "GameplayXYCoordinates",
		propertyOffsets = nil,
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
	--[[DeviceButton_x = {
		name = "PracticeCDGraph",
		textHeader = "Chord Density Graph Size:",
		properties = {"Width", "Height"},
		element = {},
		elementTree = "GameplaySizes",
		propertyOffsets = nil,
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
	},]]
	DeviceButton_x = {
		name = "BPMText",
		textHeader = "BPM Text Position:",
		element = {},
		properties = {"X", "Y"},
		elementTree = "GameplayXYCoordinates",
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
	DeviceButton_c = {
		name = "BPMText",
		textHeader = "BPM Text Size:",
		element = {},
		properties = {"Zoom"},
		elementTree = "GameplaySizes",
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		}
	},
	DeviceButton_v = {
		name = "MusicRate",
		textHeader = "Music Rate Position:",
		element = {},
		properties = {"X", "Y"},
		elementTree = "GameplayXYCoordinates",
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
	DeviceButton_b = {
		name = "MusicRate",
		textHeader = "Music Rate Size:",
		element = {},
		properties = {"Zoom"},
		elementTree = "GameplaySizes",
		DeviceButton_up = {
			property = "Zoom",
			inc = 0.01
		},
		DeviceButton_down = {
			property = "Zoom",
			inc = -0.01
		}
	},
}

local function updatetext(button)
	local text = {Movable[button].textHeader}
	for _, prop in ipairs(Movable[button].properties) do
		local fullProp = Movable[button].name .. prop
		text[#text + 1] = prop .. ": " .. MovableValues[fullProp]
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
				newVal = MovableValues[prop] + (curKey.inc * ((curKey.notefieldY and not usingReverse) and -1 or 1))
			end
			
			MovableValues[prop] = newVal
			if curKey.arbitraryFunction then
				if curKey.arbitraryInc then
					curKey.arbitraryFunction(curKey.inc)
				else
					curKey.arbitraryFunction(newVal)
				end
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
			-- commented this to save I/O time and reduce lag
			-- just make sure to call this somewhere else to make sure stuff saves.
			-- (like when the screen changes....)
			--playerConfig:save(pn_to_profile_slot(PLAYER_1))
		end
	end
	return false
end

function setBorderAlignment(self, h, v)
	self:RunCommandsOnChildren(
		function(self)
			self:halign(h):valign(v)
		end
	)
	self:GetChild("hideybox"):addx(-2 * (h - 0.5))
	self:GetChild("hideybox"):addy(-2 * (v - 0.5))
end

function setBorderToText(b, t)
	b:playcommand("ChangeWidth", {val = t:GetZoomedWidth()})
	b:playcommand("ChangeHeight", {val = t:GetZoomedHeight()})
	b:playcommand("ChangeZoom", {val = t:GetParent():GetZoom()})
end

-- this is supreme lazy -mina
local function elementtobutton(name)
	name = name == "Judgment" and "Judge" or name
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
			if Movable[b].propertyOffsets ~= nil then
				nx = nx - Movable[b].propertyOffsets[1]
				ny = ny - Movable[b].propertyOffsets[2]
			end
			MovableInput({DeviceInput = {button = b}, hellothisismouse = true, axis = "x", val = nx})
			MovableInput({DeviceInput = {button = b}, hellothisismouse = true, axis = "y", val = ny})
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
