function SMOnlineScreen() -- used for various SMOnline-enabled screens:
	if not IsNetSMOnline() then
		return "ScreenSelectMusic"
	end
	if not IsSMOnlineLoggedIn() then
		return "ScreenSMOnlineLogin"
	end
	return "ScreenNetRoom"
end
Branch.StartGame = function()
	multiplayer = false
	if SONGMAN:GetNumSongs() == 0 and SONGMAN:GetNumAdditionalSongs() == 0 and #DLMAN:GetDownloads() == 0 then
		return "ScreenCoreBundleSelect"
	end
	if PROFILEMAN:GetNumLocalProfiles() >= 2 then
		return "ScreenSelectProfile"
	else
		return "ScreenProfileLoad"
	end
end
Branch.MultiScreen = function()
	if IsNetSMOnline() then
		if not IsSMOnlineLoggedIn() then
			return "ScreenNetSelectProfile"
		else
			return "ScreenNetSelectProfile" --return "ScreenNetRoom" 	-- cant do this, we need to select a local profile even
		end																-- if logged into smo -mina
	else
		return "ScreenNetworkOptions"
	end
end
Branch.OptionsEdit = function()
	-- Similar to above, don't let anyone in here with 0 songs.
	if SONGMAN:GetNumSongs() == 0 and SONGMAN:GetNumAdditionalSongs() == 0 then
		return "ScreenCoreBundleSelect"
	end
	return "ScreenOptionsEdit"
end
Branch.AfterSelectStyle = function()
	if IsNetConnected() then
		ReportStyle()
		GAMESTATE:ApplyGameCommand("playmode,regular")
	end
	return "ScreenProfileLoad"
end
Branch.AfterSelectProfile = function()
	return "ScreenSelectMusic"
end

Branch.LeavePackDownloader = function()
	if PROFILEMAN:GetProfile(1):GetDisplayName() == "" then	-- this is suuuuper hacky and will mess with people using "" as display names, but they're idiots anyway -mina
		return "ScreenTitleMenu"
	end
	if IsSMOnlineLoggedIn(PLAYER_1) then
		return "ScreenNetSelectMusic"
	end
	return "ScreenSelectMusic"
end

Branch.LeaveAssets = function()
	if IsSMOnlineLoggedIn(PLAYER_1) then
		if NSMAN:GetCurrentRoomName() then
			return "ScreenNetSelectMusic"
		else
			return "ScreenNetRoom"
		end
	end
	return "ScreenSelectMusic"
end
