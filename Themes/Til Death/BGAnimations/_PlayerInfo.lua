-- Old avatar actor frame.. renamed since much more will be placed here (hopefully?)
local t = Def.ActorFrame{
	Name="PlayerAvatar"
}

local profile

local profileName = "No Profile"
local playCount = 0
local playTime = 0
local noteCount = 0
local skillrating = 0
local speedrating = 0
local stamrating = 0
local jackrating = 0
local numfaves = 0

local AvatarX = 0
local AvatarY = SCREEN_HEIGHT-50

t[#t+1] = Def.Actor{
	BeginCommand=cmd(queuecommand,"Set");
	SetCommand=function(self)
		if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
			profile = GetPlayerOrMachineProfile(PLAYER_1)
			if profile ~= nil then
				if profile == PROFILEMAN:GetMachineProfile() then
					profileName = "Player 1"
				else
					profileName = profile:GetDisplayName()
				end
				playCount = profile:GetTotalNumSongsPlayed()
				playTime = profile:GetTotalSessionSeconds()
				noteCount = profile:GetTotalTapsAndHolds()
				skillrating = profile:GetPlayerRating()
				speedrating = profile:GetPlayerSpeedRating()
				stamrating = profile:GetPlayerStamRating()
				jackrating = profile:GetPlayerJackRating()
				numfaves = profile:GetNumFaves()
			else 
				profileName = "No Profile"
				playCount = 0
				playTime = 0
				noteCount = 0
			end; 
		else
			profileName = "No Profile"
			playCount = 0
			playTime = 0
			noteCount = 0
		end;
	end;
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set");
	PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set");
}

t[#t+1] = Def.ActorFrame{
	Name="Avatar"..PLAYER_1,
	BeginCommand=cmd(queuecommand,"Set"),
	SetCommand=function(self)
		if profile == nil then
			self:visible(false)
		else
			self:visible(true)
		end
	end,
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
	PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),

	Def.Sprite {
		Name="Image",
		InitCommand=cmd(visible,true;halign,0;valign,0;xy,AvatarX,AvatarY),
		BeginCommand=cmd(queuecommand,"ModifyAvatar"),
		PlayerJoinedMessageCommand=cmd(queuecommand,"ModifyAvatar"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"ModifyAvatar"),
		ModifyAvatarCommand=function(self)
			self:finishtweening()
			self:LoadBackground(THEME:GetPathG("","../"..getAvatarPath(PLAYER_1)))
			self:zoomto(50,50)
		end,
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+53,AvatarY+7;halign,0;zoom,0.6;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settext(profileName)
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+53,AvatarY+20;halign,0;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settext(playCount.." Plays")
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+53,AvatarY+30;halign,0;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settext(noteCount.." Arrows Smashed")
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+53,AvatarY+40;halign,0;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			local time = SecondsToHHMMSS(playTime)
			self:settextf(time.." PlayTime")
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+180,AvatarY+40;halign,0;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settext("Judge: "..GetTimingDifficulty())
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+170,AvatarY+7;halign,0;zoom,0.6;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settextf("Overall Rating: %5.2f", skillrating)
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+170,AvatarY+20;halign,0;zoom,0.6;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settextf("Speed Rating: %5.2f", speedrating)
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+370,AvatarY+7;halign,0;zoom,0.6;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settextf("Stam Rating: %5.2f", stamrating)
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+370,AvatarY+20;halign,0;zoom,0.6;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settextf("Jack Rating: %5.2f", jackrating)
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,SCREEN_WIDTH-5,AvatarY+20;halign,1;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settextf("Songs Loaded: %i", SONGMAN:GetNumSongs())
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,SCREEN_WIDTH-5,AvatarY+30;halign,1;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settextf("Songs Favorited: %i", numfaves)
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
}

local function Update(self)
	t.InitCommand=cmd(SetUpdateFunction,Update);
	if getAvatarUpdateStatus(PLAYER_1) then
    	self:GetChild("Avatar"..PLAYER_1):GetChild("Image"):queuecommand("ModifyAvatar")
    	setAvatarUpdateStatus(PLAYER_1,false)
    end;
end
t.InitCommand=cmd(SetUpdateFunction,Update);

return t