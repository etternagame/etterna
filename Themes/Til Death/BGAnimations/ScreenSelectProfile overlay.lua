-- should maybe make some of these generic
local function highlight(self)
	if self:IsVisible() then
		self:queuecommand("Highlight")
	end
end


local translated_info = {
	Title = THEME:GetString("ScreenSelectProfile", "Title"),
	SongPlayed = THEME:GetString("ScreenSelectProfile", "SongPlayed"),
	SongsPlayed = THEME:GetString("ScreenSelectProfile", "SongsPlayed"),
	NoProfile = THEME:GetString("GeneralInfo", "NoProfile"),
	PressStart = THEME:GetString("ScreenSelectProfile", "PressStartToJoin")
}

local function GetLocalProfiles()
	local t = {}

	for p = 0, PROFILEMAN:GetNumLocalProfiles() - 1 do
		local profileID = PROFILEMAN:GetLocalProfileIDFromIndex(p)
		local profile = PROFILEMAN:GetLocalProfileFromIndex(p)
		local ProfileCard = Def.ActorFrame {
			Name = p,
			InitCommand = function(self)
				self:SetUpdateFunction(highlight):SetUpdateFunctionInterval(0.025)
			end,
			Def.Quad {
				InitCommand = function(self)
					self:y(-3.25):align(0.5,0.5):zoomto(260, 39.5):ztest(true)
					self:diffusealpha(0)
				end,
				LeftClickMessageCommand = function(self)
					if isOver(self) then
						SCREENMAN:GetTopScreen():SetProfileIndex(PLAYER_1, self:GetParent():GetName() + 1)
						SCREENMAN:GetTopScreen():Finish()
					end
				end,
				HighlightCommand = function(self)
					if isOver(self) then
						self:GetParent():GetChild("PlayerName"):diffusealpha(0.8)
						self:GetParent():GetChild("SongsPlayed"):diffusealpha(0.8)
					else
						self:GetParent():GetChild("PlayerName"):diffusealpha(1)
						self:GetParent():GetChild("SongsPlayed"):diffusealpha(1)
					end
				end,
			},
			LoadFont("Common Large") ..  {
				Name = "PlayerName",
				Text = string.format("%s: %.2f", profile:GetDisplayName(), profile:GetPlayerRating()),
				InitCommand = function(self)
					self:xy(38 / 2, -12):zoom(0.4):ztest(true):maxwidth((260 - 40 - 4) / 0.4)
				end
			},
			LoadFont("Common Normal") ..  {
				Name = "SongsPlayed",
				InitCommand = function(self)
					self:xy(38 / 2, 8):zoom(0.5):vertspacing(-8):ztest(true):maxwidth((260 - 40 - 4 - 60) / 0.5)
				end,
				BeginCommand = function(self)
					local numSongsPlayed = profile:GetNumTotalSongsPlayed()
					self:settextf("%i %s", numSongsPlayed, translated_info["SongPlayed"])
				end
			},
			Def.Sprite {
				InitCommand = function(self)
					self:visible(true):halign(0):xy(-127, -3):ztest(true)
				end,
				BeginCommand = function(self)
					self:queuecommand("ModifyAvatar")
				end,
				ModifyAvatarCommand = function(self)
					self:finishtweening()
					self:Load(getAssetPathFromProfileID("avatar", profileID))
					self:zoomto(36, 36)
				end
			}
		}
		t[#t + 1] = ProfileCard
	end

	return t
end

local function LoadCard(cColor)
	local t = Def.ActorFrame {
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(260 + 4, 230 + 4)
			end,
			OnCommand = function(self)
				self:diffuse(color("1,1,1,1"))
			end
		},
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(260, 230)
			end,
			OnCommand = function(self)
				self:diffusealpha(0.5):diffuse(cColor)
			end
		}
	}
	return t
end
local function LoadPlayerStuff(Player)
	local t = {}

	local pn = (Player == PLAYER_1) and 1

	t[#t + 1] = Def.ActorFrame {
		Name = "JoinFrame",
		LoadCard(Color("Purple")),
		LoadFont("Common Normal") .. {
			Text = translated_info["PressStart"],
			InitCommand = function(self)
				self:shadowlength(1)
			end,
			OnCommand = function(self)
				self:diffuseshift():effectcolor1(Color("White")):effectcolor2(color("0.5,0.5,0.5"))
			end
		}
	}

	t[#t + 1] = Def.ActorFrame {
		Name = "BigFrame",
		LoadCard(Brightness(getMainColor("positive"), 0.3)),
	}
	t[#t + 1] = Def.ActorFrame {
		Name = "SmallFrame",
		InitCommand = function(self)
			self:y(-2)
		end,
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(260, 40 + 2)
			end,
			OnCommand = function(self)
				self:diffusealpha(0.3)
			end
		}
	}

	t[#t + 1] = Def.ActorScroller {
		Name = "Scroller",
		NumItemsToDraw = 6,
		OnCommand = function(self)
			self:y(1):SetFastCatchup(true):SetMask(270, 58):SetSecondsPerItem(0.15)
		end,
		TransformFunction = function(self, offset, itemIndex, numItems)
			local focus = scale(math.abs(offset), 0, 2, 1, 0)
			self:visible(false)
			self:y(math.floor(offset * 40))
		end,
		children = GetLocalProfiles()
	}

	t[#t + 1] = Def.ActorFrame {
		Name = "EffectFrame"
	}
	t[#t + 1] = LoadFont("Common Normal") .. {
		--is there a reason we dont use this just for showing the "no profile" text??
		Name = "SelectedProfileText",
		InitCommand = function(self)
			self:y(160):maxwidth(SCREEN_WIDTH * 0.9):visible(0)
		end
	}

	return t
