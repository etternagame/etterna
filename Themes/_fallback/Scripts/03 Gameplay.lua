-- sm-ssc fallback theme | script ring 03 | Gameplay.lua
-- [en] This file is used to store settings that should be different in each
-- game mode.

-- shakesoda calls this pump.lua
local function CurGameName()
	return GAMESTATE:GetCurrentGame():GetName()
end

-- Check the active game mode against a string. Cut down typing this in metrics.
function IsGame(str)
	return CurGameName():lower() == str:lower()
end

-- GetExtraColorThreshold()
-- [en] returns the difficulty threshold in meter
-- for songs that should be counted as boss songs.
function GetExtraColorThreshold()
	local Modes = {
		dance = 10,
		pump = 21,
		beat = 12,
		kb7 = 10,
	}
	return Modes[CurGameName()] or 10
end

-- GameplayMargins exists to provide a layer of backwards compatibility for
-- people using the X position metrics to set where the notefields are.
-- This makes it somewhat complex.
-- Rather than trying to understand how it works, you can simply do this:
-- (example values in parentheses)
-- 1.  Decide how much space you want in the center between notefields. (80)
-- 2.  Decide how much space you want on each side. (40)
-- 3.  Write a simple function that just returns those numbers:
--     function GameplayMargins() return 40, 80, 40 end
-- Then the engine does the work of figuring out where each notefield should
-- be centered.
function GameplayMargins(enabled_players, styletype)
	local other = {[PLAYER_1] = PLAYER_2, [PLAYER_2] = PLAYER_1}
	local margins = {[PLAYER_1] = {40, 40}, [PLAYER_2] = {40, 40}}
	-- Use a fake style width because calculating the real style width throws off
	-- the code in the engine.
	local fake_style_width = 272
	-- Handle the case of a single player that is centered first because it's
	-- simpler.
	if Center1Player() then
		local pn = enabled_players[1]
		fake_style_width = 544
		local center = _screen.cx
		local left = center - (fake_style_width / 2)
		local right = _screen.w - center - (fake_style_width / 2)
		-- center margin width will be ignored.
		return left, 80, right
	end
	local half_screen = _screen.w / 2
	local left = {[PLAYER_1] = 0, [PLAYER_2] = half_screen}
	for i, pn in ipairs(enabled_players) do
		local edge = left[pn]
		local center =
			THEME:GetMetric("ScreenGameplay", "Player" .. ToEnumShortString(pn) .. ToEnumShortString(styletype) .. "X")
		-- Adjust for the p2 center being on the right side.
		center = center - edge
		margins[pn][1] = center - (fake_style_width / 2)
		margins[pn][2] = half_screen - center - (fake_style_width / 2)
		if #enabled_players == 1 then
			margins[other[pn]][1] = margins[pn][2]
			margins[other[pn]][2] = margins[pn][1]
		end
	end
	local left = margins[PLAYER_1][1]
	local center = margins[PLAYER_1][2] + margins[PLAYER_2][1]
	local right = margins[PLAYER_2][2]
	return left, center, right
end

-- AllowOptionsMenu()
-- [en] returns if you are able to select options
-- on ScreenSelectMusic.
function AllowOptionsMenu()
	return true
end

-- GameCompatibleModes:
-- [en] returns possible modes for ScreenSelectPlayMode
function GameCompatibleModes()
	local Modes = {
		dance = "Single,Double",
		pump = "Single,Double,HalfDouble",
		beat = "5Keys,7Keys,10Keys,14Keys",
		kb7 = "KB7",
		maniax = "Single,Double",
		solo = "Single"
	}
	return Modes[CurGameName()]
end

function SelectProfileKeys()
	local sGame = CurGameName()
	if sGame == "dance" then
		return "Up,Down,Start,Back,Up2,Down2"
	else
		return "Up,Down,Start,Back,Up2,Down2"
	end
end

-- ScoreKeeperClass:
-- [en] Determines the correct ScoreKeeper class to use.
function ScoreKeeperClass()
	return "ScoreKeeperNormal"
