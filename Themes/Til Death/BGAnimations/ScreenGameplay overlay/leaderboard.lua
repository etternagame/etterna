local keymode = getCurrentKeyMode()
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local leaderboardEnabled = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).leaderboardEnabled and DLMAN:IsLoggedIn()
local values = {
	LeaderboardX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].LeaderboardX,
	LeaderboardY = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates[keymode].LeaderboardY,
	LeaderboardSpacing = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].LeaderboardSpacing,
	LeaderboardWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].LeaderboardWidth,
	LeaderboardHeight = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes[keymode].LeaderboardHeight
}

local entryActors = {}
local function arbitraryLeaderboardSpacing(value)
	for i, entry in ipairs(entryActors) do
		entry.container:addy((i-1) * value)
	end
end

local t =
	Widg.Container {
	x = values.LeaderboardX,
	y = values.LeaderboardY
}

local propsFunctions = {
	X = Actor.x,
	Y = Actor.y,
	Height = Actor.zoomtoheight,
	Width = Actor.zoomtowidth
}

local movable = {
	current = "",
	pressed = false,
	DeviceButton_a = {
		name = "Leaderboard",
		element = {},
		elementTree = "GameplayXYCoordinates",
		condition = leaderboardEnabled,
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
		element = {},
		elementTree = "GameplaySizes",
		condition = leaderboardEnabled,
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
		elementTree = "GameplaySizes",
		condition = leaderboardEnabled,
		DeviceButton_up = {
			arbitraryFunction = arbitraryLeaderboardSpacing,
			property = "Spacing",
			inc = -0.3
		},
		DeviceButton_down = {
			arbitraryFunction = arbitraryLeaderboardSpacing,
			property = "Spacing",
			inc = 0.3
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
			if curKey.arbitraryFunction then
				curKey.arbitraryFunction(curKey.inc)
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

if not leaderboardEnabled then
	return t
end
local CRITERIA = "GetWifeScore"
local NUM_ENTRIES = 5
local ENTRY_HEIGHT = IsUsingWideScreen() and 35 or 20
local WIDTH = SCREEN_WIDTH * (IsUsingWideScreen() and 0.3 or 0.275)
local jdgs = {
	-- Table of judgments for the judgecounter
	"TapNoteScore_W1",
	"TapNoteScore_W2",
	"TapNoteScore_W3",
	"TapNoteScore_W4",
	"TapNoteScore_W5"
}

if not DLMAN:GetCurrentRateFilter() then
	DLMAN:ToggleRateFilter()
end
local onlineScores = DLMAN:RequestChartLeaderBoard(GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey())
local sortFunction = function(h1, h2)
	return h1[CRITERIA](h1) > h2[CRITERIA](h2)
end
table.sort(onlineScores, sortFunction)
local curScore
curScore = {
	GetDisplayName = function()
		return DLMAN:GetUsername()
	end,
	GetWifeGrade = function()
		return curScore.curGrade
	end,
	GetWifeScore = function()
		return curScore.curWifeScore
	end,
	GetSkillsetSSR = function()
		return -1
	end,
	GetJudgmentString = function()
		local str = ""
		for i, v in ipairs(jdgs) do
			str = str .. curScore.jdgVals[v] .. " | "
		end
		return str .. "x" .. curScore.combo
	end
}
curScore.combo = 0
curScore.curWifeScore = 0
curScore.curGrade = "Grade_Tier02"
curScore.jdgVals = {
	["TapNoteScore_W1"] = 0,
	["TapNoteScore_W2"] = 0,
	["TapNoteScore_W3"] = 0,
	["TapNoteScore_W4"] = 0,
	["TapNoteScore_W5"] = 0,
	["TapNoteScore_Miss"] = 0
}
local scoreboard = {}
for i = 1, NUM_ENTRIES - 1 do
	scoreboard[i] = onlineScores[i]
end
local done = false
for i = 1, NUM_ENTRIES do
	if not done and not scoreboard[i] then
		scoreboard[i] = curScore
		done = true
	end
end

for i = 1, NUM_ENTRIES do
	entryActors[i] = {}
end
function scoreEntry(i)
	local entryActor
	local entry =
		Widg.Container {
		x = WIDTH / 40,
		y = (i - 1) * ENTRY_HEIGHT * 1.3,
		onInit = function(self)
			entryActor = self
			entryActors[i]["container"] = self
			self.update = function(self, hs)
				self:visible(not (not hs))
			end
			self:update(scoreboard[i])
		end
	}
	entry:add(
		Widg.Rectangle {
			width = WIDTH,
			height = ENTRY_HEIGHT,
			color = getLeaderboardColor("background"),
			halign = 0.5
		}
	)
	local labelContainer =
		Widg.Container {
		x = WIDTH / 5
	}
	entry:add(labelContainer)
	local y
	local addLabel = function(name, fn, x, y)
		y = (y or 0) - (IsUsingWideScreen() and 0 or ENTRY_HEIGHT / 3.2)
		labelContainer:add(
			Widg.Label {
				onInit = function(self)
					entryActors[i][name] = self
					self.update = function(self, hs)
						if hs then
							self:visible(true)
							fn(self, hs)
						else
							self:visible(false)
						end
					end
					self:update(scoreboard[i])
				end,
				halign = 0,
				scale = 0.4,
				x = (x - WIDTH / 2) * 0.4,
				y = 10 + y,
				color = getLeaderboardColor("text")
			}
		)
	end
	addLabel(
		"rank",
		function(self, hs)
			self:settext(tostring(i))
		end,
		5,
		ENTRY_HEIGHT / 4
	)
	addLabel(
		"ssr",
		function(self, hs)
			local ssr = hs:GetSkillsetSSR("Overall")
			if ssr < 0 then
				self:settext("")
			else
				self:settextf("%.2f", ssr):diffuse(byMSD(ssr))
			end
		end,
		WIDTH / 5
	)
	addLabel(
		"name",
		function(self, hs)
			self:settext(hs:GetDisplayName())
		end,
		WIDTH / 1.3
	)
	--WIDTH - 84
	addLabel(
		"wife",
		function(self, hs)
			self:settextf("%05.2f%%", hs:GetWifeScore() * 100):diffuse(byGrade(hs:GetWifeGrade()))
		end,
		1.8 * WIDTH
	)
	addLabel(
		"judges",
		function(self, hs)
			self:settext(hs:GetJudgmentString())
		end,
		WIDTH / 5,
		ENTRY_HEIGHT / 2
	)
	return entry
end
for i = 1, NUM_ENTRIES do
	t[#t + 1] = scoreEntry(i)
end

t.ComboChangedMessageCommand = function(self, params)
	curScore.combo = params.PlayerStageStats and params.PlayerStageStats:GetCurrentCombo() or params.OldCombo
end
t.JudgmentMessageCommand = function(self, params)
	if curScore.jdgVals[params.Judgment] then
		curScore.jdgVals[params.Judgment] = params.Val
	end
	-- params.curWifeScore retrieves the Judgment Message curWifeScore which is a raw number for calculations; very large
	-- the online highscore curWifeScore is the wife percent...
	-- params.WifePercent is our current calculated wife percent.
	local old = curScore.curWifeScore
	curScore.curWifeScore = notShit.floor(params.WifePercent * 100) / 10000
	if old ~= curScore.curWifeScore then
		table.sort(scoreboard, sortFunction)
		for i, entry in ipairs(entryActors) do
			for name, label in pairs(entry) do
				label:update(scoreboard[i])
			end
		end
	end
end

t.OnCommand = function(self, params)
	if (allowedCustomization) then
		SCREENMAN:GetTopScreen():AddInputCallback(input)
		movable.DeviceButton_a.element = self
		movable.DeviceButton_s.element = self
	end
	arbitraryLeaderboardSpacing(values.LeaderboardSpacing)
	self:zoomtowidth(values.LeaderboardWidth)
	self:zoomtoheight(values.LeaderboardHeight)
end

return t
