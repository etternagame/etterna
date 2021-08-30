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

function GameOverOrContinue()
	if THEME:GetMetric("ScreenContinue", "ContinueEnabled") then
		return "ScreenContinue"
	else
		return "ScreenGameOver"
	end
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
		if GAMESTATE:GetCoinMode() == "CoinMode_Home" then
			return Branch.TitleMenu()
		else
			return "ScreenLogo"
		end
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
		-- Might be a little too broken? -- Midiman
		if GAMESTATE:IsEventMode() then
			return "ScreenSelectMusic"
		elseif STATSMAN:GetCurStageStats():Failed() then
			return GameOverOrContinue()
		else
			return "ScreenSelectMusic"
		end
	end,
	AfterProfileLoad = function()
		return "ScreenSelectMusic"
	end,
	AfterNetProfileSave = function()
		-- Might be a little too broken? -- Midiman
		if GAMESTATE:IsEventMode() then
			return "ScreenNetSelectMusic"
		elseif STATSMAN:GetCurStageStats():Failed() then
			return GameOverOrContinue()
		else
			return "ScreenNetSelectMusic"
		end
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
		elseif GAMESTATE:IsEventMode() or stagesLeft >= 1 then
			return "ScreenProfileSave"
		elseif song:IsLong() and maxStages <= 2 and stagesLeft < 1 and Failed then
			return "ScreenProfileSaveSummary"
		elseif song:IsMarathon() and maxStages <= 3 and stagesLeft < 1 and Failed then
			return "ScreenProfileSaveSummary"
		elseif maxStages >= 2 and stagesLeft < 1 and Failed then
			return "ScreenProfileSaveSummary"
		elseif Failed then
			return "ScreenProfileSaveSummary"
		else
			return "ScreenProfileSave"
		end
	end,
	AfterNetEvaluation = function()
		local Failed = STATSMAN:GetCurStageStats():Failed()
		local song = GAMESTATE:GetCurrentSong()

		if GAMESTATE:IsEventMode() or stagesLeft >= 1 then
			return "ScreenNetProfileSave"
		elseif song:IsLong() and maxStages <= 2 and stagesLeft < 1 and Failed then
			return "ScreenProfileSaveSummary"
		elseif song:IsMarathon() and maxStages <= 3 and stagesLeft < 1 and Failed then
			return "ScreenProfileSaveSummary"
		elseif maxStages >= 2 and stagesLeft < 1 and Failed then
			return "ScreenProfileSaveSummary"
		elseif Failed then
			return "ScreenProfileSaveSummary"
		else
			return "ScreenNetProfileSave"
		end
	end,
	AfterSummary = function()
		return "ScreenProfileSaveSummary"
	end,
	Network = function()
		return IsNetConnected() and Branch.MultiScreen() or "ScreenTitleMenu"
	end,
	BackOutOfNetwork = function()
		return "ScreenTitleMenu"
	end,
	AfterSaveSummary = function()
		return GameOverOrContinue()
	end,
	AfterContinue = function()
		if GAMESTATE:GetNumPlayersEnabled() == 0 then
			return "ScreenGameOver"
		end

		if STATSMAN:GetStagesPlayed() == 0 then
			return "ScreenSelectStyle"
		end

		return "ScreenProfileLoad"
	end
}
