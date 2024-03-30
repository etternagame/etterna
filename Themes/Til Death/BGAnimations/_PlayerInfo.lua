-- Old avatar actor frame.. renamed since much more will be placed here (hopefully?)
local t =
	Def.ActorFrame {
	Name = "PlayerAvatar"
}

local profile

local profileName = THEME:GetString("GeneralInfo", "NoProfile")
local playCount = 0
local playTime = 0
local noteCount = 0
local numfaves = 0
local AvatarX = 0
local AvatarY = SCREEN_HEIGHT - 50
local playerRating = 0
local uploadbarwidth = 100
local uploadbarheight = 10

local ButtonColor = getMainColor("positive")
local nonButtonColor = ColorMultiplier(getMainColor("positive"), 1.25)
--------UNCOMMENT THIS NEXT LINE IF YOU WANT THE OLD LOOK--------
--nonButtonColor = getMainColor("positive")

local setnewdisplayname = function(answer)
	if answer ~= "" then
		profile:RenameProfile(answer)
		profileName = answer
		MESSAGEMAN:Broadcast("ProfileRenamed", {doot = answer})
	end
end

local function highlightIfOver(self)
	if isOver(self) then
		local topname = SCREENMAN:GetTopScreen():GetName()
		if topname ~= "ScreenEvaluationNormal" and topname ~= "ScreenNetEvaluation" then
			self:diffusealpha(0.6)
		end
	else
		self:diffusealpha(1)
	end
end

local translated_info = {
	ProfileNew = THEME:GetString("ProfileChanges", "ProfileNew"),
	NameChange = THEME:GetString("ProfileChanges", "ProfileNameChange"),
	ClickLogin = THEME:GetString("GeneralInfo", "ClickToLogin"),
	ClickLogout = THEME:GetString("GeneralInfo", "ClickToLogout"),
	NotLoggedIn = THEME:GetString("GeneralInfo", "NotLoggedIn"),
	LoggedInAs = THEME:GetString("GeneralInfo", "LoggedInAs.."),
	LoginFailed = THEME:GetString("GeneralInfo", "LoginFailed"),
	LoginSuccess = THEME:GetString("GeneralInfo", "LoginSuccess"),
	LoginCanceled = THEME:GetString("GeneralInfo", "LoginCanceled"),
	Password = THEME:GetString("GeneralInfo","Password"),
	Username = THEME:GetString("GeneralInfo","Username"),
	Plays = THEME:GetString("GeneralInfo", "ProfilePlays"),
	TapsHit = THEME:GetString("GeneralInfo", "ProfileTapsHit"),
	Playtime = THEME:GetString("GeneralInfo", "ProfilePlaytime"),
	Judge = THEME:GetString("GeneralInfo", "ProfileJudge"),
	RefreshSongs = THEME:GetString("GeneralInfo", "DifferentialReloadTrigger"),
	SongsLoaded = THEME:GetString("GeneralInfo", "ProfileSongsLoaded"),
	SessionTime = THEME:GetString("GeneralInfo", "SessionTime")
}

