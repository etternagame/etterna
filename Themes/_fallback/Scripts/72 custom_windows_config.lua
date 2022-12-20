local defaultConfig = {

	-- each entry here corresponds to a key in the customWindowConfigs table below
	-- the order you place them here will be the order which they show up
	-- any that are left out of this list are not used at all
	customWindowOrder = {
		"dpJ4",
		"wife2J4",
		"osumaniaOD10",
	},

	--[[
		Please note that if your game hard freezes when trying to rescore something, it is probably your fault from modifying this table.
		Each entry in this below table looks similar to this:

		customWindow1 = { -- change "customWindow1" to your internal custom window name. this will be the name written in the above table for ordering purposes
			displayName = "Custom Window 1",
			customWindowNames = { -- OPTIONAL: if left out (nil) then the game's defaults are chosen
				W1 = "Marvelous",
				W2 = "Perfect",
				W3 = "Great",
				W4 = "Good",
				W5 = "Bad",
				Miss = "Miss",
			},

			-- NOTE: if BOTH customWindowCurveFunction and customWindowWorths are defined, only customWindowCurveFunction is used
			-- NOTE: if NEITHER customWindowCurveFunction nor customWindowWorths are defined, wife3 J4 is used
			customWindowCurveFunction = -- OPTIONAL: if left out, defaults to customWindowWorths
			function(tapOffset)
				return math.abs(tapOffset * 1000) / 180 -- just an example, returns [0,1]
			end,

			customWindowWorths = { -- OPTIONAL: if left out, defaults to customWindowCurveFunction
				W1 = 2,
				W2 = 2,
				W3 = 1,
				W4 = 0,
				W5 = -4,
				Miss = -8,
			},

			customWindowWindows = { -- OPTIONAL: if left out, defaults to the J4 windows
				-- write these in terms of milliseconds.
				-- each entry is the "max" so if you put 22 for window 1 and the offset being parsed is exactly 22, the result is window 1, not window 2.
				W1 = 22.5,
				W2 = 45,
				W3 = 90,
				W4 = 135,
				W5 = 180,
				-- putting miss in this list does nothing
			},

			customWindowHoldWorths = { -- OPTIONAL: if left out, defaults to wife3 values
				LetGo = -4.5,
				Missed = -4.5,
				Held = 0,
			},

			customWindowMineHitWorth = -2, -- OPTIONAL: if left out, defaults to the wife3 value

			customWindowTapNoteTypeWorths = { -- OPTIONAL: if left out, defaults to wife3 value (the list below)
				Tap = 2,
				HoldHead = 2, -- this tends to be the worth of a Held hold as well as a Tap, so add both of them.
				Lift = 2,
				Mine = 0, -- only counted for mine hits, not mine "misses". 0 is the normal behavior. probably dont change this.
			},

			judgeByOldestNote = false -- OPTIONAL: defaults to false. true means that the oldest note in the hit window counts first. use true for osu, basically

		}
	]]
	customWindowConfigs = {

		dpJ4 = {
			displayName = "DP Judge 4",
			customWindowNames = nil,
			customWindowCurveFunction = nil,
			customWindowWorths = {
				W1 = 2,
				W2 = 2,
				W3 = 1,
				W4 = 0,
				W5 = -4,
				Miss = -8,
			},
			customWindowWindows = {
				W1 = 22.5,
				W2 = 45.0,
				W3 = 90.0,
				W4 = 135.0,
				W5 = 180.0,
			},
			customWindowHoldWorths = {
				LetGo = -6,
				Missed = -6,
				Held = 6,
			},
			customWindowMineHitWorth = -8,
			customWindowTapNoteTypeWorths = {
				Tap = 2,
				HoldHead = 8,
				Lift = 2,
				Mine = 0,
			},
			judgeByOldestNote = false,
		},

		wife2J4 = {
			displayName = "Wife2 J4",
			customWindowNames = nil,
			customWindowCurveFunction = function(offset)
				offset = offset * 1000
				local avedeviation = 95
				local y = 1 - (2 ^ (-1 * offset * offset / (avedeviation * avedeviation)))
				y = y ^ 2
				return (2 - -8) * (1 - y) + -8
			end,
			customWindowWorths = nil,
			customWindowWindows = nil,
			customWindowHoldWorths = {
				LetGo = -6,
				Missed = -6,
				Held = 0,
			},
			customWindowMineHitWorth = -8,
			customWindowTapNoteTypeWorths = {
				Tap = 2,
				HoldHead = 2,
				Lift = 2,
				Mine = 0,
			},
			judgeByOldestNote = false,
		},

		osumaniaOD10 = {
			displayName = "osu!mania OD10",
			customWindowNames = {
				W1 = "300g",
				W2 = "300",
				W3 = "200",
				W4 = "100",
				W5 = "50",
				Miss = "Miss",
			},
			customWindowCurveFunction = nil,
			customWindowWindows = {
				W1 = 16.0,
				W2 = 34.0,
				W3 = 67.0,
				W4 = 97.0,
				W5 = 121.0,
			},
			customWindowWorths = {
				W1 = 3,
				W2 = 3,
				W3 = 2,
				W4 = 1,
				W5 = 0.5,
				Miss = 0,
			},
			customWindowHoldWorths = {
				LetGo = -3,
				Missed = -3,
				Held = 0.5,
			},
			customWindowMineHitWorth = 0,
			customWindowTapNoteTypeWorths = {
				Tap = 3,
				HoldHead = 3.5,
				Lift = 3,
				Mine = 0,
			},
			judgeByOldestNote = true,
		}

	},
}

