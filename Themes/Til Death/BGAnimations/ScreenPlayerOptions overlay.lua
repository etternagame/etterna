local t = Def.ActorFrame{}
local topFrameHeight = 35
local bottomFrameHeight = 54
local borderWidth = 4


local t = Def.ActorFrame{
	Name="PlayerAvatar";
};

local profileP1
local profileP2

local profileNameP1 = "No Profile"
local playCountP1 = 0
local playTimeP1 = 0
local noteCountP1 = 0

local profileNameP2 = "No Profile"
local playCountP2 = 0
local playTimeP2 = 0
local noteCountP2 = 0


local AvatarXP1 = 10
local AvatarYP1 = 50
local AvatarXP2 = SCREEN_WIDTH-40
local AvatarYP2 = 50

local bpms = {}
if GAMESTATE:GetCurrentSong() then
	bpms= GAMESTATE:GetCurrentSong():GetDisplayBpms()
	bpms[1] = math.round(bpms[1])
	bpms[2] = math.round(bpms[2])
end

-- P1 Avatar
t[#t+1] = Def.Actor{
	BeginCommand=function(self)
		self:queuecommand("Set")
	end;
	SetCommand=function(self)
		if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
			profileP1 = GetPlayerOrMachineProfile(PLAYER_1)
			if profileP1 ~= nil then
				if profileP1 == PROFILEMAN:GetMachineProfile() then
					profileNameP1 = "Machine Profile"
				else
					profileNameP1 = profileP1:GetDisplayName()
				end
				playCountP1 = profileP1:GetTotalNumSongsPlayed()
				playTimeP1 = profileP1:GetTotalSessionSeconds()
				noteCountP1 = profileP1:GetTotalTapsAndHolds()
			else 
				profileNameP1 = "No Profile"
				playCountP1 = 0
				playTimeP1 = 0
				noteCountP1 = 0
			end; 
		else
			profileNameP1 = "No Profile"
			playCountP1 = 0
			playTimeP1 = 0
			noteCountP1 = 0
		end;
	end;
	PlayerJoinedMessageCommand=function(self)
		self:queuecommand("Set")
	end;
	PlayerUnjoinedMessageCommand=function(self)
		self:queuecommand("Set")
	end;
}

t[#t+1] = Def.ActorFrame{
	Name="Avatar"..PLAYER_1;
	BeginCommand=function(self)
		self:queuecommand("Set")
	end;
	SetCommand=function(self)
		if profileP1 == nil then
			self:visible(false)
		else
			self:visible(true)
		end;
	end;
	PlayerJoinedMessageCommand=function(self)
		self:queuecommand("Set")
	end;
	PlayerUnjoinedMessageCommand=function(self)
		self:queuecommand("Set")
	end;

	Def.Sprite {
		Name="Image";
		InitCommand=function(self)
			self:visible(true):halign(0):valign(0):xy(AvatarXP1,AvatarYP1)
		end;
		BeginCommand=function(self)
			self:queuecommand("ModifyAvatar")
		end;
		PlayerJoinedMessageCommand=function(self)
			self:queuecommand("ModifyAvatar")
		end;
		PlayerUnjoinedMessageCommand=function(self)
			self:queuecommand("ModifyAvatar")
		end;
		ModifyAvatarCommand=function(self)
			self:finishtweening();
			self:LoadBackground(THEME:GetPathG("","../"..getAvatarPath(PLAYER_1)));
			self:zoomto(30,30)
		end;
	};
	LoadFont("Common Normal") .. {
		InitCommand=function(self)
			self:xy(AvatarXP1+33,AvatarYP1+6):halign(0):zoom(0.45)
		end;
		BeginCommand=function(self)
			self:queuecommand("Set")
		end;
		SetCommand=function(self)
			self:settext(profileNameP1.."'s Scroll Speed:")
		end;
		PlayerJoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end;
		PlayerUnjoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end;
	};
	LoadFont("Common Normal") .. {
		InitCommand=function(self)
			self:xy(AvatarXP1+33,AvatarYP1+19):halign(0):zoom(0.40)
		end;
		BeginCommand=function(self)
			local speed, mode= GetSpeedModeAndValueFromPoptions(PLAYER_1)
			self:playcommand("SpeedChoiceChanged", {pn= PLAYER_1, mode= mode, speed= speed})
		end;
		PlayerJoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end;
		PlayerUnjoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end;
		SpeedChoiceChangedMessageCommand=function(self,param)
			if param.pn == PLAYER_1 then
				local text = ""
				if param.mode == "x" then
					if not bpms[1] then
						text = "??? - ???"
					elseif bpms[1] == bpms[2] then
						text = math.round(bpms[1]*param.speed/100)
					else
						text = string.format("%d - %d",math.round(bpms[1]*param.speed/100),math.round(bpms[2]*param.speed/100))
					end
				elseif param.mode == "C" then
					text = param.speed
				else
					if not bpms[1] then
						text = "??? - "..param.speed
					elseif bpms[1] == bpms[2] then
						text = param.speed
					else
						local factor = param.speed/bpms[2]
						text = string.format("%d - %d",math.round(bpms[1]*factor),param.speed)
					end
				end
				self:settext(text)
			end
		end;
	}
}

--Frames
t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:xy(0,0):halign(0):valign(0):zoomto(SCREEN_WIDTH,topFrameHeight):diffuse(color("#000000"))
	end;
};
t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:xy(0,topFrameHeight):halign(0):valign(1):zoomto(SCREEN_WIDTH,borderWidth):diffuse(getMainColor('highlight')):diffusealpha(0.5)
	end;
};

--t[#t+1] = LoadActor("_frame");
t[#t+1] = LoadFont("Common Large")..{
	InitCommand=function(self)
		self:xy(5,32):halign(0):valign(1):zoom(0.55):diffuse(getMainColor('positive')):settext("Player Options:")
	end;
}

return t