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
			ProgressBarPos = 1,
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
			ProgressBarPos = 1,
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
			ProgressBarPos = 1,
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
			ProgressBarPos = 1,
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
			ProgressBarPos = 1,
		},
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
		},
	},
}

playerConfig = create_setting("playerConfig", "playerConfig.lua", defaultConfig, -1)
playerConfig:load()

function LoadProfileCustom(profile, dir)
	local players = GAMESTATE:GetEnabledPlayers()
	local playerProfile
	local pn
	for k,v in pairs(players) do
		playerProfile = PROFILEMAN:GetProfile(v)
		if playerProfile:GetGUID() == profile:GetGUID() then
			pn = v
		end;
	end; 

	if pn then
		playerConfig:load(pn_to_profile_slot(pn))
	end
end

function SaveProfileCustom(profile, dir)
	local players = GAMESTATE:GetEnabledPlayers()
	local playerProfile
	local pn
	for k,v in pairs(players) do
		playerProfile = PROFILEMAN:GetProfile(v)
		if playerProfile:GetGUID() == profile:GetGUID() then
			pn = v
		end;
	end; 

	if pn then
		playerConfig:set_dirty(pn_to_profile_slot(pn))
		playerConfig:save(pn_to_profile_slot(pn))
	end
end