customWindowsConfig = create_setting("customWindowsConfig", "customWindowsConfig.lua", defaultConfig, -1)
customWindowsConfig:load()

-- this is restricted to the bounds of table customWindowsConfig:get_data().customWindowOrder
currentCustomWindowConfigIndex = 1

function getCurrentCustomWindowConfigIndex()
	return currentCustomWindowConfigIndex
end

function moveCustomWindowConfigIndex(direction)
	currentCustomWindowConfigIndex = currentCustomWindowConfigIndex + direction
	if currentCustomWindowConfigIndex > getTotalCustomWindowConfigs() then
		currentCustomWindowConfigIndex = 1
	elseif currentCustomWindowConfigIndex < 1 then
		currentCustomWindowConfigIndex = getTotalCustomWindowConfigs()
	end
end

local function getcurrentrealconfig()
	return customWindowsConfig:get_data().customWindowConfigs[getCurrentCustomWindowConfig()]
end

function getCurrentCustomWindowConfig()
	return customWindowsConfig:get_data().customWindowOrder[currentCustomWindowConfigIndex]
end

function getTotalCustomWindowConfigs()
	return #customWindowsConfig:get_data().customWindowOrder
end

function getCurrentCustomWindowConfigName()
	return getcurrentrealconfig().displayName or "NAME MISSING"
end

function currentCustomWindowConfigUsesOldestNoteFirst()
	return getcurrentrealconfig().judgeByOldestNote or false
end

