--[[
Lines with a single string (e.g. TitleMenu = "ScreenTitleMenu") are referenced
in the metrics as Branch.keyname.
If the line is a function, you'll have to use Branch.keyname() instead.
--]]
-- used for various SMOnline-enabled screens:
function SMOnlineScreen()
	if not IsNetSMOnline() then
		return "ScreenSelectMusic"
	end
	for pn in ivalues(GAMESTATE:GetHumanPlayers()) do
		if not IsSMOnlineLoggedIn(pn) then
			return "ScreenSMOnlineLogin"
		end
	end
	if not IsSMOnlineLoggedIn(pn) then
		return "ScreenSMOnlineLogin"
	end
	return "ScreenNetRoom"
end

function SelectMusicOrCourse()
	if IsNetSMOnline() then
		return "ScreenNetSelectMusic"
	else
		return "ScreenSelectMusic"
	end
end

function GameOverOrContinue()
	if THEME:GetMetric("ScreenContinue", "ContinueEnabled") then
		return "ScreenContinue"
	else
		return "ScreenGameOver"
	end
end
Branch = {
	Init = function()
		return Branch.TitleMenu()
	end,
	AfterInit = function()
		if GAMESTATE:GetCoinMode() == "CoinMode_Home" then
			return Branch.TitleMenu()
		else
			return "ScreenLogo"
		end
	end,
	NoiseTrigger = function()
		local hour = Hour()
		return hour > 3 and hour < 6 and "ScreenNoise" or "ScreenHighScores"
	end,
	TitleMenu = function()
		-- home mode is the most assumed use of sm-ssc.
		if GAMESTATE:GetCoinMode() == "CoinMode_Home" then
			return "ScreenTitleMenu"
		end
		-- arcade junk:
		if GAMESTATE:GetCoinsNeededToJoin() > GAMESTATE:GetCoins() then
			-- if no credits are inserted, don't show the Join screen. SM4 has
			-- this as the initial screen, but that means we'd be stuck in a
			-- loop with ScreenInit. No good.
			return "ScreenTitleJoin"
		else
			return "ScreenTitleJoin"
		end
	end,
	AfterTitleMenu = function()
		return Branch.StartGame()
	end,
	StartGame = function()
		multiplayer = false
		if SONGMAN:GetNumSongs() == 0 and SONGMAN:GetNumAdditionalSongs() == 0 and #DLMAN:GetDownloads() == 0 then
			return "ScreenCoreBundleSelect"
		end
		if PROFILEMAN:GetNumLocalProfiles() >= 2 then
			return "ScreenSelectProfile"
		else
			return "ScreenProfileLoad"
		end
	end,
	OptionsEdit = function()
		-- Similar to above, don't let anyone in here with 0 songs.
		if SONGMAN:GetNumSongs() == 0 and SONGMAN:GetNumAdditionalSongs() == 0 then
			return "ScreenCoreBundleSelect"
		end
		return "ScreenOptionsEdit"
	end,
	AfterSelectStyle = function()
		if IsNetConnected() then
			ReportStyle()
			GAMESTATE:ApplyGameCommand("playmode,regular")
		end
		return "ScreenProfileLoad"
	end,
	AfterSelectProfile = function()
		return "ScreenSelectMusic"
	end,
	AfterNetSelectProfile = function()
		return SMOnlineScreen()
	end,
	AfterProfileLoad = function()
		return "ScreenSelectMusic"
	end,
	AfterProfileSave = function()
		if GAMESTATE:IsEventMode() then
			return "ScreenSelectMusic"
		elseif STATSMAN:GetCurStageStats():AllFailed() then
			return GameOverOrContinue()
		else
			return "ScreenSelectMusic"
		end
	end,
	AfterNetProfileSave = function()
		if GAMESTATE:IsEventMode() then
			return "ScreenNetSelectMusic"
		elseif STATSMAN:GetCurStageStats():AllFailed() then
			return GameOverOrContinue()
		else
			return "ScreenNetSelectMusic"
		end
	end,
	GetGameInformationScreen = function()
		bTrue = PREFSMAN:GetPreference("ShowInstructions")
		return (bTrue and GoToMusic() or "ScreenGameInformation")
	end,
	AfterSMOLogin = SMOnlineScreen,
	MultiScreen = function()
		if IsNetSMOnline() then
			if not IsSMOnlineLoggedIn(PLAYER_1) then
				return "ScreenNetSelectProfile"
			else
				return "ScreenNetSelectProfile" --return "ScreenNetRoom" 	-- cant do this, we need to select a local profile even 	
			end																-- if logged into smo -mina
		else
			return "ScreenNetworkOptions"
		end
	end,
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
			return "ScreenStageInformation"
		end
	end,
	PlayerOptions = function()
		if SCREENMAN:GetTopScreen():GetGoToOptions() then
			return "ScreenPlayerOptions"
		else
			return "ScreenStageInformation"
		end
	end,
	SongOptions = function()
		if SCREENMAN:GetTopScreen():GetGoToOptions() then
			return "ScreenSongOptions"
		else
			return "ScreenStageInformation"
		end
	end,
	AfterGameplay = function()
		return "ScreenEvaluationNormal"
	end,
	AfterNetGameplay = function()
		return "ScreenNetEvaluation"
	end,
	AfterEvaluation = function()
		local allFailed = STATSMAN:GetCurStageStats():AllFailed()
		local song = GAMESTATE:GetCurrentSong()

		if GAMESTATE:IsEventMode() or stagesLeft >= 1 then
			return "ScreenProfileSave"
		elseif song:IsLong() and maxStages <= 2 and stagesLeft < 1 and allFailed then
			return "ScreenProfileSaveSummary"
		elseif song:IsMarathon() and maxStages <= 3 and stagesLeft < 1 and allFailed then
			return "ScreenProfileSaveSummary"
		elseif maxStages >= 2 and stagesLeft < 1 and allFailed then
			return "ScreenProfileSaveSummary"
		elseif allFailed then
			return "ScreenProfileSaveSummary"
		else
			return "ScreenProfileSave"
		end
	end,
	AfterNetEvaluation = function()
		local allFailed = STATSMAN:GetCurStageStats():AllFailed()
		local song = GAMESTATE:GetCurrentSong()

		if GAMESTATE:IsEventMode() or stagesLeft >= 1 then
			return "ScreenNetProfileSave"
		elseif song:IsLong() and maxStages <= 2 and stagesLeft < 1 and allFailed then
			return "ScreenProfileSaveSummary"
		elseif song:IsMarathon() and maxStages <= 3 and stagesLeft < 1 and allFailed then
			return "ScreenProfileSaveSummary"
		elseif maxStages >= 2 and stagesLeft < 1 and allFailed then
			return "ScreenProfileSaveSummary"
		elseif allFailed then
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
		--		[[ Enable when Finished ]]
		-- 		return GAMESTATE:AnyPlayerHasRankingFeats() and "ScreenNameEntryTraditional" or "ScreenGameOver"
	end,
	AfterContinue = function()
		if GAMESTATE:GetNumPlayersEnabled() == 0 then
			return "ScreenGameOver"
		end

		if STATSMAN:GetStagesPlayed() == 0 then
			return "ScreenSelectStyle"
		end

		return "ScreenProfileLoad"
	end,
	LeavePackDownloader = function()
		if PROFILEMAN:GetProfile(1):GetDisplayName() == "" then	-- this is suuuuper hacky and will mess with people using "" as display names, but they're idiots anyway -mina
			return "ScreenTitleMenu"
		end
		if IsSMOnlineLoggedIn(PLAYER_1) then
			return "ScreenNetSelectMusic"
		end
		return "ScreenSelectMusic"
	end
}
