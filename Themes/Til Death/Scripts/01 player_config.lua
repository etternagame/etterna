local defaultGameplayCoordinates = {
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
	JudgeCounterX = SCREEN_CENTER_X -200,
	JudgeCounterY = SCREEN_CENTER_Y,
	DisplayPercentX = SCREEN_CENTER_X - 170,
	DisplayPercentY = SCREEN_CENTER_Y - 60,
	NPSDisplayX = 5,
	NPSDisplayY = SCREEN_BOTTOM - 170,
	NPSGraphX = 0,
	NPSGraphY = SCREEN_BOTTOM - 160,
	NotefieldX = 0,
	NotefieldY = 0,
	ProgressBarPos = 1,
	LeaderboardX = 0,
	LeaderboardY = SCREEN_HEIGHT / 10,
	ReplayButtonsX = SCREEN_WIDTH - 45,
	ReplayButtonsY = SCREEN_HEIGHT / 2 - 100,
	LifeP1X = 178,
	LifeP1Y = 10,
	LifeP1Rotation = 0,
	PracticeCDGraphX = 10,
	PracticeCDGraphY = 85,
	BPMTextX = SCREEN_CENTER_X,
	BPMTextY = SCREEN_BOTTOM - 20,
	MusicRateX = SCREEN_CENTER_X,
	MusicRateY = SCREEN_BOTTOM - 10
}

local defaultGameplaySizes = {
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
	NotefieldHeight = 1.0,
	NotefieldSpacing = 0.0,
	LeaderboardWidth = 1.0,
	LeaderboardHeight = 1.0,
	LeaderboardSpacing = 0.0,
	ReplayButtonsZoom = 1.0,
	ReplayButtonsSpacing = 0.0,
	LifeP1Width = 1.0,
	LifeP1Height = 1.0,
	PracticeCDGraphWidth = 0.8,
	PracticeCDGraphHeight = 1,
	MusicRateZoom = 1.0,
	BPMTextZoom = 1.0
}

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
	ComboText = true,
	ReceptorSize = 100,
	ErrorBarCount = 30,
	BackgroundType = 1,
	UserName = "",
	PasswordToken = "",
	CustomizeGameplay = false,
	CustomEvaluationWindowTimings = false,
	PracticeMode = false,
	GameplayXYCoordinates = {
		["3K"] = DeepCopy(defaultGameplayCoordinates),
		["4K"] = DeepCopy(defaultGameplayCoordinates),
		["5K"] = DeepCopy(defaultGameplayCoordinates),
		["6K"] = DeepCopy(defaultGameplayCoordinates),
		["7K"] = DeepCopy(defaultGameplayCoordinates),
		["8K"] = DeepCopy(defaultGameplayCoordinates),
		["10K"] = DeepCopy(defaultGameplayCoordinates),
		["12K"] = DeepCopy(defaultGameplayCoordinates),
		["16K"] = DeepCopy(defaultGameplayCoordinates)
	},
	GameplaySizes = {
		["3K"] = DeepCopy(defaultGameplaySizes),
		["4K"] = DeepCopy(defaultGameplaySizes),
		["5K"] = DeepCopy(defaultGameplaySizes),
		["6K"] = DeepCopy(defaultGameplaySizes),
		["7K"] = DeepCopy(defaultGameplaySizes),
		["8K"] = DeepCopy(defaultGameplaySizes),
		["10K"] = DeepCopy(defaultGameplaySizes),
		["12K"] = DeepCopy(defaultGameplaySizes),
		["16K"] = DeepCopy(defaultGameplaySizes)
	}
}

playerConfig = create_setting("playerConfig", "playerConfig.lua", defaultConfig, -1)
local tmp2 = playerConfig.load
playerConfig.load = function(self, slot)
	local tmp = force_table_elements_to_match_type
	force_table_elements_to_match_type = function()
	end
	local x = create_setting("playerConfig", "playerConfig.lua", {}, -1)
	x = x:load(slot)
	local coords = x.GameplayXYCoordinates
	local sizes = x.GameplaySizes
	if sizes and not sizes["4K"] then
		defaultConfig.GameplaySizes["3K"] = sizes
		defaultConfig.GameplaySizes["4K"] = sizes
		defaultConfig.GameplaySizes["5K"] = sizes
		defaultConfig.GameplaySizes["6K"] = sizes
		defaultConfig.GameplaySizes["7K"] = sizes
		defaultConfig.GameplaySizes["8K"] = sizes
		defaultConfig.GameplaySizes["10K"] = sizes
		defaultConfig.GameplaySizes["12K"] = sizes
		defaultConfig.GameplaySizes["16K"] = sizes

	end
	if coords and not coords["4K"] then
		defaultConfig.GameplayXYCoordinates["3K"] = coords
		defaultConfig.GameplayXYCoordinates["4K"] = coords
		defaultConfig.GameplayXYCoordinates["5K"] = coords
		defaultConfig.GameplayXYCoordinates["6K"] = coords
		defaultConfig.GameplayXYCoordinates["7K"] = coords
		defaultConfig.GameplayXYCoordinates["8K"] = coords
		defaultConfig.GameplayXYCoordinates["10K"] = coords
		defaultConfig.GameplayXYCoordinates["12K"] = coords
		defaultConfig.GameplayXYCoordinates["16K"] = coords
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