-- the parameter to this is a config entry from the table above
function loadCustomWindowConfig(config)
	REPLAYS:ResetCustomScoringFunctions()
	local loadedAFunctionalCustomConfig = false

	if config.customWindowWorths ~= nil then
		loadedAFunctionalCustomConfig = true
		local fallback = 2
		if config.customWindowTapNoteTypeWorths ~= nil then
			if config.customWindowTapNoteTypeWorths["Tap"] ~= nil then
				fallback = config.customWindowTapNoteTypeWorths["Tap"]
			end
		end
		local fallbackTable = {
			TapNoteScore_W1 = config.customWindowWorths["W1"] or 2,
			TapNoteScore_W2 = config.customWindowWorths["W2"] or 2,
			TapNoteScore_W3 = config.customWindowWorths["W3"] or 1,
			TapNoteScore_W4 = config.customWindowWorths["W4"] or 0,
			TapNoteScore_W5 = config.customWindowWorths["W5"] or -4,
			TapNoteScore_Miss = config.customWindowWorths["Miss"] or -8,
		}
		REPLAYS:SetTapScoringFunction(
			function(tapOffset, tapNoteScore, judgeScalar)
				return fallbackTable[tapNoteScore] or fallback
			end
		)
	elseif config.customWindowCurveFunction ~= nil then
		loadedAFunctionalCustomConfig = true
		REPLAYS:SetTapScoringFunction(
			function(tapOffset, tapNoteScore, judgeScalar)
				return config.customWindowCurveFunction(tapOffset) or 0
			end
		)
	end

	if config.customWindowMineHitWorth ~= nil then
		loadedAFunctionalCustomConfig = true
		REPLAYS:SetMineScoringFunction(
			function()
				return config.customWindowMineHitWorth
			end
		)
	end

	if config.customWindowHoldWorths ~= nil then
		loadedAFunctionalCustomConfig = true
		local fallbackTable = {
			HoldNoteScore_Held = config.customWindowHoldWorths and config.customWindowHoldWorths["Held"] or 0,
			HoldNoteScore_LetGo = config.customWindowHoldWorths and config.customWindowHoldWorths["LetGo"] or -4.5,
			HoldNoteScore_Missed = config.customWindowHoldWorths and config.customWindowHoldWorths["Missed"] or -4.5,
		}
		REPLAYS:SetHoldNoteScoreScoringFunction(
			function(holdNoteScore)
				return fallbackTable[holdNoteScore] or 0
			end
		)
	end

	if config.customWindowTapNoteTypeWorths ~= nil then
		loadedAFunctionalCustomConfig = true
		local fallbackTable = {
			TapNoteType_Tap = config.customWindowTapNoteTypeWorths and config.customWindowTapNoteTypeWorths["Tap"] or 2,
			TapNoteType_HoldHead = config.customWindowTapNoteTypeWorths and config.customWindowTapNoteTypeWorths["HoldHead"] or 2,
			TapNoteType_Lift = config.customWindowTapNoteTypeWorths and config.customWindowTapNoteTypeWorths["Lift"] or 2,
			TapNoteType_Mine = config.customWindowTapNoteTypeWorths and config.customWindowTapNoteTypeWorths["Mine"] or 0,
		}
		REPLAYS:SetTotalWifePointsCalcFunction(
			function(tapNoteType)
				return fallbackTable[tapNoteType] or 0
			end
		)
	end

	if config.customWindowWindows ~= nil then
		loadedAFunctionalCustomConfig = true
		REPLAYS:SetOffsetJudgingFunction(
			function(tapOffset)
				tapOffset = math.abs(tapOffset * 1000)
				if tapOffset <= (config.customWindowWindows["W1"] or 22.5) then
					return "TapNoteScore_W1"
				elseif tapOffset <= (config.customWindowWindows["W2"] or 45) then
					return "TapNoteScore_W2"
				elseif tapOffset <= (config.customWindowWindows["W3"] or 90) then
					return "TapNoteScore_W3"
				elseif tapOffset <= (config.customWindowWindows["W4"] or 135) then
					return "TapNoteScore_W4"
				elseif tapOffset <= (config.customWindowWindows["W5"] or 180) then
					return "TapNoteScore_W5"
				else
					return "TapNoteScore_Miss"
				end
			end
		)

		if config.customWindowWindows["W5"] ~= nil then
			local w5 = config.customWindowWindows["W5"] / 1000
			REPLAYS:SetMissWindowFunction(function() return w5 end)
		end
	end

	if loadedAFunctionalCustomConfig then
		print("Set custom window config to window config: ".. (config.displayName or "NAME MISSING"))
	else
		print("Reset custom window config to game defaults.")
	end
end

function loadCustomWindowConfigByIndex(index)
	local name = customWindowsConfig:get_data().customWindowOrder[index]
	loadCustomWindowConfig(customWindowsConfig:get_data().customWindowConfigs[name])
end

function loadCurrentCustomWindowConfig()
	loadCustomWindowConfigByIndex(currentCustomWindowConfigIndex)
end

function unloadCustomWindowConfig()
	print("Unloading Custom Window Config")
	REPLAYS:ResetCustomScoringFunctions()
end

function getCustomWindowConfigJudgmentName(judgmentName)
	local baseNames = {
		W1 = THEME:GetString("TapNoteScore", "W1"),
		W2 = THEME:GetString("TapNoteScore", "W2"),
		W3 = THEME:GetString("TapNoteScore", "W3"),
		W4 = THEME:GetString("TapNoteScore", "W4"),
		W5 = THEME:GetString("TapNoteScore", "W5"),
		Miss = THEME:GetString("TapNoteScore", "Miss"),
	}
	local nm = judgmentName:gsub("TapNoteScore_", "")
	return getcurrentrealconfig().customWindowNames and getcurrentrealconfig().customWindowNames[nm] or baseNames[nm]
end

-- this returns in whole milliseconds
function getCustomWindowConfigJudgmentWindow(judgmentName)
	local baseWindows = {
		W1 = 22.5,
		W2 = 45,
		W3 = 90,
		W4 = 135,
		W5 = 180,
	}
	local nm = judgmentName:gsub("TapNoteScore_", "")
	return getcurrentrealconfig().customWindowWindows and getcurrentrealconfig().customWindowWindows[nm] or baseWindows[nm] or 0
end

function getCustomWindowConfigJudgmentWindowLowerBound(judgmentName)
	local baseWindows = {
		W0 = 0,
		W1 = 22.5,
		W2 = 45,
		W3 = 90,
		W4 = 135,
		W5 = 180,
	}
	if judgmentName:gsub("TapNoteScore_", "") == "Miss" then return 180 end
	local nm = judgmentName:gsub("TapNoteScore_W", "")
	-- nm here should be a number, we need to subtract 1 from it.
	nm = "W" .. tostring(tonumber(nm) - 1)
	return getcurrentrealconfig().customWindowWindows and getcurrentrealconfig().customWindowWindows[nm] or baseWindows[nm] or 0
