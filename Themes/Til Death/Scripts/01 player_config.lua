local defaultConfig = {
	ScreenFilter = 1,
	JudgeType = 1,
	AvgScoreType = 0,
	GhostScoreType = 1,
	DisplayPercent = true,
	TargetTracker = true,
	TargetGoal = 93,
	TargetTrackerMode = 0,
	JudgeCounter = true,
	ErrorBar = 1,
	leaderboardEnabled = false,
	PlayerInfo = true,
	FullProgressBar = true,
	MiniProgressBar = true,
	LaneCover = 0, -- soon to be changed to: 0=off, 1=sudden, 2=hidden
	LaneCoverHeight = 10,
	NPSDisplay = true,
	NPSGraph = false,
	CBHighlight = false,
	OneShotMirror = false,
	JudgmentText = true,
	ReceptorSize = 100,
	BackgroundType = 1,
	UserName = "",
	PasswordToken = "",
	CustomizeGameplay = false,
	CustomEvaluationWindowTimings = false,
	GameplayXYCoordinates = {
		["4K"] = {
			JudgeX = 0,
			JudgeY = 40,
			ComboX = 30,
			ComboY = -20,
			ErrorBarX = SCREEN_CENTER_X,
			ErrorBarY = SCREEN_CENTER_Y + 53,
			TargetTrackerX = SCREEN_CENTER_X + 26,
			TargetTrackerY = SCREEN_CENTER_Y + 30,
			MiniProgressBarX = SCREEN_CENTER_X + 44,
			MiniProgressBarY = SCREEN_CENTER_Y + 34,
			FullProgressBarX = SCREEN_CENTER_X,
			FullProgressBarY = 20,
			JudgeCounterX = 0,
			JudgeCounterY = 0,
			DisplayPercentX = 0,
			DisplayPercentY = 0,
			NPSDisplayX = 5,
			NPSDisplayY = SCREEN_BOTTOM - 170,
			NPSGraphX = 0,
			NPSGraphY = SCREEN_BOTTOM - 160,
			NotefieldX = 0,
			NotefieldY = 0,
			ProgressBarPos = 1
		},
		["5K"] = {
			JudgeX = 0,
			JudgeY = 40,
			ComboX = 30,
			ComboY = -20,
			ErrorBarX = SCREEN_CENTER_X,
			ErrorBarY = SCREEN_CENTER_Y + 53,
			TargetTrackerX = SCREEN_CENTER_X + 26,
			TargetTrackerY = SCREEN_CENTER_Y + 30,
			MiniProgressBarX = SCREEN_CENTER_X + 44,
			MiniProgressBarY = SCREEN_CENTER_Y + 34,
			FullProgressBarX = SCREEN_CENTER_X,
			FullProgressBarY = 20,
			JudgeCounterX = 0,
			JudgeCounterY = 0,
			DisplayPercentX = 0,
			DisplayPercentY = 0,
			NPSDisplayX = 5,
			NPSDisplayY = SCREEN_BOTTOM - 170,
			NPSGraphX = 0,
			NPSGraphY = SCREEN_BOTTOM - 160,
			NotefieldX = 0,
			NotefieldY = 0,
			ProgressBarPos = 1
		},
		["6K"] = {
			JudgeX = 0,
			JudgeY = 40,
			ComboX = 30,
			ComboY = -20,
			ErrorBarX = SCREEN_CENTER_X,
			ErrorBarY = SCREEN_CENTER_Y + 53,
			TargetTrackerX = SCREEN_CENTER_X + 26,
			TargetTrackerY = SCREEN_CENTER_Y + 30,
			MiniProgressBarX = SCREEN_CENTER_X + 44,
			MiniProgressBarY = SCREEN_CENTER_Y + 34,
			FullProgressBarX = SCREEN_CENTER_X,
			FullProgressBarY = 20,
			JudgeCounterX = 0,
			JudgeCounterY = 0,
			DisplayPercentX = 0,
			DisplayPercentY = 0,
			NPSDisplayX = 5,
			NPSDisplayY = SCREEN_BOTTOM - 170,
			NPSGraphX = 0,
			NPSGraphY = SCREEN_BOTTOM - 160,
			NotefieldX = 0,
			NotefieldY = 0,
			ProgressBarPos = 1
		},
		["7K"] = {
			JudgeX = 0,
			JudgeY = 40,
			ComboX = 30,
			ComboY = -20,
			ErrorBarX = SCREEN_CENTER_X,
			ErrorBarY = SCREEN_CENTER_Y + 53,
			TargetTrackerX = SCREEN_CENTER_X + 26,
			TargetTrackerY = SCREEN_CENTER_Y + 30,
			MiniProgressBarX = SCREEN_CENTER_X + 44,
			MiniProgressBarY = SCREEN_CENTER_Y + 34,
			FullProgressBarX = SCREEN_CENTER_X,
			FullProgressBarY = 20,
			JudgeCounterX = 0,
			JudgeCounterY = 0,
			DisplayPercentX = 0,
			DisplayPercentY = 0,
			NPSDisplayX = 5,
			NPSDisplayY = SCREEN_BOTTOM - 170,
			NPSGraphX = 0,
			NPSGraphY = SCREEN_BOTTOM - 160,
			NotefieldX = 0,
			NotefieldY = 0,
			ProgressBarPos = 1
		},
		["8K"] = {
			JudgeX = 0,
			JudgeY = 40,
			ComboX = 30,
			ComboY = -20,
			ErrorBarX = SCREEN_CENTER_X,
			ErrorBarY = SCREEN_CENTER_Y + 53,
			TargetTrackerX = SCREEN_CENTER_X + 26,
			TargetTrackerY = SCREEN_CENTER_Y + 30,
			MiniProgressBarX = SCREEN_CENTER_X + 44,
			MiniProgressBarY = SCREEN_CENTER_Y + 34,
			FullProgressBarX = SCREEN_CENTER_X,
			FullProgressBarY = 20,
			JudgeCounterX = 0,
			JudgeCounterY = 0,
			DisplayPercentX = 0,
			DisplayPercentY = 0,
			NPSDisplayX = 5,
			NPSDisplayY = SCREEN_BOTTOM - 170,
			NPSGraphX = 0,
			NPSGraphY = SCREEN_BOTTOM - 160,
			NotefieldX = 0,
			NotefieldY = 0,
			ProgressBarPos = 1
		}
	},
	GameplaySizes = {
		["4K"] = {
			JudgeZoom = 1.0,
			ComboZoom = 0.6,
			ErrorBarWidth = 240,
			ErrorBarHeight = 10,
			TargetTrackerZoom = 0.4,
			FullProgressBarWidth = 1.0,
			FullProgressBarHeight = 1.0,
			DisplayPercentZoom = 1,
			NPSDisplayZoom = 0.4,
			NPSGraphWidth = 1.0,
			NPSGraphHeight = 1.0,
			NotefieldWidth = 1.0,
			NotefieldHeight = 1.0
		},
		["5K"] = {
			JudgeZoom = 1.0,
			ComboZoom = 0.6,
			ErrorBarWidth = 240,
			ErrorBarHeight = 10,
			TargetTrackerZoom = 0.4,
			FullProgressBarWidth = 1.0,
			FullProgressBarHeight = 1.0,
			DisplayPercentZoom = 1,
			NPSDisplayZoom = 0.4,
			NPSGraphWidth = 1.0,
			NPSGraphHeight = 1.0,
			NotefieldWidth = 1.0,
			NotefieldHeight = 1.0
		},
		["6K"] = {
			JudgeZoom = 1.0,
			ComboZoom = 0.6,
			ErrorBarWidth = 240,
			ErrorBarHeight = 10,
			TargetTrackerZoom = 0.4,
			FullProgressBarWidth = 1.0,
			FullProgressBarHeight = 1.0,
			DisplayPercentZoom = 1,
			NPSDisplayZoom = 0.4,
			NPSGraphWidth = 1.0,
			NPSGraphHeight = 1.0,
			NotefieldWidth = 1.0,
			NotefieldHeight = 1.0
		},
		["7K"] = {
			JudgeZoom = 1.0,
			ComboZoom = 0.6,
			ErrorBarWidth = 240,
			ErrorBarHeight = 10,
			TargetTrackerZoom = 0.4,
			FullProgressBarWidth = 1.0,
			FullProgressBarHeight = 1.0,
			DisplayPercentZoom = 1,
			NPSDisplayZoom = 0.4,
			NPSGraphWidth = 1.0,
			NPSGraphHeight = 1.0,
			NotefieldWidth = 1.0,
			NotefieldHeight = 1.0
		},
		["8K"] = {
			JudgeZoom = 1.0,
			ComboZoom = 0.6,
			ErrorBarWidth = 240,
			ErrorBarHeight = 10,
			TargetTrackerZoom = 0.4,
			FullProgressBarWidth = 1.0,
			FullProgressBarHeight = 1.0,
			DisplayPercentZoom = 1,
			NPSDisplayZoom = 0.4,
			NPSGraphWidth = 1.0,
			NPSGraphHeight = 1.0,
			NotefieldWidth = 1.0,
			NotefieldHeight = 1.0
		}
	}
}