end

-- ComboContinue:
-- [en]
function ComboContinue()
	local Continue = {
		dance = "TapNoteScore_W5",
		pump = "TapNoteScore_W5",
		beat = "TapNoteScore_W5",
		kb7 = "TapNoteScore_W5",
	}
	return Continue[CurGameName()] or "TapNoteScore_W5"
end

function ComboMaintain()
	local Maintain = {
		dance = "TapNoteScore_W5",
		pump = "TapNoteScore_W5",
		beat = "TapNoteScore_W5",
		kb7 = "TapNoteScore_W5",
	}
	return Maintain[CurGameName()] or "TapNoteScore_W5"
end

local ComboThresholds = {
	-- each game can be defined here to override default
	-- dance = {Hit = 69, Miss = 0, Fail = 1}
	-------------------------------------------
	default = {Hit = 2, Miss = 2, Fail = -1}
}

function HitCombo()
	if ComboThresholds[CurGameName()] then
		return ComboThresholds[CurGameName()].Hit
	end
	return ComboThresholds["default"].Hit
end

function MissCombo()
	if ComboThresholds[CurGameName()] then
		return ComboThresholds[CurGameName()].Miss
	end
	return ComboThresholds["default"].Miss
end

-- FailCombo:
-- [en] The combo that causes game failure.
function FailCombo()
	-- ITG (dance) uses 30. Pump Pro uses 30, real Pump uses 51
	if ComboThresholds[CurGameName()] then
		return ComboThresholds[CurGameName()].Fail
	end
	return ComboThresholds["default"].Fail
end

local CodeDetectorCodes = {
	-- each game can also be defined
	-- this overrides the defaults
	-- ex:
	--[[
	{
		default = "",
		dance = "",
		solo = "",
		pump = "",
		beat = "",
		kb7 = "",
		popn = "",
	}
	]]

	-- steps
	PrevSteps1 = {
		default = "Up,Up",
	},
	PrevSteps2 = {
		default = "MenuUp,MenuUp",
	},
	NextSteps1 = {
		default = "Down,Down",
	},
	NextSteps2 = {
		default = "MenuDown,MenuDown",
	},
	-- group
	NextGroup = {
		default = "",
	},
	PrevGroup = {
		default = "",
	},
	CloseCurrentFolder1 = {
		default = "MenuUp-MenuDown"
	},
	CloseCurrentFolder2 = {
		default = "Up-Down"
	},
	-- sorts
	NextSort1 = {
		default = "MenuLeft-MenuRight",
	},
	NextSort2 = {
		default = "",
	},
	NextSort3 = {
		default = "",
	},
	NextSort4 = {
		default = "",
	},
	-- modemenu
	ModeMenu1 = {
		default = "Up,Down,Up,Down",
	},
	ModeMenu2 = {
		default = "MenuUp,MenuDown,MenuUp,MenuDown"
	},
	-- Evaluation:
	SaveScreenshot1 = {
		default = "MenuLeft-MenuRight"
	},
	SaveScreenshot2 = {
		default = "Select"
	},
	-- modifiers section
	CancelAll = {
		default = "",
	},
	--- specific modifiers
	Mirror = {
		default = "",
	},
	Left = {
		default = "",
	},
	Right = {
		default = "",
	},
	Shuffle = {
		default = "",
	},
	SuperShuffle = {
		default = "",
	},
	Reverse = {
		default = "",
	},
	Mines = {
		default = ""
	},
	Hidden = {
		default = "",
	},
	NextScrollSpeed = {
		default = "",
	},
	PreviousScrollSpeed = {
		default = "",
	},
	-- cancel all in player options
	CancelAllPlayerOptions = {
		default = "",
	}
}

function GetCodeForGame(codeName)
	local gameName = string.lower(CurGameName())
	local inputCode = CodeDetectorCodes[codeName]
	return inputCode[gameName] or inputCode["default"]
end