end

function getCurrentCustomWindowConfigJudgmentWindowTable()
	local baseWindows = {
		TapNoteScore_W1 = getcurrentrealconfig().customWindowWindows and getcurrentrealconfig().customWindowWindows["W1"] or 22.5,
		TapNoteScore_W2 = getcurrentrealconfig().customWindowWindows and getcurrentrealconfig().customWindowWindows["W2"] or 45,
		TapNoteScore_W3 = getcurrentrealconfig().customWindowWindows and getcurrentrealconfig().customWindowWindows["W3"] or 90,
		TapNoteScore_W4 = getcurrentrealconfig().customWindowWindows and getcurrentrealconfig().customWindowWindows["W4"] or 135,
		TapNoteScore_W5 = getcurrentrealconfig().customWindowWindows and getcurrentrealconfig().customWindowWindows["W5"] or 180,
	}
	return baseWindows
end

--[[ REPLAYS (ReplayManager) documentation
	-----
	REPLAYS:SetTapScoringFunction(function(tapOffset, tapNoteScore, judgeScalar) end)
	for setting the function that scores each individual tap
	
	for a curve:
	function(tapOffset, tapNoteScore, scalar)
		-- this is roughly wife2
		tapOffset = tapOffset * 1000
		local avedeviation = 95 * scalar
		local y = 1 - math.pow(2, -1 * tapOffset * tapOffset / (avedeviation * avedeviation)))
		y = math.pow(y, 2)
		return (2 - -8) * (1 - y) - 8
	end

	for window based:
	function(tapOffset, tapNoteScore, scalar)
		-- this is roughly sm5 DP
		local worths = {
			TapNoteScore_W1 = 2,
			TapNoteScore_W2 = 2,
			TapNoteScore_W3 = 1,
			TapNoteScore_W4 = 0,
			TapNoteScore_W5 = -4,
			TapNoteScore_Miss = -8,
		}
		local fallback = 0
		local worth = worths[tapNoteScore] or fallback
		return worth
	end


	-----
	REPLAYS:SetHoldNoteScoreScoringFunction(function(holdNoteScore) end)
	for setting the function that scores each HoldNoteScore worth
	
	function(holdNoteScore)
		local worths = {
			HNS_LetGo = -2, -- the hold was dropped
			HNS_Held = 0, -- this hold was completed
			HNS_Missed = -2, -- this hold was dropped because the head was missed
		}
		local fallback = 0
		local worth = worths[holdNoteScore] or fallback
		return worth
	end


	-----
	REPLAYS:SetMineScoringFunction(function() end)
	for setting the function that scores each mine hit
	there is no parameter -- this just lets you tell us how much a mine hit hurts your score
	
	function()
		-- just return how much a mine hit hurts your score
		return -2 -- make sure that it is a negative number...
	end


	-----
	REPLAYS:SetTotalWifePointsCalcFunction(function(tapNoteType) end)
	for setting the function that scores how much each TapNoteType is worth when totaling the max points for a file
	usually this means it is the maximum possible points obtained for a note
	when considering mines here, this is what they are worth IF THEY ARE HIT
	mines are completely ignored otherwise. for that reason, we set them to 0 by default.
	fakes are also not present because they should never be scored.
	
	function(tapNoteType)
		local worths = {
			TapNoteType_Tap = 2,
			TapNoteType_HoldHead = 2, -- this tends to be the worth of a hold including the head, so the tap plus the completion.
			TapNoteType_Lift = 2,
			TapNoteType_Mine = 0,
		}
		local fallback = 0
		local worth = worths[tapNoteType] or fallback
		return worth
	end


	-----
	REPLAYS:SetOffsetJudgingFunction(function(tapOffset) end)
	for setting which judgment results from individual tap offsets
	the purpose of this is to work in conjunction with the function defined in SetTapScoringFunction
	use this if it is important that your custom window sizes show up correctly in judgment counters (like the offset hover)
	also, it is separated so that it can be used as a scoring function for gameplay
	
	function(tapOffset)
		local ms = tapOffset * 1000
		if ms <= 22.5 then
			return "TapNoteScore_W1"
		elseif ms <= 45 then
			return "TapNoteScore_W2"
		elseif ms <= 90 then
			return "TapNoteScore_W3"
		elseif ms <= 135 then
			return "TapNoteScore_W4"
		elseif ms <= 180 then
			return "TapNoteScore_W5"
		else
			return "TapNoteScore_Miss"
		end
	end


	-----
	REPLAYS:ResetCustomScoringFunctions()
	this will reset all the custom scoring functions set on REPLAYS (ReplayManager) back to the internal defaults.
]]