playerConfig = create_setting("playerConfig", "playerConfig.lua", defaultConfig, -1)
local tmp2 = playerConfig.load
playerConfig.load = function(self, slot)
	local tmp = force_table_elements_to_match_type
	force_table_elements_to_match_type = function()
	end
	local x = create_setting("playerConfig", "playerConfig.lua", {}, -1)
	x = x:load()
	local coords = x.GameplayXYCoordinates
	local sizes = x.GameplaySizes
	if sizes then
		defaultConfig.GameplaySizes["4K"] = sizes
		defaultConfig.GameplaySizes["5K"] = sizes
		defaultConfig.GameplaySizes["6K"] = sizes
		defaultConfig.GameplaySizes["7K"] = sizes
		defaultConfig.GameplaySizes["8K"] = sizes
	end
	if coords then
		defaultConfig.GameplayXYCoordinates["4K"] = coords
		defaultConfig.GameplayXYCoordinates["5K"] = coords
		defaultConfig.GameplayXYCoordinates["6K"] = coords
		defaultConfig.GameplayXYCoordinates["7K"] = coords
		defaultConfig.GameplayXYCoordinates["8K"] = coords
	end
	force_table_elements_to_match_type = tmp
	return tmp2(self, slot)
end
playerConfig:load()

function LoadProfileCustom(profile, dir)
	local players = GAMESTATE:GetEnabledPlayers()
	local playerProfile
	local pn
	for k, v in pairs(players) do
		playerProfile = PROFILEMAN:GetProfile(v)
		if playerProfile:GetGUID() == profile:GetGUID() then
			pn = v
		end
	end

	if pn then
		local conf = playerConfig:load(pn_to_profile_slot(pn))
	end
end

function SaveProfileCustom(profile, dir)
	local players = GAMESTATE:GetEnabledPlayers()
	local playerProfile
	local pn
	for k, v in pairs(players) do
		playerProfile = PROFILEMAN:GetProfile(v)
		if playerProfile:GetGUID() == profile:GetGUID() then
			pn = v
		end
	end

	if pn then
		playerConfig:set_dirty(pn_to_profile_slot(pn))
		playerConfig:save(pn_to_profile_slot(pn))
	end
end
