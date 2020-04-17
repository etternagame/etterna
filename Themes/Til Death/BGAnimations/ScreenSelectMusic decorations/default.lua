local t = Def.ActorFrame {}

t[#t + 1] = LoadActor("tabs")
t[#t + 1] = LoadActor("wifetwirl")
t[#t + 1] = LoadActor("msd")
t[#t + 1] = LoadActor("songsearch")
t[#t + 1] = LoadActor("score")
t[#t + 1] = LoadActor("profile")
t[#t + 1] = LoadActor("filter")
t[#t + 1] = LoadActor("goaltracker")
t[#t + 1] = LoadActor("playlists")
t[#t + 1] = LoadActor("downloads")
t[#t + 1] = LoadActor("tags")

local itsOn = false

local stepsdisplayx = SCREEN_WIDTH * 0.56 - capWideScale(48, 56)

t[#t + 1] =
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
	DelayedChartUpdateMessageCommand = function(self)
		local leaderboardEnabled =
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).leaderboardEnabled and DLMAN:IsLoggedIn()
		if leaderboardEnabled and GAMESTATE:GetCurrentSteps(PLAYER_1) then
			local chartkey = GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey()
			if SCREENMAN:GetTopScreen():GetMusicWheel():IsSettled() then
				DLMAN:RequestChartLeaderBoardFromOnline(
					chartkey,
					function(leaderboard)
					end
				)
			end
		end
	end,
	ChartPreviewOnMessageCommand = function(self)
		if not itsOn then
			self:addx(capWideScale(12, 0)):addy(capWideScale(18, 0))
			itsOn = true
		end
	end,
	ChartPreviewOffMessageCommand = function(self)
		if itsOn then
			self:addx(capWideScale(-12, 0)):addy(capWideScale(-18, 0))
			itsOn = false
		end
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

t[#t + 1] = LoadActor("../_mousewheelscroll")
collectgarbage()
return t
