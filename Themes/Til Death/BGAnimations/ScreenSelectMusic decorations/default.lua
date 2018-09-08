local t = Def.ActorFrame{}

t[#t+1] = LoadActor("wifetwirl")
t[#t+1] = LoadActor("msd")
t[#t+1] = LoadActor("songsearch")
t[#t+1] = LoadActor("tabs")
t[#t+1] = LoadActor("songinfo")
t[#t+1] = LoadActor("score")
t[#t+1] = LoadActor("profile")
t[#t+1] = LoadActor("filter")
t[#t+1] = LoadActor("goaltracker")
t[#t+1] = LoadActor("playlists")
t[#t+1] = LoadActor("downloads")
t[#t+1] = LoadActor("tags")

t[#t+1] = Def.ActorFrame {
	InitCommand=function(self)
		self:xy(capWideScale(get43size(384),384)+26,70,halign,0):valign(0):zoom(math.min(1,SCREEN_WIDTH/854))
	end,
	OffCommand=function(self)
		self:bouncebegin(0.2):xy(capWideScale(get43size(384),384)+26-500,70) -- visible(false)
	end,
	OnCommand=function(self)
		self:bouncebegin(0.2):xy(capWideScale(get43size(384),384)+26,70)
	end,
	TabChangedMessageCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 0 then
			self:playcommand("On")
		else 
			self:playcommand("Off")
		end
	end,
	CurrentSongChangedMessageCommand=function(self)
		local song = GAMESTATE:GetCurrentSong()
		if song then
			self:finishtweening()
			self:playcommand("TweenOn")
		elseif not song and self:GetZoomX() == 1 then
			self:finishtweening()
			self:playcommand("TweenOff")
		end
	end,
	Def.StepsDisplayList {
		Name="StepsDisplayListRow",
		CursorP1 = Def.ActorFrame {
			InitCommand=function(self)
				self:x(55):player(PLAYER_1)
			end,
			Def.Quad{
				InitCommand=function(self)
					self:zoomto(6,22):halign(1):valign(0.5)
				end,
				BeginCommand=function(self)
					self:queuecommand("Set")
				end,
				SetCommand=function(self)
					if GAMESTATE:GetNumPlayersEnabled()>=2 then
						self:zoomy(11)
						self:valign(1)
					else
						self:zoomy(22)
						self:valign(0.5)
					end
				end,
			},
			LoadFont("Common Normal") .. {
				InitCommand=function(self)
					self:x(-1):halign(1):valign(0.5):zoom(0.3):diffuse(color("#000000"))
				end,
				BeginCommand=function(self)
					self:queuecommand("Set")
				end,
				SetCommand=function(self)
					self:settext('1')
					if GAMESTATE:GetNumPlayersEnabled()>=2 then
						self:y(-6)
					else
						self:y(0)
					end
				end,
			}
		},
		CursorP2 = Def.ActorFrame {
			InitCommand=function(self)
				self:x(55):player(PLAYER_2)
			end,
			Def.Quad{
				InitCommand=function(self)
					self:zoomto(6,22):halign(1):valign(0.5)
				end,
				BeginCommand=function(self)
					self:queuecommand("Set")
				end,
				SetCommand=function(self)
					if GAMESTATE:GetNumPlayersEnabled()>=2 then
						self:zoomy(11)
						self:valign(0)
					else
						self:zoomy(22)
						self:valign(0.5)
					end
				end,
			},
			LoadFont("Common Normal") .. {
				InitCommand=function(self)
					self:x(-1):halign(1):valign(0.5):zoom(0.3):diffuse(color("#000000"))
				end,
				BeginCommand=function(self)
					self:queuecommand("Set")
				end,
				SetCommand=function(self)
					self:settext('2')
					if GAMESTATE:GetNumPlayersEnabled()>=2 then
						self:y(6)
					else
						self:y(0)
					end
				end,
			}
		},
		CursorP1Frame = Def.Actor{
			ChangeCommand=function(self)
				self:stoptweening():decelerate(0.05)
			end	
		},
		CursorP2Frame = Def.Actor{
			ChangeCommand=function(self)
				self:stoptweening():decelerate(0.05)
			end	
		}
	}
}

t[#t+1] = LoadActor("../_mousewheelscroll")
return t