local function selectprofile(self)
	if isOver(self) then
		SCREENMAN:GetTopScreen():SetProfileIndex(PLAYER_1, self:GetParent():GetName() + 1)
		SCREENMAN:GetTopScreen():Finish()
	end
end
local function genericHighlight(self, highlight, base, clickaction)
	local highlight = highlight or 0.6
	local base = base or 1
	self:SetUpdateFunction(function(self)
		if self:IsVisible() then
			self:RunCommandsOnChildren(
				function(self)
					if isOver(self) then
						self:diffusealpha(highlight)
					else
						self:diffusealpha(base)
					end
				end
				)
			end
		end
	)
	self:SetUpdateFunctionInterval(0.025)
	if clickaction then
		self:RunCommandsOnChildren(
			function(self) 
				self:addcommand("LeftClickMessage", clickaction)
			end
		)
	end
end

local translated_info = {
	Title = THEME:GetString("ScreenSelectProfile", "Title"),
	SongPlayed = THEME:GetString("ScreenSelectProfile", "SongPlayed"),
	SongsPlayed = THEME:GetString("ScreenSelectProfile", "SongsPlayed"),
	NoProfile = THEME:GetString("GeneralInfo", "NoProfile"),
	PressStart = THEME:GetString("ScreenSelectProfile", "PressStartToJoin")
}

function GetLocalProfiles()
	local t = {}

	for p = 0, PROFILEMAN:GetNumLocalProfiles() - 1 do
		local profileID = PROFILEMAN:GetLocalProfileIDFromIndex(p)
		local profile = PROFILEMAN:GetLocalProfileFromIndex(p)
		local ProfileCard =
			Def.ActorFrame {
				Name = p,
				InitCommand = function(self) 
					genericHighlight(self, 0.75, 1, selectprofile)
				end,
			LoadFont("Common Large") ..
				{
					Text = string.format("%s: %.2f", profile:GetDisplayName(), profile:GetPlayerRating()),
					InitCommand = function(self)
						self:xy(34 / 2, -10):zoom(0.4):ztest(true, maxwidth, (200 - 34 - 4) / 0.4)	
					end
				},
			LoadFont("Common Normal") ..
				{
					InitCommand = function(self)
						self:xy(34 / 2, 8):zoom(0.5):vertspacing(-8):ztest(true):maxwidth((200 - 34 - 4) / 0.5)
					end,
					BeginCommand = function(self)
						local numSongsPlayed = profile:GetNumTotalSongsPlayed()
						local s = numSongsPlayed == 1 and translated_info["SongPlayed"] or translated_info["SongsPlayed"]
						-- todo: localize
						self:settext(numSongsPlayed .. " " .. s)
					end
				},
			Def.Sprite {
				InitCommand = function(self)
					self:visible(true):halign(0):xy(-98, -2):ztest(true)
				end,
				BeginCommand = function(self)
					self:queuecommand("ModifyAvatar")
				end,
				ModifyAvatarCommand = function(self)
					self:finishtweening()
					self:Load(getAssetPathFromProfileID("avatar", profileID))
					self:zoomto(30, 30)
				end
			}
		}
		t[#t + 1] = ProfileCard
	end

	return t
end

function LoadCard(cColor)
	local t =
		Def.ActorFrame {
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(200 + 4, 230 + 4)
			end,
			OnCommand = function(self)
				self:diffuse(color("1,1,1,1"))
			end
		},
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(200, 230)
			end,
			OnCommand = function(self)
				self:diffusealpha(0.5):diffuse(cColor)
			end
		}
	}
	return t