local function UpdateTime(self)
	local year = Year()
	local month = MonthOfYear() + 1
	local day = DayOfMonth()
	local hour = Hour()
	local minute = Minute()
	local second = Second()
	self:GetChild("CurrentTime"):settextf("%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second)

	local sessiontime = GAMESTATE:GetSessionTime()
	self:GetChild("SessionTime"):settextf("%s: %s", translated_info["SessionTime"], SecondsToHHMMSS(sessiontime))
	self:diffuse(nonButtonColor)
end

-- handle logging in
local function loginStep1(self)
	local redir = SCREENMAN:get_input_redirected(PLAYER_1)
	local function off()
		if redir then
			SCREENMAN:set_input_redirected(PLAYER_1, false)
		end
	end
	local function on()
		if redir then
			SCREENMAN:set_input_redirected(PLAYER_1, true)
		end
	end
	off()

	username = ""

	-- this sets up 2 text entry windows to pull your username and pass
	-- if you press escape or just enter nothing, you are forced out
	-- input redirects are controlled here because we want to be careful not to break any prior redirects
	easyInputStringOKCancel(
		translated_info["Username"]..":", 255, false,
		function(answer)
			username = answer
			-- moving on to step 2 if the answer isnt blank
			if answer:gsub("^%s*(.-)%s*$", "%1") ~= "" then
				self:sleep(0.04):queuecommand("LoginStep2")
			else
				ms.ok(translated_info["LoginCanceled"])
				on()
			end
		end,
		function()
			ms.ok(translated_info["LoginCanceled"])
			on()
		end
	)
end

-- do not use this function outside of first calling loginStep1
local function loginStep2()
	local redir = SCREENMAN:get_input_redirected(PLAYER_1)
	local function off()
		if redir then
			SCREENMAN:set_input_redirected(PLAYER_1, false)
		end
	end
	local function on()
		if redir then
			SCREENMAN:set_input_redirected(PLAYER_1, true)
		end
	end
	-- try to keep the scope of password here
	-- if we open up the scope, if a lua error happens on this screen
	-- the password may show up in the error message
	local password = ""
	easyInputStringOKCancel(
		translated_info["Password"]..":", 255, true,
		function(answer)
			password = answer
			-- log in if not blank
			if answer:gsub("^%s*(.-)%s*$", "%1") ~= "" then
				DLMAN:Login(username, password)
			else
				ms.ok(translated_info["LoginCanceled"])
			end
			on()
		end,
		function()
			ms.ok(translated_info["LoginCanceled"])
			on()
		end
	)
end



t[#t + 1] = Def.Actor {
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	SetCommand = function(self)
		profile = GetPlayerOrMachineProfile(PLAYER_1)
		profileName = profile:GetDisplayName()
		playCount = SCOREMAN:GetTotalNumberOfScores()
		playTime = profile:GetTotalSessionSeconds()
		noteCount = profile:GetTotalTapsAndHolds()
		playerRating = profile:GetPlayerRating()
	end,
	PlayerRatingUpdatedMessageCommand = function(self)
		playerRating = profile:GetPlayerRating()
		self:GetParent():GetChild("AvatarPlayerNumber_P1"):GetChild("Name"):playcommand("Set")
	end
}

t[#t + 1] = Def.ActorFrame {
	Name = "Avatar" .. PLAYER_1,
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	SetCommand = function(self)
		if profile == nil then
			self:visible(false)
		else
			self:visible(true)
		end
	end,
	UIElements.SpriteButton(1, 1, nil) .. {
		Name = "Image",
		InitCommand = function(self)
			self:visible(true):halign(0):valign(0):xy(AvatarX, AvatarY)
		end,
		BeginCommand = function(self)
			self:queuecommand("ModifyAvatar")
		end,
		ModifyAvatarCommand = function(self)
			self:finishtweening()
			self:Load(getAvatarPath(PLAYER_1))
			self:zoomto(50, 50)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and not SCREENMAN:get_input_redirected(PLAYER_1) then
				local top = SCREENMAN:GetTopScreen()
				SCREENMAN:SetNewScreen("ScreenAssetSettings")
			end
		end
	},
	UIElements.TextToolTip(1, 1, "Common Normal") .. {
		Name = "Name",
		InitCommand = function(self)
			self:halign(0)
			self:xy(AvatarX + 54, AvatarY + 8)
			self:zoom(0.55)
			self:maxwidth(capWideScale(360,500))
			self:maxheight(22)
			self:diffuse(ButtonColor)
		end,
		SetCommand = function(self)
			self:settextf("%s: %5.2f", profileName, playerRating)
			if profileName == "Default Profile" or profileName == "" then
				easyInputStringWithFunction(
					translated_info["ProfileNew"],
					64,
					false,
					setnewdisplayname
				)
			end
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and not SCREENMAN:get_input_redirected(PLAYER_1) then
				easyInputStringWithFunction(translated_info["NameChange"], 64, false, setnewdisplayname)
			end
		end,
		ProfileRenamedMessageCommand = function(self, params)
			self:settextf("%s: %5.2f", params.doot, playerRating)
		end,
		MouseOverCommand = function(self)
			highlightIfOver(self)
		end,
		MouseOutCommand = function(self)
			highlightIfOver(self)
		end,
	},
	UIElements.TextToolTip(1, 1, "Common Normal") .. {
		Name = "loginlogout",
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, AvatarY + 8):halign(0.5):zoom(0.45):diffuse(ButtonColor)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if DLMAN:IsLoggedIn() then
				self:queuecommand("Login")
			else
				self:queuecommand("LogOut")
			end
		end,
		LogOutMessageCommand = function(self)
			local top = SCREENMAN:GetTopScreen():GetName()
			if DLMAN:IsLoggedIn() then
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).UserName = ""
				playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).PasswordToken = ""
				playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
				playerConfig:save(pn_to_profile_slot(PLAYER_1))
				DLMAN:Logout()
			end
			if top == "ScreenSelectMusic" or top == "ScreenTextEntry" then
				self:settext(translated_info["ClickLogin"])
			else
				self:settext(translated_info["NotLoggedIn"])
			end
		end,
		LoginMessageCommand = function(self)
			if not SCREENMAN:GetTopScreen() then return end -- ?????
			local top = SCREENMAN:GetTopScreen():GetName()
			if not DLMAN:IsLoggedIn() then return end
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).UserName = DLMAN:GetUsername()
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).PasswordToken = DLMAN:GetToken()
			playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
			playerConfig:save(pn_to_profile_slot(PLAYER_1))
			if top == "ScreenSelectMusic" or top == "ScreenTextEntry" then
				self:settext(translated_info["ClickLogout"])
			else
				self:settext("")
			end
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and not SCREENMAN:get_input_redirected(PLAYER_1) then
				if DLMAN:IsLoggedIn() then
					self:queuecommand("LogOut")
				else
					loginStep1(self)
				end
			end
		end,
		OnlineUpdateMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		MouseOverCommand = function(self)
			highlightIfOver(self)
		end,
		MouseOutCommand = function(self)
			highlightIfOver(self)
		end,
		LoginFailedMessageCommand = function(self)
			ms.ok(translated_info["LoginFailed"])
		end,
		LoginHotkeyPressedMessageCommand = function(self)
			if DLMAN:IsLoggedIn() then
				self:queuecommand("LogOut")
			else
				loginStep1(self)
			end
		end,
		LoginStep2Command = function(self)
			loginStep2()
		end
	},
	UIElements.TextToolTip(1, 1, "Common Normal") .. {
		Name = "LoggedInAs",
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, AvatarY + 23.5):halign(0.5):zoom(0.45):diffuse(ButtonColor)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if DLMAN:IsLoggedIn() then
				self:queuecommand("Login")
			else
				self:queuecommand("LogOut")
			end
		end,
		LogOutMessageCommand = function(self)
			self:settext("")
		end,
		LoginMessageCommand = function(self) --this seems a little clunky -mina
			if not DLMAN:IsLoggedIn() then
				self:halign(0.5)
				return
			end
			self:settextf(
				"%s %s (%5.2f: #%i)",
				translated_info["LoggedInAs"],
				DLMAN:GetUsername(),
				DLMAN:GetSkillsetRating("Overall"),
				DLMAN:GetSkillsetRank(ms.SkillSets[1])
			)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				local userpage = "urlnoexit,https://etternaonline.com/user/" .. DLMAN:GetUsername()
				GAMESTATE:ApplyGameCommand(userpage)
			end
		end,
		OnlineUpdateMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		MouseOverCommand = function(self)
			highlightIfOver(self)
		end,
		MouseOutCommand = function(self)
			highlightIfOver(self)
		end,
	},
	LoadFont("Common Normal") .. {
		InitCommand = function(self)
			self:xy(AvatarX + 54, AvatarY + 21):halign(0):zoom(0.35):diffuse(nonButtonColor)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			self:settextf("%s %s", playCount, translated_info["Plays"])
		end
	},
	LoadFont("Common Normal") .. {
		InitCommand = function(self)
			self:xy(AvatarX + 54, AvatarY + 31.5):halign(0):zoom(0.35):diffuse(nonButtonColor)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local time = SecondsToHHMMSS(playTime)
			self:settextf("%s %s", time, translated_info["Playtime"])
		end
	},
	LoadFont("Common Normal") .. {
		InitCommand = function(self)
			self:xy(AvatarX + 54, AvatarY + 42):halign(0):zoom(0.35):diffuse(nonButtonColor)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			self:settextf("%s %s", noteCount, translated_info["TapsHit"])
		end
	},
	UIElements.TextToolTip(1, 1, "Common Normal") .. {
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X - capWideScale(125,175), AvatarY + 41):halign(0.5):zoom(0.4):diffuse(ButtonColor)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		OptionsScreenClosedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local online = IsNetSMOnline() and IsSMOnlineLoggedIn() and NSMAN:IsETTP()
			self:y(AvatarY + 41 - (online and 18 or 0))
			self:settextf("%s: %s", translated_info["Judge"], GetTimingDifficulty())
		end,
		MouseOverCommand = function(self)
			highlightIfOver(self)
		end,
		MouseOutCommand = function(self)
			highlightIfOver(self)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				-- raise judge
				local cur_judge = GetTimingDifficulty()
				if (cur_judge < 9) then
					local scale = ms.JudgeScalers[cur_judge+1]
					SetTimingDifficulty(scale)
					self:queuecommand("Set")
				end
			end
			if params.event == "DeviceButton_right mouse button" then
				-- lower judge
				local cur_judge = GetTimingDifficulty()
				if (cur_judge > 4) then
					local scale = ms.JudgeScalers[cur_judge-1]
					SetTimingDifficulty(scale)
					self:queuecommand("Set")
				end
			end
		end
	},
	UIElements.TextToolTip(1, 1, "Common Normal") .. {
		Name = "Version",
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH - 3, AvatarY + 8):halign(1):zoom(0.42):diffuse(ButtonColor)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			self:settext(GAMESTATE:GetEtternaVersion())
		end,
		MouseOverCommand = function(self)
			highlightIfOver(self)
		end,
		MouseOutCommand = function(self)
			highlightIfOver(self)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				local tag = "urlnoexit,https://github.com/etternagame/etterna/releases/tag/v" .. GAMESTATE:GetEtternaVersion()
				GAMESTATE:ApplyGameCommand(tag)
			end
		end
	},
	UIElements.TextToolTip(1, 1, "Common Normal") .. {
		Name = "refreshbutton",
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH - 3, AvatarY + 19):halign(1):zoom(0.35):diffuse(ButtonColor)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			self:settextf(translated_info["RefreshSongs"])
		end,
		MouseOverCommand = function(self)
			highlightIfOver(self)
		end,
		MouseOutCommand = function(self)
			highlightIfOver(self)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				SONGMAN:DifferentialReload()
			end
		end
	},
	LoadFont("Common Normal") .. {
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH - 3, AvatarY + 30):halign(1):zoom(0.35):diffuse(nonButtonColor)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			self:settextf("%s: %i", translated_info["SongsLoaded"], SONGMAN:GetNumSongs())
		end,
		DFRFinishedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	},
	-- ok coulda done this as a separate object to avoid copy paste but w.e
	-- upload progress bar bg
	Def.Quad {
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH * 2/3, AvatarY + 41):zoomto(uploadbarwidth, uploadbarheight)
			self:diffuse(color("#111111")):diffusealpha(0):halign(0)
		end,
		UploadProgressMessageCommand = function(self, params)
			self:diffusealpha(1)
			if params.percent == 1 then
				self:diffusealpha(0)
			end
		end
	},
	-- fill bar
	Def.Quad {
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH * 2/3, AvatarY + 41):zoomto(0, uploadbarheight)
			self:diffuse(color("#AAAAAAA")):diffusealpha(0):halign(0)
		end,
		UploadProgressMessageCommand = function(self, params)
			self:diffusealpha(1)
			self:zoomto(params.percent * uploadbarwidth, uploadbarheight)
			if params.percent == 1 then
				self:diffusealpha(0)
			end
		end
		},
	-- super required explanatory text
	LoadFont("Common Normal") .. {
	    InitCommand = function(self)
			self:xy(SCREEN_WIDTH * 2/3, AvatarY + 27):halign(0):valign(0)
			self:diffuse(nonButtonColor):diffusealpha(0):zoom(0.35)
        	self:settext("Uploading Scores...")
		end,
		UploadProgressMessageCommand = function(self, params)
			self:diffusealpha(1)
			if params.percent == 1 then
				self:diffusealpha(0)
			end
		end
	}
}

t[#t + 1] = Def.ActorFrame {
	InitCommand = function(self)
		self:SetUpdateFunction(UpdateTime)
	end,
	LoadFont("Common Normal") .. {
		Name = "CurrentTime",
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH - 3, SCREEN_BOTTOM - 3.5):halign(1):valign(1):zoom(0.45)
		end
	},

	LoadFont("Common Normal") .. {
		Name = "SessionTime",
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, SCREEN_BOTTOM - 5):halign(0.5):valign(1):zoom(0.45)
		end
	}
}

local function UpdateAvatar(self)
	if getAvatarUpdateStatus() then
		self:GetChild("Avatar" .. PLAYER_1):GetChild("Image"):queuecommand("ModifyAvatar")
		setAvatarUpdateStatus(PLAYER_1, false)
	end
end
t.InitCommand = function(self)
	self:SetUpdateFunction(UpdateAvatar)
end

return t
