local t = Def.ActorFrame {}
if NSMAN:IsETTP() then
	t[#t + 1] =
		Def.ActorFrame {
		LeftClickMessageCommand = function()
			SCREENMAN:SystemMessage("asdasdasd")
			MESSAGEMAN:Broadcast("MouseLeftClick")
		end
	}
	t[#t + 1] = LoadActor("../ScreenSelectMusic decorations/default")
	return t
end
t[#t + 1] = LoadActor("../_chatbox")
t[#t + 1] = LoadActor("../ScreenSelectMusic decorations/profile")
t[#t + 1] = LoadActor("../ScreenSelectMusic decorations/msd")
t[#t + 1] = LoadActor("../ScreenSelectMusic decorations/songsearch")
t[#t + 1] = LoadActor("tabs")
t[#t + 1] = LoadActor("../ScreenSelectMusic decorations/score")
t[#t + 1] = LoadActor("dumbrate")
t[#t + 1] = LoadActor("../ScreenSelectMusic decorations/filters")

local g =
	Def.ActorFrame {
	TabChangedMessageCommand = function(self)
		local top = SCREENMAN:GetTopScreen()
		if getTabIndex() == 0 then
			self:visible(true)
			top:ChatboxVisible(true)
			top:ChatboxInput(true)
		else
			self:visible(false)
			top:ChatboxVisible(false)
			top:ChatboxInput(false)
		end
	end
}

g[#g + 1] =
	Def.Banner {
	InitCommand = function(self)
		self:x(10):y(60):halign(0):valign(0)
	end,
	SetMessageCommand = function(self)
		local top = SCREENMAN:GetTopScreen()
		if top:GetName() == "ScreenSelectMusic" or top:GetName() == "ScreenNetSelectMusic" then
			local song = GAMESTATE:GetCurrentSong()
			local group = top:GetMusicWheel():GetSelectedSection()
			if song then
				self:LoadFromSong(song)
			elseif group then
				self:LoadFromSongGroup(group)
			end
		end
		self:scaletoclipped(capWideScale(get43size(384), 384), capWideScale(get43size(120), 120))
	end,
	CurrentSongChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	CurrentStepsP1ChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	CurrentStepsP2ChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}
g[#g + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(10, 60 + capWideScale(get43size(120), 120) - capWideScale(get43size(10), 10)):zoomto(
			capWideScale(get43size(384), 384),
			capWideScale(get43size(20), 20)
		):halign(0):diffuse(color("#000000")):diffusealpha(0.7)
	end
}
g[#g + 1] =
	LoadFont("Common Normal") ..
	{
		Name = "songTitle",
		InitCommand = function(self)
			self:xy(15, 60 + capWideScale(get43size(120), 120) - capWideScale(get43size(10), 10)):visible(true):halign(0):zoom(
				capWideScale(get43size(0.45), 0.45)
			):maxwidth(capWideScale(get43size(340), 340) / capWideScale(get43size(0.45), 0.45))
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local song = GAMESTATE:GetCurrentSong()
			if song ~= nil then
				self:settext(song:GetDisplayMainTitle() .. " // " .. song:GetDisplayArtist())
			else
				self:settext("")
			end
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

g[#g + 1] = LoadActor("wifeonline")
g[#g + 1] = LoadActor("onlinebpm")
g[#g + 1] = LoadActor("radaronline")

g[#g + 1] =
	Def.ActorFrame {
	Name = "StepsDisplay",
	InitCommand = function(self)
		self:xy(stepsdisplayx, 70):valign(0)
	end,
	OffCommand = function(self)
		self:visible(false)
	end,
	OnCommand = function(self)
		self:visible(true)
	end,
	TabChangedMessageCommand = function(self)
		self:finishtweening()
		if getTabIndex() < 3 and GAMESTATE:GetCurrentSong() then
			self:playcommand("On")
		else
			self:playcommand("Off")
		end
	end,
	CurrentSongChangedMessageCommand = function(self)
		local song = GAMESTATE:GetCurrentSong()
		if song and getTabIndex() < 3 then
			self:playcommand("On")
		elseif not song then
			self:playcommand("Off")
		end
	end,
	PlayingSampleMusicMessageCommand = function(self)
		local leaderboardEnabled =
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).leaderboardEnabled and DLMAN:IsLoggedIn()
		if leaderboardEnabled and GAMESTATE:GetCurrentSteps(PLAYER_1) then
			local chartkey = GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey()
			DLMAN:RequestChartLeaderBoardFromOnline(
				chartkey,
				function(leaderboard)
				end
			)
		end
	end,
	ChartPreviewOnMessageCommand = function(self)
		self:addx(capWideScale(12, 0)):addy(capWideScale(18, 0))
	end,
	ChartPreviewOffMessageCommand = function(self)
		self:addx(capWideScale(-12, 0)):addy(capWideScale(-18, 0))
	end,
	Def.StepsDisplayList {
		Name = "StepsDisplayListRow",
		CursorP1 = Def.ActorFrame {
			InitCommand = function(self)
				self:player(PLAYER_1)
			end,
			Def.Quad {
				InitCommand = function(self)
					self:x(54):zoomto(6, 20):halign(1):valign(0.5)
				end,
				BeginCommand = function(self)
					self:queuecommand("Set")
				end,
				SetCommand = function(self)
					self:zoomy(20)
				end
			}
		},
		CursorP2 = Def.ActorFrame {},
		CursorP1Frame = Def.Actor {
			ChangeCommand = function(self)
				self:stoptweening():decelerate(0.05)
			end
		},
		CursorP2Frame = Def.Actor {}
	}
}
t[#t + 1] = g

return t