end
function LoadPlayerStuff(Player)
	local t = {}

	local pn = (Player == PLAYER_1) and 1

	t[#t + 1] =
		Def.ActorFrame {
		Name = "JoinFrame",
		LoadCard(Color("Purple")),
		LoadFont("Common Normal") ..
			{
				Text = translated_info["PressStart"],
				InitCommand = function(self)
					self:shadowlength(1)
				end,
				OnCommand = function(self)
					self:diffuseshift():effectcolor1(Color("White")):effectcolor2(color("0.5,0.5,0.5"))
				end
			}
	}

	t[#t + 1] =
		Def.ActorFrame {
		Name = "BigFrame",
		LoadCard(PlayerColor(Player))
	}
	t[#t + 1] =
		Def.ActorFrame {
		Name = "SmallFrame",
		InitCommand = function(self)
			self:y(-2)
		end,
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(200, 40 + 2)
			end,
			OnCommand = function(self)
				self:diffusealpha(0.3)
			end
		}
	}

	t[#t + 1] =
		Def.ActorScroller {
		Name = "Scroller",
		NumItemsToDraw = 6,
		OnCommand = function(self)
			self:y(1):SetFastCatchup(true):SetMask(200, 58):SetSecondsPerItem(0.15)
		end,
		TransformFunction = function(self, offset, itemIndex, numItems)
			local focus = scale(math.abs(offset), 0, 2, 1, 0)
			self:visible(false)
			self:y(math.floor(offset * 40))
		end,
		children = GetLocalProfiles()
	}

	t[#t + 1] =
		Def.ActorFrame {
		Name = "EffectFrame"
	}
	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			Name = "SelectedProfileText",
			InitCommand = function(self)
				self:y(160):maxwidth(SCREEN_WIDTH * 0.9)
			end
		}

	return t
end

function UpdateInternal3(self, Player)
	local pn = (Player == PLAYER_1) and 1
	local frame = self:GetChild(string.format("P%uFrame", pn))
	local scroller = frame:GetChild("Scroller")
	local seltext = frame:GetChild("SelectedProfileText")
	local joinframe = frame:GetChild("JoinFrame")
	local smallframe = frame:GetChild("SmallFrame")
	local bigframe = frame:GetChild("BigFrame")

	if GAMESTATE:IsHumanPlayer(Player) then
		frame:visible(true)
			--using profile if any
			joinframe:visible(false)
			smallframe:visible(true)
			bigframe:visible(true)
			seltext:visible(true)
			scroller:visible(true)
			local ind = SCREENMAN:GetTopScreen():GetProfileIndex(Player)
			if ind > 0 then
				scroller:SetDestinationItem(ind - 1)
				seltext:settext(PROFILEMAN:GetLocalProfileFromIndex(ind - 1):GetDisplayName())
			else
				if SCREENMAN:GetTopScreen():SetProfileIndex(Player, 1) then
					scroller:SetDestinationItem(0)
					self:queuecommand("UpdateInternal2")
				else
					joinframe:visible(true)
					smallframe:visible(false)
					bigframe:visible(false)
					scroller:visible(false)
					seltext:settext(translated_info["NoProfile"])
				end
			end
	else
		joinframe:visible(true)
		scroller:visible(false)
		seltext:visible(false)
		smallframe:visible(false)
		bigframe:visible(false)
	end
end

local t = Def.ActorFrame {}

t[#t + 1] =
	Def.ActorFrame {
	StorageDevicesChangedMessageCommand = function(self, params)
		self:queuecommand("UpdateInternal2")
	end,
	CodeMessageCommand = function(self, params)
		if params.Name == "Start" or params.Name == "Center" then
			MESSAGEMAN:Broadcast("StartButton")
			if not GAMESTATE:IsHumanPlayer(params.PlayerNumber) then
				SCREENMAN:GetTopScreen():SetProfileIndex(params.PlayerNumber, -1)
			else
				SCREENMAN:GetTopScreen():Finish()
			end
		end
		if params.Name == "Up" or params.Name == "Up2" or params.Name == "DownLeft" then
			if GAMESTATE:IsHumanPlayer(params.PlayerNumber) then
				local ind = SCREENMAN:GetTopScreen():GetProfileIndex(params.PlayerNumber)
				if ind > 1 then
					if SCREENMAN:GetTopScreen():SetProfileIndex(params.PlayerNumber, ind - 1) then
						MESSAGEMAN:Broadcast("DirectionButton")
						self:queuecommand("UpdateInternal2")
					end
				end
			end
		end
		if params.Name == "Down" or params.Name == "Down2" or params.Name == "DownRight" then
			if GAMESTATE:IsHumanPlayer(params.PlayerNumber) then
				local ind = SCREENMAN:GetTopScreen():GetProfileIndex(params.PlayerNumber)
				if ind > 0 then
					if SCREENMAN:GetTopScreen():SetProfileIndex(params.PlayerNumber, ind + 1) then
						MESSAGEMAN:Broadcast("DirectionButton")
						self:queuecommand("UpdateInternal2")
					end
				end
			end
		end
		if params.Name == "Back" then
			SCREENMAN:GetTopScreen():Cancel()
		end
	end,
	PlayerJoinedMessageCommand = function(self, params)
		self:queuecommand("UpdateInternal2")
	end,
	PlayerUnjoinedMessageCommand = function(self, params)
		self:queuecommand("UpdateInternal2")
	end,
	OnCommand = function(self, params)
		self:queuecommand("UpdateInternal2")
	end,
	UpdateInternal2Command = function(self)
		UpdateInternal3(self, PLAYER_1)
	end,
	children = {
		Def.ActorFrame {
			Name = "P1Frame",
			InitCommand = function(self)
				self:x(SCREEN_CENTER_X):y(SCREEN_CENTER_Y)
			end,
			OnCommand = function(self)
				self:zoom(0):bounceend(0.35):zoom(1)
			end,
			OffCommand = function(self)
				self:bouncebegin(0.35):zoom(0)
			end,
			PlayerJoinedMessageCommand = function(self, param)
				if param.Player == PLAYER_1 then
					self:zoom(1.15):bounceend(0.175):zoom(1.0)
				end
			end,
			children = LoadPlayerStuff(PLAYER_1)
		},
		-- sounds
		LoadActor(THEME:GetPathS("Common", "start")) ..
			{
				StartButtonMessageCommand = function(self)
					self:play()
				end
			},
		LoadActor(THEME:GetPathS("Common", "cancel")) ..
			{
				BackButtonMessageCommand = function(self)
					self:play()
				end
			},
		LoadActor(THEME:GetPathS("Common", "value")) ..
			{
				DirectionButtonMessageCommand = function(self)
					self:play()
				end
			}
	}
}
t[#t + 1] = LoadActor("_frame")
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(5, 32):halign(0):valign(1):zoom(0.55):diffuse(getMainColor("positive"))
			self:settextf("%s:", translated_info["Title"])
		end
	}

return t