end

local function UpdateInternal3(self, Player)
	local pn = (Player == PLAYER_1) and 1
	local frame = self:GetChild(string.format("P%uFrame", pn))
	local scroller = frame:GetChild("Scroller")
	local seltext = frame:GetChild("SelectedProfileText")
	local joinframe = frame:GetChild("JoinFrame")
	local smallframe = frame:GetChild("SmallFrame")
	local bigframe = frame:GetChild("BigFrame")

	if GAMESTATE:IsHumanPlayer() then
		frame:visible(true)
			--using profile if any
			joinframe:visible(false)
			smallframe:visible(true)
			bigframe:visible(true)
			seltext:visible(false)--
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
					seltext:visible(true):settext(translated_info["NoProfile"])
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

t[#t + 1] = Def.ActorFrame {
	StorageDevicesChangedMessageCommand = function(self, params)
		self:queuecommand("UpdateInternal2")
	end,
	BeginCommand = function(self)
		ms.ok("BEING BEINGIN EIN GIERN GIERN")
		SCREENMAN:GetTopScreen():AddInputCallback(function(event)
			ms.ok(event)
			if event.type == "InputEventType_FirstPress" then
				if event.button == "Start" then
					MESSAGEMAN:Broadcast("StartButton")
					if not GAMESTATE:IsHumanPlayer() then
						SCREENMAN:GetTopScreen():SetProfileIndex(PLAYER_1, -1)
					else
						SCREENMAN:GetTopScreen():Finish()
					end
				elseif event.button == "MenuUp"  or event.button == "Up" then
					if GAMESTATE:IsHumanPlayer() then
						local ind = SCREENMAN:GetTopScreen():GetProfileIndex(PLAYER_1)
						if ind > 1 then
							if SCREENMAN:GetTopScreen():SetProfileIndex(PLAYER_1, ind - 1) then
								MESSAGEMAN:Broadcast("DirectionButton")
								self:queuecommand("UpdateInternal2")
							end
						end
					end
				elseif event.button == "MenuDown" or event.button == "Down" then
					if GAMESTATE:IsHumanPlayer() then
						local ind = SCREENMAN:GetTopScreen():GetProfileIndex(PLAYER_1)
						if ind > 0 then
							if SCREENMAN:GetTopScreen():SetProfileIndex(PLAYER_1, ind + 1) then
								MESSAGEMAN:Broadcast("DirectionButton")
								self:queuecommand("UpdateInternal2")
							end
						end
					end
				elseif event.button == "Back" then
					SCREENMAN:GetTopScreen():Cancel()
				end
			end
		end)
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
				self:Center()
			end,
			OnCommand = function(self)
				self:zoom(0):bounceend(0.25):zoom(1)
			end,
			OffCommand = function(self)
				self:bouncebegin(0.25):zoom(0)
			end,
			PlayerJoinedMessageCommand = function(self, param)
				if param.Player == PLAYER_1 then
					self:zoom(1.15):bounceend(0.175):zoom(1.0)
				end
			end,
			children = LoadPlayerStuff(PLAYER_1)
		},
		-- sounds
		LoadActor(THEME:GetPathS("Common", "start")) .. {
			StartButtonMessageCommand = function(self)
				self:play()
			end
		},
		LoadActor(THEME:GetPathS("Common", "cancel")) .. {
			BackButtonMessageCommand = function(self)
				self:play()
			end
		},
		LoadActor(THEME:GetPathS("Common", "value")) .. {
			DirectionButtonMessageCommand = function(self)
				self:play()
			end
		}
	}
}
t[#t + 1] = LoadActor("_frame")
t[#t + 1] = LoadFont("Common Large") .. {
	InitCommand = function(self)
		self:xy(5, 32):halign(0):valign(1):zoom(0.55):diffuse(getMainColor("positive"))
		self:settextf("%s:", translated_info["Title"])
	end
}

return t
