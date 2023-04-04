--- Lua helpers for Screen Branching.
-- When defining a Screen class in the metrics.ini, a PrevScreen and NextScreen is required. To easily introduce custom logic that dynamically changes those destination screens, the Branches file exists.
-- @module 02_Branches

--[[
Lines with a single string (e.g. TitleMenu = "ScreenTitleMenu") are referenced
in the metrics as Branch.keyname.
For functions in the Branch table, you'll have to use Branch.keyname() instead.
--]]

function SMOnlineScreen() -- used for various SMOnline-enabled screens:
	if not IsNetSMOnline() then
		return "ScreenSelectMusic"
	end
	if not IsSMOnlineLoggedIn() then
		return "ScreenSMOnlineLogin"
	end
	return "ScreenNetRoom"
end

function ToGameplay()
	local mode = GAMESTATE:GetGameplayMode()
	if mode == "GameplayMode_Practice" then
		return "ScreenGameplayPractice"
	elseif mode == "GameplayMode_Replay" then
		return "ScreenGameplayReplay"
	else
		return "ScreenGameplay"
	end
end

function ToStageInformation()
	if not IsSMOnlineLoggedIn() then
		return "ScreenStageInformation"
	else
		return "ScreenNetStageInformation"
	end
end

Branch = {
	Init = function()
		return "ScreenInit"
	end,
	AfterInit = function()
		return Branch.TitleMenu()
	end,
	TitleMenu = function()
		return "ScreenTitleMenu"
	end,
	AfterTitleMenu = function()
		return Branch.StartGame()
	end,
	StartGame = function()
		if PROFILEMAN:GetNumLocalProfiles() >= 2 then
			return "ScreenSelectProfile"
		else
			if THEME:GetMetric("Common", "AutoSetStyle") == false then
				return "ScreenSelectStyle"
			else
				return "ScreenProfileLoad"
			end
		end
	end,
	MultiScreen = function()
		if IsNetSMOnline() then
			if not IsSMOnlineLoggedIn() then
				return "ScreenNetSelectProfile"
			else
				return "ScreenNetRoom"
			end
		else
			return "ScreenNetworkOptions"
		end
	end,
	OptionsEdit = function()
		return "ScreenOptionsEdit"
	end,
	AfterSelectStyle = function()
		if IsNetConnected() then
			ReportStyle()
		end
		return "ScreenProfileLoad"
	end,
	AfterSelectProfile = function()
		if (THEME:GetMetric("Common", "AutoSetStyle") == true) then
			return "ScreenSelectMusic"
		else
			return "ScreenSelectStyle"
		end
	end,
	AfterNetSelectProfile = function()
		return SMOnlineScreen()
	end,
	AfterProfileSave = function()
		return "ScreenSelectMusic"
	end,
	AfterProfileLoad = function()
		return "ScreenSelectMusic"
	end,
	AfterNetProfileSave = function()
		return "ScreenNetSelectMusic"
	end,
	AfterSMOLogin = SMOnlineScreen,
	BackOutOfPlayerOptions = function()
		return "ScreenSelectMusic"
	end,
	BackOutOfNetPlayerOptions = function()
		return "ScreenNetSelectMusic"
	end,
	BackOutOfStageInformation = function()
		return "ScreenSelectMusic"
	end,
	BackOutOfNetStageInformation = function()
		return "ScreenNetSelectMusic"
	end,
	AfterSelectMusic = function()
		if SCREENMAN:GetTopScreen():GetGoToOptions() then
			return SelectFirstOptionsScreen()
		else
			return ToStageInformation()
		end
	end,
	PlayerOptions = function()
		if SCREENMAN:GetTopScreen():GetGoToOptions() then
			return "ScreenPlayerOptions"
		else
			return ToStageInformation()
		end
	end,
	SongOptions = function()
		if SCREENMAN:GetTopScreen():GetGoToOptions() then
			return "ScreenSongOptions"
		else
			return ToStageInformation()
		end
	end,
	AfterGameplay = function()
		return "ScreenEvaluationNormal"
	end,
	AfterNetGameplay = function()
		return "ScreenNetEvaluation"
	end,
	AfterEvaluation = function()
		local Failed = STATSMAN:GetCurStageStats():Failed()
		local song = GAMESTATE:GetCurrentSong()

		if not SCREENMAN:GetTopScreen():GetStageStats():GetLivePlay() then
			return "ScreenSelectMusic"
		else
			return "ScreenProfileSave"
		end
	end,
	AfterNetEvaluation = function()
		return "ScreenNetProfileSave"
	end,
	Network = function()
		return IsNetConnected() and Branch.MultiScreen() or "ScreenTitleMenu"
	end,
	BackOutOfNetwork = function()
		return "ScreenTitleMenu"
	end,
	AfterContinue = function()

		if STATSMAN:GetStagesPlayed() == 0 then
			return "ScreenSelectStyle"
		end

		return "ScreenProfileLoad"
	end
}
